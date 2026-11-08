#include "portal.h"

#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>

#include "board.h"
#include "lines.h"
#include "net.h"
#include "portal_html.h"
#include "prim.h"
#include "sncf_api.h"
#include "settings.h"
#include "siri.h"  // copyStr
#include "ui.h"

static WebServer server(80);
static DNSServer dns;
static bool active = false;
static char apName[24];
static uint32_t lastActivity = 0;
static uint32_t lastDraw = 0;
static bool mdnsStarted = false;
static wl_status_t lastSta = WL_IDLE_STATUS;

static const uint32_t PORTAL_TIMEOUT_MS = 15UL * 60 * 1000;  // 15 min sans activité

bool portalActive() { return active; }
const char* portalApName() { return apName; }

static void sendJson(int code, const String& body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json; charset=utf-8", body);
}

static void sendError(int code, const char* msg) {
  JsonDocument d;
  d["error"] = msg;
  String s;
  serializeJson(d, s);
  sendJson(code, s);
}

static bool staConnected() { return WiFi.status() == WL_CONNECTED; }

// ----------------------------------------------------------------------------
//  Connexion au WiFi de la box pendant la configuration
// ----------------------------------------------------------------------------
// L'ESP32 n'a qu'une radio : pour rejoindre la box, le point d'accès Metro-XXXX
// passe sur le canal de celle-ci, et le téléphone perd la page un instant. D'où :
//   - le point d'accès démarre d'emblée sur le canal le plus probable (réseau
//     enregistré visible, sinon le plus fort), voir portalStart() ;
//   - un réseau saisi dans la page est enregistré dès que la connexion réussit,
//     sans attendre le bouton « Enregistrer » ;
//   - la raison d'un échec (mot de passe refusé…) s'affiche sur l'écran de
//     l'afficheur et dans la page.
enum class StaState : uint8_t { Idle, Connecting, Connected, Failed };
static const char* const STA_STATE_NAMES[] = {"idle", "connecting", "connected", "failed"};
static StaState staState = StaState::Idle;
static String staSsid, staPass;  // réseau en cours d'essai
static bool staRemember = false;  // saisi dans la page : à enregistrer s'il répond
static bool staSaved = false;
static uint8_t staFailReason = 0;  // 0 : pas de réponse
static uint32_t staStart = 0, staFailSeen = 0;
static String scanCache;  // recherche faite au démarrage, avant d'ouvrir le point d'accès
static uint32_t scanCacheAt = 0;

static const uint32_t STA_TIMEOUT_MS = 30000;
static const uint32_t STA_RETRY_GRACE_MS = 9000;  // le cœur ESP32 refait un essai après un échec

static void drawPortalScreen();

static void staConnect(const String& ssid, const String& pass, bool remember) {
  staSsid = ssid;
  staPass = pass;
  staRemember = remember;
  staSaved = false;
  staFailReason = 0;
  staFailSeen = 0;
  WiFi.disconnect(false, false);
  delay(50);
  wifiClearReason();
  WiFi.begin(staSsid.c_str(), staPass.c_str());
  staState = StaState::Connecting;
  staStart = millis();
  log_i("Portail : connexion à %s", staSsid.c_str());
  drawPortalScreen();
}

// Place le réseau en tête de la liste enregistrée (3 au maximum)
static void rememberWifi(const String& ssid, const String& pass) {
  if (ssid == settings.wifi[0].ssid && pass == settings.wifi[0].pass) return;
  WifiCred list[MAX_WIFI];
  memset(list, 0, sizeof(list));
  copyStr(list[0].ssid, ssid.c_str(), sizeof(list[0].ssid));
  copyStr(list[0].pass, pass.c_str(), sizeof(list[0].pass));
  int n = 1;
  for (const auto& w : settings.wifi)
    if (w.ssid[0] && ssid != w.ssid && n < MAX_WIFI) list[n++] = w;
  memcpy(settings.wifi, list, sizeof(list));
  if (settingsSave()) log_i("Portail : WiFi %s enregistré", ssid.c_str());
}

