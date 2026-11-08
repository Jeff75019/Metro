// Tableaux des départs / arrivées des gares, selon la source choisie :
//   - Train Nomade (par défaut, sans clé) : https://api.trainnomade.fr/gare/{uic}.json
//   - API SNCF (clé) : https://api.sncf.com, format Navitia, provenance par /vehicle_journeys
// Recherche de gares : open data SNCF (sans clé) ou API SNCF.
#pragma once
#include <Arduino.h>

#include "prim.h"  // FetchResult
#include "settings.h"
#include "sncf.h"

// Tableau des départs (arrivals = false) ou des arrivées d'une gare.
FetchResult sncfFetchBoard(const Station& st, bool arrivals, Board& out);

// Complète la provenance des trains à l'arrivée : d'abord depuis le cache, puis
// au plus un appel à l'API (pour garder l'écran réactif). Ne regarde que les
// maxRows premiers trains. Renvoie true si le tableau a changé.
bool sncfResolveOrigins(Board& b, const Station& st, int maxRows);

// Pour la page de configuration : recherche de gares par nom (JSON).
// useSncfApi : API SNCF (clé nécessaire), sinon open data SNCF (sans clé).
int sncfSearchJson(const String& query, bool useSncfApi, const char* key, String& out);
