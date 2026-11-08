// Tests sur PC du module siri.cpp (analyse des réponses PRIM).
//   cd test && make test
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

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

  printf("Tous les tests SIRI passent.\n");
  return 0;
}
