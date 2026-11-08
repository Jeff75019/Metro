// Dessin des écrans (320 x 170, paysage) dans un tampon mémoire (sprite)
// envoyé d'un bloc à l'écran : pas de scintillement.
#pragma once
#include <LovyanGFX.hpp>

#include "lines.h"
#include "siri.h"
#include "sncf.h"

enum class NetState : uint8_t { Off, Connecting, Online, Failed };

struct UiState {
  // En-tête
  const char* badge;     // texte du pictogramme de ligne ("1", "A", "T3a", "91"...)
  LineMode mode;
  uint32_t badgeColor;   // 0xRRGGBB
  uint32_t badgeText;    // 0xRRGGBB
  const char* title;     // nom de l'arrêt (ou nom donné au favori)
  int battery;           // 0..100, -1 si inconnu
  // Corps
  const View* view;
  bool hasData;          // au moins une réponse reçue pour ce favori
  int64_t now;           // heure courante (epoch UTC)
  // Pied de page
  NetState net;
  bool busy;             // requête en cours
  const char* status;    // message d'erreur (ou nullptr)
  int ageS;              // âge des données en secondes (-1 : aucune)
  int favIndex, favCount;
};

// Tableau des trains d'une gare SNCF
struct BoardUi {
  const struct Board* board;  // nullptr : pas encore de données
  const char* station;        // nom court de la gare
  bool arrivals;
  int64_t now;
  int battery;
  NetState net;
  bool busy;
  const char* status;  // message d'erreur (ou nullptr)
  int ageS;
  int index, count;    // gare affichée / nombre de gares
};

void uiBegin(lgfx::LovyanGFX* display);
void uiDrawMain(const UiState& s);
void uiDrawMessage(const char* title, const char* line1, const char* line2 = nullptr,
                   uint32_t accent = 0xFFC629);
void uiDrawPortal(const char* apName, const char* qrPayload, const char* url, const char* url2);
void uiDrawSplash(const char* version);
void uiDrawBoard(const BoardUi& s);
// Carte affichée brièvement quand on passe d'un programme à l'autre
void uiDrawAppTitle(bool trains, const char* credit = nullptr);

// Accès au tampon (utilisé par les tests sur PC pour faire des captures)
LGFX_Sprite& uiCanvas();
