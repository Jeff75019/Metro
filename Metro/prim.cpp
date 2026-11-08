#include "prim.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <Preferences.h>
#include <esp_attr.h>
#include <sys/time.h>
#include <time.h>

#include "lines.h"

static const char* PRIM_URL = "https://prim.iledefrance-mobilites.fr/marketplace/stop-monitoring";
static const char* ODS_V1 = "https://data.iledefrance-mobilites.fr/api/records/1.0/search/?dataset=zones-d-arrets&rows=30&q=";
static const char* ODS_V21 =
    "https://data.iledefrance-mobilites.fr/api/explore/v2.1/catalog/datasets/zones-d-arrets/records?limit=30&where=";

// ----------------------------------------------------------------------------
//  HTTPS
// ----------------------------------------------------------------------------
// Vérification des certificats avec le magasin de certificats intégré au cœur
// ESP32 (fonction disponible depuis les versions 3.3.x) ; sinon pas de vérif.
template <typename T>
static auto tlsSetup(T& c, int) -> decltype(c.useBuiltinCACertBundle(), void()) {
  c.useBuiltinCACertBundle();
}
template <typename T>
static void tlsSetup(T& c, long) {
  c.setInsecure();
}

static String urlEncode(const String& s, bool keepColon) {
  static const char* hex = "0123456789ABCDEF";
  String o;
  o.reserve(s.length() * 3);
  for (size_t i = 0; i < s.length(); i++) {
    uint8_t c = (uint8_t)s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || (keepColon && c == ':')) {
      o += (char)c;
    } else {
      o += '%';
      o += hex[c >> 4];
      o += hex[c & 15];
    }
  }
  return o;
}

static int httpGet(const String& url, const char* apiKey, String& body) {
  for (int attempt = 0; attempt < 2; attempt++) {
    NetworkClientSecure client;
    if (attempt == 0) tlsSetup(client, 0);
    else client.setInsecure();
    HTTPClient http;
    http.setConnectTimeout(8000);
    http.setTimeout(12000);
    http.setUserAgent("AfficheurMetro/" FW_VERSION " (ESP32-S3)");
    if (!http.begin(client, url)) return HTTPC_ERROR_CONNECTION_REFUSED;
    if (apiKey && *apiKey) http.addHeader("apikey", apiKey);
    http.addHeader("Accept", "application/json");
    int code = http.GET();
    if (code > 0) {
      body = http.getString();
      http.end();
      return code;
    }
    http.end();
    // Échec de connexion (souvent la négociation TLS) : un second essai sans
    // vérification de certificat, pour ne pas bloquer l'afficheur.
    if (code != HTTPC_ERROR_CONNECTION_REFUSED || attempt == 1) return code;
    log_w("Connexion HTTPS refusée, nouvel essai sans vérification du certificat");
  }
  return HTTPC_ERROR_CONNECTION_REFUSED;
}

static void describeHttp(int code, char* msg, size_t n) {
  if (code == 401 || code == 403) snprintf(msg, n, "Clé API refusée (%d)", code);
  else if (code == 404) snprintf(msg, n, "Arrêt introuvable (404)");
  else if (code == 429) snprintf(msg, n, "Quota PRIM dépassé (429)");
  else if (code >= 500) snprintf(msg, n, "Serveur PRIM indisponible (%d)", code);
  else if (code < 0) snprintf(msg, n, "Réseau : %s", HTTPClient::errorToString(code).c_str());
  else snprintf(msg, n, "Erreur HTTP %d", code);
}

// ----------------------------------------------------------------------------
//  Quota d'appels (mémorisé en RAM RTC, qui survit à la veille, et en flash)
// ----------------------------------------------------------------------------
RTC_DATA_ATTR static uint32_t qDay = 0;
RTC_DATA_ATTR static uint16_t qCount = 0;
RTC_DATA_ATTR static bool qLoaded = false;

static uint32_t todayKey() {
  time_t t = time(nullptr);
  if (t < 1700000000) return 0;  // heure inconnue
  struct tm lt;
  localtime_r(&t, &lt);
  return (uint32_t)((lt.tm_year + 1900) * 10000 + (lt.tm_mon + 1) * 100 + lt.tm_mday);
}

static void quotaSync() {
  if (!qLoaded) {
    Preferences p;
    if (p.begin("quota", true)) {
      qDay = p.getUInt("day", 0);
      qCount = p.getUShort("n", 0);
      p.end();
    }
    qLoaded = true;
  }
  uint32_t d = todayKey();
  if (d && d != qDay) {
    qDay = d;
    qCount = 0;
  }
}

static void quotaCount() {
  quotaSync();
  qCount++;
  Preferences p;
  if (p.begin("quota", false)) {
    p.putUInt("day", qDay);
    p.putUShort("n", qCount);
    p.end();
  }
}

uint16_t quotaUsedToday() {
  quotaSync();
  return qCount;
}

bool quotaAvailable(uint16_t dailyQuota) { return quotaUsedToday() < dailyQuota; }

// ----------------------------------------------------------------------------
static void syncClock(int64_t serverTs) {
  if (serverTs < 1700000000) return;
  int64_t now = (int64_t)time(nullptr);
  if (now < 1700000000 || llabs(now - serverTs) > 15) {
    struct timeval tv = {(time_t)serverTs, 0};
    settimeofday(&tv, nullptr);
    log_i("Horloge réglée sur l'heure du serveur");
  }
}

static String stopMonitoringUrl(const char* stopRef, const char* line) {
  String url = String(PRIM_URL) + "?MonitoringRef=" + urlEncode(stopRef, true);
  if (line && *line) url += "&LineRef=" + urlEncode(String("STIF:Line::") + line + ":", true);
  return url;
}

