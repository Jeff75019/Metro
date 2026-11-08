#include "sncf_api.h"

#include <ArduinoJson.h>
#include <time.h>

#include "net.h"

static const char* SNCF_API = "https://api.sncf.com/v1/coverage/sncf/";
static const char* TN_API = "https://api.trainnomade.fr/gare/";
static const char* TN_TIMETABLE = "https://api.trainnomade.fr/gares/horaires/";
static const char* GARES_SEARCH =
    "https://ressources.data.sncf.com/api/explore/v2.1/catalog/datasets/gares-de-voyageurs/records"
    "?limit=12&select=nom,codes_uic&where=";

static HttpAuth sncfAuth(const char* key) {
  HttpAuth a;
  a.key = key;
  a.basic = true;  // la clé sert d'identifiant HTTP Basic, mot de passe vide
  return a;
}

// ----------------------------------------------------------------------------
// Horaires prévus d'une gare sur 14 jours : téléchargés une fois par jour et
// gardés en mémoire (quelques dizaines de Ko par gare, en PSRAM)
struct TimetableCache {
  char slug[40];
  uint32_t day;  // AAAAMMJJ du téléchargement
  String body;
};
static TimetableCache ttCache[4];
static uint8_t ttNext = 0;

static const String* timetableFor(const char* slug) {
  time_t now = time(nullptr);
  if (now < 1700000000) return nullptr;  // heure inconnue
  struct tm lt;
  localtime_r(&now, &lt);
  const uint32_t day = (uint32_t)((lt.tm_year + 1900) * 10000 + (lt.tm_mon + 1) * 100 + lt.tm_mday);
  for (auto& c : ttCache)
    if (c.day == day && !strcmp(c.slug, slug)) return &c.body;
  String body;
  if (httpGet(String(TN_TIMETABLE) + slug + ".json", HttpAuth(), body) != 200) return nullptr;
  TimetableCache& c = ttCache[ttNext];
  ttNext = (ttNext + 1) % (sizeof(ttCache) / sizeof(ttCache[0]));
  copyStr(c.slug, slug, sizeof(c.slug));
  c.day = day;
  c.body = body;
  return &c.body;
}

// Train Nomade : un appel par gare (jusqu'à 4 codes UIC), sans clé
static FetchResult trainNomadeFetch(const Station& st, bool arrivals, Board& out) {
  FetchResult r;
  memset(&r, 0, sizeof(r));
  char uic[48];
  stationUicList(st.id, uic, sizeof(uic));
  String body;
  r.http = httpGet(String(TN_API) + uic + ".json", HttpAuth(), body);  // chiffres et virgules seulement
  if (r.http != 200) {
    describeHttp(r.http, "Train Nomade", r.msg, sizeof(r.msg));
    return r;
  }
  static Board tmp;
  char err[64];
  if (parseTrainNomade(body.c_str(), body.length(), arrivals, settings.mainlineOnly, tmp, err, sizeof(err)) !=
      SiriStatus::Ok) {
    copyStr(r.msg, err, sizeof(r.msg));
    return r;
  }
  tmp.fetchedAt = (int64_t)time(nullptr);
  // Trains pas encore suivis en temps réel (tôt le matin, départs lointains) :
  // complétés par les horaires prévus, pour les 145 grandes gares
  const char* slug = timetableSlug(st.id);
  if (slug && tmp.fetchedAt > 1700000000) {
    if (const String* tt = timetableFor(slug))
      mergeTimetable(tt->c_str(), tt->length(), arrivals, tmp.fetchedAt, 3 * 3600, tmp);
  }
  out = tmp;
  r.ok = true;
  return r;
}

FetchResult sncfFetchBoard(const Station& st, bool arrivals, Board& out) {
  if (settings.trainSource == TrainSource::TrainNomade) return trainNomadeFetch(st, arrivals, out);
  FetchResult r;
  memset(&r, 0, sizeof(r));
  // On en demande davantage quand on filtre (beaucoup de RER/Transilien à Paris)
  char area[48];
  stationStopArea(st.id, area, sizeof(area));
  String url = String(SNCF_API) + "stop_areas/" + urlEncode(area, true) + (arrivals ? "/arrivals" : "/departures") +
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
// Recherche sans clé dans le jeu "gares-de-voyageurs" de l'open data SNCF
static int garesSearchJson(const String& query, String& out) {
  JsonDocument res;
  String q = query;
  q.replace("\"", " ");
  String body;
  int code = httpGet(String(GARES_SEARCH) + urlEncode("\"" + q + "\"", false), HttpAuth(), body);
  if (code != 200) {
    char msg[64];
    describeHttp(code, "open data SNCF", msg, sizeof(msg));
    res["error"] = msg;
    serializeJson(res, out);
    return code;
  }
  static StationHit hits[12];
  int n = parseGaresVoyageurs(body.c_str(), body.length(), hits, 12);
  JsonArray arr = res["stations"].to<JsonArray>();
  for (int i = 0; i < n; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["id"] = hits[i].id;
    o["name"] = hits[i].name;
  }
  serializeJson(res, out);
  return 200;
}

int sncfSearchJson(const String& query, bool useSncfApi, const char* key, String& out) {
  if (!useSncfApi || !key || !*key) return garesSearchJson(query, out);
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
