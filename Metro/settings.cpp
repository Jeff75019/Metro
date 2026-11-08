#include "settings.h"

#include <ArduinoJson.h>
#include <Preferences.h>

#include "lines.h"
#include "siri.h"  // copyStr

Settings settings;

static const char* NS = "metro";

// Codes UIC des grandes gares (plusieurs codes = halls ou niveaux d'une même gare)
const ParisStation PARIS_STATIONS[] = {
    {"87686006,87686030", "Gare de Lyon"},          {"87271007,87271031,87271023", "Gare du Nord"},
    {"87113001", "Gare de l'Est"},                  {"87391003,87391102,87391011", "Montparnasse"},
    {"87384008", "Saint-Lazare"},                   {"87547000,87547026", "Austerlitz"},
    {"87686667", "Bercy"},
};
const int PARIS_STATION_COUNT = sizeof(PARIS_STATIONS) / sizeof(PARIS_STATIONS[0]);

void settingsDefaults(Settings& s) {
  memset(&s, 0, sizeof(s));
  s.refreshS = DEFAULT_REFRESH_S;
  s.awakeS = DEFAULT_AWAKE_S;
  s.dailyQuota = DEFAULT_DAILY_QUOTA;
  s.brightness = DEFAULT_BRIGHTNESS;
  s.sleepMode = SleepMode::Deep;
  s.motionWake = false;
  s.flip = false;
  s.mainlineOnly = true;
  s.sncfQuota = DEFAULT_SNCF_QUOTA;
  for (int i = 0; i < PARIS_STATION_COUNT && i < MAX_STATIONS; i++) {
    copyStr(s.stations[i].id, PARIS_STATIONS[i].id, sizeof(s.stations[i].id));
    copyStr(s.stations[i].name, PARIS_STATIONS[i].name, sizeof(s.stations[i].name));
    s.stationCount++;
  }
}

void settingsLoad() {
  settingsDefaults(settings);
  Preferences p;
  if (!p.begin(NS, true)) return;  // premier démarrage : rien d'enregistré
  String json = p.getString("cfg", "");
  p.end();
  if (json.length()) {
    String err;
    if (!settingsFromJson(settings, json.c_str(), err)) log_e("Réglages illisibles : %s", err.c_str());
  }
}

bool settingsSave() {
  Preferences p;
  if (!p.begin(NS, false)) return false;
  String json = settingsToJson(settings, false);
  bool ok = p.putString("cfg", json) == json.length();
  p.end();
  return ok;
}

void settingsErase() {
  Preferences p;
  if (p.begin(NS, false)) {
    p.clear();
    p.end();
  }
  settingsDefaults(settings);
}

bool metroReady() { return settings.apiKey[0] && settings.favCount > 0; }
bool trainsReady() {
  return settings.stationCount > 0 && (settings.trainSource == TrainSource::TrainNomade || settings.sncfKey[0]);
}
bool settingsReady() { return settings.wifi[0].ssid[0] && (metroReady() || trainsReady()); }

String normalizeStopRef(const String& raw) {
  String s = raw;
  s.trim();
  if (s.isEmpty()) return s;
  if (s.startsWith("STIF:")) {
    if (!s.endsWith(":")) s += ":";
    return s;
  }
  if (s.startsWith("IDFM:monomodalStopPlace:")) return "STIF:StopArea:SP:" + s.substring(24) + ":";
  if (s.startsWith("IDFM:")) return "STIF:StopPoint:Q:" + s.substring(5) + ":";
  bool digits = true;
  for (size_t i = 0; i < s.length(); i++)
    if (!isDigit(s[i])) digits = false;
  if (digits) return "STIF:StopArea:SP:" + s + ":";  // identifiant de "zone d'arrêt"
  return s;
}

