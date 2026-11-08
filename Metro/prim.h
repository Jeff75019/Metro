// Accès réseau : API PRIM d'Île-de-France Mobilités (prochains passages) et
// open data IDFM (recherche d'arrêts par nom).
#pragma once
#include <Arduino.h>

#include "settings.h"
#include "siri.h"

struct FetchResult {
  int http;          // code HTTP (ou < 0 : erreur réseau)
  bool ok;           // données valides dans StopData
  char msg[64];      // message d'erreur lisible (vide si ok)
};

// Prochains passages pour un favori. Met l'horloge à l'heure du serveur si
// elle n'est pas encore réglée (ou dérive de plus de 15 s).
FetchResult primFetch(const Favorite& fav, const char* apiKey, StopData& out);

// Pour la page de configuration : lignes desservies à un arrêt (JSON).
int primLinesJson(const String& stopRef, const char* apiKey, String& out);

// Recherche d'arrêts par nom (open data IDFM, pas de clé nécessaire).
int searchStopsJson(const String& query, String& out);
