#include "traffic.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lines.h"
#include "siri.h"  // copyStr
#include "sncf.h"  // parseNavitiaTime

void trafficFilter(JsonDocument& f) {
  f["lastUpdatedDate"] = true;
  JsonObject d = f["disruptions"][0].to<JsonObject>();
  for (const char* k : {"id", "cause", "severity", "title", "shortMessage", "message"}) d[k] = true;
  d["applicationPeriods"][0]["begin"] = true;
  d["applicationPeriods"][0]["end"] = true;
  JsonObject l = f["lines"][0].to<JsonObject>();
  l["id"] = true;
  l["impactedObjects"][0]["disruptionIds"] = true;
}

// ----------------------------------------------------------------------------
//  Texte : UTF-8 limité aux caractères des polices de l'afficheur
// ----------------------------------------------------------------------------
static bool inFont(uint32_t c) {
  return (c >= 0x20 && c < 0x7F) || (c >= 0xA0 && c <= 0xFF) || c == 0x152 || c == 0x153 || c == 0x2013 ||
         c == 0x2014 || c == 0x2019 || c == 0x2026 || c == 0x20AC;
}

struct TextOut {
  char* p;
  size_t n, len;
  bool space;   // espace en attente
  int newline;  // retours à la ligne en attente (2 au plus : une ligne vide)
  int blank;    // retours à la ligne consécutifs déjà écrits
  void raw(const char* s, size_t k) {
    if (len + k + 1 > n) return;
    memcpy(p + len, s, k);
    len += k;
    p[len] = 0;
  }
  void flushPending() {
    if (newline && len) {
      for (; blank < newline; blank++) raw("\n", 1);
    } else if (space && len && blank == 0) {
      raw(" ", 1);
    }
    newline = 0;
    space = false;
  }
  void cp(uint32_t c) {
    if (c == '\n') {
      if (newline < 2) newline++;
      return;
    }
    if (c == ' ' || c == '\t' || c == '\r' || c == 0xA0 || c == 0x202F || c == 0x2009) {
      space = true;
      return;
    }
    if (c == 0x200B || c == 0xFEFF) return;  // invisibles
    // équivalents présents dans les polices
    const char* sub = nullptr;
    if (c == 0x2018 || c == 0x2032) c = 0x2019;
    else if (c == 0x201C || c == 0x201D || c == 0x201E) c = '"';
    else if (c >= 0x2010 && c <= 0x2012) c = '-';
    else if (c == 0x2022 || c == 0x25CF || c == 0x25AA) c = 0xB7;
    else if (c == 0x2192) sub = "->";
    else if (c == 0x2190) sub = "<-";
    else if (c == 0x2194) sub = "<->";
    if (!sub && !inFont(c)) return;
    flushPending();
    blank = 0;
    if (sub) return raw(sub, strlen(sub));
    char b[4];
    size_t k;
    if (c < 0x80) {
      b[0] = (char)c;
      k = 1;
    } else if (c < 0x800) {
      b[0] = (char)(0xC0 | (c >> 6));
      b[1] = (char)(0x80 | (c & 0x3F));
      k = 2;
    } else {
      b[0] = (char)(0xE0 | (c >> 12));
      b[1] = (char)(0x80 | ((c >> 6) & 0x3F));
      b[2] = (char)(0x80 | (c & 0x3F));
      k = 3;
    }
    raw(b, k);
  }
};

// Lit un caractère UTF-8 ; renvoie le nombre d'octets consommés
static int utf8(const char* s, uint32_t* c) {
  const unsigned char* u = (const unsigned char*)s;
  if (u[0] < 0x80) {
    *c = u[0];
    return 1;
  }
  if ((u[0] & 0xE0) == 0xC0 && (u[1] & 0xC0) == 0x80) {
    *c = ((u[0] & 0x1F) << 6) | (u[1] & 0x3F);
    return 2;
  }
  if ((u[0] & 0xF0) == 0xE0 && (u[1] & 0xC0) == 0x80 && (u[2] & 0xC0) == 0x80) {
    *c = ((u[0] & 0x0F) << 12) | ((u[1] & 0x3F) << 6) | (u[2] & 0x3F);
    return 3;
  }
  if ((u[0] & 0xF8) == 0xF0 && (u[1] & 0xC0) == 0x80 && (u[2] & 0xC0) == 0x80 && (u[3] & 0xC0) == 0x80) {
    *c = ((u[0] & 0x07) << 18) | ((u[1] & 0x3F) << 12) | ((u[2] & 0x3F) << 6) | (u[3] & 0x3F);
    return 4;
  }
  *c = '?';
  return 1;
}

