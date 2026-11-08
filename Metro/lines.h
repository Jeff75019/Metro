// Référentiel (simplifié) des lignes ferrées d'Île-de-France : identifiant IDFM,
// nom court et couleurs officielles. Source : "Référentiel des lignes" IDFM.
#pragma once
#include <stddef.h>
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
LineMode lineModeFromName(const char* name);  // "metro", "rer", "train", "tram", "navette", "bus"
uint32_t parseHexColor(const char* s, uint32_t def);  // "#FF5A00" ou "ff5a00"

// Lignes absentes de la table (bus, Noctilien...) : infos lues dans le jeu de
// données "referentiel-des-lignes" de l'open data IDFM (API explore v2.1).
struct LineRefInfo {
  char id[12];
  char name[12];
  LineMode mode;
  uint32_t color;
  uint32_t textColor;
};
int parseLineReferential(const char* json, size_t len, LineRefInfo* out, int maxN);
