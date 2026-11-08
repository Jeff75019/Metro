#include "lines.h"

#include <string.h>

static const LineInfo LINES[] = {
    // Métro
    {"C01371", "1", LineMode::Metro, 0xFFBE00, 0x000000},
    {"C01372", "2", LineMode::Metro, 0x0055C8, 0xFFFFFF},
    {"C01373", "3", LineMode::Metro, 0x6E6E00, 0xFFFFFF},
    {"C01386", "3B", LineMode::Metro, 0x82C8E6, 0x000000},
    {"C01374", "4", LineMode::Metro, 0xA0006E, 0xFFFFFF},
    {"C01375", "5", LineMode::Metro, 0xFF5A00, 0x000000},
    {"C01376", "6", LineMode::Metro, 0x82DC73, 0x000000},
    {"C01377", "7", LineMode::Metro, 0xFF82B4, 0x000000},
    {"C01387", "7B", LineMode::Metro, 0x82DC73, 0x000000},
    {"C01378", "8", LineMode::Metro, 0xD282BE, 0x000000},
    {"C01379", "9", LineMode::Metro, 0xD2D200, 0x000000},
    {"C01380", "10", LineMode::Metro, 0xDC9600, 0x000000},
    {"C01381", "11", LineMode::Metro, 0x6E491E, 0xFFFFFF},
    {"C01382", "12", LineMode::Metro, 0x00643C, 0xFFFFFF},
    {"C01383", "13", LineMode::Metro, 0x82C8E6, 0x000000},
    {"C01384", "14", LineMode::Metro, 0x640082, 0xFFFFFF},
    // RER
    {"C01742", "A", LineMode::Rer, 0xEB2132, 0xFFFFFF},
    {"C01743", "B", LineMode::Rer, 0x5091CB, 0xFFFFFF},
    {"C01727", "C", LineMode::Rer, 0xFFCC30, 0x000000},
    {"C01728", "D", LineMode::Rer, 0x008B5B, 0xFFFFFF},
    {"C01729", "E", LineMode::Rer, 0xB94E9A, 0xFFFFFF},
    // Transilien
    {"C01737", "H", LineMode::Train, 0x84653D, 0xFFFFFF},
    {"C01739", "J", LineMode::Train, 0xCEC73D, 0x000000},
    {"C01738", "K", LineMode::Train, 0x9B9842, 0xFFFFFF},
    {"C01740", "L", LineMode::Train, 0xC4A4CC, 0x000000},
    {"C01736", "N", LineMode::Train, 0x00B297, 0xFFFFFF},
    {"C01730", "P", LineMode::Train, 0xF58F53, 0x000000},
    {"C01731", "R", LineMode::Train, 0xF49FB3, 0x000000},
    {"C01741", "U", LineMode::Train, 0xB6134C, 0xFFFFFF},
    {"C02711", "V", LineMode::Train, 0x9F9825, 0xFFFFFF},
    // Tramway
    {"C01389", "T1", LineMode::Tram, 0x0055C8, 0xFFFFFF},
    {"C01390", "T2", LineMode::Tram, 0xA0006E, 0xFFFFFF},
    {"C01391", "T3a", LineMode::Tram, 0xFF5A00, 0x000000},
    {"C01679", "T3b", LineMode::Tram, 0x00643C, 0xFFFFFF},
    {"C01843", "T4", LineMode::Tram, 0xDC9600, 0x000000},
    {"C01684", "T5", LineMode::Tram, 0x640082, 0xFFFFFF},
    {"C01794", "T6", LineMode::Tram, 0xFF0000, 0xFFFFFF},
    {"C01774", "T7", LineMode::Tram, 0x6E491E, 0xFFFFFF},
    {"C01795", "T8", LineMode::Tram, 0x6E6E00, 0xFFFFFF},
    {"C02317", "T9", LineMode::Tram, 0x3C91DC, 0xFFFFFF},
    {"C02528", "T10", LineMode::Tram, 0x6E6E00, 0xFFFFFF},
    {"C01999", "T11", LineMode::Tram, 0xFF5A00, 0x000000},
    {"C02529", "T12", LineMode::Tram, 0xA50034, 0xFFFFFF},
    {"C02344", "T13", LineMode::Tram, 0x8D653D, 0xFFFFFF},
    {"C02732", "T14", LineMode::Tram, 0x00A092, 0xFFFFFF},
    // Navettes automatiques
    {"C01388", "ORLYVAL", LineMode::Shuttle, 0x5EC5ED, 0xFFFFFF},
    {"C00563", "CDGVAL", LineMode::Shuttle, 0x5CC5ED, 0xFFFFFF},
};

const LineInfo* findLine(const char* id) {
  if (!id || !*id) return nullptr;
  char buf[16];
  lineIdFromRef(id, buf, sizeof(buf));
  for (const auto& l : LINES) {
    if (strcmp(l.id, buf) == 0) return &l;
  }
  return nullptr;
}

void lineIdFromRef(const char* ref, char* out, unsigned outLen) {
  if (!out || outLen == 0) return;
  out[0] = 0;
  if (!ref) return;
  // "STIF:Line::C01371:" -> "C01371" ; "IDFM:C01371" -> "C01371" ; "C01371" -> "C01371"
  const char* start = ref;
  const char* p = strstr(ref, "::");
  if (p) {
    start = p + 2;
  } else if ((p = strrchr(ref, ':')) && p[1]) {
    start = p + 1;
  }
  unsigned n = 0;
  while (start[n] && start[n] != ':' && n + 1 < outLen) {
    out[n] = start[n];
    n++;
  }
  out[n] = 0;
}

const char* lineModeName(LineMode m) {
  switch (m) {
    case LineMode::Metro: return "metro";
    case LineMode::Rer: return "rer";
    case LineMode::Train: return "train";
    case LineMode::Tram: return "tram";
    case LineMode::Shuttle: return "navette";
    default: return "bus";
  }
}
