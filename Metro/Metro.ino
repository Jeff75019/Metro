// =============================================================================
//  Afficheur Métro — prochains passages en temps réel (Île-de-France)
//  pour Waveshare ESP32-S3-Touch-LCD-1.9 (+ batterie LiPo)
//
//  Accueil : Métro, RER, Tram, Trains SNCF (+ « Mes arrêts », « Mes gares »).
//    - Métro / RER / Tram : ligne -> station -> prochains passages dans chaque
//      sens (API PRIM d'Île-de-France Mobilités, clé gratuite)
//    - Trains SNCF : gare -> départs / arrivées (Train Nomade, sans clé)
//    - Mes arrêts / Mes gares : favoris réglés sur la page de configuration
//
//  Utilisation :
//    - Premier démarrage : l'écran affiche un QR code. Rejoignez le WiFi
//      « Metro-XXXX » et ouvrez http://192.168.4.1 pour régler le WiFi et la clé.
//    - Toucher                 : choisir (case, ligne, station, gare) ;
//                                passages : sens suivant ; trains : départs <-> arrivées
//    - Flèche « ‹ » ou glisser vers la droite : retour
//    - Glisser haut/bas        : listes : défiler ; ailleurs : accueil
//    - Glisser gauche/droite   : favoris : arrêt / gare suivant(e) ou précédent(e)
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
#include "reseau.h"
#include "prim.h"
#include "settings.h"
#include "siri.h"
#include "sncf.h"
#include "sncf_api.h"
#include "ui.h"

// Écrans : accueil, favoris, et choix libre (lignes -> stations -> passages ;
// initiales -> gares -> tableau)
enum class Scr : uint8_t { Hub = 0, MyStops, MyStations, Lines, Stops, Stop, Groups, Gares, Board };

// Choix libre : ce que l'utilisateur a choisi (conservé pendant la veille)
struct Explore {
  uint8_t mode;   // lignes affichées : 0 métro, 1 RER, 2 tramway
  uint8_t line;   // ligne choisie (indice dans le réseau embarqué)
  uint8_t stop;   // station choisie (0xFF : aucune)
  uint8_t top;    // première station visible dans la liste
  uint8_t group;  // groupe de gares : 0 Paris, puis initiales
  uint8_t pick;   // gare choisie (indice dans la liste des gares)
  uint8_t gtop;   // première gare visible dans la liste
  uint8_t page;   // page des destinations (2 par page)
  bool arrivals;  // arrivées plutôt que départs
};

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
RTC_DATA_ATTR static uint8_t rtcScr = 0;                          // écran affiché
RTC_DATA_ATTR static Explore rtcEx;                               // choix libre en cours
RTC_DATA_ATTR static uint16_t rtcExKey = 0xFFFF;                  // station de rtcData (choix libre)
RTC_DATA_ATTR static uint8_t rtcExPick = 0xFF;                    // gare de rtcBoard (choix libre)
RTC_DATA_ATTR static uint8_t rtcStation = 0;                      // gare affichée
RTC_DATA_ATTR static bool rtcArrivals = false;                    // arrivées plutôt que départs
RTC_DATA_ATTR static Board rtcBoard;                              // dernier tableau SNCF
RTC_DATA_ATTR static uint8_t rtcBoardKey = 0xFF;                  // gare * 2 + arrivées

