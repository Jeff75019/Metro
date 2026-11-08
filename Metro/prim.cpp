#include "prim.h"

#include <ArduinoJson.h>
#include <time.h>

#include "lines.h"
#include "net.h"

static const char* PRIM_URL = "https://prim.iledefrance-mobilites.fr/marketplace/stop-monitoring";
static const char* ODS_V1 = "https://data.iledefrance-mobilites.fr/api/records/1.0/search/?dataset=zones-d-arrets&rows=30&q=";
static const char* ODS_REF =
    "https://data.iledefrance-mobilites.fr/api/explore/v2.1/catalog/datasets/referentiel-des-lignes/records"
    "?limit=20&select=id_line,shortname_line,transportmode,colourweb_hexa,textcolourweb_hexa&where=";
static const char* ODS_V21 =
    "https://data.iledefrance-mobilites.fr/api/explore/v2.1/catalog/datasets/zones-d-arrets/records?limit=30&where=";

static HttpAuth primAuth(const char* key) {
  HttpAuth a;
  a.header = "apikey";
  a.key = key;
  return a;
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
  quotaCount(Api::Prim);
  r.http = httpGet(stopMonitoringUrl(fav.stopRef, fav.line), primAuth(apiKey), body);
  if (r.http == 400 && fav.line[0]) {
    // Paramètre LineRef refusé : on demande tout l'arrêt et on filtre nous-mêmes
    quotaCount(Api::Prim);
    r.http = httpGet(stopMonitoringUrl(fav.stopRef, nullptr), primAuth(apiKey), body);
  }
  if (r.http != 200) {
    describeHttp(r.http, "PRIM", r.msg, sizeof(r.msg));
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
  quotaCount(Api::Prim);
  int code = httpGet(stopMonitoringUrl(ref.c_str(), nullptr), primAuth(apiKey), body);
  if (code != 200) {
    char msg[64];
    describeHttp(code, "PRIM", msg, sizeof(msg));
    doc["error"] = msg;
    serializeJson(doc, out);
    return code;
  }
  static LineSummary ls[16];
  char stop[TXT_LEN];
  int n = summarizeLines(body.c_str(), body.length(), ls, 16, stop, sizeof(stop));

  // Lignes absentes de la table (bus, Noctilien...) : nom et couleurs dans le
  // référentiel des lignes de l'open data IDFM (un seul appel, sans clé)
  static LineRefInfo refs[16];
  int nref = 0;
  String where;
  for (int i = 0; i < n; i++) {
    if (findLine(ls[i].line)) continue;
    if (where.length()) where += " OR ";
    where += String("id_line=\"") + ls[i].line + "\"";
  }
  if (where.length()) {
    String rbody;
    int rc = httpGet(String(ODS_REF) + urlEncode(where, false), HttpAuth(), rbody);
    if (rc == 200) nref = parseLineReferential(rbody.c_str(), rbody.length(), refs, 16);
  }
  doc["ref"] = ref;
  doc["stopName"] = stop;
  JsonArray arr = doc["lines"].to<JsonArray>();
  for (int i = 0; i < n; i++) {
    JsonObject o = arr.add<JsonObject>();
    const LineInfo* li = findLine(ls[i].line);
    char col[12];
    const LineRefInfo* ri = nullptr;
    for (int k = 0; k < nref && !li; k++)
      if (!strcmp(refs[k].id, ls[i].line)) ri = &refs[k];
    o["id"] = ls[i].line;
    o["name"] = li ? li->name : (ri ? ri->name : ls[i].line);
    o["mode"] = lineModeName(li ? li->mode : (ri ? ri->mode : LineMode::Bus));
    snprintf(col, sizeof(col), "#%06X", (unsigned)(li ? li->color : (ri ? ri->color : 0x5A6E8C)));
    o["color"] = col;
    snprintf(col, sizeof(col), "#%06X", (unsigned)(li ? li->textColor : (ri ? ri->textColor : 0xFFFFFF)));
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
  // API "explore v2.1" (actuelle), puis l'ancienne API v1 en secours
  String q = query;
  q.replace("\"", " ");
  const uint32_t t0 = millis();
  int code = httpGet(String(ODS_V21) + urlEncode("\"" + q + "\"", false), HttpAuth(), body);
  // Ancienne API seulement si la nouvelle répond par une erreur (pas en cas de
  // panne réseau : même serveur, ce serait une longue attente pour rien)
  if (code >= 300) code = httpGet(String(ODS_V1) + urlEncode(query, false), HttpAuth(), body);
  log_i("Recherche d'arrêts « %s » : code %d, %u octets, %lu ms", query.c_str(), code, (unsigned)body.length(),
        (unsigned long)(millis() - t0));
  JsonDocument res;
  if (code != 200) {
    char msg[64];
    describeHttp(code, "open data IDFM", msg, sizeof(msg));
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
      JsonObjectConst f = v1 ? it["fields"].as<JsonObjectConst>() : it;
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
