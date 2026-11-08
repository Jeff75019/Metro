// Tests sur PC du module siri.cpp (analyse des réponses PRIM).
//   cd test && make test
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <string>

#include "../Metro/lines.h"
#include "../Metro/siri.h"

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
  // --- dates ISO 8601
  int64_t t;
  CHECK(parseIsoTime("1970-01-01T00:00:00Z", &t) && t == 0);
  CHECK(parseIsoTime("2026-11-08T08:42:30.000Z", &t) && t == 1794127350LL);
  CHECK(parseIsoTime("2026-11-08T09:42:30+01:00", &t) && t == 1794127350LL);
  CHECK(parseIsoTime("2024-02-29T12:00:00Z", &t) && t == 1709208000LL);
  CHECK(!parseIsoTime("hier", &t));

  // --- texte
  char s[64];
  strcpy(s, "ChÃ¢teau de  Vincennes ");
  fixText(s);
  CHECK(strcmp(s, "Château de Vincennes") == 0);
  strcpy(s, "La D\xEF\xBF\xBD"
            "fense");
  fixText(s);
  CHECK(strcmp(s, "La Défense") == 0);
  strcpy(s, "Déjà correct");
  fixText(s);
  CHECK(strcmp(s, "Déjà correct") == 0);
  normalizeAscii("Saint-Denis Université", s, sizeof(s));
  CHECK(strcmp(s, "saint denis universite") == 0);
  CHECK(matchesFilter("La Défense", "defense"));
  CHECK(matchesFilter("Châtillon - Montrouge", "Asnières|châtillon"));
  CHECK(!matchesFilter("La Défense", "Vincennes"));
  CHECK(matchesFilter("La Défense", ""));

  // --- analyse d'une réponse
  std::string json = readFile("data/bastille_m1.json");
  StopData d;
  char err[64];
  SiriStatus st = parseStopMonitoring(json.data(), json.size(), "C01371", d, err, sizeof(err));
  CHECK(st == SiriStatus::Ok);
  printf("Arrêt : %s, %d départs (ligne 1)\n", d.stopName, d.count);
  CHECK(strcmp(d.stopName, "Bastille") == 0);
  CHECK(d.count == 6);  // la ligne 5 est filtrée
  for (int i = 1; i < d.count; i++) CHECK(d.dep[i - 1].when <= d.dep[i].when);
  CHECK(d.responseTime == 1794127200LL);

  int64_t now = d.responseTime;
  View v;
  buildView(d, "", "", now, v);
  printf("%d sens :", v.groupCount);
  for (int g = 0; g < v.groupCount; g++) printf(" [%s]", v.groupKey[g]);
  printf("\n");
  CHECK(v.groupCount == 2);
  CHECK(strcmp(v.groupKey[0], "Château de Vincennes") == 0);
  CHECK(v.rows == 2);
  CHECK(strcmp(v.row[0].dest, "Château de Vincennes") == 0);
  CHECK((v.row[0].when - now) == 150);
  CHECK(v.row[1].flags & DEP_CANCELLED);

  buildView(d, "", "La Défense", now, v);
  CHECK(v.groupIndex == 1 && v.rows == 2);
  CHECK(v.row[0].flags & DEP_AT_STOP);
  CHECK((v.row[1].when - now) == 245);

  buildView(d, "defense", "", now + 600, v);
  CHECK(v.groupCount == 1 && v.rows == 1);
  CHECK(!(v.row[0].flags & DEP_REALTIME));  // horaire théorique (Aimed)

  // --- sans filtre de ligne
  st = parseStopMonitoring(json.data(), json.size(), "", d, err, sizeof(err));
  CHECK(st == SiriStatus::Ok && d.count == 7);

  LineSummary ls[8];
  char stop[40];
  int n = summarizeLines(json.data(), json.size(), ls, 8, stop, sizeof(stop));
  CHECK(n == 2);
  printf("Lignes à %s :", stop);
  for (int i = 0; i < n; i++) {
    printf(" %s(", ls[i].line);
    for (int k = 0; k < ls[i].ndest; k++) printf("%s%s", k ? ", " : "", ls[i].dest[k]);
    printf(")");
  }
  printf("\n");

  // --- erreurs
  const char* bad = "{\"Siri\":{\"ServiceDelivery\":{\"StopMonitoringDelivery\":[{\"Status\":\"false\","
                    "\"ErrorCondition\":{\"ErrorInformation\":{\"ErrorText\":\"MonitoringRef inconnu\"}}}]}}}";
  st = parseStopMonitoring(bad, strlen(bad), "", d, err, sizeof(err));
  CHECK(st == SiriStatus::ServerError);
  printf("Erreur serveur : %s\n", err);
  CHECK(strcmp(err, "MonitoringRef inconnu") == 0);
  st = parseStopMonitoring("<html>", 6, "", d, err, sizeof(err));
  CHECK(st == SiriStatus::BadJson);

  // --- vraie réponse PRIM (La Défense, 3 h 47, 8 novembre 2026)
  std::string real = readFile("data/prim_ladefense_reel.json");
  st = parseStopMonitoring(real.data(), real.size(), "", d, err, sizeof(err));
  CHECK(st == SiriStatus::Ok);
  CHECK(strcmp(d.stopName, "La Défense") == 0);
  CHECK(d.count == MAX_DEPARTURES);
  int withPlatform = 0;
  for (int i = 0; i < d.count; i++) {
    CHECK(strncmp(d.dep[i].platform, "unkn", 4) != 0);  // "unknown" ignoré
    if (d.dep[i].platform[0]) withPlatform++;
  }
  CHECK(withPlatform > 0);  // RER E et Transilien L ont une voie
  st = parseStopMonitoring(real.data(), real.size(), "C01742", d, err, sizeof(err));  // RER A
  CHECK(st == SiriStatus::Ok && d.count > 0);
  buildView(d, "", "", d.responseTime, v);
  printf("RER A à La Défense : %d sens (%s / %s), prochain vers %s\n", v.groupCount, v.groupKey[0],
         v.groupCount > 1 ? v.groupKey[1] : "-", v.row[0].dest);
  CHECK(v.groupCount == 2 && strcmp(v.groupKey[0], "Aller") == 0);
  n = summarizeLines(real.data(), real.size(), ls, 8, stop, sizeof(stop));
  CHECK(n == 5);

  // --- référentiel des lignes IDFM (vraie réponse) : nom et couleurs des bus
  std::string ref = readFile("data/ods_referentiel_reel.json");
  LineRefInfo lr[8];
  n = parseLineReferential(ref.data(), ref.size(), lr, 8);
  CHECK(n == 3);
  CHECK(strcmp(lr[1].id, "C01392") == 0 && strcmp(lr[1].name, "N24") == 0);
  CHECK(lr[1].mode == LineMode::Bus && lr[1].color == 0xFF5A00 && lr[1].textColor == 0x000000);
  CHECK(lr[2].mode == LineMode::Train);
  CHECK(parseHexColor("#0055c8", 0) == 0x0055C8 && parseHexColor("zz", 7) == 7);
  CHECK(lineModeFromName("tram") == LineMode::Tram && lineModeFromName("??") == LineMode::Bus);
  // filtre à plusieurs destinations (RER A vers l'est)
  CHECK(matchesFilter("Marne-la-Vallée Chessy", "Marne-la-Vallée Chessy|Torcy|Boissy-Saint-Léger"));
  CHECK(!matchesFilter("Saint-Germain-en-Laye", "Marne-la-Vallée Chessy|Torcy|Boissy-Saint-Léger"));

  // Vraie réponse RER E (Haussmann-Saint-Lazare, 86 Ko, 61 passages), décodée au fil
  // de l'eau comme sur la carte (flux + filtre), puis tronquée : erreur propre
  {
    std::ifstream f("data/prim_rerE_haussmann_reel.json", std::ios::binary);
    JsonDocument filter, doc;
    siriFilter(filter);
    DeserializationError e = deserializeJson(doc, f, DeserializationOption::Filter(filter),
                                             DeserializationOption::NestingLimit(40));
    static StopData rd;
    char er[64];
    CHECK(parseStopMonitoringDoc(doc, e, "C01729", rd, er, sizeof(er)) == SiriStatus::Ok);
    CHECK(rd.count == MAX_DEPARTURES);
    CHECK(strstr(rd.stopName, "Haussmann") != nullptr);
    CHECK(rd.dep[0].when <= rd.dep[1].when && rd.dep[0].platform[0]);
    printf("RER E Haussmann : %d passages gardés, 1er vers %s voie %s\n", rd.count, rd.dep[0].dest, rd.dep[0].platform);
    std::string full;
    std::ifstream g("data/prim_rerE_haussmann_reel.json", std::ios::binary);
    full.assign(std::istreambuf_iterator<char>(g), std::istreambuf_iterator<char>());
    JsonDocument d2;
    DeserializationError e2 = deserializeJson(d2, full.data(), full.size() / 2, DeserializationOption::Filter(filter));
    CHECK(parseStopMonitoringDoc(d2, e2, "C01729", rd, er, sizeof(er)) == SiriStatus::BadJson);
    printf("Réponse tronquée : %s\n", er);
    CHECK(strstr(er, "JSON invalide") != nullptr);
  }

  printf("Tous les tests SIRI passent.\n");
  return 0;
}
