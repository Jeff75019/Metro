#include "sncf.h"

#include <ArduinoJson.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sncf_stations.h"

// ============================================================================
//  Utilitaires
// ============================================================================
bool parseNavitiaTime(const char* s, int64_t* out) {
  if (!s || strlen(s) < 15 || s[8] != 'T') return false;
  for (int i = 0; i < 15; i++)
    if (i != 8 && !isdigit((unsigned char)s[i])) return false;
  auto num = [&](int pos, int n) {
    int v = 0;
    for (int i = 0; i < n; i++) v = v * 10 + (s[pos + i] - '0');
    return v;
  };
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_year = num(0, 4) - 1900;
  t.tm_mon = num(4, 2) - 1;
  t.tm_mday = num(6, 2);
  t.tm_hour = num(9, 2);
  t.tm_min = num(11, 2);
  t.tm_sec = num(13, 2);
  t.tm_isdst = -1;  // heure d'été déterminée par le fuseau (TZ)
  time_t r = mktime(&t);
  if (r == (time_t)-1) return false;
  *out = (int64_t)r;
  return true;
}

void cleanStationName(char* s) {
  if (!s) return;
  fixText(s);
  size_t n = strlen(s);
  if (n > 3 && s[n - 1] == ')') {
    char* p = strrchr(s, '(');
    if (p && p > s && p[-1] == ' ') p[-1] = 0;
  }
}

// Recherche d'un mot entier dans un texte normalisé (minuscules, sans accents)
static bool hasWord(const char* text, const char* word) {
  char norm[96], padded[100], w[24];
  normalizeAscii(text ? text : "", norm, sizeof(norm));
  snprintf(padded, sizeof(padded), " %s ", norm);
  snprintf(w, sizeof(w), " %s ", word);
  return strstr(padded, w) != nullptr;
}

bool isMainline(const char* cm, const char* pm, const char* net) {
  static const char* excluded[] = {"transilien", "rer", "rapidtransit", "bus", "autocar", "car",
                                   "coach", "tramway", "metro"};
  for (const char* src : {cm, pm, net})
    for (const char* w : excluded)
      if (hasWord(src, w)) return false;
  return true;
}

uint32_t trainModeColor(const char* mode, const char* apiColor) {
  char n[48];
  normalizeAscii(mode ? mode : "", n, sizeof(n));
  struct { const char* key; uint32_t color; } map[] = {
      {"ouigo", 0xE2007A},     {"inoui", 0xA3195B},    {"lyria", 0xC8102E},
      {"tgv", 0xA3195B},       {"intercites", 0x1F7DC4}, {"eurostar", 0x2D4BA0},
      {"thalys", 0x2D4BA0},    {"ice", 0x7D8792},      {"frecciarossa", 0xC8102E},
      {"trenitalia", 0xC8102E}, {"renfe", 0x6B2C91},    {"ave", 0x6B2C91},
      {"ter", 0x4A6283},       {"transilien", 0x0C8A6E}, {"rer", 0x3E7CC0},
  };
  for (auto& m : map)
    if (strstr(n, m.key)) return m.color;
  if (apiColor && strlen(apiColor) == 6) {
    unsigned v;
    if (sscanf(apiColor, "%6x", &v) == 1 && v != 0) return v;
  }
  return 0x5A6E8C;
}

// Le nom désigne-t-il la gare consultée elle-même ? ("Paris Gare de Lyon" ~ "Gare de Lyon")
static bool sameStation(const char* a, const char* b) {
  if (!a || !b || !*a || !*b) return false;
  char na[96], nb[96];
  normalizeAscii(a, na, sizeof(na));
  normalizeAscii(b, nb, sizeof(nb));
  return strstr(na, nb) || strstr(nb, na);
}

static bool icontains(const char* text, const char* needle) {
  char n[96];
  normalizeAscii(text ? text : "", n, sizeof(n));
  return strstr(n, needle) != nullptr;
}

