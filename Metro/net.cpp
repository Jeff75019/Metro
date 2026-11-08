#include "net.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <NetworkClientSecure.h>
#include <Preferences.h>
#include <esp_attr.h>
#include <esp_heap_caps.h>

#include <functional>
#include <mbedtls/ssl.h>
#include <psa/crypto.h>
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

// Cause précise du dernier échec réseau (DNS, serveur injoignable, TLS refusé…)
static char netDetail[80];

static String urlHost(const String& url) {
  int s = url.indexOf("://");
  s = s < 0 ? 0 : s + 3;
  int e = url.indexOf('/', s);
  String h = url.substring(s, e < 0 ? url.length() : e);
  const int c = h.indexOf(':');
  return c < 0 ? h : h.substring(0, c);
}

static void explainFailure(NetworkClientSecure& client, int code, const String& url) {
  const String host = urlHost(url);
  char tls[80] = "";
  const int e = client.lastError(tls, sizeof(tls));
  const unsigned heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  const unsigned block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  IPAddress ip;
  if (code != HTTPC_ERROR_CONNECTION_REFUSED)
    snprintf(netDetail, sizeof(netDetail), "%s (%s)", HTTPClient::errorToString(code).c_str(), host.c_str());
  else if (!WiFi.hostByName(host.c_str(), ip))
    snprintf(netDetail, sizeof(netDetail), "adresse de %s introuvable (DNS)", host.c_str());
  else if (e == MBEDTLS_ERR_SSL_ALLOC_FAILED || strstr(tls, "allocation"))
    snprintf(netDetail, sizeof(netDetail), "mémoire insuffisante pour HTTPS (PSRAM activée ?)");
  else if (e < -0x1000)
    snprintf(netDetail, sizeof(netDetail), "TLS refusé par %s (-0x%04X)", host.c_str(), (unsigned)-e);
  else
    snprintf(netDetail, sizeof(netDetail), "%s injoignable", host.c_str());
  log_w("HTTPS %s : échec %d, TLS %d (%s), RAM interne libre %u o (bloc %u o)", host.c_str(), code, e, tls, heap,
        block);
}

const char* httpLastError() { return netDetail; }

// Corps d'une réponse HTTP lu directement sur la connexion : morceaux
// (« Transfer-Encoding: chunked ») reconstitués, attente patiente de chaque
// paquet. (La lecture intégrée au cœur ESP32 s'arrêtait en route sur les grosses
// réponses de PRIM, d'où des JSON incomplets.)
class HttpBody : public Stream {
 public:
  HttpBody(Client& c, bool chunked, int length, uint32_t timeoutMs)
      : c_(c), chunked_(chunked), left_(length), timeout_(timeoutMs) {
    setTimeout(20);  // délai de Stream : la vraie attente est dans fill()
  }
  int available() override { return (int)(n_ - i_) + (eof_ ? 0 : c_.available()); }
  int read() override {
    if (i_ >= n_ && !fill()) return -1;
    return buf_[i_++];
  }
  int peek() override {
    if (i_ >= n_ && !fill()) return -1;
    return buf_[i_];
  }
  size_t readBytes(char* b, size_t len) override {
    size_t got = 0;
    while (got < len) {
      if (i_ >= n_ && !fill()) break;
      size_t k = n_ - i_;
      if (k > len - got) k = len - got;
      memcpy(b + got, buf_ + i_, k);
      i_ += k;
      got += k;
    }
    return got;
  }
  size_t write(uint8_t) override { return 0; }
  bool failed() const { return failed_; }  // coupure ou délai dépassé avant la fin

 private:
  // Jusqu'à n octets dès qu'il y en a ; 0 : connexion fermée ou délai dépassé
  int rawRead(uint8_t* b, size_t n) {
    const uint32_t t0 = millis();
    for (;;) {
      const int a = c_.available();
      if (a > 0) return c_.read(b, (size_t)a < n ? (size_t)a : n);
      if (!c_.connected()) return 0;
      if (millis() - t0 > timeout_) {
        timedOut_ = true;
        return 0;
      }
      delay(1);
    }
  }
  int rawByte() {
    uint8_t b;
    return rawRead(&b, 1) == 1 ? b : -1;
  }
  bool fail() {
    failed_ = true;
    eof_ = true;
    return false;
  }
  bool fill() {
    if (eof_) return false;
    size_t want = sizeof(buf_);
    if (chunked_) {
      if (chunkLeft_ == 0) {
        if (needCrlf_) {  // « \r\n » qui termine le morceau précédent
          rawByte();
          rawByte();
          needCrlf_ = false;
        }
        char line[24];
        int k = 0, ch;
        while ((ch = rawByte()) >= 0 && ch != '\n')
          if (k < (int)sizeof(line) - 1 && ch != '\r') line[k++] = (char)ch;
        if (ch < 0) return fail();
        line[k] = 0;
        const long size = strtol(line, nullptr, 16);  // taille en hexadécimal
        if (size <= 0) {  // dernier morceau
          eof_ = true;
          return false;
        }
        chunkLeft_ = (size_t)size;
      }
      if (chunkLeft_ < want) want = chunkLeft_;
    } else if (left_ == 0) {
      eof_ = true;
      return false;
    } else if (left_ > 0 && (size_t)left_ < want) {
      want = (size_t)left_;
    }
    const int r = rawRead(buf_, want);
    if (r <= 0) {
      // Sans longueur annoncée, la fermeture de la connexion marque la fin
      if (!chunked_ && left_ < 0 && !timedOut_) {
        eof_ = true;
        return false;
      }
      return fail();
    }
    if (chunked_) {
      chunkLeft_ -= (size_t)r;
      if (chunkLeft_ == 0) needCrlf_ = true;
    } else if (left_ > 0) {
      left_ -= r;
    }
    n_ = (size_t)r;
    i_ = 0;
    return true;
  }

