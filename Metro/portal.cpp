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
    if (!li) continue;
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
  int n = WiFi.scanNetworks(false, false);
  JsonDocument d;
  JsonArray arr = d["networks"].to<JsonArray>();
  for (int i = 0; i < n && i < 25; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.isEmpty()) continue;
    bool dup = false;
    for (JsonObject o : arr)
      if (ssid == (const char*)(o["ssid"] | "")) dup = true;
    if (dup) continue;
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = ssid;
    o["rssi"] = WiFi.RSSI(i);
    o["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
  String out;
  serializeJson(d, out);
  sendJson(200, out);
}

static void handleConnect() {
  lastActivity = millis();
  JsonDocument d;
  if (deserializeJson(d, server.arg("plain"))) return sendError(400, "JSON invalide");
  String ssid = d["ssid"] | "";
  String pass = d["pass"] | "";
  if (ssid.isEmpty()) return sendError(400, "SSID manquant");
  if (pass.isEmpty())  // mot de passe déjà enregistré pour ce réseau ?
    for (auto& w : settings.wifi)
      if (ssid == w.ssid) pass = w.pass;
  WiFi.disconnect(false, false);
  WiFi.begin(ssid.c_str(), pass.c_str());
  sendJson(200, "{\"ok\":true}");
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
  String key = server.arg("key");
  key.trim();
  if (key.isEmpty()) key = settings.sncfKey;
  if (key.isEmpty()) return sendError(400, "Saisissez d'abord votre clé API SNCF.");
  String q = server.arg("q");
  q.trim();
  if (q.length() < 2) return sendError(400, "Tapez au moins 2 lettres");
  String out;
  int code = sncfSearchJson(q, key.c_str(), out);
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
  char qr[64], url2[48] = "";
  snprintf(qr, sizeof(qr), "WIFI:T:nopass;S:%s;;", apName);
  if (staConnected()) snprintf(url2, sizeof(url2), "ou http://%s", WiFi.localIP().toString().c_str());
  uiDrawPortal(apName, qr, WiFi.softAPIP().toString().c_str(), url2);
}

void portalStart() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(apName, sizeof(apName), "%s%02X%02X", CONFIG_AP_PREFIX, mac[4], mac[5]);

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName);
  delay(100);
  // Si un WiFi est déjà configuré, on s'y connecte aussi (pour la recherche d'arrêts)
  if (settings.wifi[0].ssid[0]) WiFi.begin(settings.wifi[0].ssid, settings.wifi[0].pass);

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

  wl_status_t st = WiFi.status();
  if (st != lastSta) {
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
