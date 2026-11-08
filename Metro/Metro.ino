// =============================================================================
//  Afficheur Métro — prochains passages en temps réel (Île-de-France)
//  pour Waveshare ESP32-S3-Touch-LCD-1.9 (+ batterie LiPo)
//
//  Données : API PRIM d'Île-de-France Mobilités (gratuite, clé nécessaire)
//
//  Utilisation :
//    - Premier démarrage : l'écran affiche un QR code. Rejoignez le WiFi
//      « Metro-XXXX » et ouvrez http://192.168.4.1 pour tout configurer.
//    - Toucher l'écran         : changer de sens (si plusieurs) / actualiser
//    - Glisser gauche/droite   : arrêt favori suivant / précédent
//    - Appui long (3 s) écran  : mode configuration
//    - Bouton BOOT             : réveil / mise en veille
//    - Bouton BOOT maintenu 3 s : mode configuration
// =============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <esp_attr.h>
#include <time.h>

#include "board.h"
#include "config.h"
#include "lines.h"
#include "portal.h"
#include "prim.h"
#include "settings.h"
#include "siri.h"
#include "ui.h"

// ---- État conservé pendant la veille profonde (mémoire RTC) -----------------
RTC_DATA_ATTR static uint8_t rtcFav = 0;                         // favori affiché
RTC_DATA_ATTR static char rtcDirKey[MAX_FAVORITES][TXT_LEN];     // sens choisi par favori
RTC_DATA_ATTR static StopData rtcData;                            // dernière réponse
RTC_DATA_ATTR static uint8_t rtcDataFav = 0xFF;                   // favori de rtcData
RTC_DATA_ATTR static uint8_t rtcBssid[6];                         // reconnexion WiFi rapide
RTC_DATA_ATTR static int32_t rtcChannel = 0;
RTC_DATA_ATTR static uint8_t rtcWifiIdx = 0;
RTC_DATA_ATTR static uint8_t rtcIdleMotionWakes = 0;             // réveils "secousse" sans usage
RTC_DATA_ATTR static uint32_t rtcCfgHash = 0;                     // empreinte des favoris

// ---- État en RAM ------------------------------------------------------------
static StopData cache[MAX_FAVORITES];
static bool cacheValid[MAX_FAVORITES];
static int64_t nextAttempt[MAX_FAVORITES];  // pas de nouvel essai avant (epoch)
static bool forceFetch[MAX_FAVORITES];
static char lastErr[64];
static View lastView;
static uint8_t curFav = 0;
static NetState net = NetState::Off;
static bool busy = false;
static bool screenOn = true;
static bool interacted = false;
static WakeReason wake = WakeReason::PowerOn;
static uint32_t lastInteraction = 0, lastDraw = 0, lastBattery = 0;
static int battPct = -1, battMv = 0;

// ---- WiFi (non bloquant, plusieurs réseaux) ---------------------------------
static int wifiAttempt = 0, wifiCurIdx = 0;
static uint32_t wifiStart = 0, wifiTimeout = 0;
static bool ntpStarted = false;

static int wifiCount() {
  int n = 0;
  for (auto& w : settings.wifi)
    if (w.ssid[0]) n++;
  return n;
}

static void wifiTry() {
  const int n = wifiCount();
  const bool haveFast = rtcChannel > 0 && rtcWifiIdx < n;
  const int total = n + (haveFast ? 1 : 0);
  if (n == 0 || wifiAttempt >= total) {
    net = NetState::Failed;
    snprintf(lastErr, sizeof(lastErr), "WiFi «\xC2\xA0%s\xC2\xA0» introuvable", n ? settings.wifi[0].ssid : "?");
    wifiStart = millis();
    return;
  }
  const bool fast = haveFast && wifiAttempt == 0;
  const int start = rtcWifiIdx < n ? rtcWifiIdx : 0;
  wifiCurIdx = fast ? start : (start + wifiAttempt - (haveFast ? 1 : 0)) % n;
  const WifiCred& w = settings.wifi[wifiCurIdx];
  WiFi.disconnect(false, false);
  if (fast) WiFi.begin(w.ssid, w.pass, rtcChannel, rtcBssid);
  else WiFi.begin(w.ssid, w.pass);
  wifiStart = millis();
  wifiTimeout = fast ? 5000 : 15000;
  net = NetState::Connecting;
  log_i("WiFi : connexion à %s%s", w.ssid, fast ? " (rapide)" : "");
}