// ============================================================================
//  Tableau des départs / arrivées
// ============================================================================
static void buildBoardFilter(JsonDocument& f, const char* kind) {
  JsonObject it = f[kind][0].to<JsonObject>();
  JsonObject di = it["display_informations"].to<JsonObject>();
  for (const char* k : {"direction", "commercial_mode", "physical_mode", "network", "headsign",
                        "trip_short_name", "color"})
    di[k] = true;
  auto linkFields = [](JsonObject o) {
    o["type"] = true;
    o["rel"] = true;
    o["id"] = true;
  };
  linkFields(di["links"][0].to<JsonObject>());
  linkFields(it["links"][0].to<JsonObject>());
  JsonObject sdt = it["stop_date_time"].to<JsonObject>();
  for (const char* k : {"departure_date_time", "base_departure_date_time", "arrival_date_time",
                        "base_arrival_date_time", "data_freshness", "additional_informations"})
    sdt[k] = true;
  JsonObject sl = sdt["links"][0].to<JsonObject>();
  sl["type"] = true;
  sl["rel"] = true;
  sl["id"] = true;
  for (const char* arr : {"terminus", "origins"}) {
    f[arr][0]["id"] = true;
    f[arr][0]["name"] = true;
  }
  f["disruptions"][0]["id"] = true;
  f["disruptions"][0]["severity"]["effect"] = true;
  f["context"]["current_datetime"] = true;
  f["error"]["message"] = true;
  f["message"] = true;
}

// Cherche dans les liens d'un passage celui de type `type` (et de relation `rel`, si donné)
template <typename F>
static void forEachLink(JsonObjectConst item, F fn) {
  for (JsonArrayConst arr : {item["display_informations"]["links"].as<JsonArrayConst>(),
                             item["stop_date_time"]["links"].as<JsonArrayConst>(),
                             item["links"].as<JsonArrayConst>()})
    for (JsonObjectConst l : arr) fn(l);
}

static const char* nameById(JsonArrayConst arr, const char* id) {
  if (!id) return nullptr;
  for (JsonObjectConst o : arr)
    if (strcmp(o["id"] | "", id) == 0) return o["name"] | (const char*)nullptr;
  return nullptr;
}

