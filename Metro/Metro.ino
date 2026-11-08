// =============================================================================
//  Afficheur Métro — prochains passages en temps réel (Île-de-France)
//  pour Waveshare ESP32-S3-Touch-LCD-1.9 (+ batterie LiPo)
//
//  Deux programmes :
//    - Métro · RER · Tram : vos arrêts favoris (API PRIM d'Île-de-France Mobilités)
//    - Trains grandes lignes : départs / arrivées des gares SNCF (API SNCF)
//  Les deux API sont gratuites, avec une clé à demander en ligne.
//
//  Utilisation :
//    - Premier démarrage : l'écran affiche un QR code. Rejoignez le WiFi
//      « Metro-XXXX » et ouvrez http://192.168.4.1 pour tout configurer.
//    - Glisser haut/bas        : changer de programme (Métro <-> Trains)
//    - Glisser gauche/droite   : arrêt favori / gare suivant(e) ou précédent(e)
//    - Toucher l'écran         : Métro : changer de sens (ou actualiser)
//                                Trains : départs <-> arrivées
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
#include "net.h"
#include "prim.h"
#include "settings.h"
#include "siri.h"
#include "sncf.h"
#include "sncf_api.h"
#include "ui.h"

enum class App : uint8_t { Metro = 0, Trains = 1 };

// ---- État conservé pendant la veille profonde (mémoire RTC) -----------------
RTC_DATA_ATTR static uint8_t rtcFav = 0;                         // favori affiché
RTC_DATA_ATTR static char rtcDirKey[MAX_FAVORITES][TXT_LEN];     // sens choisi par favori
RTC_DATA_ATTR static StopData rtcData;                            // dernière réponse
RTC_DATA_ATTR static uint8_t rtcDataFav = 0xFF;                   // favori de rtcData
RTC_DATA_ATTR static uint8_t rtcBssid[6];                         // reconnexion WiFi rapide
RTC_DATA_ATTR static int32_t rtcChannel = 0;
RTC_DATA_ATTR static uint8_t rtcWifiIdx = 0;
RTC_DATA_ATTR static uint8_t rtcIdleMotionWakes = 0;             // réveils "secousse" sans usage
RTC_DATA_ATTR static uint32_t rtcCfgHash = 0;                     // empreinte des favoris et gares
RTC_DATA_ATTR static uint8_t rtcApp = 0;                          // programme affiché
RTC_DATA_ATTR static uint8_t rtcStation = 0;                      // gare affichée
RTC_DATA_ATTR static bool rtcArrivals = false;                    // arrivées plutôt que départs
RTC_DATA_ATTR static Board rtcBoard;                              // dernier tableau SNCF
RTC_DATA_ATTR static uint8_t rtcBoardKey = 0xFF;                  // gare * 2 + arrivées

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
static App app = App::Metro;

// Programme Trains : un tableau (départs, arrivées) par gare
static Board boards[MAX_STATIONS][2];
static bool boardValid[MAX_STATIONS][2];
static int64_t boardNext[MAX_STATIONS][2];
static bool boardForce[MAX_STATIONS][2];
static uint8_t curStation = 0;
static bool arrivals = false;
static uint32_t lastOriginTry = 0;
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

static void drawTrains() {
  BoardUi s;
  memset(&s, 0, sizeof(s));
  const bool valid = boardValid[curStation][arrivals];
  const Board& b = boards[curStation][arrivals];
  s.board = valid ? &b : nullptr;
  s.station = settings.stations[curStation].name;
  s.arrivals = arrivals;
  s.now = nowEpoch();
  s.battery = battPct;
  s.net = net;
  s.busy = busy;
  s.status = lastErr[0] ? lastErr : nullptr;
  s.ageS = valid ? (int)(s.now - b.fetchedAt) : -1;
  s.index = curStation;
  s.count = settings.stationCount;
  uiDrawBoard(s);
  lastDraw = millis();
}