// ---- État en RAM ------------------------------------------------------------
static StopData cache[MAX_FAVORITES];
static bool cacheValid[MAX_FAVORITES];
static int64_t nextAttempt[MAX_FAVORITES];  // pas de nouvel essai avant (epoch)
static bool forceFetch[MAX_FAVORITES];
static char lastErr[96];
static View lastView;
static uint8_t curFav = 0;
static NetState net = NetState::Off;
static bool busy = false;
static bool screenOn = true;
static bool interacted = false;
static WakeReason wake = WakeReason::PowerOn;
static Scr scr = Scr::Hub;
static Explore ex;

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
static uint8_t wifiFail = 0, wifiFailIdx = 0;  // dernière raison d'échec (mot de passe refusé…)
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
    if (wifiFail && wifiFailIdx < n)
      snprintf(lastErr, sizeof(lastErr), "WiFi «\xC2\xA0%s\xC2\xA0» : %s", settings.wifi[wifiFailIdx].ssid,
               wifiReasonText(wifiFail));
    else
      snprintf(lastErr, sizeof(lastErr), "WiFi «\xC2\xA0%s\xC2\xA0» introuvable", n ? settings.wifi[0].ssid : "?");
    wifiStart = millis();
    return;
  }
  const bool fast = haveFast && wifiAttempt == 0;
  const int start = rtcWifiIdx < n ? rtcWifiIdx : 0;
  wifiCurIdx = fast ? start : (start + wifiAttempt - (haveFast ? 1 : 0)) % n;
  const WifiCred& w = settings.wifi[wifiCurIdx];
  WiFi.disconnect(false, false);
  wifiClearReason();
  if (fast) WiFi.begin(w.ssid, w.pass, rtcChannel, rtcBssid);
  else WiFi.begin(w.ssid, w.pass);
  wifiStart = millis();
  wifiTimeout = fast ? 5000 : 15000;
  net = NetState::Connecting;
  log_i("WiFi : connexion à %s%s", w.ssid, fast ? " (rapide)" : "");
}

static void wifiBegin() {
  WiFi.persistent(false);
  WiFi.setHostname(MDNS_NAME);
  WiFi.mode(WIFI_STA);
  wifiSetup();
  wifiAttempt = 0;
  wifiFail = 0;
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
        wifiFail = 0;
        if (!ntpStarted) {
          configTzTime(TZ_PARIS, "pool.ntp.org", "time.cloudflare.com");
          ntpStarted = true;
        }
        log_i("WiFi connecté : %s", WiFi.localIP().toString().c_str());
      } else if (millis() - wifiStart > wifiTimeout) {
        if (wifiAttempt == 0) rtcChannel = 0;  // l'accès rapide a échoué : on l'oublie
        if (wifiLastReason()) {
          wifiFail = wifiLastReason();
          wifiFailIdx = (uint8_t)wifiCurIdx;
        }
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
  if (!s.status && valid && b.validUntil > 0 && s.now > b.validUntil + 60)
    s.status = "Données en direct indisponibles";  // la source n'est plus à jour
  s.ageS = valid ? (int)(s.now - b.fetchedAt) : -1;
  s.index = curStation;
  s.count = settings.stationCount;
  s.back = true;
  uiDrawBoard(s);
  lastDraw = millis();
}

static void drawMyStops() {
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
  s.back = true;
  uiDrawMain(s);
  lastDraw = millis();
}

// ---- Choix libre : accueil, lignes, stations, passages, gares ------------------
static const LineMode EX_MODES[3] = {LineMode::Metro, LineMode::Rer, LineMode::Tram};
static const char* const EX_TITLES[3] = {"Métro", "RER", "Tramway"};
static const int EX_COLS[3] = {7, 5, 5};
static const char* const TN_CREDIT = "Données SNCF (ODbL) via Train Nomade";

static HubTile hubTiles[6];
static int hubCount = 0;
static int exLines[24], exLineCount = 0;
static GridItem exItems[32];
static char exGroupLabels[32][2];
static int exGares[48], exGareCount = 0;

// Station choisie : passages regroupés par destination
static StopData exStop;
static bool exStopValid = false, exStopForce = false;
static uint16_t exStopKey = 0xFFFF;
static int64_t exStopNext = 0;
static DirGroup exGroups[8];
static int exGroupCount = 0;
static char exSub[2][48];

// Gare choisie : tableaux des départs et des arrivées
static Board exBoards[2];
static bool exBoardValid[2], exBoardForce[2];
static int64_t exBoardNext[2];
static uint8_t exBoardPick = 0xFF;
static Station exStation;

static uint16_t exKey() { return (uint16_t)(ex.line << 8 | ex.stop); }

