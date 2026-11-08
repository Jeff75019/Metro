// Réglages de l'utilisateur, stockés en mémoire flash (NVS / Preferences)
// sous forme de JSON. Modifiables depuis la page web de configuration.
#pragma once
#include <Arduino.h>

#include "config.h"

struct Favorite {
  char label[24];     // nom affiché à la place du nom de l'arrêt (optionnel)
  char stopRef[48];   // ex. "STIF:StopArea:SP:71517:"
  char stopName[40];  // nom de l'arrêt (affiché avant la 1re réponse)
  char line[12];      // ex. "C01371" ; vide = toutes les lignes
  char badge[8];      // texte du pictogramme si la ligne est inconnue (bus...)
  uint32_t color;     // couleurs du pictogramme (0xRRGGBB) pour ces lignes
  uint32_t textColor;
  uint8_t mode;       // LineMode
  char filter[96];    // destinations gardées, ex. "Défense" ou "Chessy|Torcy" (optionnel)
};

struct Station {
  char id[40];    // ex. "stop_area:SNCF:87686006"
  char name[32];  // nom court affiché, ex. "Gare de Lyon"
};

struct WifiCred {
  char ssid[33];
  char pass[65];
};

enum class SleepMode : uint8_t {
  Deep = 0,      // veille profonde (sur batterie) : réveil par BOOT ou secousse
  ScreenOff = 1, // écran éteint, réveil au toucher (sur secteur)
  AlwaysOn = 2,  // toujours allumé (sur secteur)
};

struct Settings {
  WifiCred wifi[MAX_WIFI];
  char apiKey[128];
  Favorite fav[MAX_FAVORITES];
  uint8_t favCount;
  uint16_t refreshS;    // intervalle de rafraîchissement
  uint16_t awakeS;      // durée d'affichage avant veille
  uint16_t dailyQuota;  // nombre max d'appels API par jour
  uint8_t brightness;   // 0..255
  SleepMode sleepMode;
  bool motionWake;      // réveil par secousse (IMU)
  bool flip;            // écran retourné de 180°
  // Trains grandes lignes (API SNCF)
  char sncfKey[64];
  Station stations[MAX_STATIONS];
  uint8_t stationCount;
  bool mainlineOnly;    // écarter Transilien, RER et autocars
  uint16_t sncfQuota;   // nombre max d'appels API SNCF par jour
};

extern Settings settings;

void settingsDefaults(Settings& s);
void settingsLoad();
bool settingsSave();
void settingsErase();
bool metroReady();     // clé PRIM + au moins un arrêt
bool trainsReady();    // clé SNCF + au moins une gare
bool settingsReady();  // WiFi + (métro ou trains) configurés

// Gares parisiennes proposées par défaut
struct ParisStation {
  const char* id;
  const char* name;
};
extern const ParisStation PARIS_STATIONS[];
extern const int PARIS_STATION_COUNT;

// Conversion JSON (pour la page web et le stockage).
// hideSecrets : les mots de passe et la clé API ne sont pas renvoyés en clair.
String settingsToJson(const Settings& s, bool hideSecrets);
// Fusionne le JSON dans s. Un mot de passe ou une clé vide = conserver l'ancien.
bool settingsFromJson(Settings& s, const char* json, String& err);

// "IDFM:12345" -> "STIF:StopPoint:Q:12345:", "71517" -> "STIF:StopArea:SP:71517:"...
String normalizeStopRef(const String& in);
