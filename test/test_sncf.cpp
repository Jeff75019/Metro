// Tests sur PC du module sncf.cpp (tableaux des gares SNCF, format Navitia).
//   cd test && make test
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <string>

#include "../Metro/sncf.h"

static std::string readFile(const char* path) {
  FILE* f = fopen(path, "rb");
  if (!f) {
    perror(path);
    exit(1);
  }
  std::string s;
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) s.append(buf, n);
  fclose(f);
  return s;
}

#define CHECK(cond)                                                     \
  do {                                                                  \
    if (!(cond)) {                                                      \
      fprintf(stderr, "ÉCHEC %s:%d : %s\n", __FILE__, __LINE__, #cond); \
      exit(1);                                                          \
    }                                                                   \
  } while (0)

int main() {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  // --- heures locales de Paris
  int64_t t;
  CHECK(parseNavitiaTime("20261108T094700", &t) && t == 1794127620LL);  // hiver : UTC+1
  CHECK(parseNavitiaTime("20260702T120000", &t) && t == 1782986400LL);  // été : UTC+2
  CHECK(!parseNavitiaTime("2026-11-08", &t));

  char s[64];
  strcpy(s, "Marseille Saint-Charles (Marseille)");
  cleanStationName(s);
  CHECK(strcmp(s, "Marseille Saint-Charles") == 0);
  strcpy(s, "Paris Nord");
  cleanStationName(s);
  CHECK(strcmp(s, "Paris Nord") == 0);

  CHECK(isMainline("TGV INOUI", "Train grande vitesse", "SNCF"));
  CHECK(isMainline("TER", "Train", "TER"));
  CHECK(!isMainline("RER", "RapidTransit", "RER"));
  CHECK(!isMainline("Transilien", "Train", "Transilien"));
  CHECK(!isMainline("Car TER", "Autocar", "TER"));
  CHECK(trainModeColor("OUIGO", "") == 0xE2007A);
  CHECK(trainModeColor("Inconnu", "123456") == 0x123456);

  // --- départs de Paris Gare de Lyon
  std::string dep = readFile("data/sncf_gdl_departures.json");
  Board b;
  char err[64];
  CHECK(parseNavitiaBoard(dep.data(), dep.size(), false, true, "Gare de Lyon", b, err, sizeof(err)) ==
        SiriStatus::Ok);
  printf("Départs Gare de Lyon (grandes lignes) : %d trains\n", b.count);
  for (int i = 0; i < b.count; i++)
    printf("  %s %s -> %s  retard %lld min%s\n", b.t[i].mode, b.t[i].number, b.t[i].place,
           (long long)((b.t[i].real - b.t[i].base) / 60), (b.t[i].flags & TR_CANCELLED) ? " SUPPRIMÉ" : "");
  CHECK(b.count == 4);  // RER D et Transilien R écartés
  CHECK(b.serverTime == 1794127260LL);
  CHECK(strcmp(b.t[0].place, "Marseille Saint-Charles") == 0);
  CHECK(strcmp(b.t[0].number, "6111") == 0 && strcmp(b.t[0].mode, "TGV INOUI") == 0);
  CHECK(strncmp(b.t[0].vj, "vehicle_journey:SNCF:2026-11-08:6111", 36) == 0);
  CHECK(b.t[1].real - b.t[1].base == 12 * 60);  // OUIGO en retard
  CHECK(!(b.t[1].flags & TR_CANCELLED));
  CHECK(b.t[2].flags & TR_CANCELLED);           // TER supprimé (perturbation NO_SERVICE)
  CHECK(strcmp(b.t[3].place, "Genève") == 0);   // terminus via le lien "terminus"
  CHECK(!(b.t[3].flags & TR_REALTIME));

  CHECK(parseNavitiaBoard(dep.data(), dep.size(), false, false, "Gare de Lyon", b, err, sizeof(err)) ==
        SiriStatus::Ok);
  CHECK(b.count == 6);  // sans filtre : tout est gardé

  // --- arrivées à Paris Nord
  std::string arr = readFile("data/sncf_nord_arrivals.json");
  CHECK(parseNavitiaBoard(arr.data(), arr.size(), true, true, "Gare du Nord", b, err, sizeof(err)) ==
        SiriStatus::Ok);
  CHECK(b.count == 3 && b.arrivals);
  CHECK(strcmp(b.t[0].place, "London St-Pancras") == 0);  // provenance via le lien "origins"
  CHECK(!(b.t[0].flags & TR_PLACE_PENDING));
  CHECK(b.t[1].flags & TR_PLACE_PENDING);  // provenance à chercher
  CHECK(b.t[2].flags & TR_CANCELLED);      // "Train supprimé"

  std::string vj = readFile("data/sncf_vj_7208.json");
  char origin[40];
  CHECK(parseNavitiaOrigin(vj.data(), vj.size(), "Gare du Nord", origin, sizeof(origin)));
  CHECK(strcmp(origin, "Lille Flandres") == 0);
  printf("Provenance du 7208 : %s\n", origin);

  // --- erreurs
  std::string bad = readFile("data/sncf_unauthorized.json");
  CHECK(parseNavitiaBoard(bad.data(), bad.size(), false, true, "", b, err, sizeof(err)) ==
        SiriStatus::ServerError);
  CHECK(strcmp(err, "no token") == 0);
  CHECK(parseNavitiaBoard("<html>", 6, false, true, "", b, err, sizeof(err)) == SiriStatus::BadJson);

  printf("Tous les tests SNCF passent.\n");
  return 0;
}