static void wifiBegin() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(MDNS_NAME);
  wifiAttempt = 0;
  wifiTry();
}

static void wifiPoll() {
  switch (net) {
    case NetState::Connecting:
      if (WiFi.status() == WL_CONNECTED) {
        net = NetState::Online;
        rtcWifiIdx = (uint8_t)wifiCurIdx;
        const uint8_t* bssid = WiFi.BSSID();
        rtcChannel = bssid ? WiFi.channel() : 0;
        if (bssid) memcpy(rtcBssid, bssid, 6);
        if (strstr(lastErr, "WiFi")) lastErr[0] = 0;
        if (!ntpStarted) {
          configTzTime(TZ_PARIS, "pool.ntp.org", "time.cloudflare.com");
          ntpStarted = true;
        }
        log_i("WiFi connecté : %s", WiFi.localIP().toString().c_str());
      } else if (millis() - wifiStart > wifiTimeout) {
        if (wifiAttempt == 0) rtcChannel = 0;  // l'accès rapide a échoué : on l'oublie
        wifiAttempt++;
        wifiTry();
      }
      break;
    case NetState::Online:
      if (WiFi.status() != WL_CONNECTED) {
        net = NetState::Connecting;
        wifiAttempt = 1;
        wifiTry();
      }
      break;
    case NetState::Failed:
      if (millis() - wifiStart > 30000) {  // nouvel essai toutes les 30 s
        wifiAttempt = 0;
        wifiTry();
      }
      break;
    default:
      break;
  }
}

// ---- Affichage ----------------------------------------------------------------
static int64_t nowEpoch() { return (int64_t)time(nullptr); }

static void draw() {
  if (!screenOn) return;
  const Favorite& f = settings.fav[curFav];
  const LineInfo* li = findLine(f.line);
  UiState s;
  memset(&s, 0, sizeof(s));
  s.badge = li ? li->name : (f.badge[0] ? f.badge : (f.line[0] ? "?" : "Tout"));
  s.mode = li ? li->mode : LineMode::Bus;
  s.badgeColor = li ? li->color : 0x5A6E8C;
  s.badgeText = li ? li->textColor : 0xFFFFFF;
  const bool valid = cacheValid[curFav];
  s.title = f.label[0] ? f.label : (valid && cache[curFav].stopName[0] ? cache[curFav].stopName : f.stopName);
  s.now = nowEpoch();
  memset(&lastView, 0, sizeof(lastView));
  if (valid) buildView(cache[curFav], f.filter, rtcDirKey[curFav], s.now, lastView);
  s.view = valid ? &lastView : nullptr;
  s.hasData = valid;
  s.battery = battPct;
  s.net = net;
  s.busy = busy;
  s.status = lastErr[0] ? lastErr : nullptr;
  s.ageS = valid ? (int)(s.now - cache[curFav].fetchedAt) : -1;
  s.favIndex = curFav;
  s.favCount = settings.favCount;
  uiDrawMain(s);
  lastDraw = millis();
}

static void readBattery() {
  battMv = batteryMilliVolts();
  battPct = (battMv > 2500) ? batteryPercent(battMv) : -1;  // < 2,5 V : pas de batterie
  lastBattery = millis();
}

// ---- Récupération des horaires --------------------------------------------------
static void fetchNow(uint8_t idx) {
  const int64_t now = nowEpoch();
  if (!quotaAvailable(settings.dailyQuota)) {
    snprintf(lastErr, sizeof(lastErr), "Quota du jour atteint (%u appels)", (unsigned)settings.dailyQuota);
    nextAttempt[idx] = now + 300;
    return;
  }
  busy = true;
  draw();
  FetchResult r = primFetch(settings.fav[idx], settings.apiKey, cache[idx]);
  busy = false;
  if (r.ok) {
    cacheValid[idx] = true;
    lastErr[0] = 0;
    rtcData = cache[idx];
    rtcDataFav = idx;
    nextAttempt[idx] = 0;
  } else {
    copyStr(lastErr, r.msg, sizeof(lastErr));
    // erreur "définitive" (clé, quota) : on insiste moins souvent
    const bool hard = r.http == 401 || r.http == 403 || r.http == 429;
    nextAttempt[idx] = nowEpoch() + (hard ? 120 : 15);
    log_w("Échec : %s", r.msg);
  }
  draw();
}

