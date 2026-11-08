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

  // ======================= Train Nomade =======================
  char ids[48];
  stationUicList("stop_area:SNCF:87686006", ids, sizeof(ids));
  CHECK(strcmp(ids, "87686006") == 0);
  stationUicList("87271007;87271031;87271023", ids, sizeof(ids));
  CHECK(strcmp(ids, "87271007,87271031,87271023") == 0);
  stationStopArea("87271007,87271031", ids, sizeof(ids));
  CHECK(strcmp(ids, "stop_area:SNCF:87271007") == 0);
  stationStopArea("stop_area:SNCF:87113001", ids, sizeof(ids));
  CHECK(strcmp(ids, "stop_area:SNCF:87113001") == 0);
  CHECK(strcmp(trainCategoryLabel("highSpeedRail"), "TGV") == 0);

  // Gare de Lyon, 15 h 08 (sur le modèle de l'extrait réel de la documentation)
  std::string tn = readFile("data/trainnomade_gdl.json");
  CHECK(parseTrainNomade(tn.data(), tn.size(), false, true, b, err, sizeof(err)) == SiriStatus::Ok);
  printf("Train Nomade, départs Gare de Lyon : %d trains\n", b.count);
  for (int i = 0; i < b.count; i++)
    printf("  %s %s -> %s voie [%s] retard %lld min%s%s\n", b.t[i].mode, b.t[i].number, b.t[i].place, b.t[i].platform,
           (long long)((b.t[i].real - b.t[i].base) / 60), (b.t[i].flags & TR_CANCELLED) ? " SUPPRIMÉ" : "",
           (b.t[i].flags & TR_REALTIME) ? "" : " (théorique)");
  CHECK(b.count == 4);  // trains partis et Transilien écartés
  CHECK(b.serverTime == 1794146917LL && b.validUntil == 1794147795LL);
  CHECK(strcmp(b.t[0].number, "6115") == 0 && b.t[0].base == 1794147300LL && strcmp(b.t[0].platform, "K") == 0);
  CHECK(strcmp(b.t[0].place, "Marseille Saint-Charles") == 0 && strcmp(b.t[0].mode, "TGV") == 0);
  CHECK(strcmp(b.t[1].number, "6611") == 0 && b.t[1].real - b.t[1].base == 720);
  CHECK(b.t[2].flags & TR_CANCELLED);
  CHECK(strcmp(b.t[3].mode, "Intercités") == 0 && !(b.t[3].flags & TR_REALTIME) && b.t[3].base == 1794149400LL);
  CHECK(parseTrainNomade(tn.data(), tn.size(), false, false, b, err, sizeof(err)) == SiriStatus::Ok && b.count == 5);
  CHECK(parseTrainNomade(tn.data(), tn.size(), true, true, b, err, sizeof(err)) == SiriStatus::Ok);
  CHECK(b.count == 2 && b.arrivals);
  CHECK(strcmp(b.t[0].number, "6102") == 0 && strcmp(b.t[0].place, "Marseille Saint-Charles") == 0);
  CHECK(b.t[0].base == 1794147420LL && b.t[0].real - b.t[0].base == 300 && strcmp(b.t[0].platform, "H") == 0);
  CHECK(strcmp(b.t[1].place, "Dijon") == 0);

  // Vraies réponses de la nuit du 8 novembre 2026
  std::string night = readFile("data/trainnomade_gdl_nuit_reel.json");
  CHECK(parseTrainNomade(night.data(), night.size(), false, false, b, err, sizeof(err)) == SiriStatus::Ok && b.count == 0);
  CHECK(parseTrainNomade(night.data(), night.size(), true, true, b, err, sizeof(err)) == SiriStatus::Ok && b.count == 0);
  CHECK(parseTrainNomade(night.data(), night.size(), true, false, b, err, sizeof(err)) == SiriStatus::Ok && b.count == 1);
  CHECK(b.t[0].base == 1794116580LL);  // jour 2026-11-07 + 110 580 s = 8 novembre 6 h 43
  CHECK(strcmp(b.t[0].place, "Montargis") == 0);
  std::string nord = readFile("data/trainnomade_nord_reel.json");
  CHECK(parseTrainNomade(nord.data(), nord.size(), false, true, b, err, sizeof(err)) == SiriStatus::Ok && b.count == 0);
  CHECK(parseTrainNomade(nord.data(), nord.size(), false, false, b, err, sizeof(err)) == SiriStatus::Ok && b.count > 0);
  for (int i = 0; i < b.count; i++) {
    CHECK(strcmp(b.t[i].mode, "Transilien") == 0);
    for (int k = 0; k < i; k++) CHECK(strcmp(b.t[i].number, b.t[k].number) != 0);  // pas de doublon
  }
  printf("Gare du Nord à 4 h 10 (sans filtre) : %d départs, ex. %s vers %s voie [%s]\n", b.count, b.t[0].number,
         b.t[0].place, b.t[0].platform);
  CHECK(parseTrainNomade("{\"erreur\":\"introuvable\"}", 24, false, true, b, err, sizeof(err)) ==
        SiriStatus::ServerError);
  CHECK(strcmp(err, "Gare inconnue") == 0);

  // Recherche de gares (vraie réponse de l'open data SNCF)
  std::string gares = readFile("data/sncf_gares_paris_nord_reel.json");
  StationHit hits[4];
  CHECK(parseGaresVoyageurs(gares.data(), gares.size(), hits, 4) == 1);
  CHECK(strcmp(hits[0].name, "Paris Gare du Nord") == 0);
  CHECK(strcmp(hits[0].id, "87271007,87271031,87271023") == 0);

  // Horaires prévus (vrai fichier Train Nomade de la Gare de Lyon) : complément du temps réel
  CHECK(timetableSlug("87686006,87686030") && strcmp(timetableSlug("87686006,87686030"), "gare-de-lyon") == 0);
  CHECK(strcmp(timetableSlug("stop_area:SNCF:87391003"), "gare-montparnasse") == 0);
  CHECK(timetableSlug("87999999") == nullptr);
  std::string hor = readFile("data/trainnomade_horaires_gdl_reel.json");
  // 8 novembre 4 h 15 : le temps réel ne contient encore aucun TGV (vraie réponse de nuit)
  CHECK(parseTrainNomade(night.data(), night.size(), false, true, b, err, sizeof(err)) == SiriStatus::Ok && b.count == 0);
  CHECK(mergeTimetable(hor.data(), hor.size(), false, 1794107700LL, 3 * 3600, b) == 2);
  CHECK(b.count == 2 && strcmp(b.t[0].number, "7829") == 0 && strcmp(b.t[0].mode, "OUIGO") == 0);
  CHECK(b.t[0].base == 1794114180LL && !(b.t[0].flags & TR_REALTIME));  // 6 h 03, horaire théorique
  CHECK(strcmp(b.t[1].mode, "TGV INOUI") == 0 && strcmp(b.t[1].place, "Turin Porta Susa") == 0);
  printf("Complément théorique à 4 h 15 : %s %s %s, %s %s %s\n", b.t[0].mode, b.t[0].number, b.t[0].place, b.t[1].mode,
         b.t[1].number, b.t[1].place);
  // un train déjà présent en temps réel n'est pas ajouté deux fois, mais reçoit sa marque
  Board rt;
  memset(&rt, 0, sizeof(rt));
  rt.count = 1;
  copyStr(rt.t[0].number, "7829", sizeof(rt.t[0].number));
  copyStr(rt.t[0].mode, "TGV", sizeof(rt.t[0].mode));
  rt.t[0].base = 1794114180LL;
  rt.t[0].real = rt.t[0].base + 600;
  rt.t[0].flags = TR_REALTIME;
  CHECK(mergeTimetable(hor.data(), hor.size(), false, 1794107700LL, 3 * 3600, rt) == 1);
  CHECK(rt.count == 2 && strcmp(rt.t[0].number, "7829") == 0 && strcmp(rt.t[0].mode, "OUIGO") == 0);
  CHECK(rt.t[0].flags & TR_REALTIME);

  printf("Tous les tests SNCF passent.\n");
  return 0;
}