static void buildHub() {
  hubCount = 0;
  static uint32_t favColors[MAX_FAVORITES];  // couleurs des lignes des favoris
  uint8_t nfav = 0;
  for (int i = 0; i < settings.favCount && nfav < MAX_FAVORITES; i++) {
    const LineInfo* li = findLine(settings.fav[i].line);
    favColors[nfav++] = li ? li->color : (settings.fav[i].badge[0] ? settings.fav[i].color : 0x5A6E8C);
  }
  hubTiles[hubCount++] = {TileKind::Metro, "Métro", nullptr, 0};
  hubTiles[hubCount++] = {TileKind::Rer, "RER", nullptr, 0};
  hubTiles[hubCount++] = {TileKind::Tram, "Tram", nullptr, 0};
  hubTiles[hubCount++] = {TileKind::Trains, "Trains SNCF", nullptr, 0};
  if (metroReady()) hubTiles[hubCount++] = {TileKind::MyStops, "Mes arrêts", favColors, nfav};
  if (trainsReady()) hubTiles[hubCount++] = {TileKind::MyStations, "Mes gares", nullptr, 0};
}

static void loadLines() {
  if (ex.mode > 2) ex.mode = 0;
  exLineCount = netLinesOfMode(EX_MODES[ex.mode], exLines, 24);
}

static void loadGares() { exGareCount = sncfGroupStations(ex.group, exGares, 48); }

static const char* stopNameCb(int i, void*) { return netStopName(ex.line, i); }
static uint8_t stopFlagsCb(int i, void*) { return i < netLine(ex.line).count ? netStop(ex.line, i).flags : 0; }
static const char* gareNameCb(int i, void*) { return sncfPick(exGares[i]).name; }
static const char* stopNoteCb(int i, void*) {  // année d'ouverture des stations en construction
  static char y[8];
  const uint16_t year = i < netLine(ex.line).count ? netStop(ex.line, i).year : 0;
  if (!year) return nullptr;
  snprintf(y, sizeof(y), "%u", (unsigned)year);
  return y;
}

static void drawHub() {
  buildHub();
  HubUi h;
  memset(&h, 0, sizeof(h));
  h.tiles = hubTiles;
  h.count = hubCount;
  h.now = nowEpoch();
  h.battery = battPct;
  h.net = net;
  uiDrawHub(h);
}

static void drawLines() {
  loadLines();
  for (int i = 0; i < exLineCount; i++) {
    const LineInfo* li = netLineInfo(exLines[i]);
    exItems[i] = {li ? li->name : "?", EX_MODES[ex.mode], li ? li->color : 0x5A6E8C, li ? li->textColor : 0xFFFFFF,
                  true, netLine(exLines[i]).year != 0};
  }
  GridUi g;
  memset(&g, 0, sizeof(g));
  g.title = EX_TITLES[ex.mode];
  g.titleMode = EX_MODES[ex.mode];
  g.items = exItems;
  g.count = exLineCount;
  g.cols = EX_COLS[ex.mode];
  g.battery = battPct;
  uiDrawGrid(g);
}

static void drawStops() {
  const LineInfo* li = netLineInfo(ex.line);
  ListUi l;
  memset(&l, 0, sizeof(l));
  l.title = "Choisissez la station";
  l.badge = li ? li->name : "?";
  l.mode = li ? li->mode : LineMode::Metro;
  l.color = li ? li->color : 0x5A6E8C;
  l.textColor = li ? li->textColor : 0xFFFFFF;
  l.lineGraphic = true;
  l.count = netLine(ex.line).count;
  l.top = ex.top;
  l.highlight = ex.stop < l.count ? ex.stop : -1;
  l.name = stopNameCb;
  l.flags = stopFlagsCb;
  l.note = stopNoteCb;
  l.battery = battPct;
  uiDrawList(l);
}