static uint32_t refreshInterval() {
  uint32_t s = settings.refreshS;
  if (settings.sleepMode == SleepMode::AlwaysOn && settings.dailyQuota > 0) {
    // allumé en permanence : on étale le quota sur 24 h
    uint32_t minS = 86400UL / settings.dailyQuota + 1;
    if (s < minS) s = minS;
  }
  return s;
}

static void maybeFetch() {
  if (!screenOn || net != NetState::Online || busy) return;
  const int64_t now = nowEpoch();
  if (now < nextAttempt[curFav]) return;
  const bool stale = forceFetch[curFav] || !cacheValid[curFav] || now - cache[curFav].fetchedAt >= (int64_t)refreshInterval() ||
                     now < cache[curFav].fetchedAt;
  if (stale) {
    forceFetch[curFav] = false;
    fetchNow(curFav);
  }
}

// ---- Veille --------------------------------------------------------------------
static void goToSleep() {
  bool motion = settings.motionWake;
  uint32_t timerS = 0;
  if (wake == WakeReason::Motion && !interacted) rtcIdleMotionWakes++;
  else if (interacted) rtcIdleMotionWakes = 0;
  if (motion && rtcIdleMotionWakes >= 3) {
    // 3 réveils par secousse sans utilisation (dans une poche ?) : pause de 30 min
    motion = false;
    timerS = 30 * 60;
  }
  rtcFav = curFav;
  log_i("Veille profonde (secousse : %d)", motion);
  boardDeepSleep(motion, timerS);
}

static void setScreen(bool on) {
  if (on == screenOn) return;
  screenOn = on;
  if (on) {
    lcd.wakeup();
    lcd.setBrightness(settings.brightness);
    draw();
  } else {
    lcd.sleep();
  }
}

static void enterPortal() {
  uiDrawMessage("Configuration", "Démarrage du point d'accès WiFi…");
  WiFi.disconnect(false, false);
  portalStart();
}

// ---- Entrées : écran tactile et bouton BOOT -----------------------------------
static void onInteraction() {
  lastInteraction = millis();
  interacted = true;
  rtcIdleMotionWakes = 0;
}

static void onTap() {
  if (lastView.groupCount > 1) {
    uint8_t g = (lastView.groupIndex + 1) % lastView.groupCount;
    copyStr(rtcDirKey[curFav], lastView.groupKey[g], TXT_LEN);
    draw();
  } else if (cacheValid[curFav] && nowEpoch() - cache[curFav].fetchedAt > 10) {
    nextAttempt[curFav] = 0;
    forceFetch[curFav] = true;  // force une mise à jour
  }
}

static void onSwipe(int dir) {
  if (settings.favCount < 2) return;
  curFav = (uint8_t)((curFav + settings.favCount + dir) % settings.favCount);
  lastErr[0] = 0;
  draw();
}

static void handleTouch() {
  static bool down = false, longDone = false;
  static int32_t x0, y0, x1, y1;
  static uint32_t tStart = 0, tLast = 0;
  int32_t x, y;
  const uint32_t ms = millis();
  if (lcd.getTouch(&x, &y)) {
    if (!down) {
      down = true;
      longDone = false;
      x0 = x;
      y0 = y;
      tStart = ms;
      if (!screenOn) {  // écran éteint : le toucher le rallume seulement
        setScreen(true);
        longDone = true;
      }
      onInteraction();
    }
    x1 = x;
    y1 = y;
    tLast = ms;
    if (!longDone && ms - tStart > 3000 && abs(x1 - x0) < 30 && abs(y1 - y0) < 30) {
      longDone = true;
      enterPortal();
    }
  } else if (down && ms - tLast > 80) {  // relâché
    down = false;
    if (longDone) return;
    const int dx = x1 - x0, dy = y1 - y0;
    if (abs(dx) > 50 && abs(dx) > abs(dy)) onSwipe(dx < 0 ? +1 : -1);
    else if (abs(dx) < 30 && abs(dy) < 30) onTap();
  }
}

