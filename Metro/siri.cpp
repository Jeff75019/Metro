#include "siri.h"

#include <ArduinoJson.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "lines.h"

// ============================================================================
//  Petites fonctions utilitaires
// ============================================================================
void copyStr(char* dst, const char* src, size_t n) {
  if (!dst || n == 0) return;
  if (!src) {
    dst[0] = 0;
    return;
  }
  size_t len = strlen(src);
  if (len >= n) {
    len = n - 1;
    // ne pas couper un caractère UTF-8 en deux
    while (len > 0 && ((unsigned char)src[len] & 0xC0) == 0x80) len--;
  }
  memcpy(dst, src, len);
  dst[len] = 0;
}

// Décode un caractère UTF-8 ; renvoie -1 si la séquence est invalide.
static int32_t utf8Next(const unsigned char*& p) {
  unsigned c = *p;
  if (c < 0x80) {
    p++;
    return c;
  }
  int n;
  int32_t cp;
  if ((c & 0xE0) == 0xC0) {
    n = 1;
    cp = c & 0x1F;
  } else if ((c & 0xF0) == 0xE0) {
    n = 2;
    cp = c & 0x0F;
  } else if ((c & 0xF8) == 0xF0) {
    n = 3;
    cp = c & 0x07;
  } else {
    p++;
    return -1;
  }
  for (int i = 1; i <= n; i++) {
    if ((p[i] & 0xC0) != 0x80) {
      p++;
      return -1;
    }
    cp = (cp << 6) | (p[i] & 0x3F);
  }
  p += n + 1;
  return cp;
}

static bool isValidUtf8(const unsigned char* s, size_t len, bool* hasMultibyte) {
  const unsigned char* p = s;
  const unsigned char* end = s + len;
  *hasMultibyte = false;
  while (p < end) {
    if (*p >= 0x80) *hasMultibyte = true;
    if (utf8Next(p) < 0) return false;
  }
  return true;
}

void fixText(char* s) {
  if (!s || !*s) return;
  size_t len = strlen(s);

  // 1) Double encodage "Ã©" (UTF-8 lu comme Latin-1 puis ré-encodé) :
  //    si tous les caractères sont < U+0100, on retente un décodage.
  {
    unsigned char tmp[128];
    size_t n = 0;
    bool allLatin1 = true, anyHigh = false;
    const unsigned char* p = (const unsigned char*)s;
    while (*p && n < sizeof(tmp)) {
      int32_t cp = utf8Next(p);
      if (cp < 0 || cp > 0xFF) {
        allLatin1 = false;
        break;
      }
      if (cp >= 0x80) anyHigh = true;
      tmp[n++] = (unsigned char)cp;
    }
    bool multi = false;
    if (allLatin1 && anyHigh && !*p && isValidUtf8(tmp, n, &multi) && multi) {
      memcpy(s, tmp, n);
      s[n] = 0;
      len = n;
    }
  }

  // 2) Caractère de remplacement U+FFFD (EF BF BD) : en pratique un "é" perdu.
  {
    char* r;
    while ((r = strstr(s, "\xEF\xBF\xBD")) != nullptr) {
      r[0] = (char)0xC3;
      r[1] = (char)0xA9;
      memmove(r + 2, r + 3, strlen(r + 3) + 1);
    }
  }

  // 3) Espaces multiples / en début et fin
  char* w = s;
  bool space = true;
  for (char* r = s; *r; r++) {
    char c = *r;
    if (c == '\t' || c == '\n' || c == '\r') c = ' ';
    if (c == ' ') {
      if (space) continue;
      space = true;
    } else {
      space = false;
    }
    *w++ = c;
  }
  if (w > s && w[-1] == ' ') w--;
  *w = 0;
  (void)len;
}