FetchResult primFetch(const Favorite& fav, const char* apiKey, StopData& out) {
  FetchResult r;
  memset(&r, 0, sizeof(r));
  String body;
  quotaCount();
  r.http = httpGet(stopMonitoringUrl(fav.stopRef, fav.line), apiKey, body);
  if (r.http == 400 && fav.line[0]) {
    // Paramètre LineRef refusé : on demande tout l'arrêt et on filtre nous-mêmes
    quotaCount();
    r.http = httpGet(stopMonitoringUrl(fav.stopRef, nullptr), apiKey, body);
  }
  if (r.http != 200) {
    describeHttp(r.http, r.msg, sizeof(r.msg));
    return r;
  }
  static StopData tmp;  // évite d'écraser les données affichées en cas d'erreur
  char err[64];
  SiriStatus st = parseStopMonitoring(body.c_str(), body.length(), fav.line, tmp, err, sizeof(err));
  if (st != SiriStatus::Ok) {
    copyStr(r.msg, err, sizeof(r.msg));
    return r;
  }
  syncClock(tmp.responseTime);
  tmp.fetchedAt = (int64_t)time(nullptr);
  out = tmp;
  r.ok = true;
  return r;
}

int primLinesJson(const String& stopRef, const char* apiKey, String& out) {
  JsonDocument doc;
  String ref = normalizeStopRef(stopRef);
  String body;
  quotaCount();
  int code = httpGet(stopMonitoringUrl(ref.c_str(), nullptr), apiKey, body);
  if (code != 200) {
    char msg[64];
    describeHttp(code, msg, sizeof(msg));
    doc["error"] = msg;
    serializeJson(doc, out);
    return code;
  }
  static LineSummary ls[16];
  char stop[TXT_LEN];
  int n = summarizeLines(body.c_str(), body.length(), ls, 16, stop, sizeof(stop));
  doc["ref"] = ref;
  doc["stopName"] = stop;
  JsonArray arr = doc["lines"].to<JsonArray>();
  for (int i = 0; i < n; i++) {
    JsonObject o = arr.add<JsonObject>();
    const LineInfo* li = findLine(ls[i].line);
    char col[8];
    o["id"] = ls[i].line;
    o["name"] = li ? li->name : "";
    o["mode"] = li ? lineModeName(li->mode) : "bus";
    snprintf(col, sizeof(col), "#%06X", (unsigned)(li ? li->color : 0x5A6E8C));
    o["color"] = col;
    snprintf(col, sizeof(col), "#%06X", (unsigned)(li ? li->textColor : 0xFFFFFF));
    o["text"] = col;
    JsonArray d = o["dests"].to<JsonArray>();
    for (int k = 0; k < ls[i].ndest; k++) d.add(ls[i].dest[k]);
  }
  if (n < 0) doc["error"] = "Réponse illisible";
  serializeJson(doc, out);
  return 200;
}

// ----------------------------------------------------------------------------
//  Recherche d'arrêts (jeu de données "zones-d-arrets" de l'open data IDFM)
// ----------------------------------------------------------------------------
static int typeRank(const char* t) {
  if (!strcmp(t, "metro")) return 0;
  if (!strcmp(t, "rail")) return 1;
  if (!strcmp(t, "tram")) return 2;
  if (!strcmp(t, "bus")) return 4;
  return 3;
}

int searchStopsJson(const String& query, String& out) {
  String body;
  int code = httpGet(String(ODS_V1) + urlEncode(query, false), nullptr, body);
  if (code != 200) {  // ancienne API indisponible : API "explore v2.1"
    String q = query;
    q.replace("\"", " ");
    code = httpGet(String(ODS_V21) + urlEncode("\"" + q + "\"", false), nullptr, body);
  }
  JsonDocument res;
  if (code != 200) {
    char msg[64];
    describeHttp(code, msg, sizeof(msg));
    res["error"] = msg;
    serializeJson(res, out);
    return code;
  }
  JsonDocument filter;
  for (const char* k : {"zdaid", "zdaname", "zdatown", "zdatype"}) {
    filter["records"][0]["fields"][k] = true;
    filter["results"][0][k] = true;
  }
  JsonDocument doc;
  if (deserializeJson(doc, body, DeserializationOption::Filter(filter))) {
    res["error"] = "Réponse illisible";
    serializeJson(res, out);
    return -1;
  }
  JsonArrayConst items = doc["records"];
  bool v1 = !items.isNull();
  if (!v1) items = doc["results"];

  JsonArray arr = res["stops"].to<JsonArray>();
  for (int rank = 0; rank <= 4; rank++) {  // métro et RER d'abord, bus à la fin
    for (JsonObjectConst it : items) {
      JsonObjectConst f = v1 ? it["fields"] : it;
      const char* zt = f["zdatype"] | "";
      const char* type = !strcmp(zt, "metroStation")   ? "metro"
                         : !strcmp(zt, "railStation")  ? "rail"
                         : !strcmp(zt, "onstreetTram") ? "tram"
                         : !strcmp(zt, "onstreetBus")  ? "bus"
                                                       : "autre";
      if (typeRank(type) != rank) continue;
      JsonVariantConst id = f["zdaid"];
      String sid = id.is<const char*>() ? String(id.as<const char*>()) : String(id.as<long>());
      if (sid.isEmpty() || sid == "0") continue;
      JsonObject o = arr.add<JsonObject>();
      o["ref"] = "STIF:StopArea:SP:" + sid + ":";
      o["name"] = f["zdaname"] | "";
      o["town"] = f["zdatown"] | "";
      o["type"] = type;
    }
  }
  serializeJson(res, out);
  return 200;
}
