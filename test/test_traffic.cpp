// Tests de l'info trafic (vraie réponse PRIM réduite aux lignes de l'afficheur
// et à 3 lignes de bus).   cd test && make test
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fstream>

#include "../Metro/lines.h"
#include "../Metro/sncf.h"
#include "../Metro/traffic.h"

static int fails = 0;
#define CHECK(c)                                              \
  do {                                                        \
    if (!(c)) {                                               \
      printf("ÉCHEC %s:%d : %s\n", __FILE__, __LINE__, #c); \
      fails++;                                                \
    }                                                         \
  } while (0)

int main() {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  // --- HTML -> texte
  char t[256];
  htmlToText("<p>P&#233;riode : en soir&#233;e.<br><br>Trafic interrompu entre A&#8211;B.</p><p>Motif&nbsp;: travaux</p>",
             t, sizeof(t));
  printf("[%s]\n", t);
  CHECK(!strcmp(t, "Période : en soirée.\n\nTrafic interrompu entre A–B.\n\nMotif : travaux"));
  htmlToText("Fl&egrave;che &rarr; ici &amp; l&agrave; \xF0\x9F\x9A\x87 fin", t, sizeof(t));  // emoji : retiré
  CHECK(!strcmp(t, "Flèche -> ici & là fin"));
  htmlToText("<ul><li>un</li><li>deux</li></ul>", t, sizeof(t));
  CHECK(!strcmp(t, "\xC2\xB7 un\n\xC2\xB7 deux"));
  htmlToText("tr\xC3\xA8s long texte", t, 6);  // coupé sans casser un caractère
  CHECK(strlen(t) <= 5);

  // --- Vraie réponse (8 novembre 2026, 18 h à Paris)
  std::ifstream f("data/prim_infotrafic_reel.json", std::ios::binary);
  JsonDocument filter, doc;
  trafficFilter(filter);
  DeserializationError e = deserializeJson(doc, f, DeserializationOption::Filter(filter));
  static TrafficData td;
  static char text[32768];
  td.text = text;
  td.textCap = sizeof(text);
  int64_t now;
  parseNavitiaTime("20261108T180000", &now);
  const int n = parseTraffic(doc, e, now, 12 * 3600, td);
  printf("%d perturbation(s) gardée(s), texte %u octets\n", n, (unsigned)td.textLen);
  CHECK(n >= 8);
  bool sawB = false, sawBus = false;
  for (int i = 0; i < td.count; i++) {
    const TrafficItem& it = td.item[i];
    const LineInfo* li = findLine(it.line);
    printf("  %-4s %s niv.%d %-24s %s\n", li ? li->name : it.line, it.active ? "EN COURS" : "à venir ", it.level, it.status,
           it.title);
    CHECK(li && (li->mode == LineMode::Metro || li->mode == LineMode::Rer || li->mode == LineMode::Tram));
    if (li && li->mode == LineMode::Bus) sawBus = true;
    if (li && !strcmp(li->name, "B") && it.active) sawB = true;
    CHECK(it.text != UINT32_MAX && strlen(td.text + it.text) > 10);
    CHECK(strncmp(it.title, "Métro", 6) != 0 && strncmp(it.title, "RER ", 4) != 0);  // préfixe retiré
    if (i > 0 && it.active && td.item[i - 1].active) CHECK(it.level <= td.item[i - 1].level);
    if (i > 0 && td.item[i - 1].active == false) CHECK(!it.active);  // en cours d'abord
  }
  CHECK(sawB && !sawBus);
  CHECK(trafficLevel(td, "C01376") == TRAFFIC_BLOCKING);   // ligne 6 : trafic interrompu (travaux)
  CHECK(trafficLevel(td, "C01743") == TRAFFIC_DISRUPTED);  // RER B : mouvement social
  CHECK(trafficLevel(td, "C01371") == 0);                  // ligne 1 : normal
  CHECK(td.updatedAt > 1700000000);
  // Réponse illisible
  JsonDocument bad;
  CHECK(parseTraffic(bad, DeserializationError::InvalidInput, now, 3600, td) == -1);

  if (fails) {
    printf("%d échec(s)\n", fails);
    return 1;
  }
  printf("Tous les tests de l'info trafic passent.\n");
  return 0;
}