  Client& c_;
  const bool chunked_;
  int left_;  // octets restants (longueur annoncée), -1 : inconnue
  const uint32_t timeout_;
  size_t chunkLeft_ = 0;
  bool needCrlf_ = false, eof_ = false, failed_ = false, timedOut_ = false;
  uint8_t buf_[1024];
  size_t n_ = 0, i_ = 0;
};

// Requête GET : onBody lit le corps des réponses 2xx (renvoie false s'il est inutilisable)
static int httpRequest(const String& url, const HttpAuth& auth, const std::function<bool(HttpBody&)>& onBody) {
  static bool psaReady = false;  // TLS 1.3 (si le cœur l'inclut) a besoin de PSA Crypto
  if (!psaReady) psaReady = psa_crypto_init() == PSA_SUCCESS;
  netDetail[0] = 0;
  for (int attempt = 0; attempt < 2; attempt++) {
    NetworkClientSecure client;
    if (attempt == 0) tlsSetup(client, 0);
    else client.setInsecure();
    client.setHandshakeTimeout(15);  // secondes (2 min par défaut)
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
    const char* keys[] = {"Transfer-Encoding"};
    http.collectHeaders(keys, 1);
    int code = http.GET();
    if (code > 0) {
      if (code >= 200 && code < 300) {
        const uint32_t t0 = millis();
        HttpBody in(*http.getStreamPtr(), http.header("Transfer-Encoding").equalsIgnoreCase("chunked"), http.getSize(),
                    12000);
        const bool ok = onBody(in);
        if (!ok || in.failed()) {
          snprintf(netDetail, sizeof(netDetail), "réponse incomplète (%s)", urlHost(url).c_str());
          log_w("HTTPS %s : réponse incomplète après %lu ms", urlHost(url).c_str(), (unsigned long)(millis() - t0));
          code = HTTPC_ERROR_READ_TIMEOUT;
        }
      }
      http.end();
      return code;
    }
    explainFailure(client, code, url);
    http.end();
    // Échec de connexion (souvent la négociation TLS) : un second essai sans
    // vérification de certificat, pour ne pas bloquer l'afficheur.
    if (code != HTTPC_ERROR_CONNECTION_REFUSED || attempt == 1) return code;
    log_w("Connexion HTTPS refusée, nouvel essai sans vérification du certificat");
  }
  return HTTPC_ERROR_CONNECTION_REFUSED;
}

int httpGet(const String& url, const HttpAuth& auth, String& body) {
  body = "";
  return httpRequest(url, auth, [&](HttpBody& in) {
    size_t cap = 0;
    char tmp[1024];
    for (;;) {
      const size_t r = in.readBytes(tmp, sizeof(tmp));
      if (!r) break;
      if (body.length() + r + 1 > cap) {  // capacité doublée : pas de recopie à chaque paquet
        cap = cap * 2 > body.length() + r + 4096 ? cap * 2 : body.length() + r + 4096;
        if (!body.reserve(cap)) {
          snprintf(netDetail, sizeof(netDetail), "mémoire insuffisante (%u octets)", (unsigned)cap);
          return false;
        }
      }
      body.concat(tmp, r);
    }
    return true;
  });
}

int httpGetJson(const String& url, const HttpAuth& auth, JsonDocument& doc, const JsonDocument& filter,
                DeserializationError* err) {
  return httpRequest(url, auth, [&](HttpBody& in) {
    *err = deserializeJson(doc, in, DeserializationOption::Filter(filter), DeserializationOption::NestingLimit(40));
    return true;  // une coupure en route est signalée par in.failed()
  });
}