void normalizeAscii(const char* in, char* out, size_t outLen) {
  if (!out || outLen == 0) return;
  size_t n = 0;
  bool lastSpace = true;
  const unsigned char* p = (const unsigned char*)(in ? in : "");
  auto put = [&](const char* t) {
    for (; *t && n + 1 < outLen; t++) {
      if (*t == ' ') {
        if (lastSpace) continue;
        lastSpace = true;
      } else {
        lastSpace = false;
      }
      out[n++] = *t;
    }
  };
  while (*p) {
    int32_t cp = utf8Next(p);
    char buf[3] = {0, 0, 0};
    if (cp < 0) continue;
    if (cp < 0x80) {
      buf[0] = isalnum(cp) ? (char)tolower(cp) : ' ';
    } else if ((cp >= 0xC0 && cp <= 0xC5) || (cp >= 0xE0 && cp <= 0xE5)) {
      buf[0] = 'a';
    } else if (cp == 0xC7 || cp == 0xE7) {
      buf[0] = 'c';
    } else if ((cp >= 0xC8 && cp <= 0xCB) || (cp >= 0xE8 && cp <= 0xEB)) {
      buf[0] = 'e';
    } else if ((cp >= 0xCC && cp <= 0xCF) || (cp >= 0xEC && cp <= 0xEF)) {
      buf[0] = 'i';
    } else if (cp == 0xD1 || cp == 0xF1) {
      buf[0] = 'n';
    } else if ((cp >= 0xD2 && cp <= 0xD6) || (cp >= 0xF2 && cp <= 0xF6) || cp == 0xD8 || cp == 0xF8) {
      buf[0] = 'o';
    } else if ((cp >= 0xD9 && cp <= 0xDC) || (cp >= 0xF9 && cp <= 0xFC)) {
      buf[0] = 'u';
    } else if (cp == 0xDD || cp == 0xFD || cp == 0xFF) {
      buf[0] = 'y';
    } else if (cp == 0x152 || cp == 0x153) {
      buf[0] = 'o';
      buf[1] = 'e';
    } else if (cp == 0xC6 || cp == 0xE6) {
      buf[0] = 'a';
      buf[1] = 'e';
    } else if (cp == 0xDF) {
      buf[0] = 's';
      buf[1] = 's';
    } else {
      buf[0] = ' ';
    }
    put(buf);
  }
  if (n > 0 && out[n - 1] == ' ') n--;
  out[n] = 0;
}

bool matchesFilter(const char* text, const char* filter) {
  if (!filter || !*filter) return true;
  char nt[96];
  normalizeAscii(text, nt, sizeof(nt));
  bool anyToken = false;
  const char* p = filter;
  while (*p) {
    const char* e = p;
    while (*e && *e != '|' && *e != ';' && *e != ',') e++;
    char tok[64];
    size_t l = (size_t)(e - p);
    if (l >= sizeof(tok)) l = sizeof(tok) - 1;
    memcpy(tok, p, l);
    tok[l] = 0;
    char nk[64];
    normalizeAscii(tok, nk, sizeof(nk));
    if (nk[0]) {
      anyToken = true;
      if (strstr(nt, nk)) return true;
    }
    p = *e ? e + 1 : e;
  }
  return !anyToken;
}

static bool readNum(const char*& p, int digits, int& v) {
  v = 0;
  for (int i = 0; i < digits; i++) {
    if (!isdigit((unsigned char)p[i])) return false;
    v = v * 10 + (p[i] - '0');
  }
  p += digits;
  return true;
}

static int64_t daysFromCivil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

