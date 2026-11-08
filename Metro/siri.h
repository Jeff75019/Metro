// Analyse des réponses SIRI Lite "StopMonitoring" de l'API PRIM
// (Île-de-France Mobilités) et préparation de ce qu'il faut afficher.
//
// Ce module est du C++ "pur" (pas de dépendance à l'ESP32) : il est compilé et
// testé aussi sur PC (voir test/).
#pragma once
#include <stddef.h>
#include <stdint.h>

#define MAX_DEPARTURES 20
#define MAX_GROUPS 4
#define TXT_LEN 40
#define LINE_DESTS 8  // destinations retenues par ligne (configuration)

enum : uint8_t {
  DEP_REALTIME = 1,   // heure "Expected" (temps réel) et non théorique
  DEP_AT_STOP = 2,    // véhicule à quai
  DEP_CANCELLED = 4,  // supprimé
  DEP_DELAYED = 8,    // retardé
};

struct Departure {
  int64_t when;            // heure de départ (epoch UTC, secondes)
  char dest[TXT_LEN];      // destination affichée
  char dir[TXT_LEN];       // clé de "sens" (sert à regrouper par quai)
  char line[12];           // identifiant ligne, ex. "C01371"
  char platform[6];        // voie (RER/Transilien), souvent vide
  uint8_t flags;
};

struct StopData {
  char stopName[TXT_LEN];
  int64_t responseTime;    // horodatage de la réponse serveur (epoch UTC)
  int64_t fetchedAt;       // horodatage local de la récupération
  uint8_t count;
  Departure dep[MAX_DEPARTURES];  // triés par heure croissante
};

enum class SiriStatus : uint8_t { Ok, BadJson, NoDelivery, ServerError };

// Analyse le JSON. lineFilter : "C01371" (ou "STIF:Line::C01371:") pour ne
// garder qu'une ligne, ou "" / nullptr pour tout garder.
// En cas d'erreur, un message lisible est copié dans err.
SiriStatus parseStopMonitoring(const char* json, size_t len, const char* lineFilter,
                               StopData& out, char* err, size_t errLen);

// Résumé des lignes présentes à un arrêt (pour la page de configuration).
struct LineSummary {
  char line[12];
  uint8_t ndest;
  char dest[LINE_DESTS][TXT_LEN];
};
int summarizeLines(const char* json, size_t len, LineSummary* out, int maxLines,
                   char* stopName, size_t stopNameLen);

// ---------------------------------------------------------------------------
// Ce qui est affiché à l'écran : les 2 prochains passages d'un "sens".
struct Row {
  int64_t when;
  uint8_t flags;
  char dest[TXT_LEN];
  char platform[6];
};

struct View {
  uint8_t groupCount;                 // nombre de sens disponibles
  uint8_t groupIndex;                 // sens affiché
  char groupKey[MAX_GROUPS][TXT_LEN]; // clés des sens (ordre alphabétique)
  uint8_t rows;                       // 0, 1 ou 2
  Row row[2];
};

// dirFilter : "" (tous les sens, sélection par selectedKey) ou une liste de
// mots séparés par "|" (ex. "Défense" ou "Châtillon|Asnières") : seuls les
// départs dont la destination contient l'un des mots sont gardés.
void buildView(const StopData& d, const char* dirFilter, const char* selectedKey, int64_t now,
               View& v);

// ---------------------------------------------------------------------------
// Utilitaires (exposés pour les tests)
bool parseIsoTime(const char* s, int64_t* out);
void fixText(char* s);  // répare l'encodage cassé (Ã©...) et les espaces
void normalizeAscii(const char* in, char* out, size_t outLen);  // "Défense" -> "defense"
bool matchesFilter(const char* text, const char* filter);
void copyStr(char* dst, const char* src, size_t n);