SiriStatus parseNavitiaBoard(const char* json, size_t len, bool arrivals, bool mainlineOnly,
                             const char* stationName, Board& out, char* err, size_t errLen) {
  memset(&out, 0, sizeof(out));
  out.arrivals = arrivals;
  if (err && errLen) err[0] = 0;
  const char* kind = arrivals ? "arrivals" : "departures";

  size_t i = 0;
  while (i < len && isspace((unsigned char)json[i])) i++;
  if (i >= len || json[i] != '{') {
    if (err) copyStr(err, "Réponse non JSON", errLen);
    return SiriStatus::BadJson;
  }

  JsonDocument filter;
  buildBoardFilter(filter, kind);
  JsonDocument doc;
  DeserializationError e = deserializeJson(doc, json, len, DeserializationOption::Filter(filter),
                                           DeserializationOption::NestingLimit(40));
  if (e) {
    if (err) snprintf(err, errLen, "JSON invalide (%s)", e.c_str());
    return SiriStatus::BadJson;
  }

  JsonArrayConst items = doc[kind];
  if (items.isNull()) {
    const char* msg = doc["error"]["message"] | (const char*)nullptr;
    if (!msg) msg = doc["message"] | "Réponse inattendue";
    if (err) {
      copyStr(err, msg, errLen);
      fixText(err);
    }
    return SiriStatus::ServerError;
  }
  parseNavitiaTime(doc["context"]["current_datetime"] | "", &out.serverTime);

  JsonArrayConst disruptions = doc["disruptions"];
  JsonArrayConst endpoints = doc[arrivals ? "origins" : "terminus"];

  for (JsonObjectConst it : items) {
    if (out.count >= BOARD_MAX) break;
    JsonObjectConst di = it["display_informations"];
    JsonObjectConst sdt = it["stop_date_time"];
    const char* cm = di["commercial_mode"] | "";
    if (mainlineOnly && !isMainline(cm, di["physical_mode"] | "", di["network"] | "")) continue;

    Train t;
    memset(&t, 0, sizeof(t));
    const char* baseS = sdt[arrivals ? "base_arrival_date_time" : "base_departure_date_time"] | "";
    const char* realS = sdt[arrivals ? "arrival_date_time" : "departure_date_time"] | "";
    if (!parseNavitiaTime(*baseS ? baseS : realS, &t.base)) continue;
    if (!parseNavitiaTime(*realS ? realS : baseS, &t.real)) t.real = t.base;
    if (strcmp(sdt["data_freshness"] | "", "realtime") == 0) t.flags |= TR_REALTIME;

    // Suppression : mention dans les informations complémentaires, ou
    // perturbation liée de type "NO_SERVICE"
    for (JsonVariantConst a : sdt["additional_informations"].as<JsonArrayConst>()) {
      const char* s = a | "";
      if (icontains(s, "cancel") || icontains(s, "suppr") || icontains(s, "deleted") ||
          icontains(s, "no service"))
        t.flags |= TR_CANCELLED;
    }
    const char* endpointId = nullptr;
    forEachLink(it, [&](JsonObjectConst l) {
      const char* type = l["type"] | "";
      const char* rel = l["rel"] | "";
      const char* id = l["id"] | "";
      if (!strcmp(type, "vehicle_journey") && !t.vj[0]) copyStr(t.vj, id, sizeof(t.vj));
      if (!strcmp(type, "stop_area") &&
          (arrivals ? (!strcmp(rel, "origins") || !strcmp(rel, "origin")) : !strcmp(rel, "terminus")))
        endpointId = id;
      if (!strcmp(type, "disruption")) {
        for (JsonObjectConst d : disruptions)
          if (!strcmp(d["id"] | "", id) && !strcmp(d["severity"]["effect"] | "", "NO_SERVICE"))
            t.flags |= TR_CANCELLED;
      }
    });

    // Destination (départs) ou provenance (arrivées)
    const char* place = nameById(endpoints, endpointId);
    if (!place && !arrivals) place = di["direction"] | (const char*)nullptr;
    if (place) {
      copyStr(t.place, place, sizeof(t.place));
      cleanStationName(t.place);
    }
    if (arrivals && (!t.place[0] || sameStation(t.place, stationName))) {
      t.place[0] = 0;
      t.flags |= TR_PLACE_PENDING;  // à chercher via /vehicle_journeys
    }

    copyStr(t.mode, cm, sizeof(t.mode));
    fixText(t.mode);
    const char* num = di["headsign"] | "";
    if (!*num) num = di["trip_short_name"] | "";
    copyStr(t.number, num, sizeof(t.number));
    t.color = trainModeColor(cm, di["color"] | "");
    out.t[out.count++] = t;
  }
  return SiriStatus::Ok;
}

bool parseNavitiaOrigin(const char* json, size_t len, const char* stationName, char* out, size_t n) {
  if (!out || !n) return false;
  out[0] = 0;
  JsonDocument filter;
  filter["vehicle_journeys"][0]["stop_times"][0]["stop_point"]["name"] = true;
  JsonDocument doc;
  if (deserializeJson(doc, json, len, DeserializationOption::Filter(filter),
                      DeserializationOption::NestingLimit(40)))
    return false;
  const char* first = nullptr;
  JsonArrayConst stops = doc["vehicle_journeys"][0]["stop_times"];
  for (JsonObjectConst st : stops) {
    const char* name = st["stop_point"]["name"] | (const char*)nullptr;
    if (!name || !*name) continue;
    if (!first) first = name;
    if (!sameStation(name, stationName)) {
      copyStr(out, name, n);
      cleanStationName(out);
      return true;
    }
  }
  if (first) {
    copyStr(out, first, n);
    cleanStationName(out);
  }
  return out[0] != 0;
}