static uint32_t namedEntity(const char* name) {
  static const struct {
    const char* n;
    uint32_t c;
  } E[] = {{"amp", '&'},       {"lt", '<'},         {"gt", '>'},         {"quot", '"'},       {"apos", '\''},
           {"nbsp", ' '},      {"eacute", 0xE9},    {"egrave", 0xE8},    {"ecirc", 0xEA},     {"euml", 0xEB},
           {"agrave", 0xE0},   {"acirc", 0xE2},     {"ccedil", 0xE7},    {"icirc", 0xEE},     {"iuml", 0xEF},
           {"ocirc", 0xF4},    {"ucirc", 0xFB},     {"ugrave", 0xF9},    {"Eacute", 0xC9},    {"Egrave", 0xC8},
           {"Agrave", 0xC0},   {"Ccedil", 0xC7},    {"laquo", 0xAB},     {"raquo", 0xBB},     {"rsquo", 0x2019},
           {"lsquo", 0x2019},  {"ldquo", '"'},      {"rdquo", '"'},      {"hellip", 0x2026},  {"ndash", 0x2013},
           {"mdash", 0x2014},  {"euro", 0x20AC},    {"deg", 0xB0},       {"oelig", 0x153},    {"bull", 0xB7},
           {"middot", 0xB7},   {"rarr", 0x2192},    {"larr", 0x2190},    {"harr", 0x2194}};
  for (auto& e : E)
    if (!strcmp(e.n, name)) return e.c;
  return 0;
}

void htmlToText(const char* html, char* out, size_t n, bool tags) {
  if (!n) return;
  out[0] = 0;
  TextOut t{out, n, 0, false, 0, 0};
  const char* s = html ? html : "";
  while (*s) {
    if (*s == '<' && tags) {  // balise
      const char* e = strchr(s, '>');
      if (!e) break;
      char tag[8] = "";
      size_t k = 0;
      const char* q = s + 1;
      if (*q == '/') q++;
      while (q < e && k < sizeof(tag) - 1 && isalnum((unsigned char)*q)) tag[k++] = (char)tolower((unsigned char)*q++);
      tag[k] = 0;
      if (!strcmp(tag, "br") || !strcmp(tag, "p") || !strcmp(tag, "div") || !strcmp(tag, "tr") || !strcmp(tag, "ul") ||
          (tag[0] == 'h' && isdigit((unsigned char)tag[1])))
        t.cp('\n');
      else if (!strcmp(tag, "li") && s[1] != '/') {
        t.cp('\n');
        t.cp(0xB7);
        t.cp(' ');
      }
      s = e + 1;
      continue;
    }
    if (*s == '&') {  // entité
      const char* e = strchr(s, ';');
      if (e && e - s <= 10) {
        uint32_t c = 0;
        if (s[1] == '#') c = (s[2] == 'x' || s[2] == 'X') ? strtoul(s + 3, nullptr, 16) : strtoul(s + 2, nullptr, 10);
        else {
          char name[12];
          size_t k = (size_t)(e - s - 1);
          memcpy(name, s + 1, k);
          name[k] = 0;
          c = namedEntity(name);
        }
        if (c) {
          t.cp(c);
          s = e + 1;
          continue;
        }
      }
    }
    uint32_t c;
    s += utf8(s, &c);
    t.cp(c);
  }
}

// ----------------------------------------------------------------------------
//  Perturbations
// ----------------------------------------------------------------------------
static uint8_t levelOf(const char* sev) {
  if (!sev) return TRAFFIC_INFO;
  if (!strcmp(sev, "BLOQUANTE")) return TRAFFIC_BLOCKING;
  if (!strcmp(sev, "PERTURBEE")) return TRAFFIC_DISRUPTED;
  return TRAFFIC_INFO;
}

// « Métro 4 : Travaux… » -> « Travaux… » (le pictogramme de ligne est déjà affiché)
static void cleanTitle(char* t) {
  static const char* prefixes[] = {"Métro", "Metro", "RER", "Ligne", "Tramway", "Tram", "T"};
  bool known = false;
  for (const char* p : prefixes)
    if (!strncmp(t, p, strlen(p))) known = true;
  if (!known) return;
  const char* cut = nullptr;
  for (const char* sep : {" : ", ": ", " - "}) {
    const char* f = strstr(t, sep);
    if (f && f - t <= 16 && (!cut || f < cut)) cut = f;
  }
  if (!cut) return;
  cut += (cut[1] == ':' || cut[1] == '-') ? 3 : 2;
  memmove(t, cut, strlen(cut) + 1);
  if (t[0] >= 'a' && t[0] <= 'z') t[0] = (char)(t[0] - 32);
}

static int lineRank(const char* id) {
  const LineInfo* li = findLine(id);
  return li ? (int)(li - findLine("C01371")) : 1000;  // ordre de la table des lignes
}

static bool ourLine(const char* id) {
  const LineInfo* li = findLine(id);
  return li && (li->mode == LineMode::Metro || li->mode == LineMode::Rer || li->mode == LineMode::Tram);
}

// Avant b dans la liste ? En cours d'abord (les plus graves), puis à venir (les plus proches)
static bool before(const TrafficItem& a, const TrafficItem& b) {
  if (a.active != b.active) return a.active;
  if (a.active) {
    if (a.level != b.level) return a.level > b.level;
    return lineRank(a.line) < lineRank(b.line);
  }
  return a.begin < b.begin;
}