bool parseIsoTime(const char* s, int64_t* out) {
  if (!s || !out) return false;
  const char* p = s;
  int Y, Mo, D, h, mi, se = 0;
  if (!readNum(p, 4, Y) || *p++ != '-' || !readNum(p, 2, Mo) || *p++ != '-' || !readNum(p, 2, D))
    return false;
  if (*p != 'T' && *p != 't' && *p != ' ') return false;
  p++;
  if (!readNum(p, 2, h) || *p++ != ':' || !readNum(p, 2, mi)) return false;
  if (*p == ':') {
    p++;
    if (!readNum(p, 2, se)) return false;
  }
  if (*p == '.' || *p == ',') {
    p++;
    while (isdigit((unsigned char)*p)) p++;
  }
  int offset = 0;
  if (*p == '+' || *p == '-') {
    int sign = (*p == '-') ? -1 : 1;
    p++;
    int oh = 0, om = 0;
    if (!readNum(p, 2, oh)) return false;
    if (*p == ':') p++;
    if (isdigit((unsigned char)*p) && !readNum(p, 2, om)) return false;
    offset = sign * (oh * 3600 + om * 60);
  }
  if (Mo < 1 || Mo > 12 || D < 1 || D > 31 || h > 23 || mi > 59 || se > 60) return false;
  *out = daysFromCivil(Y, (unsigned)Mo, (unsigned)D) * 86400 + h * 3600 + mi * 60 + se - offset;
  return true;
}

// ============================================================================
//  Lecture du JSON SIRI
// ============================================================================

// Les champs SIRI Lite sont tantôt "texte", tantôt {"value": "texte"},
// tantôt [{"value": "texte", "lang": "fr"}].
static const char* sval(JsonVariantConst v) {
  if (v.is<const char*>()) return v.as<const char*>();
  if (v.is<JsonArrayConst>()) v = v[0];
  if (v.is<const char*>()) return v.as<const char*>();
  JsonVariantConst x = v["value"];
  if (x.is<const char*>()) return x.as<const char*>();
  return nullptr;
}

static const char* firstString(JsonVariantConst v, int depth = 0) {
  if (depth > 6) return nullptr;
  if (v.is<const char*>()) return v.as<const char*>();
  if (v.is<JsonObjectConst>()) {
    for (JsonPairConst kv : v.as<JsonObjectConst>()) {
      const char* s = firstString(kv.value(), depth + 1);
      if (s && *s) return s;
    }
  } else if (v.is<JsonArrayConst>()) {
    for (JsonVariantConst e : v.as<JsonArrayConst>()) {
      const char* s = firstString(e, depth + 1);
      if (s && *s) return s;
    }
  }
  return nullptr;
}

// Voie réellement connue ? (PRIM renvoie souvent "unknown")
static bool usefulPlatform(const char* p) {
  if (!p || !*p) return false;
  char n[16];
  normalizeAscii(p, n, sizeof(n));
  return n[0] && strcmp(n, "unknown") != 0 && strcmp(n, "inconnu") != 0 && strcmp(n, "inconnue") != 0;
}

static bool ieq(const char* a, const char* b) {
  if (!a || !b) return false;
  while (*a && *b) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
    a++;
    b++;
  }
  return *a == *b;
}

static void buildFilter(JsonDocument& f) {
  JsonObject sd = f["Siri"]["ServiceDelivery"].to<JsonObject>();
  sd["ResponseTimestamp"] = true;
  JsonObject smd = sd["StopMonitoringDelivery"][0].to<JsonObject>();
  smd["ResponseTimestamp"] = true;
  smd["Status"] = true;
  smd["ErrorCondition"] = true;
  JsonObject v = smd["MonitoredStopVisit"][0].to<JsonObject>();
  v["MonitoringRef"] = true;
  JsonObject j = v["MonitoredVehicleJourney"].to<JsonObject>();
  j["LineRef"] = true;
  j["DirectionName"] = true;
  j["DirectionRef"] = true;
  j["DestinationName"] = true;
  j["DestinationRef"] = true;
  j["MonitoredCall"] = true;
}