// ============================================================================
//  Train Nomade : https://api.trainnomade.fr/gare/{uic}.json
// ============================================================================
const char* trainCategoryLabel(const char* pr) {
  if (!pr) return "Train";
  if (!strcmp(pr, "highSpeedRail")) return "TGV";
  if (!strcmp(pr, "interregionalRail")) return "Intercités";
  if (!strcmp(pr, "regionalRail") || !strcmp(pr, "local")) return "TER";
  if (!strcmp(pr, "suburbanRailway")) return "Transilien";
  if (!strcmp(pr, "tramTrain")) return "Tram-train";
  if (!strcmp(pr, "railShuttle")) return "Navette";
  if (!strcmp(pr, "longDistance")) return "Grande ligne";
  return "Train";
}

void stationUicList(const char* id, char* out, size_t n) {
  if (!out || !n) return;
  out[0] = 0;
  if (!id) return;
  const char* p = strncmp(id, "stop_area:SNCF:", 15) == 0 ? id + 15 : id;
  size_t k = 0;
  for (; *p && k + 1 < n; p++) {
    if (isdigit((unsigned char)*p)) out[k++] = *p;
    else if ((*p == ',' || *p == ';') && k && out[k - 1] != ',') out[k++] = ',';
    else if (*p == ':') break;
  }
  if (k && out[k - 1] == ',') k--;
  out[k] = 0;
}

void stationStopArea(const char* id, char* out, size_t n) {
  if (!out || !n) return;
  if (id && strncmp(id, "stop_area:", 10) == 0) {
    copyStr(out, id, n);
    return;
  }
  char uic[16] = "";
  size_t k = 0;
  for (const char* p = id ? id : ""; *p && isdigit((unsigned char)*p) && k + 1 < sizeof(uic); p++) uic[k++] = *p;
  uic[k] = 0;
  snprintf(out, n, "stop_area:SNCF:%s", uic);
}

// Heure "secondes depuis minuit du jour" (heure de Paris, au-delà de 86 400 après minuit)
static bool dayTime(const char* jour, long s, int64_t* out) {
  int y, m, d;
  if (!jour || sscanf(jour, "%4d-%2d-%2d", &y, &m, &d) != 3) return false;
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_year = y - 1900;
  t.tm_mon = m - 1;
  t.tm_mday = d;
  t.tm_hour = (int)(s / 3600);
  t.tm_min = (int)(s / 60 % 60);
  t.tm_sec = (int)(s % 60);
  t.tm_isdst = -1;  // mktime normalise (heures > 23) et choisit l'heure d'été / d'hiver
  time_t r = mktime(&t);
  if (r == (time_t)-1) return false;
  *out = (int64_t)r;
  return true;
}