static void handleButton() {
  static bool down = false, longDone = false, ignoreUntilRelease = true;
  static uint32_t t0 = 0;
  const bool pressed = bootButtonPressed();
  if (ignoreUntilRelease) {  // le bouton qui a réveillé la carte est peut-être encore enfoncé
    if (!pressed) ignoreUntilRelease = false;
    return;
  }
  const uint32_t ms = millis();
  if (pressed && !down) {
    down = true;
    longDone = false;
    t0 = ms;
    onInteraction();
  } else if (pressed && down && !longDone && ms - t0 > 3000) {
    longDone = true;
    if (portalActive()) ESP.restart();
    else enterPortal();
  } else if (!pressed && down) {
    down = false;
    if (longDone || ms - t0 < 40) return;
    // appui court
    if (portalActive()) ESP.restart();
    else if (settings.sleepMode == SleepMode::Deep) goToSleep();
    else setScreen(!screenOn);
  }
}

// =============================================================================
void setup() {
  boardEarlyInit();
  Serial.begin(115200);
  setenv("TZ", TZ_PARIS, 1);
  tzset();
  wake = boardWakeReason();
  settingsLoad();

  // Réveil programmé (pause après des secousses intempestives) : on se rendort
  // aussitôt, avec le réveil par secousse réactivé.
  if (wake == WakeReason::Timer) {
    rtcIdleMotionWakes = 0;
    boardDeepSleep(settings.motionWake, 0);
  }
  if (wake == WakeReason::Motion) imuDisableWakeOnMotion();

  boardDisplayInit(settings.flip, settings.brightness);
  uiBegin(&lcd);
  readBattery();

  // Batterie presque vide (au réveil) : on ne fait rien et on se rendort.
  // Après une mise sous tension ou un reset, on démarre quoi qu'il arrive.
  if (wake != WakeReason::PowerOn && battMv > 2500 && battMv < 3350) {
    uiDrawMessage("Batterie faible", "Branchez l'afficheur en USB-C", nullptr, 0xFF5F5F);
    delay(3000);
    boardDeepSleep(false, 0);
  }

  if (wake == WakeReason::PowerOn) {
    uiDrawSplash(FW_VERSION);
    delay(800);
  }

  if (!settingsReady()) {
    portalStart();
    return;
  }

  // Favoris modifiés depuis la dernière fois : on oublie l'état mémorisé
  uint32_t h = 2166136261u;
  const uint8_t* p = (const uint8_t*)settings.fav;
  for (size_t i = 0; i < sizeof(settings.fav); i++) h = (h ^ p[i]) * 16777619u;
  h ^= settings.favCount;
  if (h != rtcCfgHash) {
    rtcCfgHash = h;
    rtcFav = 0;
    rtcDataFav = 0xFF;
    memset(rtcDirKey, 0, sizeof(rtcDirKey));
  }

  curFav = rtcFav < settings.favCount ? rtcFav : 0;
  // Affichage immédiat de la dernière réponse connue (l'horloge a continué de
  // tourner pendant la veille : les minutes restantes sont recalculées).
  if (rtcDataFav < settings.favCount && rtcData.count <= MAX_DEPARTURES) {
    cache[rtcDataFav] = rtcData;
    cacheValid[rtcDataFav] = true;
  }
  wifiBegin();
  lastInteraction = millis();
  draw();
}

void loop() {
  handleButton();
  if (portalActive()) {
    portalLoop();
    delay(2);
    return;
  }
  wifiPoll();
  handleTouch();
  maybeFetch();

  const uint32_t ms = millis();
  if (ms - lastBattery > 20000) readBattery();
  if (ms - lastDraw >= 1000) draw();

  // Mise en veille après la durée d'affichage
  if (ms - lastInteraction > settings.awakeS * 1000UL && !busy) {
    if (settings.sleepMode == SleepMode::Deep) goToSleep();
    else if (settings.sleepMode == SleepMode::ScreenOff) setScreen(false);
  }
  delay(10);
}
