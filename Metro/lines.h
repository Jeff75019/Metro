// Référentiel (simplifié) des lignes ferrées d'Île-de-France : identifiant IDFM,
// nom court et couleurs officielles. Source : "Référentiel des lignes" IDFM.
#pragma once
#include <stdint.h>

enum class LineMode : uint8_t { Metro, Rer, Train, Tram, Shuttle, Bus };

struct LineInfo {
  const char* id;     // ex. "C01371" (dans LineRef "STIF:Line::C01371:")
  const char* name;   // ex. "1", "3B", "A", "T3a"
  LineMode mode;
  uint32_t color;     // 0xRRGGBB
  uint32_t textColor; // 0xRRGGBB
};

// Retourne nullptr si la ligne n'est pas dans la table (bus, noctilien...).
const LineInfo* findLine(const char* id);

// Extrait "C01371" de "STIF:Line::C01371:" (ou renvoie l'entrée telle quelle).
// out doit faire au moins 16 octets.
void lineIdFromRef(const char* lineRef, char* out, unsigned outLen);

const char* lineModeName(LineMode m);