SiriStatus parseTrainNomade(const char* json, size_t len, bool arrivals, bool mainlineOnly, Board& out,
                            char* err, size_t errLen) {
  memset(&out, 0, sizeof(out));
  out.arrivals = arrivals;
  if (err && errLen) err[0] = 0;
  size_t i = 0;
  while (i < len && isspace((unsigned char)json[i])) i++;
  if (i >= len || json[i] != '{') {
    if (err) copyStr(err, "Réponse non JSON", errLen);
    return SiriStatus::BadJson;
  }
  JsonDocument filter;
  filter["jour"] = true;
  filter["maintenant_s"] = true;
  filter["perime_apres"] = true;
  filter["noms"] = true;
  filter["erreur"] = true;
  for (const char* k : {"n", "a", "d", "ra", "rd", "v", "p", "x", "an", "nm", "pr", "o", "de", "j"})
    filter["passages"][0][k] = true;
  JsonDocument doc;
  DeserializationError e = deserializeJson(doc, json, len, DeserializationOption::Filter(filter));
  if (e) {
    if (err) snprintf(err, errLen, "JSON invalide (%s)", e.c_str());
    return SiriStatus::BadJson;
  }
  JsonArrayConst passages = doc["passages"];
  if (passages.isNull()) {
    if (err) copyStr(err, (doc["erreur"] | "") [0] ? "Gare inconnue" : "Réponse inattendue", errLen);
    return SiriStatus::ServerError;
  }
  const char* jour = doc["jour"] | "";
  dayTime(jour, doc["maintenant_s"] | 0L, &out.serverTime);
  parseIsoTime(doc["perime_apres"] | "", &out.validUntil);
  JsonObjectConst noms = doc["noms"];

  for (JsonObjectConst p : passages) {
    if (out.count >= BOARD_MAX) break;
    JsonVariantConst tv = p[arrivals ? "a" : "d"];
    if (!tv.is<long>()) continue;              // pas d'heure d'arrivée / de départ ici
    if (p["p"] | 0) continue;                  // déjà parti / arrivé
    const char* pr = p["pr"] | "";
    if (mainlineOnly && (!strcmp(pr, "suburbanRailway") || !strcmp(pr, "tramTrain") || !strcmp(pr, "railShuttle")))
      continue;

    Train t;
    memset(&t, 0, sizeof(t));
    const char* day = p["j"] | jour;           // train parti la veille
    if (!dayTime(day, tv.as<long>(), &t.base)) continue;
    t.real = t.base + (long)(p[arrivals ? "ra" : "rd"] | 0L);
    if (!(p["nm"] | 0)) t.flags |= TR_REALTIME;
    if ((p["an"] | 0) || (p["x"] | 0)) t.flags |= TR_CANCELLED;

    // un même train peut apparaître deux fois (codes UIC différents d'une même gare)
    char num[12];
    copyStr(num, p["n"] | "", sizeof(num));
    num[strcspn(num, "/-")] = 0;  // "6611/2" ou "129600-129601" : premier numéro
    bool dup = false;
    for (int k = 0; k < out.count; k++)
      if (!strcmp(out.t[k].number, num)) dup = true;
    if (dup) continue;
    copyStr(t.number, num, sizeof(t.number));

    const char* code = p[arrivals ? "o" : "de"] | "";
    copyStr(t.place, noms[code] | code, sizeof(t.place));
    cleanStationName(t.place);
    copyStr(t.platform, p["v"] | "", sizeof(t.platform));
    copyStr(t.mode, trainCategoryLabel(pr), sizeof(t.mode));
    t.color = trainModeColor(t.mode, "");
    out.t[out.count++] = t;
  }
  return SiriStatus::Ok;
}

int parseGaresVoyageurs(const char* json, size_t len, StationHit* out, int maxN) {
  JsonDocument filter;
  filter["results"][0]["nom"] = true;
  filter["results"][0]["codes_uic"] = true;
  JsonDocument doc;
  if (deserializeJson(doc, json, len, DeserializationOption::Filter(filter))) return -1;
  int n = 0;
  JsonArrayConst results = doc["results"];
  for (JsonObjectConst r : results) {
    if (n >= maxN) break;
    StationHit& h = out[n];
    stationUicList(r["codes_uic"] | "", h.id, sizeof(h.id));
    if (!h.id[0]) continue;
    copyStr(h.name, r["nom"] | "", sizeof(h.name));
    fixText(h.name);
    n++;
  }
  return n;
}

// ============================================================================
//  Train Nomade : horaires prévus sur 14 jours (gares/horaires/{slug}.json)
// ============================================================================
const char* timetableSlug(const char* stationId) {
  char uic[48];
  stationUicList(stationId, uic, sizeof(uic));
  const char* p = uic;
  while (*p) {  // le premier code connu de la liste
    char code[12];
    size_t k = 0;
    while (p[k] && p[k] != ',' && k + 1 < sizeof(code)) {
      code[k] = p[k];
      k++;
    }
    code[k] = 0;
    for (const auto& s : TIMETABLE_STATIONS)
      if (!strcmp(s.uic, code)) return s.slug;
    p += k;
    if (*p == ',') p++;
  }
  return nullptr;
}

