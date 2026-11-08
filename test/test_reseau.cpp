// Tests du réseau embarqué (lignes, stations, gares) et du regroupement des
// passages par destination.   cd test && make test
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "../Metro/reseau.h"
#include "../Metro/siri.h"

static int fails = 0;
#define CHECK(c)                                              \
  do {                                                        \
    if (!(c)) {                                               \
      printf("ÉCHEC %s:%d : %s\n", __FILE__, __LINE__, #c); \
      fails++;                                                \
    }                                                         \
  } while (0)

static std::string readFile(const char* path) {
  FILE* f = fopen(path, "rb");
  std::string s;
  char buf[4096];
  size_t n;
  while (f && (n = fread(buf, 1, sizeof(buf), f)) > 0) s.append(buf, n);
  if (f) fclose(f);
  return s;
}

static int findLineIdx(const char* name) {
  for (int i = 0; i < netLineCount(); i++)
    if (netLineInfo(i) && !strcmp(netLineInfo(i)->name, name)) return i;
  return -1;
}

int main() {
  // --- Lignes par mode
  int idx[32];
  CHECK(netLinesOfMode(LineMode::Metro, idx, 32) == 16);
  CHECK(netLinesOfMode(LineMode::Rer, idx, 32) == 5);
  CHECK(netLinesOfMode(LineMode::Tram, idx, 32) == 15);
  for (int i = 0; i < netLineCount(); i++) {
    CHECK(netLineInfo(i) != nullptr);
    CHECK(netLine(i).count >= 4);
    for (int k = 0; k < netLine(i).count; k++) {
      CHECK(netStopName(i, k)[0] != 0);
      CHECK(netStop(i, k).zda > 0);
    }
  }

  // --- Ordre des stations
  int l1 = findLineIdx("1");
  CHECK(l1 >= 0 && !strcmp(netStopName(l1, 0), "La Défense"));
  CHECK(l1 >= 0 && !strcmp(netStopName(l1, netLine(l1).count - 1), "Château de Vincennes"));
  CHECK(l1 >= 0 && (netStop(l1, 0).flags & NET_TERMINUS));
  int l14 = findLineIdx("14");  // tracé incomplet dans les données : stations réinsérées
  bool olympiades = false;
  for (int k = 1; l14 >= 0 && k < netLine(l14).count; k++) {
    CHECK(!(netStop(l14, k).flags & NET_BRANCH));
    if (!strcmp(netStopName(l14, k), "Olympiades")) olympiades = !strcmp(netStopName(l14, k + 1), "Maison Blanche");
  }
  CHECK(olympiades);
  int l7 = findLineIdx("7"), branches = 0;
  for (int k = 0; l7 >= 0 && k < netLine(l7).count; k++)
    if (netStop(l7, k).flags & NET_BRANCH) branches++;
  CHECK(branches == 1);
  char ref[48];
  netStopRef(l1, 0, ref, sizeof(ref));
  CHECK(!strncmp(ref, "STIF:StopArea:SP:", 17) && ref[strlen(ref) - 1] == ':');

  // --- Gares SNCF
  CHECK(!strcmp(sncfGroupLabel(0), "Paris"));
  int g[64];
  CHECK(sncfGroupStations(0, g, 64) == 7);
  CHECK(!strcmp(sncfPick(g[0]).name, "Gare de Lyon"));
  int total = 0;
  for (int i = 0; i < sncfGroupCount(); i++) {
    int n = sncfGroupStations(i, g, 64);
    CHECK(n > 0);
    total += n;
    if (i > 0) CHECK(sncfGroupLabel(i)[0] >= 'A' && sncfGroupLabel(i)[0] <= 'Z');
  }
  CHECK(total == sncfPickCount());
  for (int i = 0; i < sncfPickCount(); i++) CHECK(strlen(sncfPick(i).uic) >= 8);

  // --- Passages regroupés par destination (vraie réponse PRIM, La Défense, RER A)
  std::string j = readFile("data/prim_ladefense_reel.json");
  static StopData d;
  char err[64];
  CHECK(parseStopMonitoring(j.data(), j.size(), "C01742", d, err, sizeof(err)) == SiriStatus::Ok);
  DirGroup gr[8];
  const int ng = buildDirections(d, d.responseTime, gr, 8);
  printf("RER A à La Défense : %d destination(s)\n", ng);
  CHECK(ng >= 1);
  for (int i = 0; i < ng; i++) {
    printf("  %s :", gr[i].dest);
    for (int k = 0; k < gr[i].n; k++) printf(" %lld min", (long long)(gr[i].when[k] - d.responseTime) / 60);
    printf("\n");
    CHECK(gr[i].n >= 1 && gr[i].n <= DIR_TIMES);
    for (int k = 1; k < gr[i].n; k++) CHECK(gr[i].when[k] >= gr[i].when[k - 1]);
  }

  if (fails) {
    printf("%d échec(s)\n", fails);
    return 1;
  }
  printf("Tous les tests du réseau passent.\n");
  return 0;
}
