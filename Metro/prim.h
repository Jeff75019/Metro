// Accès réseau : API PRIM d'Île-de-France Mobilités (prochains passages) et
// open data IDFM (recherche d'arrêts par nom).
#pragma once
#include <Arduino.h>

#include "settings.h"
#include "siri.h"

struct FetchResult {
  int http;          // code HTTP (ou < 0 : erreur réseau)
  bool ok;           // données valides dans StopData
  char msg[96];      // message d'erreur lisible (vide si ok)
};

// Prochains passages pour un favori. Met l'horloge à l'heure du serveur si
// elle n'est pas encore réglée (ou dérive de plus de 15 s).
FetchResult primFetch(const Favorite& fav, const char* apiKey, StopData& out);

// Prochains passages d'une ligne (line : "C01371", ou "" pour toutes) à un arrêt
// (stopRef : "STIF:StopArea:SP:…:" ou "STIF:StopPoint:Q:…:").
FetchResult primFetchStop(const char* stopRef, const char* line, const char* apiKey, StopData& out);

// Info trafic du métro, du RER et du tramway (perturbations en cours ou dans
// les 12 h). out.text doit être alloué par l'appelant.
struct TrafficData;
FetchResult primFetchTraffic(const char* apiKey, int64_t now, TrafficData& out);

// Pour la page de configuration : lignes desservies à un arrêt (JSON).
int primLinesJson(const String& stopRef, const char* apiKey, String& out);

// Recherche d'arrêts par nom (open data IDFM, pas de clé nécessaire).
int searchStopsJson(const String& query, String& out);