String settingsToJson(const Settings& s, bool hideSecrets) {
  JsonDocument doc;
  JsonArray w = doc["wifi"].to<JsonArray>();
  for (int i = 0; i < MAX_WIFI; i++) {
    if (!s.wifi[i].ssid[0]) continue;
    JsonObject o = w.add<JsonObject>();
    o["ssid"] = s.wifi[i].ssid;
    if (hideSecrets) o["hasPass"] = s.wifi[i].pass[0] != 0;
    else o["pass"] = s.wifi[i].pass;
  }
  if (hideSecrets) {
    size_t n = strlen(s.apiKey);
    doc["apiKeySet"] = n > 0;
    if (n > 4) doc["apiKeyHint"] = String("…") + (s.apiKey + n - 4);
    n = strlen(s.sncfKey);
    doc["sncfKeySet"] = n > 0;
    if (n > 4) doc["sncfKeyHint"] = String("…") + (s.sncfKey + n - 4);
  } else {
    doc["apiKey"] = s.apiKey;
    doc["sncfKey"] = s.sncfKey;
  }
  JsonArray st = doc["stations"].to<JsonArray>();
  for (int i = 0; i < s.stationCount; i++) {
    JsonObject o = st.add<JsonObject>();
    o["id"] = s.stations[i].id;
    o["name"] = s.stations[i].name;
  }
  doc["mainlineOnly"] = s.mainlineOnly;
  doc["trainSource"] = s.trainSource == TrainSource::SncfApi ? "sncf" : "trainnomade";
  doc["sncfQuota"] = s.sncfQuota;
  JsonArray f = doc["favorites"].to<JsonArray>();
  for (int i = 0; i < s.favCount; i++) {
    const Favorite& fv = s.fav[i];
    JsonObject o = f.add<JsonObject>();
    o["label"] = fv.label;
    o["stopRef"] = fv.stopRef;
    o["stopName"] = fv.stopName;
    o["line"] = fv.line;
    o["badge"] = fv.badge;
    o["filter"] = fv.filter;
    if (fv.badge[0]) {
      char col[12];
      snprintf(col, sizeof(col), "#%06X", (unsigned)(fv.color & 0xFFFFFF));
      o["color"] = col;
      snprintf(col, sizeof(col), "#%06X", (unsigned)(fv.textColor & 0xFFFFFF));
      o["text"] = col;
      o["mode"] = lineModeName((LineMode)fv.mode);
    }
  }
  doc["refreshS"] = s.refreshS;
  doc["awakeS"] = s.awakeS;
  doc["dailyQuota"] = s.dailyQuota;
  doc["brightness"] = s.brightness;
  doc["sleepMode"] = (int)s.sleepMode;
  doc["motionWake"] = s.motionWake;
  doc["flip"] = s.flip;
  String out;
  serializeJson(doc, out);
  return out;
}

static uint16_t clampU16(long v, long lo, long hi) { return (uint16_t)(v < lo ? lo : (v > hi ? hi : v)); }