// ----------------------------------------------------------------------------
//  Gros documents JSON : en PSRAM (8 Mo) plutôt qu'en RAM interne
// ----------------------------------------------------------------------------
class PsramAllocator : public ArduinoJson::Allocator {
 public:
  void* allocate(size_t n) override {
    void* p = heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    return p ? p : malloc(n);
  }
  void deallocate(void* p) override { free(p); }
  void* reallocate(void* p, size_t n) override {
    void* q = heap_caps_realloc(p, n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    return q ? q : realloc(p, n);
  }
};

ArduinoJson::Allocator* psramAllocator() {
  static PsramAllocator a;
  return &a;
}

void describeHttp(int code, const char* who, char* msg, size_t n) {
  if (code == 401 || code == 403) snprintf(msg, n, "Clé %s refusée (%d)", who, code);
  else if (code == 404) snprintf(msg, n, "Arrêt ou gare introuvable (404)");
  else if (code == 429) snprintf(msg, n, "Quota %s dépassé (429)", who);
  else if (code >= 500) snprintf(msg, n, "Serveur %s indisponible (%d)", who, code);
  else if (code < 0 && netDetail[0]) snprintf(msg, n, "Réseau : %s", netDetail);
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

// ----------------------------------------------------------------------------
//  WiFi : diagnostic des échecs de connexion
// ----------------------------------------------------------------------------
static volatile uint8_t wifiReason = 0;
static bool wifiWatching = false;

void wifiSetup() {
  // Réglage France : canaux 1 à 13 (par défaut « monde » : 1 à 11 seulement en
  // recherche active, alors que les box françaises utilisent aussi 12 et 13)
  esp_wifi_set_country_code("FR", true);
  if (wifiWatching) return;
  wifiWatching = true;
  WiFi.onEvent(
      [](WiFiEvent_t, WiFiEventInfo_t info) {
        const uint8_t r = info.wifi_sta_disconnected.reason;
        if (r != WIFI_REASON_ASSOC_LEAVE) wifiReason = r;  // ASSOC_LEAVE : notre propre déconnexion
        log_w("WiFi : échec de connexion, raison %u (%s)", r, wifiReasonText(r));
      },
      ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

uint8_t wifiLastReason() { return wifiReason; }
void wifiClearReason() { wifiReason = 0; }

enum class WifiFail : uint8_t { None, Password, NotFound, Security, Weak, Other };

static WifiFail wifiFailKind(uint8_t r) {
  switch (r) {
    case 0:
      return WifiFail::None;
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:  // 15
    case WIFI_REASON_802_1X_AUTH_FAILED:      // 23
    case WIFI_REASON_AUTH_FAIL:               // 202
    case WIFI_REASON_HANDSHAKE_TIMEOUT:       // 204
      return WifiFail::Password;
    case WIFI_REASON_NO_AP_FOUND:  // 201
      return WifiFail::NotFound;
    case 210:  // NO_AP_FOUND_W_COMPATIBLE_SECURITY
    case 211:  // NO_AP_FOUND_IN_AUTHMODE_THRESHOLD
      return WifiFail::Security;
    case 212:  // NO_AP_FOUND_IN_RSSI_THRESHOLD
      return WifiFail::Weak;
    default:
      return WifiFail::Other;
  }
}

const char* wifiReasonText(uint8_t r) {
  static char buf[32];
  switch (wifiFailKind(r)) {
    case WifiFail::None:
      return "";
    case WifiFail::Password:
      return "mot de passe refusé";
    case WifiFail::NotFound:
      return "réseau introuvable";
    case WifiFail::Security:
      return "sécurité non gérée";
    case WifiFail::Weak:
      return "signal trop faible";
    default:
      snprintf(buf, sizeof(buf), "échec (code %u)", r);
      return buf;
  }
}

const char* wifiReasonHint(uint8_t r) {
  switch (wifiFailKind(r)) {
    case WifiFail::Password:
      return "La box refuse le mot de passe : vérifiez-le (majuscules, chiffres et symboles comptent).";
    case WifiFail::NotFound:
      return "L'afficheur ne voit pas ce réseau. Il ne capte que le WiFi 2,4 GHz : vérifiez le nom exact, que "
             "la bande 2,4 GHz de la box est active, ou rapprochez l'afficheur. Partage de connexion d'un "
             "iPhone : activez « Maximiser la compatibilité ».";
    case WifiFail::Security:
      return "Sécurité du réseau non gérée : l'afficheur accepte le WPA2 et le WPA3 personnels, pas les "
             "réseaux d'entreprise (identifiant + mot de passe) ni le WEP.";
    case WifiFail::Weak:
      return "Signal trop faible : rapprochez l'afficheur de la box.";
    case WifiFail::Other:
      return "La box a refusé la connexion. Réessayez ; si cela persiste, redémarrez la box ou essayez le "
             "partage de connexion d'un téléphone.";
    default:
      return "";
  }
}
