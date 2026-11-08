// Tableaux des départs / arrivées des gares SNCF, depuis deux sources :
//   - Train Nomade (https://trainnomade.fr/api) : gratuit, sans clé, avec les
//     voies ; données SNCF (ODbL) relevées toutes les 2 minutes. Source par défaut.
//   - API SNCF officielle (https://api.sncf.com, format Navitia) : clé nécessaire.
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
  char platform[8];     // voie, quand elle est publiée (Train Nomade)
  char vj[80];          // identifiant de la circulation (pour chercher la provenance)
  uint32_t color;       // couleur associée au mode (0xRRGGBB)
  uint8_t flags;
};

struct Board {
  bool arrivals;
  uint8_t count;
  int64_t serverTime;  // heure du serveur (epoch UTC), 0 si inconnue
  int64_t fetchedAt;   // heure locale de la récupération
  int64_t validUntil;  // au-delà, données en direct périmées (0 : inconnu)
  Train t[BOARD_MAX];
};

// Analyse une réponse /stop_areas/{id}/departures (ou /arrivals).
// mainlineOnly : écarte Transilien, RER et autocars.
// stationName : nom de la gare consultée (pour reconnaître les provenances inutiles).
SiriStatus parseNavitiaBoard(const char* json, size_t len, bool arrivals, bool mainlineOnly,
                             const char* stationName, Board& out, char* err, size_t errLen);

// Analyse une réponse de Train Nomade (https://api.trainnomade.fr/gare/{uic}.json).
SiriStatus parseTrainNomade(const char* json, size_t len, bool arrivals, bool mainlineOnly, Board& out,
                            char* err, size_t errLen);

// Une gare est identifiée soit par ses codes UIC ("87271007,87271031"), soit par
// un identifiant Navitia ("stop_area:SNCF:87271007"). Conversions dans les deux sens.
void stationUicList(const char* id, char* out, size_t n);   // -> "87271007,87271031"
void stationStopArea(const char* id, char* out, size_t n);  // -> "stop_area:SNCF:87271007"

// Recherche de gares dans le jeu "gares-de-voyageurs" de l'open data SNCF
// (ressources.data.sncf.com, sans clé) : nom et codes UIC.
struct StationHit {
  char id[40];    // codes UIC séparés par des virgules
  char name[40];
};
int parseGaresVoyageurs(const char* json, size_t len, StationHit* out, int maxN);

// Fichier d'horaires prévus Train Nomade d'une gare (145 grandes gares), ou nullptr
const char* timetableSlug(const char* stationId);

// Complète le tableau temps réel avec les trains prévus (fichier
// gares/horaires/{slug}.json) des `horizonS` prochaines secondes qui n'y sont pas
// encore (marqués théoriques), et donne leur marque (TGV INOUI, OUIGO…) aux trains
// présents. Le tableau reste trié par heure. Renvoie le nombre de trains ajoutés.
int mergeTimetable(const char* json, size_t len, bool arrivals, int64_t now, int horizonS, Board& b);

// Libellé court d'une catégorie Train Nomade ("highSpeedRail" -> "TGV")
const char* trainCategoryLabel(const char* pr);

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
