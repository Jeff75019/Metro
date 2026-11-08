// Rendu des écrans sur PC (sans matériel) : génère des images .ppm dans out/.
//   cd test && make ui
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <string>

#include "../Metro/lines.h"
#include "../Metro/siri.h"
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

  uiDrawMessage("WiFi introuvable", "Impossible de joindre «\xC2\xA0" "Freebox-ABC123\xC2\xA0»",
                "Appui long sur l'écran : configuration", 0xFF9A3C);
  dump("08_message");

  uiDrawSplash("1.0.0");
  dump("09_demarrage");
  return 0;
}