// « puis 7 et 12 min » (et la voie quand elle est connue)
static void groupSub(const DirGroup& g, int64_t now, char* out, size_t n) {
  char t[32] = "";
  int m[DIR_TIMES];
  for (int k = 0; k < g.n; k++) m[k] = g.when[k] > now ? (int)((g.when[k] - now) / 60) : 0;
  if (g.n >= 3) snprintf(t, sizeof(t), "puis %d et %d min", m[1], m[2]);
  else if (g.n == 2) snprintf(t, sizeof(t), "puis %d min", m[1]);
  if (g.platform[0] && t[0]) snprintf(out, n, "Voie %s \xC2\xB7 %s", g.platform, t);
  else if (g.platform[0]) snprintf(out, n, "Voie %s", g.platform);
  else copyStr(out, t, n);
}

// Place d'une destination sur la ligne (pour ranger les destinations d'un bout
// à l'autre : celles d'un même côté se suivent). 10000 : destination inconnue.
static int destPos(const char* dest) {
  char nd[TXT_LEN * 2], ns[TXT_LEN * 2];
  normalizeAscii(dest, nd, sizeof(nd));
  const size_t ld = strlen(nd);
  for (int k = 0; k < netLine(ex.line).count; k++) {
    normalizeAscii(netStopName(ex.line, k), ns, sizeof(ns));
    const size_t ls = strlen(ns);
    // « La Défense (Grande Arche) » <-> « La Défense », « Saint-Denis » <-> « Saint-Denis - Université »
    if (!strcmp(nd, ns) || (ls >= 5 && !strncmp(nd, ns, ls)) || (ld >= 5 && !strncmp(ns, nd, ld)))
      return netStop(ex.line, k).pos;
  }
  return 10000;
}

static void sortGroupsAlongLine() {
  int pos[8];
  for (int i = 0; i < exGroupCount; i++) pos[i] = destPos(exGroups[i].dest);
  for (int i = 1; i < exGroupCount; i++)  // tri par insertion (stable)
    for (int j = i; j > 0 && pos[j - 1] > pos[j]; j--) {
      DirGroup t = exGroups[j];
      exGroups[j] = exGroups[j - 1];
      exGroups[j - 1] = t;
      int tp = pos[j];
      pos[j] = pos[j - 1];
      pos[j - 1] = tp;
    }
}

static void drawStop() {
  const LineInfo* li = netLineInfo(ex.line);
  UiState s;
  memset(&s, 0, sizeof(s));
  s.badge = li ? li->name : "?";
  s.mode = li ? li->mode : LineMode::Metro;
  s.badgeColor = li ? li->color : 0x5A6E8C;
  s.badgeText = li ? li->textColor : 0xFFFFFF;
  s.title = netStopName(ex.line, ex.stop);
  s.now = nowEpoch();
  s.back = true;
  memset(&lastView, 0, sizeof(lastView));
  exGroupCount = exStopValid ? buildDirections(exStop, s.now, exGroups, 8) : 0;
  sortGroupsAlongLine();
  const int pages = (exGroupCount + 1) / 2;
  if (ex.page >= pages) ex.page = 0;
  lastView.groupCount = (uint8_t)(pages > 1 ? pages : 1);
  lastView.groupIndex = ex.page;
  for (int r = 0; r < 2 && ex.page * 2 + r < exGroupCount; r++) {
    const DirGroup& g = exGroups[ex.page * 2 + r];
    Row& row = lastView.row[lastView.rows++];
    row.when = g.when[0];
    row.flags = g.flags[0];
    copyStr(row.dest, g.dest, sizeof(row.dest));
    copyStr(row.platform, g.platform, sizeof(row.platform));
    groupSub(g, s.now, exSub[r], sizeof(exSub[r]));
    s.sub[r] = exSub[r];
  }
  s.view = exStopValid ? &lastView : nullptr;
  s.hasData = exStopValid;
  s.battery = battPct;
  s.net = net;
  s.busy = busy;
  s.status = lastErr[0] ? lastErr : nullptr;
  s.ageS = exStopValid ? (int)(s.now - exStop.fetchedAt) : -1;
  // Lignes en construction
  static char openText[48];
  const NetStop& ns = netStop(ex.line, ex.stop);
  const uint16_t lineYear = netLine(ex.line).year;
  if (ns.zda == 0) {  // station pas encore référencée : rien à demander
    snprintf(openText, sizeof(openText), "Ouverture prévue en %u", (unsigned)(ns.year ? ns.year : lineYear));
    s.view = nullptr;
    s.hasData = true;
    s.status = nullptr;
    s.ageS = -1;
    s.emptyTitle = "Pas encore en service";
    s.emptySub = openText;
  } else if (lineYear) {
    snprintf(openText, sizeof(openText), "Ouverture de la ligne prévue en %u", (unsigned)lineYear);
    s.emptySub = openText;
  }
  uiDrawMain(s);
}

