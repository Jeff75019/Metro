#include "sncf.h"

#include <ArduinoJson.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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