int parseTraffic(JsonDocument& doc, DeserializationError e, int64_t now, int64_t horizonS, TrafficData& out) {
  out.count = 0;
  out.updatedAt = 0;
  out.textLen = 0;
  if (out.text && out.textCap) out.text[0] = 0;
  if (e || !doc["disruptions"].is<JsonArrayConst>()) return -1;
  parseIsoTime(doc["lastUpdatedDate"] | "", &out.updatedAt);

  JsonArrayConst dis = doc["disruptions"];
  static TrafficItem cand[96];  // candidats avant le tri (plusieurs lignes par perturbation possibles)
  static uint16_t candIdx[96];
  int nc = 0;
  for (JsonObjectConst line : doc["lines"].as<JsonArrayConst>()) {
    const char* lid = line["id"] | "";
    const char* colon = strrchr(lid, ':');
    char id[12];
    copyStr(id, colon ? colon + 1 : lid, sizeof(id));
    if (!ourLine(id)) continue;
    // Perturbations citées pour la ligne, ses arrêts ou ses tronçons (sans doublon)
    const char* seen[48];
    int ns = 0;
    for (JsonObjectConst o : line["impactedObjects"].as<JsonArrayConst>())
      for (JsonVariantConst v : o["disruptionIds"].as<JsonArrayConst>()) {
        const char* did = v | "";
        bool dup = false;
        for (int k = 0; k < ns; k++)
          if (!strcmp(seen[k], did)) dup = true;
        if (dup || ns >= 48) continue;
        seen[ns++] = did;
        // La perturbation, en cours ou proche ?
        uint16_t idx = 0;
        for (JsonObjectConst d : dis) {
          if (strcmp(d["id"] | "", did) != 0) {
            idx++;
            continue;
          }
          bool active = false;
          int64_t b = 0, en = 0, nextB = 0, nextE = 0;
          for (JsonObjectConst p : d["applicationPeriods"].as<JsonArrayConst>()) {
            int64_t pb, pe;
            if (!parseNavitiaTime(p["begin"] | "", &pb) || !parseNavitiaTime(p["end"] | "", &pe)) continue;
            if (pb <= now && now <= pe) {
              active = true;
              b = pb;
              en = pe;
              break;
            }
            if (pb > now && (!nextB || pb < nextB)) {
              nextB = pb;
              nextE = pe;
            }
          }
          if (!active && (!nextB || nextB - now > horizonS)) break;
          if (nc >= (int)(sizeof(cand) / sizeof(cand[0]))) break;
          TrafficItem& it = cand[nc];
          memset(&it, 0, sizeof(it));
          copyStr(it.line, id, sizeof(it.line));
          it.level = levelOf(d["severity"] | "");
          it.works = !strcmp(d["cause"] | "", "TRAVAUX");
          it.active = active;
          it.begin = active ? b : nextB;
          it.end = active ? en : nextE;
          htmlToText(d["title"] | "", it.title, sizeof(it.title), false);  // texte simple (« A <-> B »)
          cleanTitle(it.title);
          htmlToText(d["shortMessage"] | "", it.status, sizeof(it.status), false);
          if (!it.status[0])
            copyStr(it.status,
                    it.works ? "Travaux"
                    : it.level == TRAFFIC_BLOCKING ? "Perturbation majeure"
                    : it.level == TRAFFIC_DISRUPTED ? "Trafic perturbé"
                                                    : "Information",
                    sizeof(it.status));
          candIdx[nc++] = idx;
          break;
        }
      }
  }
  // Tri, puis les TRAFFIC_MAX premiers avec leur message complet
  for (int a = 1; a < nc; a++)
    for (int b = a; b > 0 && before(cand[b], cand[b - 1]); b--) {
      TrafficItem t = cand[b];
      cand[b] = cand[b - 1];
      cand[b - 1] = t;
      uint16_t ti = candIdx[b];
      candIdx[b] = candIdx[b - 1];
      candIdx[b - 1] = ti;
    }
  for (int i = 0; i < nc && out.count < TRAFFIC_MAX; i++) {
    TrafficItem& it = out.item[out.count++];
    it = cand[i];
    it.text = UINT32_MAX;  // pas de place : pas de message
    if (out.text && out.textLen + 2 < out.textCap) {
      it.text = (uint32_t)out.textLen;
      htmlToText(dis[candIdx[i]]["message"] | "", out.text + out.textLen, out.textCap - out.textLen);
      out.textLen += strlen(out.text + out.textLen) + 1;
    }
  }
  return out.count;
}

uint8_t trafficLevel(const TrafficData& d, const char* lineId) {
  uint8_t lvl = 0;
  for (int i = 0; i < d.count; i++)
    if (d.item[i].active && !strcmp(d.item[i].line, lineId) && d.item[i].level > lvl) lvl = d.item[i].level;
  return lvl;
}
