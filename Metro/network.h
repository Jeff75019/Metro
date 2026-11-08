// Réseau embarqué dans l'afficheur, pour choisir sur l'écran tactile :
//   - lignes de métro, RER et tramway, avec leurs stations dans l'ordre de la ligne
//     (tools/make_network.py, open data IDFM) ;
//   - grandes gares SNCF (tools/make_sncf_list.py, liste Train Nomade).
// C++ "pur" : compilé et testé aussi sur PC.
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "lines.h"

enum : uint8_t {
  NET_BRANCH = 1,    // début d'une autre branche (ne suit pas la station précédente)
  NET_TERMINUS = 2,  // terminus
};

struct NetStop {
  uint32_t zda;    // zone d'arrêt : MonitoringRef PRIM « STIF:StopArea:SP:<zda>: »
  uint16_t name;   // position du nom dans la table des noms
  uint8_t flags;
  int16_t pos;     // place le long de la ligne (sert à ranger les destinations)
};

struct NetLine {
  const char* id;  // identifiant IDFM, ex. "C01371"
  uint16_t first;  // première station dans la table
  uint8_t count;   // nombre de stations
};

struct SncfPick {
  const char* name;  // ex. "Paris Gare de Lyon"
  const char* uic;   // codes UIC séparés par des virgules
};

// Lignes d'un mode (Metro, Rer, Tram), dans l'ordre d'affichage : indices globaux
int netLinesOfMode(LineMode mode, int* out, int maxN);
int netLineCount();
const NetLine& netLine(int index);
const LineInfo* netLineInfo(int index);  // nom court et couleurs (table lines.cpp)
const NetStop& netStop(int line, int i);
const char* netStopName(int line, int i);
void netStopRef(int line, int i, char* out, size_t n);  // "STIF:StopArea:SP:<zda>:"

// Gares SNCF : groupe 0 = gares de Paris, puis un groupe par initiale (sans accent)
int sncfGroupCount();
const char* sncfGroupLabel(int group);         // "Paris", "A", "B"…
int sncfGroupStations(int group, int* out, int maxN);  // indices dans la liste des gares
int sncfPickCount();
const SncfPick& sncfPick(int index);
