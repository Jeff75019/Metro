#include "sncf_api.h"

#include <ArduinoJson.h>
#include <time.h>

#include "net.h"

static const char* SNCF_API = "https://api.sncf.com/v1/coverage/sncf/";

static HttpAuth sncfAuth(const char* key) {
  HttpAuth a;
  a.key = key;
  a.basic = true;  // la clé sert d'identifiant HTTP Basic, mot de passe vide
  return a;
}

// ----------------------------------------------------------------------------
FetchResult sncfFetchBoard(const Station& st, bool arrivals, Board& out) {
  FetchResult r;
  memset(&r, 0, sizeof(r));
  // On en demande davantage quand on filtre (beaucoup de RER/Transilien à Paris)
  String url = String(SNCF_API) + "stop_areas/" + urlEncode(st.id, true) + (arrivals ? "/arrivals" : "/departures") +
               "?count=" + (settings.mainlineOnly ? "30" : "12") + "&depth=1&data_freshness=realtime";
  String body;
  quotaCount(Api::Sncf);
  r.http = httpGet(url, sncfAuth(settings.sncfKey), body);
  if (r.http != 200) {
    describeHttp(r.http, "SNCF", r.msg, sizeof(r.msg));
    return r;
  }
  static Board tmp;  // n'écrase pas le tableau affiché en cas d'erreur
  char err[64];
  SiriStatus s = parseNavitiaBoard(body.c_str(), body.length(), arrivals, settings.mainlineOnly, st.name, tmp, err,
                                   sizeof(err));
  if (s != SiriStatus::Ok) {
    copyStr(r.msg, err, sizeof(r.msg));
    return r;
  }
  syncClock(tmp.serverTime);
  tmp.fetchedAt = (int64_t)time(nullptr);
  // Provenances déjà connues : on les reprend du tableau précédent
  for (int i = 0; i < tmp.count; i++) {
    if (!(tmp.t[i].flags & TR_PLACE_PENDING)) continue;
    for (int k = 0; k < out.count; k++)
      if (out.arrivals && !strcmp(out.t[k].vj, tmp.t[i].vj) && out.t[k].place[0]) {
        copyStr(tmp.t[i].place, out.t[k].place, sizeof(tmp.t[i].place));
        tmp.t[i].flags &= ~TR_PLACE_PENDING;
      }
  }
  out = tmp;
  r.ok = true;
  return r;
}

// ----------------------------------------------------------------------------
//  Provenance des trains (une requête par train, mise en cache)
// ----------------------------------------------------------------------------
struct OriginEntry {
  uint32_t hash;
  char name[TXT_LEN];
};
static OriginEntry originCache[32];
static uint8_t originNext = 0;

static uint32_t fnv1a(const char* s) {
  uint32_t h = 2166136261u;
  while (*s) h = (h ^ (uint8_t)*s++) * 16777619u;
  return h ? h : 1;
}

static const char* cachedOrigin(uint32_t h) {
  for (auto& e : originCache)
    if (e.hash == h) return e.name;
  return nullptr;
}

bool sncfResolveOrigins(Board& b, const Station& st, int maxRows) {
  if (!b.arrivals) return false;
  bool changed = false;
  int todo = -1;
  for (int i = 0; i < b.count; i++) {
    Train& t = b.t[i];
    if (!(t.flags & TR_PLACE_PENDING)) continue;
    const char* c = t.vj[0] ? cachedOrigin(fnv1a(t.vj)) : nullptr;
    if (c || !t.vj[0]) {
      copyStr(t.place, c ? c : "", sizeof(t.place));
      t.flags &= ~TR_PLACE_PENDING;
      changed = true;
    } else if (todo < 0 && i < maxRows) {
      todo = i;
    }
  }
  if (todo < 0 || !quotaAvailable(Api::Sncf, settings.sncfQuota)) return changed;

  Train& t = b.t[todo];
  String body;
  quotaCount(Api::Sncf);
  int code = httpGet(String(SNCF_API) + "vehicle_journeys/" + urlEncode(t.vj, true), sncfAuth(settings.sncfKey), body);
  char origin[TXT_LEN] = "";
  if (code == 200) parseNavitiaOrigin(body.c_str(), body.length(), st.name, origin, sizeof(origin));
  if (code == 200 || code == 404) {  // réponse définitive : on ne redemandera pas
    OriginEntry& e = originCache[originNext];
    originNext = (originNext + 1) % (sizeof(originCache) / sizeof(originCache[0]));
    e.hash = fnv1a(t.vj);
    copyStr(e.name, origin, sizeof(e.name));
    copyStr(t.place, origin, sizeof(t.place));
    t.flags &= ~TR_PLACE_PENDING;
    changed = true;
  }
  return changed;
}

// ----------------------------------------------------------------------------
int sncfSearchJson(const String& query, const char* key, String& out) {
  JsonDocument res;
  String body;
  quotaCount(Api::Sncf);
  int code = httpGet(String(SNCF_API) + "places?type%5B%5D=stop_area&count=12&q=" + urlEncode(query, false),
                     sncfAuth(key), body);
  if (code != 200) {
    char msg[64];
    describeHttp(code, "SNCF", msg, sizeof(msg));
    res["error"] = msg;
    serializeJson(res, out);
    return code;
  }
  JsonDocument filter;
  filter["places"][0]["id"] = true;
  filter["places"][0]["name"] = true;
  filter["places"][0]["embedded_type"] = true;
  JsonDocument doc;
  if (deserializeJson(doc, body, DeserializationOption::Filter(filter))) {
    res["error"] = "Réponse illisible";
    serializeJson(res, out);
    return -1;
  }
  JsonArray arr = res["stations"].to<JsonArray>();
  for (JsonObjectConst p : doc["places"].as<JsonArrayConst>()) {
    if (strcmp(p["embedded_type"] | "stop_area", "stop_area") != 0) continue;
    char name[64];
    copyStr(name, p["name"] | "", sizeof(name));
    cleanStationName(name);
    JsonObject o = arr.add<JsonObject>();
    o["id"] = p["id"] | "";
    o["name"] = name;
  }
  serializeJson(res, out);
  return 200;
}