bool settingsFromJson(Settings& s, const char* json, String& err) {
  JsonDocument doc;
  DeserializationError e = deserializeJson(doc, json);
  if (e) {
    err = String("JSON invalide : ") + e.c_str();
    return false;
  }

  if (doc["wifi"].is<JsonArrayConst>()) {
    WifiCred old[MAX_WIFI];
    memcpy(old, s.wifi, sizeof(old));
    memset(s.wifi, 0, sizeof(s.wifi));
    int n = 0;
    for (JsonObjectConst o : doc["wifi"].as<JsonArrayConst>()) {
      const char* ssid = o["ssid"] | "";
      if (!*ssid || n >= MAX_WIFI) continue;
      copyStr(s.wifi[n].ssid, ssid, sizeof(s.wifi[n].ssid));
      const char* pass = o["pass"] | "";
      if (*pass) {
        copyStr(s.wifi[n].pass, pass, sizeof(s.wifi[n].pass));
      } else if (!o["clearPass"].as<bool>()) {
        for (auto& c : old)  // mot de passe non renvoyé par la page : on garde l'ancien
          if (strcmp(c.ssid, ssid) == 0) copyStr(s.wifi[n].pass, c.pass, sizeof(s.wifi[n].pass));
      }
      n++;
    }
  }

  const char* key = doc["apiKey"] | "";
  if (*key) {
    String k = key;
    k.trim();
    copyStr(s.apiKey, k.c_str(), sizeof(s.apiKey));
  }

  if (doc["favorites"].is<JsonArrayConst>()) {
    s.favCount = 0;
    memset(s.fav, 0, sizeof(s.fav));
    for (JsonObjectConst o : doc["favorites"].as<JsonArrayConst>()) {
      if (s.favCount >= MAX_FAVORITES) break;
      String ref = normalizeStopRef(String(o["stopRef"] | ""));
      if (ref.isEmpty()) continue;
      Favorite& fv = s.fav[s.favCount++];
      copyStr(fv.stopRef, ref.c_str(), sizeof(fv.stopRef));
      copyStr(fv.label, o["label"] | "", sizeof(fv.label));
      copyStr(fv.stopName, o["stopName"] | "", sizeof(fv.stopName));
      char line[16];
      lineIdFromRef(o["line"] | "", line, sizeof(line));
      copyStr(fv.line, line, sizeof(fv.line));
      copyStr(fv.badge, o["badge"] | "", sizeof(fv.badge));
      copyStr(fv.filter, o["filter"] | "", sizeof(fv.filter));
      fv.color = parseHexColor(o["color"] | "", 0x5A6E8C);
      fv.textColor = parseHexColor(o["text"] | "", 0xFFFFFF);
      fv.mode = (uint8_t)lineModeFromName(o["mode"] | "bus");
    }
  }

  const char* skey = doc["sncfKey"] | "";
  if (*skey) {
    String k = skey;
    k.trim();
    copyStr(s.sncfKey, k.c_str(), sizeof(s.sncfKey));
  }
  if (doc["stations"].is<JsonArrayConst>()) {
    s.stationCount = 0;
    memset(s.stations, 0, sizeof(s.stations));
    for (JsonObjectConst o : doc["stations"].as<JsonArrayConst>()) {
      const char* id = o["id"] | "";
      if (!*id || s.stationCount >= MAX_STATIONS) continue;
      Station& st = s.stations[s.stationCount++];
      copyStr(st.id, id, sizeof(st.id));
      copyStr(st.name, o["name"] | id, sizeof(st.name));
    }
  }
  if (!doc["mainlineOnly"].isNull()) s.mainlineOnly = doc["mainlineOnly"].as<bool>();
  if (!doc["trainSource"].isNull())
    s.trainSource = strcmp(doc["trainSource"] | "", "sncf") == 0 ? TrainSource::SncfApi : TrainSource::TrainNomade;
  if (!doc["sncfQuota"].isNull()) s.sncfQuota = clampU16(doc["sncfQuota"].as<long>(), 10, 60000);
  if (!doc["refreshS"].isNull()) s.refreshS = clampU16(doc["refreshS"].as<long>(), 10, 3600);
  if (!doc["awakeS"].isNull()) s.awakeS = clampU16(doc["awakeS"].as<long>(), 10, 3600);
  if (!doc["dailyQuota"].isNull()) s.dailyQuota = clampU16(doc["dailyQuota"].as<long>(), 10, 60000);
  if (!doc["brightness"].isNull()) s.brightness = (uint8_t)clampU16(doc["brightness"].as<long>(), 5, 255);
  if (!doc["sleepMode"].isNull()) s.sleepMode = (SleepMode)clampU16(doc["sleepMode"].as<long>(), 0, 2);
  if (!doc["motionWake"].isNull()) s.motionWake = doc["motionWake"].as<bool>();
  if (!doc["flip"].isNull()) s.flip = doc["flip"].as<bool>();
  return true;
}