static void drawGroups() {
  const int n = sncfGroupCount() < 32 ? sncfGroupCount() : 32;
  for (int i = 0; i < n; i++) {
    const char* lab = sncfGroupLabel(i);
    if (i > 0) {
      exGroupLabels[i][0] = lab[0];
      exGroupLabels[i][1] = 0;
      lab = exGroupLabels[i];
    }
    exItems[i] = {lab, LineMode::Train, 0, 0, false, false};
  }
  GridUi g;
  memset(&g, 0, sizeof(g));
  g.title = "Gares SNCF";
  g.titleMode = LineMode::Train;
  g.items = exItems;
  g.count = n;
  g.cols = 7;
  g.battery = battPct;
  uiDrawGrid(g);
}

static void drawGares() {
  loadGares();
  ListUi l;
  memset(&l, 0, sizeof(l));
  char title[32];
  if (ex.group == 0) copyStr(title, "Gares de Paris", sizeof(title));
  else snprintf(title, sizeof(title), "Gares \xC2\xB7 %s", sncfGroupLabel(ex.group));
  l.title = title;
  l.mode = LineMode::Train;
  l.color = 0x1A4CA8;
  l.textColor = 0xFFFFFF;
  l.count = exGareCount;
  l.top = ex.gtop;
  l.highlight = -1;
  for (int i = 0; i < exGareCount; i++)
    if (exGares[i] == ex.pick) l.highlight = i;
  l.name = gareNameCb;
  l.battery = battPct;
  uiDrawList(l);
}

static void drawExBoard() {
  BoardUi s;
  memset(&s, 0, sizeof(s));
  const bool arr = ex.arrivals;
  const bool valid = exBoardPick == ex.pick && exBoardValid[arr];
  const Board& b = exBoards[arr];
  s.board = valid ? &b : nullptr;
  s.station = sncfPick(ex.pick).name;
  s.arrivals = arr;
  s.now = nowEpoch();
  s.battery = battPct;
  s.net = net;
  s.busy = busy;
  s.status = lastErr[0] ? lastErr : nullptr;
  if (!s.status && valid && b.validUntil > 0 && s.now > b.validUntil + 60) s.status = "Données en direct indisponibles";
  s.ageS = valid ? (int)(s.now - b.fetchedAt) : -1;
  s.back = true;
  uiDrawBoard(s);
}

