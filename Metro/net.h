// Outils réseau communs : requêtes HTTPS, quotas d'appels quotidiens, horloge.
#pragma once
#include <Arduino.h>

enum class Api : uint8_t { Prim = 0, Sncf = 1 };

struct HttpAuth {
  const char* header = nullptr;  // nom d'en-tête portant la clé (PRIM : "apikey")
  const char* key = nullptr;     // valeur de la clé
  bool basic = false;            // true : authentification HTTP Basic (clé = identifiant, API SNCF)
};

// GET en HTTPS. Renvoie le code HTTP (ou < 0 : erreur réseau) et le corps de la réponse.
int httpGet(const String& url, const HttpAuth& auth, String& body);

String urlEncode(const String& s, bool keepColon);

// Message lisible pour un code HTTP (who = "PRIM" ou "SNCF")
void describeHttp(int code, const char* who, char* msg, size_t n);

// Règle l'horloge sur l'heure du serveur si elle n'est pas réglée ou dérive de plus de 15 s.
void syncClock(int64_t serverTs);

// Compteurs d'appels du jour, conservés pendant la veille (RAM RTC) et en flash
uint16_t quotaUsed(Api api);
bool quotaAvailable(Api api, uint16_t dailyLimit);
void quotaCount(Api api);
