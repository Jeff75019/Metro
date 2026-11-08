// Info trafic du métro, du RER et du tramway : API PRIM « Messages Info Trafic »
// (requête globale, disruptions_bulk/disruptions/v2). La réponse couvre toute
// l'Île-de-France (plus d'1 Mo, surtout des bus) : elle est lue au fil de l'eau
// avec un filtre, et seules les perturbations des lignes de l'afficheur sont gardées.
//
// C++ "pur" (pas de dépendance à l'ESP32) : compilé et testé aussi sur PC.
#pragma once
#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>

#define TRAFFIC_MAX 32

enum : uint8_t {
  TRAFFIC_INFO = 1,      // information
  TRAFFIC_DISRUPTED = 2, // perturbé
  TRAFFIC_BLOCKING = 3,  // bloquant (trafic interrompu, arrêt fermé…)
};

struct TrafficItem {
  char line[12];      // identifiant IDFM, ex. "C01374"
  uint8_t level;      // TRAFFIC_INFO / DISRUPTED / BLOCKING
  bool works;         // travaux
  bool active;        // en cours (sinon : à venir dans les prochaines heures)
  int64_t begin, end; // période en cours, ou prochaine (epoch UTC)
  char title[96];     // sans le préfixe « Métro 4 : »
  char status[40];    // message court : « Trafic interrompu », « Arrêt non desservi »…
  uint32_t text;      // position du message complet (texte brut) dans TrafficData::text, UINT32_MAX : aucun
};

struct TrafficData {
  int64_t updatedAt;  // mise à jour des données côté IDFM (epoch UTC, 0 : inconnue)
  uint8_t count;
  TrafficItem item[TRAFFIC_MAX];  // en cours d'abord (les plus graves), puis à venir
  char* text;         // messages, alloués par l'appelant (PSRAM sur la carte)
  size_t textCap, textLen;
};

void trafficFilter(JsonDocument& filter);

// Garde les perturbations des lignes de métro, RER et tram (table lines.cpp) en cours
// à `now`, ou qui commencent dans les `horizonS` secondes. Renvoie le nombre gardé,
// -1 si la réponse est illisible. Les heures du fichier sont celles de Paris (TZ réglé).
int parseTraffic(JsonDocument& doc, DeserializationError e, int64_t now, int64_t horizonS, TrafficData& out);

// Gravité la plus forte en cours sur une ligne (0 : rien)
uint8_t trafficLevel(const TrafficData& d, const char* lineId);

// HTML -> texte : balises retirées (<br>, </p> -> retour à la ligne ; tags = false
// pour un texte simple, où « < » est un caractère), entités décodées (&#233;
// &amp;…), caractères absents des polices remplacés.
void htmlToText(const char* html, char* out, size_t n, bool tags = true);
