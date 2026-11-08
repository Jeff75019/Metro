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
  // Station choisie sur l'afficheur (choix libre) : flèche de retour et texte
  // sous chaque destination (« puis 7 et 12 min »)
  bool back;
  const char* sub[2];
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
  bool back;           // flèche de retour (gare choisie sur l'afficheur)
};

// ---- Choix libre sur l'écran tactile : accueil, grilles et listes -----------
enum class TileKind : uint8_t { Metro, Rer, Tram, Trains, MyStops, MyStations };
struct HubTile {
  TileKind kind;
  const char* label;
};
struct HubUi {
  const HubTile* tiles;
  int count;
  int64_t now;
  int battery;
  NetState net;
};

// Case d'une grille : pictogramme de ligne (badge) ou texte (initiale de gare)
struct GridItem {
  const char* label;
  LineMode mode;  // pour la forme du pictogramme
  uint32_t color, textColor;
  bool badge;     // false : simple texte
};
struct GridUi {
  const char* title;
  LineMode titleMode;  // pictogramme devant le titre
  const GridItem* items;
  int count, cols;
  int battery;
};

// Liste défilante (stations d'une ligne, gares d'une initiale)
#define LIST_ROWS 5
struct ListUi {
  const char* title;
  const char* badge;  // pictogramme de ligne dans l'en-tête (nullptr : pictogramme du mode)
  LineMode mode;
  uint32_t color, textColor;
  bool lineGraphic;   // trait de la ligne et pastilles des stations
  int count, top;     // nombre d'éléments, premier visible
  int highlight;      // élément mis en valeur (-1 : aucun)
  const char* (*name)(int i, void* ctx);
  uint8_t (*flags)(int i, void* ctx);  // NET_BRANCH / NET_TERMINUS (lineGraphic)
  void* ctx;
  int battery;
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

void uiDrawHub(const HubUi& s);
void uiDrawGrid(const GridUi& s);
void uiDrawList(const ListUi& s);

// Zones de toucher (mêmes calculs que le dessin) : -1 si rien
bool uiHitBack(int x, int y);                    // flèche de retour en haut à gauche
int uiHitHub(int count, int x, int y);           // case de l'accueil
int uiHitGrid(int count, int cols, int x, int y);  // case d'une grille
int uiHitList(int x, int y);                     // ligne visible (0..LIST_ROWS-1)

// Accès au tampon (utilisé par les tests sur PC pour faire des captures)
LGFX_Sprite& uiCanvas();