static int64_t daysFromDate(const char* s) {  // "AAAA-MM-JJ" -> numéro de jour
  int y, m, d;
  if (!s || sscanf(s, "%4d-%2d-%2d", &y, &m, &d) != 3) return INT64_MIN;
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
  return (int64_t)era * 146097 + (int64_t)(yoe * 365 + yoe / 4 - yoe / 100 + doy);
}

static void sortBoard(Board& b) {
  for (int i = 1; i < b.count; i++)
    for (int k = i; k > 0 && b.t[k - 1].real > b.t[k].real; k--) {
      Train tmp = b.t[k];
      b.t[k] = b.t[k - 1];
      b.t[k - 1] = tmp;
    }
}

int mergeTimetable(const char* json, size_t len, bool arrivals, int64_t now, int horizonS, Board& b) {
  JsonDocument filter;
  filter["du"] = true;
  filter["produits"] = true;
  filter["trains"] = true;
  JsonDocument doc;
  if (deserializeJson(doc, json, len, DeserializationOption::Filter(filter))) return -1;
  const int64_t du = daysFromDate(doc["du"] | "");
  if (du == INT64_MIN) return -1;
  JsonArrayConst produits = doc["produits"];
  JsonArrayConst trains = doc["trains"];

  // Jour d'aujourd'hui (heure de Paris), et la veille pour les trains après minuit
  time_t nt = (time_t)now;
  struct tm lt;
  localtime_r(&nt, &lt);
  char today[40];
  snprintf(today, sizeof(today), "%04d-%02d-%02d", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday);
  const int64_t todayN = daysFromDate(today);

  // On rassemble les candidats dans un tableau plus grand, puis on garde les premiers
  static Board all;
  all = b;
  int added = 0;
  for (int dayOff = -1; dayOff <= 0; dayOff++) {
    const int rank = (int)(todayN + dayOff - du);
    if (rank < 0 || rank > 30) continue;
    struct tm mid = lt;
    mid.tm_mday += dayOff;
    mid.tm_hour = mid.tm_min = mid.tm_sec = 0;
    mid.tm_isdst = -1;
    for (JsonArrayConst t : trains) {
      if (t.size() < 7) continue;
      const uint32_t mask = t[6] | 0u;
      if (!((mask >> rank) & 1u)) continue;
      JsonVariantConst minutes = t[arrivals ? 4 : 5];
      if (!minutes.is<long>()) continue;  // le train part d'ici (arrivées) ou y finit (départs)
      struct tm tt = mid;
      tt.tm_min = (int)minutes.as<long>();  // mktime normalise (après minuit : > 1 440)
      const time_t when = mktime(&tt);
      if (when < now - 60 || when > now + horizonS) continue;

      char num[12];
      copyStr(num, t[0] | "", sizeof(num));
      const char* product = produits[t[1] | 0] | "";
      int found = -1;
      for (int k = 0; k < all.count; k++)
        if (!strcmp(all.t[k].number, num)) found = k;
      if (found >= 0) {  // déjà en temps réel : on précise seulement la marque
        if (*product && strlen(product) < sizeof(all.t[found].mode)) {
          copyStr(all.t[found].mode, product, sizeof(all.t[found].mode));
          all.t[found].color = trainModeColor(product, "");
        }
        continue;
      }
      if (all.count >= BOARD_MAX) {  // plein : on remplace le plus tardif s'il est plus tard
        sortBoard(all);
        if (all.t[all.count - 1].real <= when) continue;
        all.count--;
      }
      Train& n = all.t[all.count++];
      memset(&n, 0, sizeof(n));
      n.base = n.real = (int64_t)when;
      copyStr(n.number, num, sizeof(n.number));
      copyStr(n.place, t[arrivals ? 2 : 3] | "", sizeof(n.place));
      cleanStationName(n.place);
      copyStr(n.mode, *product ? product : "Train", sizeof(n.mode));
      n.color = trainModeColor(n.mode, "");
      added++;
    }
  }
  sortBoard(all);
  b = all;
  return added;
}
