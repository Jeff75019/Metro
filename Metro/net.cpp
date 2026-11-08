#include "net.h"

#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <Preferences.h>
#include <esp_attr.h>
#include <sys/time.h>
#include <time.h>

#include "config.h"

// ----------------------------------------------------------------------------
//  HTTPS
// ----------------------------------------------------------------------------
// Vérification des certificats avec le magasin de certificats intégré au cœur
// ESP32 (fonction disponible depuis les versions 3.3.x) ; sinon pas de vérif.
template <typename T>
static auto tlsSetup(T& c, int) -> decltype(c.useBuiltinCACertBundle(), void()) {
  c.useBuiltinCACertBundle();
}
template <typename T>
static void tlsSetup(T& c, long) {
  c.setInsecure();
}

String urlEncode(const String& s, bool keepColon) {
  static const char* hex = "0123456789ABCDEF";
  String o;
  o.reserve(s.length() * 3);
  for (size_t i = 0; i < s.length(); i++) {
    uint8_t c = (uint8_t)s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || (keepColon && c == ':')) {
      o += (char)c;
    } else {
      o += '%';
      o += hex[c >> 4];
      o += hex[c & 15];
    }
  }
  return o;
}

int httpGet(const String& url, const HttpAuth& auth, String& body) {
  for (int attempt = 0; attempt < 2; attempt++) {
    NetworkClientSecure client;
    if (attempt == 0) tlsSetup(client, 0);
    else client.setInsecure();
    HTTPClient http;
    http.setConnectTimeout(8000);
    http.setTimeout(12000);
    http.setUserAgent("AfficheurMetro/" FW_VERSION " (ESP32-S3)");
    if (!http.begin(client, url)) return HTTPC_ERROR_CONNECTION_REFUSED;
    if (auth.key && *auth.key) {
      if (auth.basic) http.setAuthorization(auth.key, "");
      else if (auth.header) http.addHeader(auth.header, auth.key);
    }
    http.addHeader("Accept", "application/json");
    int code = http.GET();
    if (code > 0) {
      body = http.getString();
      http.end();
      return code;
    }
    http.end();
    // Échec de connexion (souvent la négociation TLS) : un second essai sans
    // vérification de certificat, pour ne pas bloquer l'afficheur.
    if (code != HTTPC_ERROR_CONNECTION_REFUSED || attempt == 1) return code;
    log_w("Connexion HTTPS refusée, nouvel essai sans vérification du certificat");
  }
  return HTTPC_ERROR_CONNECTION_REFUSED;
}

void describeHttp(int code, const char* who, char* msg, size_t n) {
  if (code == 401 || code == 403) snprintf(msg, n, "Clé %s refusée (%d)", who, code);
  else if (code == 404) snprintf(msg, n, "Arrêt ou gare introuvable (404)");
  else if (code == 429) snprintf(msg, n, "Quota %s dépassé (429)", who);
  else if (code >= 500) snprintf(msg, n, "Serveur %s indisponible (%d)", who, code);
  else if (code < 0) snprintf(msg, n, "Réseau : %s", HTTPClient::errorToString(code).c_str());
  else snprintf(msg, n, "Erreur HTTP %d", code);
}

void syncClock(int64_t serverTs) {
  if (serverTs < 1700000000) return;
  int64_t now = (int64_t)time(nullptr);
  if (now < 1700000000 || llabs(now - serverTs) > 15) {
    struct timeval tv = {(time_t)serverTs, 0};
    settimeofday(&tv, nullptr);
    log_i("Horloge réglée sur l'heure du serveur");
  }
}

// ----------------------------------------------------------------------------
//  Quotas d'appels (mémorisés en RAM RTC, qui survit à la veille, et en flash)
// ----------------------------------------------------------------------------
RTC_DATA_ATTR static uint32_t qDay[2] = {0, 0};
RTC_DATA_ATTR static uint16_t qCount[2] = {0, 0};
RTC_DATA_ATTR static bool qLoaded = false;

static uint32_t todayKey() {
  time_t t = time(nullptr);
  if (t < 1700000000) return 0;  // heure inconnue
  struct tm lt;
  localtime_r(&t, &lt);
  return (uint32_t)((lt.tm_year + 1900) * 10000 + (lt.tm_mon + 1) * 100 + lt.tm_mday);
}

static void quotaSync(int i) {
  if (!qLoaded) {
    Preferences p;
    if (p.begin("quota", true)) {
      qDay[0] = p.getUInt("day", 0);
      qCount[0] = p.getUShort("n", 0);
      qDay[1] = p.getUInt("day1", 0);
      qCount[1] = p.getUShort("n1", 0);
      p.end();
    }
    qLoaded = true;
  }
  uint32_t d = todayKey();
  if (d && d != qDay[i]) {
    qDay[i] = d;
    qCount[i] = 0;
  }
}

uint16_t quotaUsed(Api api) {
  const int i = (int)api;
  quotaSync(i);
  return qCount[i];
}

bool quotaAvailable(Api api, uint16_t dailyLimit) { return quotaUsed(api) < dailyLimit; }

void quotaCount(Api api) {
  const int i = (int)api;
  quotaSync(i);
  qCount[i]++;
  Preferences p;
  if (p.begin("quota", false)) {
    p.putUInt(i ? "day1" : "day", qDay[i]);
    p.putUShort(i ? "n1" : "n", qCount[i]);
    p.end();
  }
}