static SiriStatus loadDoc(const char* json, size_t len, JsonDocument& doc, JsonObjectConst& smd,
                          int64_t* responseTime, char* err, size_t errLen) {
  // Avec un filtre, ArduinoJson est tolérant : on vérifie nous-mêmes que
  // c'est bien un objet JSON (et pas une page HTML d'erreur).
  size_t i = 0;
  while (i < len && (json[i] == ' ' || json[i] == '\n' || json[i] == '\r' || json[i] == '\t')) i++;
  if (i >= len || json[i] != '{') {
    if (err) copyStr(err, "Réponse non JSON", errLen);
    return SiriStatus::BadJson;
  }
  JsonDocument filter;
  buildFilter(filter);
  DeserializationError e = deserializeJson(doc, json, len, DeserializationOption::Filter(filter),
                                           DeserializationOption::NestingLimit(40));
  if (e) {
    if (err) snprintf(err, errLen, "JSON invalide (%s)", e.c_str());
    return SiriStatus::BadJson;
  }
  JsonObjectConst sd = doc["Siri"]["ServiceDelivery"];
  JsonArrayConst arr = sd["StopMonitoringDelivery"];
  if (arr.isNull() || arr.size() == 0) {
    if (err) copyStr(err, "Réponse vide", errLen);
    return SiriStatus::NoDelivery;
  }
  smd = arr[0];
  if (responseTime) {
    *responseTime = 0;
    const char* ts = smd["ResponseTimestamp"] | (const char*)"";
    if (!*ts) ts = sd["ResponseTimestamp"] | (const char*)"";
    parseIsoTime(ts, responseTime);
  }
  JsonVariantConst ec = smd["ErrorCondition"];
  JsonArrayConst visits = smd["MonitoredStopVisit"];
  if (!ec.isNull() && (visits.isNull() || visits.size() == 0)) {
    const char* msg = firstString(ec);
    if (err) copyStr(err, msg ? msg : "Erreur serveur", errLen);
    if (err) fixText(err);
    return SiriStatus::ServerError;
  }
  return SiriStatus::Ok;
}

static void insertSorted(StopData& out, const Departure& d) {
  int n = out.count;
  if (n == MAX_DEPARTURES) {
    if (d.when >= out.dep[n - 1].when) return;  // plus tardif que tout : ignoré
    n--;                                        // on écrase le dernier
  }
  int i = n;
  while (i > 0 && out.dep[i - 1].when > d.when) {
    out.dep[i] = out.dep[i - 1];
    i--;
  }
  out.dep[i] = d;
  out.count = (uint8_t)(n + 1);
}

SiriStatus parseStopMonitoring(const char* json, size_t len, const char* lineFilter, StopData& out,
                               char* err, size_t errLen) {
  memset(&out, 0, sizeof(out));
  if (err && errLen) err[0] = 0;

  char wantLine[16] = "";
  if (lineFilter && *lineFilter) lineIdFromRef(lineFilter, wantLine, sizeof(wantLine));

  JsonDocument doc;
  JsonObjectConst smd;
  SiriStatus st = loadDoc(json, len, doc, smd, &out.responseTime, err, errLen);
  if (st != SiriStatus::Ok) return st;

  for (JsonObjectConst visit : smd["MonitoredStopVisit"].as<JsonArrayConst>()) {
    JsonObjectConst mvj = visit["MonitoredVehicleJourney"];
    JsonObjectConst call = mvj["MonitoredCall"];
    if (mvj.isNull() || call.isNull()) continue;

    Departure d;
    memset(&d, 0, sizeof(d));
    lineIdFromRef(sval(mvj["LineRef"]), d.line, sizeof(d.line));
    if (wantLine[0] && strcmp(d.line, wantLine) != 0) continue;

    // Train qui termine son trajet ici : ce n'est pas un départ.
    const char* mref = sval(visit["MonitoringRef"]);
    const char* dref = sval(mvj["DestinationRef"]);
    if (mref && dref && strcmp(mref, dref) == 0) continue;

    const char* t = call["ExpectedDepartureTime"] | (const char*)nullptr;
    if (!t) t = call["ExpectedArrivalTime"] | (const char*)nullptr;
    if (t) d.flags |= DEP_REALTIME;
    if (!t) t = call["AimedDepartureTime"] | (const char*)nullptr;
    if (!t) t = call["AimedArrivalTime"] | (const char*)nullptr;
    if (!parseIsoTime(t, &d.when)) continue;

    const char* dest = sval(mvj["DestinationName"]);
    if (!dest || !*dest) dest = sval(call["DestinationDisplay"]);
    if (!dest || !*dest) dest = sval(mvj["DirectionName"]);
    copyStr(d.dest, dest ? dest : "?", sizeof(d.dest));
    fixText(d.dest);

    const char* dir = sval(mvj["DirectionRef"]);
    if (!dir || !*dir) dir = sval(mvj["DirectionName"]);
    copyStr(d.dir, (dir && *dir) ? dir : d.dest, sizeof(d.dir));
    fixText(d.dir);

    const char* plat = sval(call["DeparturePlatformName"]);
    if (!usefulPlatform(plat)) plat = sval(call["ArrivalPlatformName"]);
    if (usefulPlatform(plat)) copyStr(d.platform, plat, sizeof(d.platform));

    if (call["VehicleAtStop"] | false) d.flags |= DEP_AT_STOP;
    const char* ds = call["DepartureStatus"] | (const char*)"";
    const char* as = call["ArrivalStatus"] | (const char*)"";
    if (ieq(ds, "cancelled") || ieq(as, "cancelled")) d.flags |= DEP_CANCELLED;
    if (ieq(ds, "delayed") || ieq(as, "delayed")) d.flags |= DEP_DELAYED;

    if (!out.stopName[0]) {
      copyStr(out.stopName, sval(call["StopPointName"]), sizeof(out.stopName));
      fixText(out.stopName);
    }
    insertSorted(out, d);
  }
  return SiriStatus::Ok;
}

