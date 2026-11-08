// Tableaux des départs / arrivées des gares SNCF (API SNCF, format Navitia).
//
// C++ "pur" (pas de dépendance à l'ESP32) : compilé et testé aussi sur PC.
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "siri.h"  // SiriStatus, TXT_LEN, copyStr, fixText, normalizeAscii

#define BOARD_MAX 8   // trains gardés par tableau
#define BOARD_ROWS 4  // trains affichés à l'écran

enum : uint8_t {
  TR_CANCELLED = 1,      // train supprimé
  TR_REALTIME = 2,       // horaire mis à jour en temps réel
  TR_PLACE_PENDING = 4,  // provenance encore inconnue (arrivées)
};

struct Train {
  int64_t base;         // heure prévue (epoch UTC)
  int64_t real;         // heure réelle / estimée
  char place[TXT_LEN];  // destination (départs) ou provenance (arrivées)
  char mode[16];        // "TGV INOUI", "OUIGO", "TER"...
  char number[12];      // numéro du train
  char vj[80];          // identifiant de la circulation (pour chercher la provenance)
  uint32_t color;       // couleur associée au mode (0xRRGGBB)
  uint8_t flags;
};

struct Board {
  bool arrivals;
  uint8_t count;
  int64_t serverTime;  // heure du serveur (epoch UTC), 0 si inconnue
  int64_t fetchedAt;   // heure locale de la récupération
  Train t[BOARD_MAX];
};

// Analyse une réponse /stop_areas/{id}/departures (ou /arrivals).
// mainlineOnly : écarte Transilien, RER et autocars.
// stationName : nom de la gare consultée (pour reconnaître les provenances inutiles).
SiriStatus parseNavitiaBoard(const char* json, size_t len, bool arrivals, bool mainlineOnly,
                             const char* stationName, Board& out, char* err, size_t errLen);

// Analyse une réponse /vehicle_journeys/{id} : renvoie la gare d'origine du train.
bool parseNavitiaOrigin(const char* json, size_t len, const char* stationName, char* out, size_t n);

// "20261108T140700" (heure de Paris) -> epoch UTC. Le fuseau TZ doit être réglé.
bool parseNavitiaTime(const char* s, int64_t* out);

// "Marseille Saint-Charles (Marseille)" -> "Marseille Saint-Charles"
void cleanStationName(char* s);

// Couleur d'un mode commercial (TGV INOUI, OUIGO, Intercités, TER...)
uint32_t trainModeColor(const char* mode, const char* apiColor);

// Vrai pour un train "grandes lignes" (TGV, Intercités, TER, internationaux)
bool isMainline(const char* commercialMode, const char* physicalMode, const char* network);