static void staPoll() {
  if (staState == StaState::Connected && !staConnected()) staState = StaState::Idle;
  if (staState != StaState::Connecting) return;
  if (staConnected()) {
    staState = StaState::Connected;
    if (staRemember && WiFi.SSID() == staSsid) {
      rememberWifi(staSsid, staPass);
      staSaved = true;
    }
    log_i("Portail : connecté à %s (%s)", staSsid.c_str(), WiFi.localIP().toString().c_str());
    return;
  }
  const uint8_t r = wifiLastReason();
  if (r && !staFailSeen) staFailSeen = millis();
  const bool failed = staFailSeen && millis() - staFailSeen > STA_RETRY_GRACE_MS;
  if (!failed && millis() - staStart < STA_TIMEOUT_MS) return;
  staState = StaState::Failed;
  staFailReason = r;
  WiFi.disconnect(false, false);  // arrête les essais : le point d'accès reste stable
  log_w("Portail : échec de connexion à %s (%s)", staSsid.c_str(), r ? wifiReasonText(r) : "pas de réponse");
}

// Liste des réseaux visibles, en JSON ; *bestChannel : canal du réseau le plus probable
static String scanJson(int* bestChannel) {
  int n = WiFi.scanNetworks(false, false);
  if (n < 0 && !staConnected()) {  // radio occupée par un essai de connexion
    WiFi.disconnect(false, false);
    delay(100);
    n = WiFi.scanNetworks(false, false);
  }
  if (n < 0) return String();
  JsonDocument d;
  JsonArray arr = d["networks"].to<JsonArray>();
  int best = 0, bestRssi = -1000, saved = 0, savedRank = MAX_WIFI;
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.isEmpty()) continue;
    const int rssi = WiFi.RSSI(i);
    if (rssi > bestRssi) {
      bestRssi = rssi;
      best = WiFi.channel(i);
    }
    for (int k = 0; k < savedRank; k++)
      if (ssid == settings.wifi[k].ssid) {
        savedRank = k;
        saved = WiFi.channel(i);
      }
    if (arr.size() >= 25) continue;
    bool dup = false;
    for (JsonObject o : arr)
      if (ssid == (const char*)(o["ssid"] | "")) dup = true;
    if (dup) continue;
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = ssid;
    o["rssi"] = rssi;
    o["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
  if (bestChannel) *bestChannel = saved ? saved : best;
  String out;
  serializeJson(d, out);
  return out;
}

// ----------------------------------------------------------------------------
static void handleRoot() {
  lastActivity = millis();
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html; charset=utf-8", PORTAL_HTML);
}