static void draw() {
  if (!screenOn) return;
  if (app == App::Trains) return drawTrains();
  const Favorite& f = settings.fav[curFav];
  const LineInfo* li = findLine(f.line);
  UiState s;
  memset(&s, 0, sizeof(s));
  s.badge = li ? li->name : (f.badge[0] ? f.badge : (f.line[0] ? "?" : "Tout"));
  const bool own = !li && f.badge[0];  // ligne hors table (bus...) : infos du favori
  s.mode = li ? li->mode : (own ? (LineMode)f.mode : LineMode::Bus);
  s.badgeColor = li ? li->color : (own ? f.color : 0x5A6E8C);
  s.badgeText = li ? li->textColor : (own ? f.textColor : 0xFFFFFF);
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
  if (!quotaAvailable(Api::Prim, settings.dailyQuota)) {
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

static void fetchBoardNow(uint8_t st, bool arr) {
  const int64_t now = nowEpoch();
  if (!quotaAvailable(Api::Sncf, settings.sncfQuota)) {
    snprintf(lastErr, sizeof(lastErr), "Quota SNCF du jour atteint (%u appels)", (unsigned)settings.sncfQuota);
    boardNext[st][arr] = now + 300;
    return;
  }
  busy = true;
  draw();
  FetchResult r = sncfFetchBoard(settings.stations[st], arr, boards[st][arr]);
  busy = false;
  if (r.ok) {
    boardValid[st][arr] = true;
    lastErr[0] = 0;
    boardNext[st][arr] = 0;
    rtcBoard = boards[st][arr];
    rtcBoardKey = (uint8_t)(st * 2 + (arr ? 1 : 0));
  } else {
    copyStr(lastErr, r.msg, sizeof(lastErr));
    const bool hard = r.http == 401 || r.http == 403 || r.http == 429;
    boardNext[st][arr] = nowEpoch() + (hard ? 120 : 20);
    log_w("Échec SNCF : %s", r.msg);
  }
  draw();
}

static uint32_t trainsInterval() {
  uint32_t s = settings.refreshS < TRAINS_REFRESH_MIN_S ? TRAINS_REFRESH_MIN_S : settings.refreshS;
  if (settings.sleepMode == SleepMode::AlwaysOn && settings.sncfQuota > 0) {
    // allumé en permanence : environ 2 appels par mise à jour (tableau + provenances)
    uint32_t minS = 2 * 86400UL / settings.sncfQuota + 1;
    if (s < minS) s = minS;
  }
  return s;
}

static void maybeFetchTrains() {
  if (!screenOn || net != NetState::Online || busy) return;
  const int64_t now = nowEpoch();
  const uint8_t st = curStation;
  const bool arr = arrivals;
  if (now < boardNext[st][arr]) return;
  Board& b = boards[st][arr];
  const bool stale = boardForce[st][arr] || !boardValid[st][arr] ||
                     now - b.fetchedAt >= (int64_t)trainsInterval() || now < b.fetchedAt;
  if (stale) {
    boardForce[st][arr] = false;
    fetchBoardNow(st, arr);
    return;
  }
  // Arrivées : provenances à compléter, une requête à la fois
  if (arr && millis() - lastOriginTry > 1500) {
    lastOriginTry = millis();
    if (sncfResolveOrigins(b, settings.stations[st], BOARD_ROWS)) {
      rtcBoard = b;
      rtcBoardKey = (uint8_t)(st * 2 + (arr ? 1 : 0));
      draw();
    }
  }
}

static void maybeFetch() {
  if (app == App::Trains) return maybeFetchTrains();
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
  rtcApp = (uint8_t)app;
  rtcStation = curStation;
  rtcArrivals = arrivals;
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

static void switchApp() {
  const App next = app == App::Metro ? App::Trains : App::Metro;
  const bool ready = next == App::Trains ? trainsReady() : metroReady();
  if (!ready) {
    if (next == App::Trains)
      uiDrawMessage("Trains non configurés", "Ajoutez une clé API SNCF et des gares",
                    "Appui long sur l'écran : configuration", 0x1A4CA8);
    else
      uiDrawMessage("Métro non configuré", "Ajoutez une clé API PRIM et un arrêt",
                    "Appui long sur l'écran : configuration", 0xFFC12E);
    delay(1800);
    draw();
    return;
  }
  app = next;
  lastErr[0] = 0;
  uiDrawAppTitle(app == App::Trains);
  delay(450);
  draw();
}

static void onTap() {
  if (app == App::Trains) {  // départs <-> arrivées
    arrivals = !arrivals;
    lastErr[0] = 0;
    draw();
    return;
  }
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
  if (app == App::Trains) {
    if (settings.stationCount < 2) return;
    curStation = (uint8_t)((curStation + settings.stationCount + dir) % settings.stationCount);
    lastErr[0] = 0;
    draw();
    return;
  }
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
    else if (abs(dy) > 40 && abs(dy) > abs(dx)) switchApp();
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

  // Favoris ou gares modifiés depuis la dernière fois : on oublie l'état mémorisé
  uint32_t h = 2166136261u;
  auto mixIn = [&](const void* data, size_t n) {
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 16777619u;
  };
  mixIn(settings.fav, sizeof(settings.fav));
  mixIn(&settings.favCount, 1);
  mixIn(settings.stations, sizeof(settings.stations));
  mixIn(&settings.stationCount, 1);
  mixIn(&settings.mainlineOnly, 1);
  if (h != rtcCfgHash) {
    rtcCfgHash = h;
    rtcFav = 0;
    rtcDataFav = 0xFF;
    memset(rtcDirKey, 0, sizeof(rtcDirKey));
    rtcStation = 0;
    rtcArrivals = false;
    rtcBoardKey = 0xFF;
  }

  // Programme au démarrage : le dernier utilisé, s'il est configuré
  app = (rtcApp == (uint8_t)App::Trains && trainsReady()) || !metroReady() ? App::Trains : App::Metro;
  curStation = rtcStation < settings.stationCount ? rtcStation : 0;
  arrivals = rtcArrivals;
  if (rtcBoardKey / 2 < settings.stationCount && rtcBoard.count <= BOARD_MAX) {
    boards[rtcBoardKey / 2][rtcBoardKey & 1] = rtcBoard;
    boardValid[rtcBoardKey / 2][rtcBoardKey & 1] = true;
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