static void draw() {
  if (!screenOn) return;
  switch (scr) {
    case Scr::MyStops: drawMyStops(); break;
    case Scr::MyStations: drawTrains(); break;
    case Scr::Lines: drawLines(); break;
    case Scr::Stops: drawStops(); break;
    case Scr::Stop: drawStop(); break;
    case Scr::Groups: drawGroups(); break;
    case Scr::Gares: drawGares(); break;
    case Scr::Board: drawExBoard(); break;
    default: drawHub(); break;
  }
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
  if (settings.trainSource == TrainSource::SncfApi && !quotaAvailable(Api::Sncf, settings.sncfQuota)) {
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

// Station choisie sur l'écran : une requête PRIM (arrêt + ligne) par mise à jour
static void maybeFetchExStop() {
  if (!screenOn || net != NetState::Online || busy) return;
  if (netStop(ex.line, ex.stop).zda == 0) return;  // station pas encore en service
  if (!settings.apiKey[0]) {
    copyStr(lastErr, "Clé API PRIM manquante : appui long 3 s pour la configuration", sizeof(lastErr));
    return;
  }
  if (exKey() != exStopKey) {
    exStopKey = exKey();
    exStopValid = false;
    exStopNext = 0;
    exStopForce = true;
    lastErr[0] = 0;
  }
  const int64_t now = nowEpoch();
  if (now < exStopNext) return;
  const bool stale = exStopForce || !exStopValid || now - exStop.fetchedAt >= (int64_t)refreshInterval() ||
                     now < exStop.fetchedAt;
  if (!stale) return;
  exStopForce = false;
  if (!quotaAvailable(Api::Prim, settings.dailyQuota)) {
    snprintf(lastErr, sizeof(lastErr), "Quota du jour atteint (%u appels)", (unsigned)settings.dailyQuota);
    exStopNext = now + 300;
    return;
  }
  char ref[48];
  netStopRef(ex.line, ex.stop, ref, sizeof(ref));
  busy = true;
  draw();
  FetchResult r = primFetchStop(ref, netLine(ex.line).id, settings.apiKey, exStop);
  busy = false;
  if (r.ok) {
    exStopValid = true;
    lastErr[0] = 0;
    exStopNext = 0;
    rtcData = exStop;
    rtcDataFav = 0xFE;  // donnée du choix libre
    rtcExKey = exStopKey;
  } else {
    copyStr(lastErr, r.msg, sizeof(lastErr));
    const bool hard = r.http == 401 || r.http == 403 || r.http == 429;
    exStopNext = nowEpoch() + (hard ? 120 : 15);
    log_w("Échec : %s", r.msg);
  }
  draw();
}

// Gare choisie sur l'écran : tableau des départs ou des arrivées
static void maybeFetchExBoard() {
  if (!screenOn || net != NetState::Online || busy) return;
  const bool arr = ex.arrivals;
  if (exBoardPick != ex.pick) {
    exBoardPick = ex.pick;
    exBoardValid[0] = exBoardValid[1] = false;
    exBoardNext[0] = exBoardNext[1] = 0;
    lastErr[0] = 0;
  }
  const int64_t now = nowEpoch();
  if (now < exBoardNext[arr]) return;
  Board& b = exBoards[arr];
  const bool stale = exBoardForce[arr] || !exBoardValid[arr] || now - b.fetchedAt >= (int64_t)trainsInterval() ||
                     now < b.fetchedAt;
  if (!stale) return;
  exBoardForce[arr] = false;
  if (settings.trainSource == TrainSource::SncfApi && !quotaAvailable(Api::Sncf, settings.sncfQuota)) {
    snprintf(lastErr, sizeof(lastErr), "Quota SNCF du jour atteint (%u appels)", (unsigned)settings.sncfQuota);
    exBoardNext[arr] = now + 300;
    return;
  }
  copyStr(exStation.id, sncfPick(ex.pick).uic, sizeof(exStation.id));
  copyStr(exStation.name, sncfPick(ex.pick).name, sizeof(exStation.name));
  busy = true;
  draw();
  FetchResult r = sncfFetchBoard(exStation, arr, b);
  busy = false;
  if (r.ok) {
    exBoardValid[arr] = true;
    lastErr[0] = 0;
    exBoardNext[arr] = 0;
    rtcBoard = b;
    rtcBoardKey = (uint8_t)(0xF0 | (arr ? 1 : 0));  // tableau du choix libre
    rtcExPick = ex.pick;
  } else {
    copyStr(lastErr, r.msg, sizeof(lastErr));
    const bool hard = r.http == 401 || r.http == 403 || r.http == 429;
    exBoardNext[arr] = nowEpoch() + (hard ? 120 : 20);
    log_w("Échec SNCF : %s", r.msg);
  }
  draw();
}

static void maybeFetch() {
  switch (scr) {
    case Scr::MyStations: return maybeFetchTrains();
    case Scr::Stop: return maybeFetchExStop();
    case Scr::Board: return maybeFetchExBoard();
    case Scr::MyStops: break;
    default: return;
  }
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
  rtcScr = (uint8_t)scr;
  rtcEx = ex;
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

static void goScr(Scr next) {
  scr = next;
  lastErr[0] = 0;
  draw();
}

static void onBack() {
  switch (scr) {
    case Scr::Stops: goScr(Scr::Lines); break;
    case Scr::Stop: goScr(Scr::Stops); break;
    case Scr::Gares: goScr(Scr::Groups); break;
    case Scr::Board: goScr(Scr::Gares); break;
    case Scr::Hub: break;
    default: goScr(Scr::Hub); break;  // lignes, initiales, favoris
  }
}

static void showTrainsCredit() {
  uiDrawAppTitle(true, settings.trainSource == TrainSource::TrainNomade ? TN_CREDIT : "Données : API SNCF");
  delay(450);
}

static void onHubTap(int x, int y) {
  buildHub();
  const int i = uiHitHub(hubCount, x, y);
  if (i < 0) return;
  switch (hubTiles[i].kind) {
    case TileKind::Metro: ex.mode = 0; goScr(Scr::Lines); break;
    case TileKind::Rer: ex.mode = 1; goScr(Scr::Lines); break;
    case TileKind::Tram: ex.mode = 2; goScr(Scr::Lines); break;
    case TileKind::Trains: showTrainsCredit(); goScr(Scr::Groups); break;
    case TileKind::MyStops: goScr(Scr::MyStops); break;
    case TileKind::MyStations: showTrainsCredit(); goScr(Scr::MyStations); break;
  }
}

static void onTap(int x, int y) {
  log_i("Toucher (%d, %d), écran %d", x, y, (int)scr);
  if (scr != Scr::Hub && uiHitBack(x, y)) return onBack();
  switch (scr) {
    case Scr::Hub:
      onHubTap(x, y);
      break;
    case Scr::MyStations:  // départs <-> arrivées
      arrivals = !arrivals;
      lastErr[0] = 0;
      draw();
      break;
    case Scr::MyStops:
      if (lastView.groupCount > 1) {
        uint8_t g = (lastView.groupIndex + 1) % lastView.groupCount;
        copyStr(rtcDirKey[curFav], lastView.groupKey[g], TXT_LEN);
        draw();
      } else if (cacheValid[curFav] && nowEpoch() - cache[curFav].fetchedAt > 10) {
        nextAttempt[curFav] = 0;
        forceFetch[curFav] = true;  // force une mise à jour
      }
      break;
    case Scr::Lines: {
      loadLines();
      const int i = uiHitGrid(exLineCount, EX_COLS[ex.mode], x, y);
      if (i < 0) break;
      if (exLines[i] != ex.line) {
        ex.line = (uint8_t)exLines[i];
        ex.top = 0;
        ex.stop = 0xFF;
      }
      goScr(Scr::Stops);
      break;
    }
    case Scr::Stops: {
      const int r = uiHitList(x, y);
      const int i = ex.top + r;
      if (r < 0 || i >= netLine(ex.line).count) break;
      ex.stop = (uint8_t)i;
      ex.page = 0;
      goScr(Scr::Stop);
      break;
    }
    case Scr::Stop:
      if (lastView.groupCount > 1) {  // destinations suivantes
        ex.page = (uint8_t)((ex.page + 1) % lastView.groupCount);
        draw();
      } else if (exStopValid && nowEpoch() - exStop.fetchedAt > 10) {
        exStopNext = 0;
        exStopForce = true;  // force une mise à jour
      }
      break;
    case Scr::Groups: {
      const int i = uiHitGrid(sncfGroupCount(), 7, x, y);
      if (i < 0) break;
      if (i != ex.group) {
        ex.group = (uint8_t)i;
        ex.gtop = 0;
      }
      goScr(Scr::Gares);
      break;
    }
    case Scr::Gares: {
      loadGares();
      const int r = uiHitList(x, y);
      const int i = ex.gtop + r;
      if (r < 0 || i >= exGareCount) break;
      ex.pick = (uint8_t)exGares[i];
      goScr(Scr::Board);
      break;
    }
    case Scr::Board:  // départs <-> arrivées
      ex.arrivals = !ex.arrivals;
      lastErr[0] = 0;
      draw();
      break;
  }
}

// dir = +1 : doigt vers la gauche (élément suivant) ; -1 : vers la droite
static void onSwipe(int dir) {
  if (scr == Scr::MyStations) {
    if (settings.stationCount < 2) return;
    curStation = (uint8_t)((curStation + settings.stationCount + dir) % settings.stationCount);
    lastErr[0] = 0;
    draw();
    return;
  }
  if (scr == Scr::MyStops) {
    if (settings.favCount < 2) return;
    curFav = (uint8_t)((curFav + settings.favCount + dir) % settings.favCount);
    lastErr[0] = 0;
    draw();
    return;
  }
  if (dir < 0) return onBack();  // glisser vers la droite : retour
  if (scr == Scr::Stop && lastView.groupCount > 1) {
    ex.page = (uint8_t)((ex.page + 1) % lastView.groupCount);
    draw();
  }
}

// dir = +1 : doigt vers le haut (suite de la liste) ; -1 : vers le bas
static void scrollList(uint8_t& top, int count, int dir) {
  int t = top + dir * (LIST_ROWS - 1);
  if (t > count - LIST_ROWS) t = count - LIST_ROWS;
  if (t < 0) t = 0;
  top = (uint8_t)t;
  draw();
}

static void onVSwipe(int dir) {
  if (scr == Scr::Stops) return scrollList(ex.top, netLine(ex.line).count, dir);
  if (scr == Scr::Gares) {
    loadGares();
    return scrollList(ex.gtop, exGareCount, dir);
  }
  if (scr != Scr::Hub) goScr(Scr::Hub);
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
    else if (abs(dy) > 40 && abs(dy) > abs(dx)) onVSwipe(dy < 0 ? +1 : -1);
    else if (abs(dx) < 30 && abs(dy) < 30) onTap(x0, y0);
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

  // Écran au démarrage : le dernier affiché (s'il est encore valable), sinon l'accueil
  ex = rtcEx;
  if (wake == WakeReason::PowerOn) {  // mise sous tension : rien de choisi
    memset(&ex, 0, sizeof(ex));
    ex.stop = 0xFF;
    ex.pick = 0xFF;
  }
  if (ex.mode > 2) ex.mode = 0;
  if (ex.line >= netLineCount()) ex.line = 0;
  if (ex.stop != 0xFF && ex.stop >= netLine(ex.line).count) ex.stop = 0xFF;
  if (ex.top >= netLine(ex.line).count) ex.top = 0;
  if (ex.group >= sncfGroupCount()) ex.group = 0;
  if (ex.pick != 0xFF && ex.pick >= sncfPickCount()) ex.pick = 0xFF;
  scr = rtcScr <= (uint8_t)Scr::Board ? (Scr)rtcScr : Scr::Hub;
  if ((scr == Scr::MyStops && !metroReady()) || (scr == Scr::MyStations && !trainsReady())) scr = Scr::Hub;
  if (scr == Scr::Stop && ex.stop == 0xFF) scr = Scr::Stops;
  if (scr == Scr::Board && ex.pick == 0xFF) scr = Scr::Gares;
  if (rtcDataFav == 0xFE && rtcExKey == exKey() && rtcData.count <= MAX_DEPARTURES) {
    exStop = rtcData;
    exStopValid = true;
    exStopKey = rtcExKey;
  }
  if ((rtcBoardKey & 0xF0) == 0xF0 && rtcExPick == ex.pick && rtcBoard.count <= BOARD_MAX) {
    exBoards[rtcBoardKey & 1] = rtcBoard;
    exBoardValid[rtcBoardKey & 1] = true;
    exBoardPick = ex.pick;
  }
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
