// Rendu des écrans sur PC (sans matériel) : génère des images .ppm dans out/.
//   cd test && make ui
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <string>

#include "../Metro/lines.h"
#include "../Metro/reseau.h"
#include "../Metro/siri.h"
#include "../Metro/sncf.h"
#include "../Metro/ui.h"

static void dump(const char* name) {
  LGFX_Sprite& c = uiCanvas();
  mkdir("out", 0755);
  std::string path = std::string("out/") + name + ".ppm";
  FILE* f = fopen(path.c_str(), "wb");
  fprintf(f, "P6\n%d %d\n255\n", (int)c.width(), (int)c.height());
  for (int y = 0; y < c.height(); y++)
    for (int x = 0; x < c.width(); x++) {
      auto p = c.readPixelRGB(x, y);
      fputc(p.R8(), f);
      fputc(p.G8(), f);
      fputc(p.B8(), f);
    }
  fclose(f);
  printf("-> %s\n", path.c_str());
}

static std::string readFile(const char* path) {
  FILE* f = fopen(path, "rb");
  std::string s;
  char buf[4096];
  size_t n;
  while (f && (n = fread(buf, 1, sizeof(buf), f)) > 0) s.append(buf, n);
  if (f) fclose(f);
  return s;
}

static UiState base(const LineInfo* li, const char* title) {
  UiState s;
  memset(&s, 0, sizeof(s));
  s.badge = li->name;
  s.mode = li->mode;
  s.badgeColor = li->color;
  s.badgeText = li->textColor;
  s.title = title;
  s.battery = 78;
  s.net = NetState::Online;
  s.ageS = 12;
  s.favIndex = 0;
  s.favCount = 3;
  return s;
}

