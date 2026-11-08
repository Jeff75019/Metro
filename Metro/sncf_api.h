// Accès à l'API SNCF (https://api.sncf.com, format Navitia) : tableaux des
// départs / arrivées des gares, provenance des trains, recherche de gares.
// Clé gratuite : https://numerique.sncf.com/startup/api/token-developpeur/
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
int sncfSearchJson(const String& query, const char* key, String& out);
