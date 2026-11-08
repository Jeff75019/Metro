// Outils réseau communs : requêtes HTTPS, quotas d'appels quotidiens, horloge.
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

enum class Api : uint8_t { Prim = 0, Sncf = 1 };

struct HttpAuth {
  const char* header = nullptr;  // nom d'en-tête portant la clé (PRIM : "apikey")
  const char* key = nullptr;     // valeur de la clé
  bool basic = false;            // true : authentification HTTP Basic (clé = identifiant, API SNCF)
};

// GET en HTTPS. Renvoie le code HTTP (ou < 0 : erreur réseau) et le corps de la réponse
// (lu seulement pour un code 2xx). Les réponses envoyées en morceaux (« chunked »)
// sont reconstituées en attendant chaque paquet : une réponse coupée en route
// renvoie HTTPC_ERROR_READ_TIMEOUT (« réponse incomplète »).
int httpGet(const String& url, const HttpAuth& auth, String& body);

// GET en HTTPS avec lecture du JSON au fil de l'eau, sans garder la réponse
// entière en mémoire (filtre ArduinoJson : seuls les champs utiles sont gardés).
// *err : résultat du décodage (si le code est 2xx).
int httpGetJson(const String& url, const HttpAuth& auth, JsonDocument& doc, const JsonDocument& filter,
                DeserializationError* err);

// Allocateur ArduinoJson en PSRAM (gros documents), à défaut en RAM interne
ArduinoJson::Allocator* psramAllocator();

// Cause précise du dernier échec réseau de httpGet ("" si aucun) : DNS, serveur
// injoignable, TLS refusé (code mbedTLS), mémoire insuffisante. Détail complet
// dans le journal série.
const char* httpLastError();

String urlEncode(const String& s, bool keepColon);

// Message lisible pour un code HTTP (who = "PRIM" ou "SNCF")
void describeHttp(int code, const char* who, char* msg, size_t n);

// Règle l'horloge sur l'heure du serveur si elle n'est pas réglée ou dérive de plus de 15 s.
void syncClock(int64_t serverTs);

// Compteurs d'appels du jour, conservés pendant la veille (RAM RTC) et en flash
uint16_t quotaUsed(Api api);
bool quotaAvailable(Api api, uint16_t dailyLimit);
void quotaCount(Api api);

// ---- WiFi : diagnostic des échecs de connexion --------------------------------
// Mémorise la raison du dernier échec de connexion (codes wifi_err_reason_t de
// l'ESP-IDF), pour afficher un message précis plutôt que « introuvable ».
void wifiSetup();                 // pays (canaux 1 à 13) + suivi des échecs ; après WiFi.mode()
uint8_t wifiLastReason();         // 0 : aucun échec depuis wifiClearReason()
void wifiClearReason();
const char* wifiReasonText(uint8_t reason);  // court, sans majuscule : « mot de passe refusé »…
const char* wifiReasonHint(uint8_t reason);  // conseil détaillé pour la page de configuration