int main() {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  uiBegin(nullptr);

  std::string json = readFile("data/bastille_m1.json");
  static StopData d;
  char err[64];
  parseStopMonitoring(json.data(), json.size(), "C01371", d, err, sizeof(err));
  static View v;

  // 1) Ligne 1, sens Château de Vincennes (dont un train supprimé)
  buildView(d, "", "", d.responseTime, v);
  UiState s = base(findLine("C01371"), d.stopName);
  s.view = &v;
  s.hasData = true;
  s.now = d.responseTime;
  uiDrawMain(s);
  dump("01_metro1");

  // 2) Autre sens : train à quai + 4 min
  buildView(d, "", "La Défense", d.responseTime, v);
  s.now = d.responseTime;
  s.favIndex = 1;
  uiDrawMain(s);
  dump("02_metro1_sens2");

  // 3) RER B, destinations longues, voie, horaire théorique, retard
  static StopData r;
  memset(&r, 0, sizeof(r));
  int64_t now = 1794127200;
  r.count = 2;
  r.dep[0].when = now + 7 * 60 + 20;
  copyStr(r.dep[0].dest, "Aéroport Charles de Gaulle 2 TGV", TXT_LEN);
  copyStr(r.dep[0].dir, "Nord", TXT_LEN);
  copyStr(r.dep[0].platform, "2", 6);
  r.dep[0].flags = DEP_REALTIME | DEP_DELAYED;
  r.dep[1].when = now + 18 * 60;
  copyStr(r.dep[1].dest, "Saint-Rémy-lès-Chevreuse", TXT_LEN);
  copyStr(r.dep[1].dir, "Nord", TXT_LEN);
  r.dep[1].flags = 0;
  buildView(r, "", "", now, v);
  s = base(findLine("C01743"), "Châtelet - Les Halles");
  s.view = &v;
  s.hasData = true;
  s.now = now;
  s.battery = 34;
  s.ageS = 75;
  s.favCount = 1;
  uiDrawMain(s);
  dump("03_rer_b");

  // 4) Tram T3a : à l'approche + horaire lointain
  r.count = 2;
  r.dep[0].when = now + 30;
  copyStr(r.dep[0].dest, "Pont du Garigliano", TXT_LEN);
  r.dep[0].flags = DEP_REALTIME;
  r.dep[0].platform[0] = 0;
  r.dep[1].when = now + 125 * 60;
  copyStr(r.dep[1].dest, "Pont du Garigliano", TXT_LEN);
  r.dep[1].flags = DEP_REALTIME;
  buildView(r, "", "", now, v);
  s = base(findLine("C01391"), "Porte d'Italie");
  s.view = &v;
  s.hasData = true;
  s.now = now + 1;
  s.battery = 12;
  uiDrawMain(s);
  dump("04_tram");

  // 5) Erreurs / états
  s = base(findLine("C01383"), "Saint-Denis - Université");
  s.view = nullptr;
  s.hasData = false;
  s.status = "Clé API refusée (401)";
  s.ageS = -1;
  uiDrawMain(s);
  dump("05_erreur");

  s.status = nullptr;
  s.hasData = true;
  View empty;
  memset(&empty, 0, sizeof(empty));
  s.view = &empty;
  uiDrawMain(s);
  dump("06_aucun");

  uiDrawPortal("Metro-3F2A", "WIFI:T:nopass;S:Metro-3F2A;;", "192.168.4.1", "ou http://metro.local");
  dump("07_portail");
  uiDrawPortal("Metro-3F2A", "WIFI:T:nopass;S:Metro-3F2A;;", "192.168.4.1", "WiFi : mot de passe refusé");
  dump("07b_portail_echec");

  uiDrawMessage("WiFi introuvable", "Impossible de joindre «\xC2\xA0" "Freebox-ABC123\xC2\xA0»",
                "Appui long sur l'écran : configuration", 0xFF9A3C);
  dump("08_message");

  uiDrawSplash("1.0.0");
  dump("09_demarrage");

  // 10) Premier chargement (squelette)
  s = base(findLine("C01374"), "Montparnasse - Bienvenüe");
  s.view = nullptr;
  s.hasData = false;
  s.net = NetState::Connecting;
  s.ageS = -1;
  s.now = now;
  uiDrawMain(s);
  dump("10_chargement");

  // 11) Bus (ligne absente de la table), données un peu anciennes
  r.count = 2;
  r.dep[0].when = now + 4 * 60 + 10;
  copyStr(r.dep[0].dest, "Gare Saint-Lazare", TXT_LEN);
  r.dep[0].flags = DEP_REALTIME;
  r.dep[1].when = now + 16 * 60;
  copyStr(r.dep[1].dest, "Gare Saint-Lazare", TXT_LEN);
  r.dep[1].flags = DEP_REALTIME;
  buildView(r, "", "", now, v);
  memset(&s, 0, sizeof(s));
  s.badge = "21";
  s.mode = LineMode::Bus;
  s.badgeColor = 0x5A6E8C;
  s.badgeText = 0xFFFFFF;
  s.title = "Porte de Gentilly";
  s.battery = 92;
  s.view = &v;
  s.hasData = true;
  s.now = now;
  s.net = NetState::Online;
  s.ageS = 240;
  s.favIndex = 2;
  s.favCount = 3;
  uiDrawMain(s);
  dump("11_bus");

  // 12) Programme Trains : départs de Paris Gare de Lyon
  std::string dep = readFile("data/sncf_gdl_departures.json");
  static Board bd;
  parseNavitiaBoard(dep.data(), dep.size(), false, true, "Gare de Lyon", bd, err, sizeof(err));
  BoardUi bu;
  memset(&bu, 0, sizeof(bu));
  bu.board = &bd;
  bu.station = "Gare de Lyon";
  bu.now = bd.serverTime;
  bu.battery = 80;
  bu.net = NetState::Online;
  bu.ageS = 20;
  bu.index = 0;
  bu.count = 7;
  uiDrawBoard(bu);
  dump("12_trains_departs");

  // 13) Arrivées à Paris Nord (une provenance encore en cours de recherche)
  std::string arr = readFile("data/sncf_nord_arrivals.json");
  static Board ba;
  parseNavitiaBoard(arr.data(), arr.size(), true, true, "Gare du Nord", ba, err, sizeof(err));
  // provenances complétées (comme après les requêtes /vehicle_journeys)
  copyStr(ba.t[1].place, "Lille Flandres", TXT_LEN);
  ba.t[1].flags &= ~TR_PLACE_PENDING;
  copyStr(ba.t[2].place, "Amiens", TXT_LEN);
  ba.t[2].flags &= ~TR_PLACE_PENDING;
  bu.board = &ba;
  bu.station = "Gare du Nord";
  bu.arrivals = true;
  bu.now = ba.serverTime;
  bu.index = 1;
  uiDrawBoard(bu);
  dump("13_trains_arrivees");

  // 14) Chargement, 15) aucun train
  bu.board = nullptr;
  bu.arrivals = false;
  bu.station = "Paris Bercy Bourgogne - Pays d'Auvergne";
  bu.ageS = -1;
  bu.busy = true;
  bu.index = 6;
  uiDrawBoard(bu);
  dump("14_trains_chargement");
  static Board empty2;
  memset(&empty2, 0, sizeof(empty2));
  bu.board = &empty2;
  bu.busy = false;
  bu.station = "Austerlitz";
  bu.index = 5;
  uiDrawBoard(bu);
  dump("15_trains_vide");

  uiDrawAppTitle(true, "Données SNCF (ODbL) via Train Nomade");
  dump("16_titre_trains");

  // 18) Source Train Nomade : voies affichées
  std::string tnj = readFile("data/trainnomade_gdl.json");
  static Board bt;
  parseTrainNomade(tnj.data(), tnj.size(), false, true, bt, err, sizeof(err));
  bu.board = &bt;
  bu.station = "Gare de Lyon";
  bu.arrivals = false;
  bu.now = bt.serverTime;
  bu.busy = false;
  bu.ageS = 30;
  bu.index = 0;
  uiDrawBoard(bu);
  dump("18_trains_voies");
  static Board ba2;
  parseTrainNomade(tnj.data(), tnj.size(), true, true, ba2, err, sizeof(err));
  bu.board = &ba2;
  bu.arrivals = true;
  uiDrawBoard(bu);
  dump("19_trains_voies_arrivees");
  uiDrawAppTitle(false);
  dump("17_titre_metro");

  // ===== Choix libre sur l'écran tactile =====
  // 20) Accueil
  HubTile tiles[] = {{TileKind::Metro, "Métro"},         {TileKind::Rer, "RER"},
                     {TileKind::Tram, "Tram"},           {TileKind::Trains, "Trains SNCF"},
                     {TileKind::MyStops, "Mes arrêts"},  {TileKind::MyStations, "Mes gares"}};
  HubUi hu;
  memset(&hu, 0, sizeof(hu));
  hu.tiles = tiles;
  hu.count = 6;
  hu.now = 1794130800;  // 8 nov. 2026, 11:20 à Paris
  hu.battery = 78;
  hu.net = NetState::Online;
  uiDrawHub(hu);
  dump("20_accueil");
  hu.count = 4;
  uiDrawHub(hu);
  dump("20b_accueil_sans_favoris");

  // 21-23) Lignes d'un mode
  static GridItem items[32];
  const LineMode modes[3] = {LineMode::Metro, LineMode::Rer, LineMode::Tram};
  const char* titles[3] = {"Métro", "RER", "Tramway"};
  const int cols[3] = {8, 5, 5};
  const char* names[3] = {"21_lignes_metro", "22_lignes_rer", "23_lignes_tram"};
  for (int m = 0; m < 3; m++) {
    int idx[32];
    const int n = netLinesOfMode(modes[m], idx, 32);
    for (int i = 0; i < n; i++) {
      const LineInfo* li = netLineInfo(idx[i]);
      items[i] = {li->name, modes[m], li->color, li->textColor, true};
    }
    GridUi g;
    memset(&g, 0, sizeof(g));
    g.title = titles[m];
    g.titleMode = modes[m];
    g.items = items;
    g.count = n;
    g.cols = cols[m];
    g.battery = 78;
    uiDrawGrid(g);
    dump(names[m]);
  }

  // 24-26) Stations d'une ligne
  struct Ctx {
    int line;
  } ctx;
  auto lineIdx = [](const char* name) {
    for (int i = 0; i < netLineCount(); i++)
      if (!strcmp(netLineInfo(i)->name, name)) return i;
    return 0;
  };
  ListUi l;
  memset(&l, 0, sizeof(l));
  l.title = "Choisissez la station";
  l.lineGraphic = true;
  l.name = [](int i, void* c) { return netStopName(((Ctx*)c)->line, i); };
  l.flags = [](int i, void* c) -> uint8_t {
    const int line = ((Ctx*)c)->line;
    return i < netLine(line).count ? netStop(line, i).flags : 0;
  };
  l.ctx = &ctx;
  l.battery = 78;
  const char* lists[][3] = {{"1", "0", "24_stations_ligne1"}, {"7", "31", "25_stations_ligne7_branche"},
                            {"A", "8", "26_stations_rerA"},  {"T3b", "26", "26b_stations_T3b"}};
  for (auto& e : lists) {
    ctx.line = lineIdx(e[0]);
    const LineInfo* li = netLineInfo(ctx.line);
    l.badge = li->name;
    l.mode = li->mode;
    l.color = li->color;
    l.textColor = li->textColor;
    l.count = netLine(ctx.line).count;
    l.top = atoi(e[1]);
    l.highlight = l.top + 2;
    uiDrawList(l);
    dump(e[2]);
  }

  // 27) Passages à une station choisie, dans chaque sens (RER A à La Défense)
  std::string ldj = readFile("data/prim_ladefense_reel.json");
  static StopData sd;
  parseStopMonitoring(ldj.data(), ldj.size(), "C01742", sd, err, sizeof(err));
  DirGroup gr[8];
  const int ng = buildDirections(sd, sd.responseTime, gr, 8);
  const LineInfo* rerA = findLine("C01742");
  UiState ss = base(rerA, "La Défense");
  ss.now = sd.responseTime;
  ss.back = true;
  ss.favCount = 0;
  static View vv;
  memset(&vv, 0, sizeof(vv));
  vv.groupCount = (uint8_t)((ng + 1) / 2);
  vv.groupIndex = 0;
  static char subs[2][48];
  for (int r = 0; r < 2 && r < ng; r++) {
    Row& row = vv.row[vv.rows++];
    row.when = gr[r].when[0];
    row.flags = gr[r].flags[0];
    copyStr(row.dest, gr[r].dest, sizeof(row.dest));
    subs[r][0] = 0;  // même texte que le firmware (Metro.ino, groupSub)
    if (gr[r].n >= 3)
      snprintf(subs[r], sizeof(subs[r]), "puis %d et %d min", (int)((gr[r].when[1] - ss.now) / 60),
               (int)((gr[r].when[2] - ss.now) / 60));
    else if (gr[r].n == 2)
      snprintf(subs[r], sizeof(subs[r]), "puis %d min", (int)((gr[r].when[1] - ss.now) / 60));
    ss.sub[r] = subs[r];
  }
  ss.view = &vv;
  ss.hasData = true;
  uiDrawMain(ss);
  dump("27_passages_station");

  // 28-29) Gares SNCF : initiales, puis gares
  static char lab[32][2];
  const int ngr = sncfGroupCount();
  for (int i = 0; i < ngr; i++) {
    const char* t = sncfGroupLabel(i);
    if (i > 0) {
      lab[i][0] = t[0];
      lab[i][1] = 0;
      t = lab[i];
    }
    items[i] = {t, LineMode::Train, 0, 0, false};
  }
  GridUi gg;
  memset(&gg, 0, sizeof(gg));
  gg.title = "Gares SNCF";
  gg.titleMode = LineMode::Train;
  gg.items = items;
  gg.count = ngr;
  gg.cols = 7;
  gg.battery = 78;
  uiDrawGrid(gg);
  dump("28_gares_initiales");
  static int gidx[48];
  static int gcount;
  gcount = sncfGroupStations(1, gidx, 48);
  ListUi gl;
  memset(&gl, 0, sizeof(gl));
  gl.title = "Gares \xC2\xB7 A";
  gl.mode = LineMode::Train;
  gl.color = 0x1A4CA8;
  gl.textColor = 0xFFFFFF;
  gl.count = gcount;
  gl.top = 0;
  gl.highlight = 1;
  gl.name = [](int i, void*) { return sncfPick(gidx[i]).name; };
  gl.battery = 78;
  uiDrawList(gl);
  dump("29_gares_A");

  // 30) Tableau d'une gare choisie (avec flèche de retour)
  bu.board = &bt;
  bu.station = "Paris Gare de Lyon";
  bu.arrivals = false;
  bu.back = true;
  bu.count = 0;
  uiDrawBoard(bu);
  dump("30_tableau_gare_choisie");

  // Zones de toucher : cohérentes avec le dessin
  if (uiHitHub(6, 60, 70) != 0 || uiHitHub(6, 160, 140) != 4 || !uiHitBack(12, 18) || uiHitList(100, 40) != 0 ||
      uiHitList(100, 165) != 4 || uiHitGrid(16, 8, 15, 70) != 0) {
    printf("ÉCHEC : zones de toucher incohérentes\n");
    return 1;
  }
  return 0;
}