int summarizeLines(const char* json, size_t len, LineSummary* out, int maxLines, char* stopName,
                   size_t stopNameLen) {
  if (stopName && stopNameLen) stopName[0] = 0;
  JsonDocument doc;
  JsonObjectConst smd;
  if (loadDoc(json, len, doc, smd, nullptr, nullptr, 0) != SiriStatus::Ok) return -1;
  int n = 0;
  for (JsonObjectConst visit : smd["MonitoredStopVisit"].as<JsonArrayConst>()) {
    JsonObjectConst mvj = visit["MonitoredVehicleJourney"];
    JsonObjectConst call = mvj["MonitoredCall"];
    char line[12];
    lineIdFromRef(sval(mvj["LineRef"]), line, sizeof(line));
    if (!line[0]) continue;
    if (stopName && !stopName[0]) {
      copyStr(stopName, sval(call["StopPointName"]), stopNameLen);
      fixText(stopName);
    }
    int i = 0;
    while (i < n && strcmp(out[i].line, line) != 0) i++;
    if (i == n) {
      if (n >= maxLines) continue;
      memset(&out[n], 0, sizeof(out[n]));
      copyStr(out[n].line, line, sizeof(out[n].line));
      n++;
    }
    char dest[TXT_LEN];
    const char* ds = sval(mvj["DestinationName"]);
    if (!ds || !*ds) ds = sval(call["DestinationDisplay"]);
    copyStr(dest, ds ? ds : "", sizeof(dest));
    fixText(dest);
    if (!dest[0]) continue;
    LineSummary& ls = out[i];
    bool known = false;
    for (int k = 0; k < ls.ndest; k++)
      if (strcmp(ls.dest[k], dest) == 0) known = true;
    if (!known && ls.ndest < LINE_DESTS) copyStr(ls.dest[ls.ndest++], dest, TXT_LEN);
  }
  return n;
}