static void handleGetConfig() {
  lastActivity = millis();
  JsonDocument doc;
  deserializeJson(doc, settingsToJson(settings, true));
  // Ajoute nom et couleurs des lignes connues, pour l'affichage dans la page
  for (JsonObject f : doc["favorites"].as<JsonArray>()) {
    const LineInfo* li = findLine(f["line"] | "");
    if (!li) {
      if (*(f["badge"] | "")) {
        JsonObject l = f["_line"].to<JsonObject>();
        l["name"] = f["badge"].as<const char*>();
        l["mode"] = f["mode"].as<const char*>();
        l["color"] = f["color"].as<const char*>();
        l["text"] = f["text"].as<const char*>();
      }
      continue;
    }
    JsonObject l = f["_line"].to<JsonObject>();
    char col[12];
    l["name"] = li->name;
    l["mode"] = lineModeName(li->mode);
    snprintf(col, sizeof(col), "#%06X", (unsigned)li->color);
    l["color"] = col;
    snprintf(col, sizeof(col), "#%06X", (unsigned)li->textColor);
    l["text"] = col;
  }
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static void handlePostConfig() {
  lastActivity = millis();
  String err;
  Settings s = settings;
  if (!settingsFromJson(s, server.arg("plain").c_str(), err)) return sendError(400, err.c_str());
  settings = s;
  if (!settingsSave()) return sendError(500, "Écriture en mémoire impossible");
  JsonDocument d;
  d["ok"] = true;
  d["ready"] = settingsReady();
  String missing;
  if (!settings.wifi[0].ssid[0]) missing += "un réseau WiFi, ";
  if (!metroReady() && !trainsReady()) {
    if (settings.sncfKey[0] && !settings.stationCount) missing += "au moins une gare, ";
    else if (settings.apiKey[0] && !settings.favCount) missing += "au moins un arrêt, ";
    else missing += "une clé API (PRIM pour le métro ou SNCF pour les trains), ";
  }
  if (missing.length()) missing.remove(missing.length() - 2);
  d["missing"] = missing;
  String out;
  serializeJson(d, out);
  sendJson(200, out);
}

static void handleStatus() {
  JsonDocument d;
  d["sta"] = staConnected();
  d["ssid"] = WiFi.SSID();
  d["ip"] = staConnected() ? WiFi.localIP().toString() : String("");
  d["rssi"] = staConnected() ? WiFi.RSSI() : 0;
  d["ap"] = apName;
  d["staState"] = STA_STATE_NAMES[(int)staState];
  d["staTarget"] = staSsid;
  d["staSaved"] = staSaved;
  if (staState == StaState::Failed) {
    d["staError"] = staFailReason ? wifiReasonText(staFailReason) : "pas de réponse du réseau";
    d["staHint"] = staFailReason ? wifiReasonHint(staFailReason)
                                 : "Rapprochez l'afficheur de la box, vérifiez le nom du réseau, puis réessayez.";
  }
  d["battery"] = batteryPercent(batteryMilliVolts());
  d["quotaUsed"] = quotaUsed(Api::Prim);
  d["quota"] = settings.dailyQuota;
  d["sncfUsed"] = quotaUsed(Api::Sncf);
  d["sncfQuota"] = settings.sncfQuota;
  d["version"] = FW_VERSION;
  String out;
  serializeJson(d, out);
  sendJson(200, out);
}

static void handleScan() {
  lastActivity = millis();
  if (staState == StaState::Connecting) return sendError(409, "Connexion en cours : réessayez dans quelques secondes.");
  String out;
  if (scanCache.length() && millis() - scanCacheAt < 60000) out = scanCache;  // pas de recherche pendant 1 min
  else out = scanJson(nullptr);
  scanCache = "";
  if (out.isEmpty()) return sendError(503, "Recherche impossible pour l'instant : réessayez.");
  sendJson(200, out);
}

static void handleConnect() {
  lastActivity = millis();
  JsonDocument d;
  if (deserializeJson(d, server.arg("plain"))) return sendError(400, "JSON invalide");
  String ssid = d["ssid"] | "";
  String pass = d["pass"] | "";
  if (ssid.isEmpty()) return sendError(400, "Nom du réseau manquant");
  if (ssid.length() > 32) return sendError(400, "Nom de réseau trop long (32 caractères au maximum)");
  if (pass.isEmpty())  // mot de passe déjà enregistré pour ce réseau ?
    for (auto& w : settings.wifi)
      if (ssid == w.ssid) pass = w.pass;
  if (pass.length() && pass.length() < 8)
    return sendError(400, "Mot de passe trop court : un WiFi protégé en demande au moins 8 caractères.");
  if (pass.length() > 64) return sendError(400, "Mot de passe trop long (64 caractères au maximum)");
  sendJson(200, "{\"ok\":true}");
  staConnect(ssid, pass, true);
}

static void handleSearch() {
  lastActivity = millis();
  if (!staConnected()) return sendError(503, "Pas d'accès Internet : ajoutez d'abord un réseau WiFi (étape 1).");
  String q = server.arg("q");
  q.trim();
  if (q.length() < 2) return sendError(400, "Tapez au moins 2 lettres");
  String out;
  int code = searchStopsJson(q, out);
  sendJson(code == 200 ? 200 : 502, out);
}

static void handleLines() {
  lastActivity = millis();
  if (!staConnected()) return sendError(503, "Pas d'accès Internet : ajoutez d'abord un réseau WiFi (étape 1).");
  String key = server.arg("key");
  key.trim();
  if (key.isEmpty()) key = settings.apiKey;
  if (key.isEmpty()) return sendError(400, "Saisissez d'abord votre clé API PRIM (étape 2).");
  String out;
  int code = primLinesJson(server.arg("ref"), key.c_str(), out);
  sendJson(code == 200 ? 200 : 502, out);
}

static void handleSncfSearch() {
  lastActivity = millis();
  if (!staConnected()) return sendError(503, "Pas d'accès Internet : ajoutez d'abord un réseau WiFi (étape 1).");
  const bool useApi = server.arg("src") == "sncf";
  String key = server.arg("key");
  key.trim();
  if (key.isEmpty()) key = settings.sncfKey;
  if (useApi && key.isEmpty()) return sendError(400, "Saisissez d'abord votre clé API SNCF.");
  String q = server.arg("q");
  q.trim();
  if (q.length() < 2) return sendError(400, "Tapez au moins 2 lettres");
  String out;
  int code = sncfSearchJson(q, useApi, key.c_str(), out);
  sendJson(code == 200 ? 200 : 502, out);
}

static void handleReboot() {
  sendJson(200, "{\"ok\":true}");
  delay(500);
  ESP.restart();
}

static void handleReset() {
  settingsErase();
  sendJson(200, "{\"ok\":true}");
  delay(500);
  ESP.restart();
}

// Portail captif : toute autre adresse renvoie vers la page de configuration
static void handleNotFound() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

// ----------------------------------------------------------------------------
static void drawPortalScreen() {
  char qr[64], url2[64] = "";
  snprintf(qr, sizeof(qr), "WIFI:T:nopass;S:%s;;", apName);
  if (staConnected())
    snprintf(url2, sizeof(url2), "ou http://%s", WiFi.localIP().toString().c_str());
  else if (staState == StaState::Connecting)
    snprintf(url2, sizeof(url2), "WiFi : connexion…");
  else if (staState == StaState::Failed)
    snprintf(url2, sizeof(url2), "WiFi : %s", staFailReason ? wifiReasonText(staFailReason) : "pas de réponse");
  uiDrawPortal(apName, qr, WiFi.softAPIP().toString().c_str(), url2);
}

void portalStart() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(apName, sizeof(apName), "%s%02X%02X", CONFIG_AP_PREFIX, mac[4], mac[5]);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  wifiSetup();
  WiFi.setAutoReconnect(false);  // les essais sont gérés ici (ils gêneraient le point d'accès)
  WiFi.disconnect(false, false);
  // Recherche des réseaux avant d'ouvrir le point d'accès, pour le placer sur le
  // canal du WiFi enregistré, sinon du plus fort (souvent la box de la maison)
  int channel = 0;
  scanCache = scanJson(&channel);
  scanCacheAt = millis();
  if (channel < 1 || channel > 13) channel = 1;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName, nullptr, channel);
  delay(100);
  log_i("Point d'accès %s sur le canal %d", apName, channel);
  // Si un WiFi est déjà configuré, on s'y connecte aussi (pour la recherche d'arrêts)
  if (settings.wifi[0].ssid[0]) staConnect(settings.wifi[0].ssid, settings.wifi[0].pass, false);

  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handlePostConfig);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/api/connect", HTTP_POST, handleConnect);
  server.on("/api/search", HTTP_GET, handleSearch);
  server.on("/api/lines", HTTP_GET, handleLines);
  server.on("/api/sncf/search", HTTP_GET, handleSncfSearch);
  server.on("/api/reboot", HTTP_POST, handleReboot);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.onNotFound(handleNotFound);
  server.begin();

  active = true;
  lastActivity = millis();
  drawPortalScreen();
  lastDraw = millis();
  log_i("Portail de configuration : %s", apName);
}

void portalLoop() {
  dns.processNextRequest();
  server.handleClient();

  const StaState before = staState;
  staPoll();
  wl_status_t st = WiFi.status();
  if (st != lastSta || staState != before) {
    lastSta = st;
    if (st == WL_CONNECTED && !mdnsStarted) mdnsStarted = MDNS.begin(MDNS_NAME);
    drawPortalScreen();
  }
  if (millis() - lastDraw > 30000) {
    drawPortalScreen();
    lastDraw = millis();
  }
  if (millis() - lastActivity > PORTAL_TIMEOUT_MS) {
    log_i("Portail inactif : redémarrage");
    ESP.restart();
  }
}
