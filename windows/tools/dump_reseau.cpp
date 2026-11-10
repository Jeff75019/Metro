// Exporte le réseau embarqué (lignes, stations, gares SNCF) en JSON
#include <stdio.h>
#include <string>
#include "reseau.h"
#include "lines.h"
static std::string esc(const char* s) {
  std::string o;
  for (; *s; s++) {
    if (*s == '"' || *s == '\\') o += '\\';
    o += *s;
  }
  return o;
}
static const char* modeName(LineMode m) {
  switch (m) {
    case LineMode::Metro: return "metro";
    case LineMode::Rer: return "rer";
    case LineMode::Tram: return "tram";
    case LineMode::Train: return "train";
    default: return "bus";
  }
}
int main() {
  printf("{\"lines\":[");
  for (int i = 0; i < netLineCount(); i++) {
    const NetLine& l = netLine(i);
    const LineInfo* li = netLineInfo(i);
    printf("%s{\"id\":\"%s\",\"name\":\"%s\",\"mode\":\"%s\",\"color\":\"#%06X\",\"text\":\"#%06X\",\"year\":%u,\"stops\":[",
           i ? "," : "", l.id, li ? esc(li->name).c_str() : "", li ? modeName(li->mode) : "", li ? (unsigned)li->color : 0,
           li ? (unsigned)li->textColor : 0xFFFFFF, (unsigned)l.year);
    for (int k = 0; k < l.count; k++) {
      const NetStop& s = netStop(i, k);
      printf("%s[%u,\"%s\",%u,%u]", k ? "," : "", (unsigned)s.zda, esc(netStopName(i, k)).c_str(), (unsigned)s.flags,
             (unsigned)s.year);
    }
    printf("]}");
  }
  printf("],\"sncf\":[");
  for (int i = 0; i < sncfPickCount(); i++)
    printf("%s[\"%s\",\"%s\"]", i ? "," : "", esc(sncfPick(i).name).c_str(), sncfPick(i).uic);
  printf("]}\n");
}