// ============================================================================
//  Préparation de l'affichage
// ============================================================================
void buildView(const StopData& d, const char* dirFilter, const char* selectedKey, int64_t now,
               View& v) {
  memset(&v, 0, sizeof(v));
  const bool filtered = dirFilter && *dirFilter;

  const Departure* cand[MAX_DEPARTURES];
  int nc = 0;
  for (int i = 0; i < d.count; i++) {
    const Departure& e = d.dep[i];
    int64_t late = now - e.when;
    // Départ passé (on garde un peu plus longtemps un train encore à quai)
    if (late > 30 && !((e.flags & DEP_AT_STOP) && late < 90)) continue;
    if (filtered && !matchesFilter(e.dest, dirFilter) && !matchesFilter(e.dir, dirFilter)) continue;
    cand[nc++] = &e;
  }
  if (nc == 0) return;

  const char* key = nullptr;
  if (filtered) {
    v.groupCount = 1;
    copyStr(v.groupKey[0], dirFilter, TXT_LEN);
  } else {
    // Sens distincts, triés par ordre alphabétique (ordre stable à l'écran)
    for (int i = 0; i < nc; i++) {
      bool known = false;
      for (int g = 0; g < v.groupCount; g++)
        if (strcmp(v.groupKey[g], cand[i]->dir) == 0) known = true;
      if (!known && v.groupCount < MAX_GROUPS) copyStr(v.groupKey[v.groupCount++], cand[i]->dir, TXT_LEN);
    }
    for (int a = 0; a < v.groupCount; a++)
      for (int b = a + 1; b < v.groupCount; b++) {
        char na[TXT_LEN * 2], nb[TXT_LEN * 2];
        normalizeAscii(v.groupKey[a], na, sizeof(na));
        normalizeAscii(v.groupKey[b], nb, sizeof(nb));
        if (strcmp(na, nb) > 0) {
          char tmp[TXT_LEN];
          memcpy(tmp, v.groupKey[a], TXT_LEN);
          memcpy(v.groupKey[a], v.groupKey[b], TXT_LEN);
          memcpy(v.groupKey[b], tmp, TXT_LEN);
        }
      }
    v.groupIndex = 0;
    if (selectedKey && *selectedKey)
      for (int g = 0; g < v.groupCount; g++)
        if (strcmp(v.groupKey[g], selectedKey) == 0) v.groupIndex = (uint8_t)g;
    key = v.groupKey[v.groupIndex];
  }

  for (int i = 0; i < nc && v.rows < 2; i++) {
    if (key && strcmp(cand[i]->dir, key) != 0) continue;
    Row& r = v.row[v.rows++];
    r.when = cand[i]->when;
    r.flags = cand[i]->flags;
    copyStr(r.dest, cand[i]->dest, sizeof(r.dest));
    copyStr(r.platform, cand[i]->platform, sizeof(r.platform));
  }
}

int buildDirections(const StopData& d, int64_t now, DirGroup* out, int maxN) {
  int n = 0;
  for (int i = 0; i < d.count; i++) {
    const Departure& e = d.dep[i];
    const int64_t late = now - e.when;
    if (late > 30 && !((e.flags & DEP_AT_STOP) && late < 90)) continue;  // déjà parti
    int g = 0;
    while (g < n && strcmp(out[g].dest, e.dest) != 0) g++;
    if (g == n) {
      if (n >= maxN) continue;
      memset(&out[n], 0, sizeof(out[n]));
      copyStr(out[n].dest, e.dest, sizeof(out[n].dest));
      copyStr(out[n].platform, e.platform, sizeof(out[n].platform));
      n++;
    }
    DirGroup& gr = out[g];
    if (gr.n < DIR_TIMES) {
      gr.when[gr.n] = e.when;
      gr.flags[gr.n] = e.flags;
      gr.n++;
    }
  }
  // Tri par destination (sans accents) : ordre stable d'une mise à jour à l'autre.
  // (Le sens « Aller / Retour » n'est pas fiable sur le RER : l'appelant peut
  // ensuite ranger les destinations selon leur place sur la ligne.)
  for (int a = 0; a < n; a++)
    for (int b = a + 1; b < n; b++) {
      char da[TXT_LEN * 2], db[TXT_LEN * 2];
      normalizeAscii(out[a].dest, da, sizeof(da));
      normalizeAscii(out[b].dest, db, sizeof(db));
      if (strcmp(da, db) > 0) {
        DirGroup tmp = out[a];
        out[a] = out[b];
        out[b] = tmp;
      }
    }
  return n;
}
