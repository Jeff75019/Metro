#include "ui.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "fonts_metro.h"
#include "reseau.h"

// ============================================================================
//  Palette
//  Fond bleu nuit, texte blanc cassé, attentes en ambre (comme les écrans de
//  quai), états en pastilles colorées. Couleurs en 0xRRGGBB, converties en
//  RGB565 pour l'écran.
// ============================================================================
static constexpr uint16_t rgb(uint32_t c) {
  return (uint16_t)((((c >> 16) & 0xF8) << 8) | (((c >> 8) & 0xFC) << 3) | ((c & 0xFF) >> 3));
}
// Mélange de deux couleurs (t = 0..255 : part de b)
static constexpr uint32_t mix(uint32_t a, uint32_t b, uint32_t t) {
  return ((((a >> 16) & 0xFF) * (255 - t) + ((b >> 16) & 0xFF) * t) / 255) << 16 |
         ((((a >> 8) & 0xFF) * (255 - t) + ((b >> 8) & 0xFF) * t) / 255) << 8 |
         (((a & 0xFF) * (255 - t) + (b & 0xFF) * t) / 255);
}

static constexpr uint32_t C_BG = 0x0A1020;
static constexpr uint32_t C_HAIR = 0x1D2843;
static constexpr uint32_t C_TEXT = 0xF4F6FB;
static constexpr uint32_t C_MUTED = 0x8C98B4;
static constexpr uint32_t C_FAINT = 0x3A4562;
static constexpr uint32_t C_SKEL = 0x172139;
static constexpr uint32_t C_AMBER = 0xFFC12E;
static constexpr uint32_t C_GREEN = 0x34D399;
static constexpr uint32_t C_RED = 0xFF6B6B;
static constexpr uint32_t C_ORANGE = 0xFFA94D;

static const uint16_t BG = rgb(C_BG), HAIR = rgb(C_HAIR), TEXT = rgb(C_TEXT), MUTED = rgb(C_MUTED),
                      FAINT = rgb(C_FAINT), SKEL = rgb(C_SKEL), AMBER = rgb(C_AMBER),
                      AMBER_DIM = rgb(mix(C_AMBER, C_BG, 90)), GREEN = rgb(C_GREEN), RED = rgb(C_RED),
                      ORANGE = rgb(C_ORANGE);
static inline uint16_t tint(uint32_t c, uint32_t t = 60) { return rgb(mix(C_BG, c, t)); }

// Couleurs des lignes de métro, pour le liseré "signature" en bas de certains écrans
static const uint32_t METRO_COLORS[] = {0xFFBE00, 0x0055C8, 0x6E6E00, 0x82C8E6, 0xA0006E, 0xFF5A00,
                                        0x82DC73, 0xFF82B4, 0xD282BE, 0xD2D200, 0xDC9600, 0x6E491E,
                                        0x00643C, 0x82C8E6, 0x640082};

static constexpr int W = 320;
static constexpr int H = 170;

// Grille de l'écran principal
static constexpr int HEADER_H = 40;  // en-tête 0..39, filet en 40
static constexpr int ROW_H = 58;
static constexpr int ROW1_Y = 41;
static constexpr int ROW2_Y = ROW1_Y + ROW_H + 1;  // 100
static constexpr int FOOT_Y = 164;                 // milieu du pied de page (158..170)
static constexpr int MARGIN = 10;

static LGFX_Sprite canvas;
static lgfx::LovyanGFX* lcd = nullptr;

static lgfx::PointerWrapper wTiny(font_tiny, sizeof(font_tiny));
static lgfx::PointerWrapper wSmall(font_small, sizeof(font_small));
static lgfx::PointerWrapper wMedium(font_medium, sizeof(font_medium));
static lgfx::PointerWrapper wCond(font_cond, sizeof(font_cond));
static lgfx::PointerWrapper wCondS(font_cond_small, sizeof(font_cond_small));
static lgfx::PointerWrapper wLarge(font_large, sizeof(font_large));
static lgfx::PointerWrapper wBig(font_big, sizeof(font_big));
static lgfx::VLWfont fTiny, fSmall, fMedium, fCond, fCondS, fLarge, fBig;

LGFX_Sprite& uiCanvas() { return canvas; }

void uiBegin(lgfx::LovyanGFX* display) {
  lcd = display;
  fTiny.loadFont(&wTiny);
  fSmall.loadFont(&wSmall);
  fMedium.loadFont(&wMedium);
  fCond.loadFont(&wCond);
  fCondS.loadFont(&wCondS);
  fLarge.loadFont(&wLarge);
  fBig.loadFont(&wBig);
  canvas.setColorDepth(16);
  canvas.setPsram(true);
  if (!canvas.createSprite(W, H)) {
    canvas.setPsram(false);  // PSRAM non activée : on se rabat sur la RAM interne
    canvas.createSprite(W, H);
  }
}

static void present() {
  if (lcd) canvas.pushSprite(lcd, 0, 0);
}

// ============================================================================
//  Texte
// ============================================================================
static int textW(const char* s, const lgfx::IFont* f) {
  canvas.setFont(f);
  return canvas.textWidth(s);
}

static void text(const char* s, int x, int y, const lgfx::IFont* f, uint16_t color, lgfx::textdatum_t datum) {
  canvas.setFont(f);
  canvas.setTextColor(color);
  canvas.setTextDatum(datum);
  canvas.drawString(s, x, y);
}

// Copie s dans out en le raccourcissant (avec « … ») pour tenir dans maxW.
static void fitText(char* out, size_t n, const char* s, int maxW, const lgfx::IFont* f) {
  canvas.setFont(f);
  copyStr(out, s, n);
  if (canvas.textWidth(out) <= maxW) return;
  size_t len = strlen(out);
  char tmp[96];
  while (len > 0) {
    do {
      len--;
    } while (len > 0 && ((unsigned char)out[len] & 0xC0) == 0x80);
    size_t l = len;
    while (l > 0 && (out[l - 1] == ' ' || out[l - 1] == '-')) l--;
    if (l + 4 > sizeof(tmp)) continue;
    memcpy(tmp, out, l);
    strcpy(tmp + l, "\xE2\x80\xA6");  // …
    if (canvas.textWidth(tmp) <= maxW) {
      copyStr(out, tmp, n);
      return;
    }
  }
  out[0] = 0;
}

// Coupe s en deux lignes de largeur maxW (la 2e est raccourcie si besoin).
static bool wrap2(const char* s, int maxW, const lgfx::IFont* f, char* l1, char* l2, size_t n) {
  canvas.setFont(f);
  const char* best = nullptr;
  for (const char* p = s; *p; p++) {
    if (*p != ' ' && *p != '-') continue;
    char tmp[96];
    size_t l = (size_t)(p - s) + (*p == '-' ? 1 : 0);
    if (l >= sizeof(tmp)) break;
    memcpy(tmp, s, l);
    tmp[l] = 0;
    if (canvas.textWidth(tmp) <= maxW) best = p;
    else break;
  }
  if (!best) return false;
  size_t l = (size_t)(best - s) + (*best == '-' ? 1 : 0);
  if (l >= n) l = n - 1;
  memcpy(l1, s, l);
  l1[l] = 0;
  const char* rest = best + 1;
  while (*rest == ' ') rest++;
  fitText(l2, n, rest, maxW, f);
  return true;
}

// ============================================================================
//  Éléments graphiques
// ============================================================================
// Pastille de texte (À quai, Supprimé...). xRight = bord droit. Renvoie la largeur.
static int pill(int xRight, int cy, const char* s, const lgfx::IFont* f, uint16_t fg, uint16_t bg, int h = 28) {
  const int padX = h / 2 - 2;
  const int w = textW(s, f) + padX * 2;
  canvas.fillSmoothRoundRect(xRight - w, cy - h / 2, w, h, h / 2, bg);
  text(s, xRight - w / 2, cy + 1, f, fg, lgfx::middle_center);
  return w;
}

// Pictogramme du mode de transport, façon signalétique IDFM (blanc sur fond sombre).
static int drawMode(int x, int cy, LineMode mode) {
  const char* label;
  bool round = false;
  switch (mode) {
    case LineMode::Metro: label = "M"; round = true; break;
    case LineMode::Tram: label = "T"; round = true; break;
    case LineMode::Rer: label = "RER"; break;
    case LineMode::Train: label = "TRAIN"; break;
    case LineMode::Shuttle: label = "VAL"; break;
    default: label = "BUS"; break;
  }
  if (round) {
    const int r = 12;
    canvas.fillSmoothCircle(x + r, cy, r, TEXT);
    text(label, x + r, cy + 1, &fSmall, BG, lgfx::middle_center);
    return 2 * r + 1;
  }
  const int h = 20, w = textW(label, &fTiny) + 10;
  canvas.fillSmoothRoundRect(x, cy - h / 2, w, h, 5, TEXT);
  text(label, x + w / 2, cy + 1, &fTiny, BG, lgfx::middle_center);
  return w;
}

// Pictogramme de ligne : rond (métro), carré arrondi (RER/Transilien),
// rectangle (tram, bus, navette). Renvoie la largeur dessinée.
static int drawBadge(int x, int cy, const char* name, LineMode mode, uint32_t color, uint32_t txt) {
  const uint16_t bg = rgb(color), fg = rgb(txt);
  const lgfx::IFont* f = &fMedium;
  int tw = textW(name, f);
  if ((mode == LineMode::Metro && tw > 20) || tw > 48) {
    f = &fSmall;
    tw = textW(name, f);
  }
  int w;
  switch (mode) {
    case LineMode::Metro:
      w = 27;
      canvas.fillSmoothCircle(x + 13, cy, 13, bg);
      break;
    case LineMode::Rer:
    case LineMode::Train:
      w = 26;
      canvas.fillSmoothRoundRect(x, cy - 13, w, 26, 6, bg);
      break;
    default:
      w = tw + 12 < 26 ? 26 : tw + 12;
      canvas.fillSmoothRoundRect(x, cy - 12, w, 24, 4, bg);
      break;
  }
  text(name, x + w / 2, cy + 1, f, fg, lgfx::middle_center);
  return w;
}

static void drawBattery(int xRight, int cy, int pct) {
  if (pct < 0) return;
  const int w = 22, h = 12;
  const int x = xRight - w - 2;
  const uint16_t frame = pct <= 15 ? RED : MUTED;
  canvas.fillSmoothRoundRect(x, cy - h / 2, w, h, 3, frame);
  canvas.fillSmoothRoundRect(x + 1, cy - h / 2 + 1, w - 2, h - 2, 2, BG);
  canvas.fillSmoothRoundRect(x + w, cy - 3, 3, 6, 1, frame);
  const int fw = (w - 4) * pct / 100;
  if (fw > 0) canvas.fillSmoothRoundRect(x + 2, cy - h / 2 + 2, fw < 2 ? 2 : fw, h - 4, 1, pct <= 15 ? RED : TEXT);
  if (pct <= 15) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d %%", pct);
    text(buf, x - 6, cy + 1, &fTiny, RED, lgfx::middle_right);
  }
}

static void drawDots(int xRight, int cy, int index, int count) {
  const int step = 9;
  const int x0 = xRight - (count - 1) * step;
  for (int i = 0; i < count; i++)
    canvas.fillSmoothCircle(x0 + i * step, cy, i == index ? 3 : 2, i == index ? TEXT : FAINT);
}

// Flèche de retour « ‹ » (choix libre sur l'écran)
static int drawBack(int x, int cy, uint16_t color) {
  canvas.drawWideLine(x + 7, cy - 7, x + 1, cy, 1.6f, color);
  canvas.drawWideLine(x + 1, cy, x + 7, cy + 7, 1.6f, color);
  return 14;
}

// Liseré aux couleurs des lignes de métro
static void drawSignature(int y, int h) {
  const int n = sizeof(METRO_COLORS) / sizeof(METRO_COLORS[0]);
  for (int i = 0; i < n; i++) {
    int x0 = W * i / n, x1 = W * (i + 1) / n;
    canvas.fillRect(x0, y, x1 - x0, h, rgb(METRO_COLORS[i]));
  }
}

// Icônes simples dessinées (anticrénelées)
static void iconAlert(int cx, int cy, uint32_t color) {
  canvas.fillSmoothCircle(cx, cy, 17, tint(color, 70));
  canvas.drawWideLine(cx, cy - 8, cx, cy + 2, 1.8f, rgb(color));
  canvas.fillSmoothCircle(cx, cy + 8, 2, rgb(color));
}

static void iconClock(int cx, int cy) {
  canvas.fillSmoothCircle(cx, cy, 17, tint(C_MUTED, 50));
  canvas.fillSmoothCircle(cx, cy, 12, MUTED);
  canvas.fillSmoothCircle(cx, cy, 10, tint(C_MUTED, 50));
  canvas.drawWideLine(cx, cy, cx, cy - 7, 1.2f, MUTED);
  canvas.drawWideLine(cx, cy, cx + 5, cy + 2, 1.2f, MUTED);
}

static void localHHMM(int64_t t, char* out, size_t n) {
  time_t tt = (time_t)t;
  struct tm lt;
  localtime_r(&tt, &lt);
  snprintf(out, n, "%02d:%02d", lt.tm_hour, lt.tm_min);
}

// ============================================================================
//  Écran principal
// ============================================================================
// Une ligne de départ : liseré couleur de ligne, destination, attente.
static void drawRow(int y0, const Row* r, int64_t now, uint32_t lineColor, const char* subOverride = nullptr) {
  const int cy = y0 + ROW_H / 2;
  const int xr = W - MARGIN;
  const bool cancelled = r->flags & DEP_CANCELLED;
  const int64_t diff = r->when - now;

  canvas.fillSmoothRoundRect(MARGIN, cy - 15, 4, 30, 2, cancelled ? FAINT : rgb(lineColor));

  // --- à droite : l'attente
  int rightW;
  if (cancelled) {
    rightW = pill(xr, cy, "Supprimé", &fSmall, RED, tint(C_RED, 55));
  } else if ((r->flags & DEP_AT_STOP) || diff < 0) {
    rightW = pill(xr, cy, "À quai", &fSmall, GREEN, tint(C_GREEN, 55));
  } else if (diff < 45) {
    // clignote comme sur les quais
    const bool on = now & 1;
    rightW = pill(xr, cy, "À l'approche", &fSmall, on ? BG : AMBER, on ? AMBER : tint(C_AMBER, 55));
  } else if (diff >= 100 * 60) {
    char hhmm[16];
    localHHMM(r->when, hhmm, sizeof(hhmm));
    text(hhmm, xr, cy + 2, &fLarge, AMBER, lgfx::middle_right);
    rightW = textW(hhmm, &fLarge);
  } else {
    char num[16];
    snprintf(num, sizeof(num), "%d", (int)(diff / 60));
    const uint16_t c = (r->flags & DEP_REALTIME) ? AMBER : AMBER_DIM;
    const int base = y0 + 47;
    const int minW = textW("min", &fSmall);
    text("min", xr, base, &fSmall, c, lgfx::baseline_right);
    text(num, xr - minW - 4, base, &fBig, c, lgfx::baseline_right);
    rightW = minW + 4 + textW(num, &fBig);
  }

  // --- à gauche : destination (+ mention)
  const int tx = MARGIN + 14;
  const int maxW = xr - rightW - 14 - tx;
  char sub[48] = "";
  uint16_t subCol = MUTED;
  if (subOverride && *subOverride && !cancelled) {
    fitText(sub, sizeof(sub), subOverride, maxW, &fTiny);
  } else if (cancelled) {
  } else if (r->flags & DEP_DELAYED) {
    copyStr(sub, "Retardé", sizeof(sub));
    subCol = ORANGE;
  } else if (r->platform[0]) {
    snprintf(sub, sizeof(sub), "Voie %s", r->platform);
  } else if (!(r->flags & DEP_REALTIME)) {
    copyStr(sub, "Horaire théorique", sizeof(sub));
  }
  const uint16_t destCol = cancelled ? MUTED : TEXT;

  // Police la plus grande qui tient sur une ligne ; sinon deux lignes condensées
  const lgfx::IFont* f = nullptr;
  for (const lgfx::IFont* c : {(const lgfx::IFont*)&fMedium, (const lgfx::IFont*)&fCond, (const lgfx::IFont*)&fCondS})
    if (!f && textW(r->dest, c) <= maxW) f = c;

  char l1[64], l2[64];
  if (f) {
    const int dy = sub[0] ? -8 : 1;
    text(r->dest, tx, cy + dy, f, destCol, lgfx::middle_left);
    if (cancelled) canvas.fillRect(tx, cy + dy, textW(r->dest, f), 2, MUTED);  // barré
    if (sub[0]) text(sub, tx, cy + 14, &fTiny, subCol, lgfx::middle_left);
  } else if (wrap2(r->dest, maxW, &fCondS, l1, l2, sizeof(l1))) {
    text(l1, tx, cy - 10, &fCondS, destCol, lgfx::middle_left);
    text(l2, tx, cy + 10, &fCondS, destCol, lgfx::middle_left);
    // la mention passe au bout de la 2e ligne si elle y tient
    const int endX = tx + textW(l2, &fCondS) + 8;
    if (sub[0] && endX + textW(sub, &fTiny) <= tx + maxW) {
      canvas.fillSmoothCircle(endX, cy + 11, 2, subCol);
      text(sub, endX + 7, cy + 11, &fTiny, subCol, lgfx::middle_left);
    }
  } else {
    fitText(l1, sizeof(l1), r->dest, maxW, &fCondS);
    text(l1, tx, cy + 1, &fCondS, destCol, lgfx::middle_left);
  }
}

// Ligne "fantôme" pendant le premier chargement
static void drawSkeletonRow(int y0, int64_t now, int phase) {
  const int cy = y0 + ROW_H / 2;
  const uint16_t c = ((now + phase) & 1) ? SKEL : rgb(mix(C_SKEL, C_HAIR, 128));
  canvas.fillSmoothRoundRect(MARGIN, cy - 15, 4, 30, 2, FAINT);
  canvas.fillSmoothRoundRect(MARGIN + 14, cy - 7, phase ? 120 : 160, 14, 7, c);
  canvas.fillSmoothRoundRect(W - MARGIN - 62, cy - 14, 62, 28, 8, c);
}

static void drawBodyMessage(bool alert, const char* title, const char* sub, uint32_t color,
                            int top = HEADER_H + 1, int bottom = 156) {
  const int cx = W / 2;
  const int cy = (top + bottom) / 2;
  if (alert) iconAlert(cx, cy - 26, color);
  else iconClock(cx, cy - 26);
  char buf[96];
  fitText(buf, sizeof(buf), title, W - 2 * MARGIN, &fMedium);
  text(buf, cx, cy + 10, &fMedium, alert ? rgb(color) : TEXT, lgfx::middle_center);
  if (sub && *sub) {
    fitText(buf, sizeof(buf), sub, W - 2 * MARGIN, &fSmall);
    text(buf, cx, cy + 34, &fSmall, MUTED, lgfx::middle_center);
  }
}

void uiDrawMain(const UiState& s) {
  canvas.fillScreen(BG);

  // --- En-tête : mode + ligne + arrêt ............................ batterie
  const int hy = HEADER_H / 2;
  int x = MARGIN;
  if (s.back) x += drawBack(x, hy, TEXT) + 4;
  x += drawMode(x, hy, s.mode) + 6;
  x += drawBadge(x, hy, s.badge && *s.badge ? s.badge : "?", s.mode, s.badgeColor, s.badgeText) + 10;
  const int titleMax = W - x - (s.battery >= 0 ? (s.battery <= 15 ? 76 : 38) : MARGIN);
  const char* title = s.title ? s.title : "";
  const lgfx::IFont* tf = textW(title, &fMedium) <= titleMax ? &fMedium : &fCond;
  char buf[96];
  fitText(buf, sizeof(buf), title, titleMax, tf);
  text(buf, x, hy + 1, tf, TEXT, lgfx::middle_left);
  drawBattery(W - MARGIN, hy, s.battery);
  canvas.drawFastHLine(0, HEADER_H, W, HAIR);

  // --- Corps : deux prochains passages
  const View* v = s.view;
  if (v && v->rows > 0) {
    drawRow(ROW1_Y, &v->row[0], s.now, s.badgeColor, s.sub[0]);
    canvas.drawFastHLine(MARGIN + 14, ROW2_Y - 1, W - 2 * MARGIN - 14, HAIR);
    if (v->rows > 1) drawRow(ROW2_Y, &v->row[1], s.now, s.badgeColor, s.sub[1]);
  } else if (!s.hasData && s.status && *s.status) {
    drawBodyMessage(true, s.status, "Nouvel essai dans quelques secondes", C_ORANGE);
  } else if (!s.hasData) {
    drawSkeletonRow(ROW1_Y, s.now, 0);
    canvas.drawFastHLine(MARGIN + 14, ROW2_Y - 1, W - 2 * MARGIN - 14, HAIR);
    drawSkeletonRow(ROW2_Y, s.now, 1);
  } else {
    drawBodyMessage(false, "Aucun passage prévu", "Service terminé ou information indisponible", C_TEXT);
  }

  // --- Pied de page : état ......................... sens · favoris
  int rightX = W - MARGIN;
  if (s.favCount > 1) {
    drawDots(rightX - 2, FOOT_Y, s.favIndex, s.favCount);
    rightX -= (s.favCount - 1) * 9 + 14;
  }
  if (v && v->groupCount > 1) {
    char g[16];
    snprintf(g, sizeof(g), "sens %d/%d", v->groupIndex + 1, v->groupCount);
    text(g, rightX, FOOT_Y, &fTiny, MUTED, lgfx::middle_right);
    rightX -= textW(g, &fTiny) + 12;
  }

  char foot[64] = "";
  uint16_t dot = GREEN, fc = MUTED;
  const bool error = s.status && *s.status;
  if (s.busy) {
    copyStr(foot, "Mise à jour…", sizeof(foot));
    dot = (s.now & 1) ? AMBER : FAINT;
  } else if (error && s.hasData) {
    copyStr(foot, s.status, sizeof(foot));
    dot = ORANGE;
    fc = ORANGE;
  } else if (s.net == NetState::Connecting) {
    copyStr(foot, "Connexion au WiFi…", sizeof(foot));
    dot = (s.now & 1) ? MUTED : FAINT;
  } else if (s.ageS >= 0) {
    if (s.ageS < 10) copyStr(foot, "À l'instant", sizeof(foot));
    else if (s.ageS < 60) snprintf(foot, sizeof(foot), "Il y a %d s", s.ageS);
    else snprintf(foot, sizeof(foot), "Il y a %d min", s.ageS / 60);
    if (s.ageS > 180) dot = AMBER;
  } else {
    dot = FAINT;
  }
  if (foot[0] || dot != FAINT) {
    canvas.fillSmoothCircle(MARGIN + 3, FOOT_Y, 3, dot);
    fitText(buf, sizeof(buf), foot, rightX - (MARGIN + 12), &fTiny);
    text(buf, MARGIN + 12, FOOT_Y, &fTiny, fc, lgfx::middle_left);
  }

  present();
}

// ============================================================================
//  Autres écrans
// ============================================================================
// Texte centré à partir de y (haut), sur une ou deux lignes. Renvoie la hauteur utilisée.
static int drawLines(const char* line, int y, int maxW, uint16_t color) {
  if (textW(line, &fSmall) <= maxW) {
    text(line, W / 2, y + 10, &fSmall, color, lgfx::middle_center);
    return 22;
  }
  char a[64], b[64];
  if (wrap2(line, maxW, &fSmall, a, b, sizeof(a))) {
    text(a, W / 2, y + 10, &fSmall, color, lgfx::middle_center);
    text(b, W / 2, y + 29, &fSmall, color, lgfx::middle_center);
    return 41;
  }
  fitText(a, sizeof(a), line, maxW, &fSmall);
  text(a, W / 2, y + 10, &fSmall, color, lgfx::middle_center);
  return 22;
}

void uiDrawMessage(const char* title, const char* line1, const char* line2, uint32_t accent) {
  canvas.fillScreen(BG);
  iconAlert(W / 2, 34, accent);
  char buf[96];
  fitText(buf, sizeof(buf), title ? title : "", W - 2 * MARGIN, &fLarge);
  text(buf, W / 2, 76, &fLarge, TEXT, lgfx::middle_center);
  int y = 98;
  if (line1 && *line1) y += drawLines(line1, y, W - 2 * MARGIN, MUTED) + 4;
  if (line2 && *line2) drawLines(line2, y, W - 2 * MARGIN, MUTED);
  drawSignature(H - 3, 3);
  present();
}

static void stepNumber(int x, int cy, const char* n) {
  canvas.fillSmoothCircle(x + 8, cy, 8, tint(C_AMBER, 70));
  text(n, x + 8, cy + 1, &fTiny, AMBER, lgfx::middle_center);
}

void uiDrawPortal(const char* apName, const char* qrPayload, const char* url, const char* url2) {
  canvas.fillScreen(BG);

  // QR code sur carte blanche : le scanner rejoint directement le WiFi
  const int card = 140, cx0 = MARGIN, cy0 = (H - 3 - card) / 2;
  canvas.fillSmoothRoundRect(cx0, cy0, card, card, 10, TEXT);
  canvas.qrcode(qrPayload, cx0 + 8, cy0 + 8, card - 16, 2);

  const int tx = cx0 + card + 14;
  const int tw = W - tx - MARGIN;
  char buf[64];
  text("Configuration", tx, 20, &fMedium, TEXT, lgfx::middle_left);

  stepNumber(tx, 50, "1");
  text("Scannez ou rejoignez", tx + 22, 50, &fTiny, MUTED, lgfx::middle_left);
  fitText(buf, sizeof(buf), apName, tw, &fMedium);
  text(buf, tx, 73, &fMedium, AMBER, lgfx::middle_left);

  stepNumber(tx, 102, "2");
  text("Ouvrez dans le navigateur", tx + 22, 102, &fTiny, MUTED, lgfx::middle_left);
  fitText(buf, sizeof(buf), url, tw, &fMedium);
  text(buf, tx, 125, &fMedium, AMBER, lgfx::middle_left);
  if (url2 && *url2) {
    fitText(buf, sizeof(buf), url2, tw, &fTiny);
    text(buf, tx, 147, &fTiny, MUTED, lgfx::middle_left);
  } else {
    text("BOOT : quitter", tx, 147, &fTiny, FAINT, lgfx::middle_left);
  }
  drawSignature(H - 3, 3);
  present();
}

void uiDrawSplash(const char* version) {
  canvas.fillScreen(BG);
  canvas.fillSmoothCircle(W / 2, 62, 36, TEXT);
  text("M", W / 2, 66, &fBig, BG, lgfx::middle_center);
  text("Afficheur Métro", W / 2, 122, &fMedium, TEXT, lgfx::middle_center);
  char buf[32];
  snprintf(buf, sizeof(buf), "Version %s", version);
  text(buf, W / 2, 146, &fTiny, MUTED, lgfx::middle_center);
  drawSignature(H - 4, 4);
  present();
}

// ============================================================================
//  Programme « Trains » : tableau des départs / arrivées d'une gare SNCF
// ============================================================================
static constexpr uint32_t C_DEP = 0x1A4CA8;  // bandeau des départs (bleu)
static constexpr uint32_t C_ARR = 0x0E7A4B;  // bandeau des arrivées (vert)
static constexpr int B_HEADER = 32;
static constexpr int B_ROW = 33;

static void drawTrainRow(int y0, const Train& t, int64_t now) {
  const int cy = y0 + B_ROW / 2;
  const int xr = W - MARGIN;
  const bool cancelled = t.flags & TR_CANCELLED;
  const int delayMin = (int)((t.real - t.base) / 60);

  // Heure prévue
  char hhmm[16];
  localHHMM(t.base, hhmm, sizeof(hhmm));
  text(hhmm, MARGIN, cy + 1, &fMedium, cancelled ? MUTED : AMBER, lgfx::middle_left);
  if (cancelled) canvas.fillRect(MARGIN, cy + 1, textW(hhmm, &fMedium), 2, MUTED);

  // État à droite
  int rightW = 0;
  if (cancelled) {
    rightW = pill(xr, cy, "Supprimé", &fTiny, RED, tint(C_RED, 55), 22);
  } else if (delayMin >= 1) {
    // retard, et nouvelle heure en dessous
    char d[16], nt[24];
    snprintf(d, sizeof(d), "+%d min", delayMin);
    localHHMM(t.real, hhmm, sizeof(hhmm));
    snprintf(nt, sizeof(nt), "%s", hhmm);
    text(d, xr, cy - 6, &fSmall, ORANGE, lgfx::middle_right);
    text(nt, xr, cy + 10, &fTiny, ORANGE, lgfx::middle_right);
    // petite flèche « → » dessinée devant la nouvelle heure
    const int ax = xr - textW(nt, &fTiny) - 4, ay = cy + 10;
    canvas.drawWideLine(ax - 9, ay, ax - 1, ay, 0.7f, ORANGE);
    canvas.drawWideLine(ax - 4, ay - 3, ax - 1, ay, 0.7f, ORANGE);
    canvas.drawWideLine(ax - 4, ay + 3, ax - 1, ay, 0.7f, ORANGE);
    rightW = textW(d, &fSmall);
  } else if (t.flags & TR_REALTIME) {
    text("à l'heure", xr, cy + 1, &fTiny, GREEN, lgfx::middle_right);
    rightW = textW("à l'heure", &fTiny);
  } else {  // pas encore suivi en temps réel : horaire prévu
    text("prévu", xr, cy + 1, &fTiny, MUTED, lgfx::middle_right);
    rightW = textW("prévu", &fTiny);
  }

  // Destination / provenance, puis type et numéro du train
  const int tx = MARGIN + 60;
  const int maxW = xr - (rightW ? rightW + 10 : 0) - tx;
  char buf[64];
  const char* place = t.place;
  uint16_t pc = cancelled ? MUTED : TEXT;
  if (!place[0]) {
    place = (t.flags & TR_PLACE_PENDING) ? "\xE2\x80\xA6" : "Provenance non communiquée";
    pc = MUTED;
  }
  fitText(buf, sizeof(buf), place, maxW, &fCondS);
  text(buf, tx, cy - 6, &fCondS, pc, lgfx::middle_left);

  int x = tx;
  if (t.mode[0]) {
    const int cw = textW(t.mode, &fTiny) + 8;
    canvas.fillSmoothRoundRect(x, cy + 4, cw, 13, 3, cancelled ? FAINT : rgb(t.color));
    text(t.mode, x + cw / 2, cy + 11, &fTiny, TEXT, lgfx::middle_center);
    x += cw + 5;
  }
  if (t.number[0]) {
    text(t.number, x, cy + 11, &fTiny, MUTED, lgfx::middle_left);
    x += textW(t.number, &fTiny) + 8;
  }
  // Voie (quand elle est connue) : encadrée, comme sur les tableaux des gares
  if (t.platform[0] && !cancelled) {
    char v[16];
    snprintf(v, sizeof(v), "voie %s", t.platform);
    const int vw = textW(v, &fTiny) + 8;
    if (x + vw <= tx + maxW) {
      canvas.fillSmoothRoundRect(x, cy + 4, vw, 13, 3, TEXT);
      text(v, x + vw / 2, cy + 11, &fTiny, BG, lgfx::middle_center);
    }
  }
}

void uiDrawBoard(const BoardUi& s) {
  canvas.fillScreen(BG);

  // --- Bandeau : Départs / Arrivées · gare ..................... heure
  const uint32_t band = s.arrivals ? C_ARR : C_DEP;
  canvas.fillRect(0, 0, W, B_HEADER, rgb(band));
  const int hy = B_HEADER / 2;
  const char* title = s.arrivals ? "Arrivées" : "Départs";
  int x = MARGIN;
  if (s.back) x += drawBack(x, hy, TEXT) + 4;
  text(title, x, hy + 1, &fMedium, TEXT, lgfx::middle_left);
  x += textW(title, &fMedium) + 8;
  canvas.fillSmoothCircle(x, hy + 1, 2, rgb(mix(band, C_TEXT, 150)));
  x += 8;

  char clock[16] = "";
  if (s.now > 1700000000) localHHMM(s.now, clock, sizeof(clock));
  int right = W - MARGIN;
  if (clock[0]) {
    text(clock, right, hy + 1, &fMedium, TEXT, lgfx::middle_right);
    right -= textW(clock, &fMedium) + 10;
  }
  // Pastille d'état : vert = à jour, ambre = ancien, orange = erreur, clignote pendant la mise à jour
  uint16_t dot = rgb(mix(band, C_TEXT, 90));
  if (s.busy) dot = (s.now & 1) ? TEXT : rgb(mix(band, C_TEXT, 90));
  else if (s.status && *s.status) dot = ORANGE;
  else if (s.ageS >= 0) dot = s.ageS > 180 ? AMBER : GREEN;
  canvas.fillSmoothCircle(right - 3, hy + 1, 3, dot);
  right -= 12;
  if (s.battery >= 0 && s.battery <= 15) {
    drawBattery(right, hy, s.battery);
    right -= 64;
  }
  char buf[64];
  const lgfx::IFont* tf = textW(s.station, &fMedium) <= right - x ? &fMedium : &fCond;
  fitText(buf, sizeof(buf), s.station ? s.station : "", right - x, tf);
  text(buf, x, hy + 1, tf, TEXT, lgfx::middle_left);

  // --- Trains
  const Board* b = s.board;
  const int top = B_HEADER;
  if (b && b->count > 0) {
    const int n = b->count < BOARD_ROWS ? b->count : BOARD_ROWS;
    for (int i = 0; i < n; i++) {
      const int y0 = top + i * B_ROW;
      if (i) canvas.drawFastHLine(MARGIN, y0, W - 2 * MARGIN, HAIR);
      drawTrainRow(y0, b->t[i], s.now);
    }
  } else if (b) {
    drawBodyMessage(false, s.arrivals ? "Aucune arrivée annoncée" : "Aucun départ annoncé",
                    "dans les prochaines heures", C_TEXT, top, H - 6);
  } else if (s.status && *s.status) {
    drawBodyMessage(true, s.status, "Nouvel essai dans quelques secondes", C_ORANGE, top, H - 6);
  } else {
    for (int i = 0; i < BOARD_ROWS; i++) {
      const int y0 = top + i * B_ROW, cy = y0 + B_ROW / 2;
      const uint16_t c = ((s.now + i) & 1) ? SKEL : rgb(mix(C_SKEL, C_HAIR, 128));
      if (i) canvas.drawFastHLine(MARGIN, y0, W - 2 * MARGIN, HAIR);
      canvas.fillSmoothRoundRect(MARGIN, cy - 7, 46, 14, 7, c);
      canvas.fillSmoothRoundRect(MARGIN + 60, cy - 10, i & 1 ? 120 : 160, 11, 5, c);
      canvas.fillSmoothRoundRect(MARGIN + 60, cy + 4, 60, 9, 4, c);
    }
  }

  // --- Gares (points) en bas
  if (s.count > 1) {
    const int step = 8, y = H - 3;
    const int x0 = W / 2 - (s.count - 1) * step / 2;
    for (int i = 0; i < s.count; i++)
      canvas.fillSmoothCircle(x0 + i * step, y, i == s.index ? 2 : 1, i == s.index ? TEXT : FAINT);
  }
  present();
}

void uiDrawAppTitle(bool trains, const char* credit) {
  canvas.fillScreen(BG);
  const int cx = W / 2, cy = 58;
  if (trains) {
    canvas.fillSmoothRoundRect(cx - 44, cy - 20, 88, 40, 10, TEXT);
    text("TRAIN", cx, cy + 1, &fMedium, BG, lgfx::middle_center);
    text("Trains grandes lignes", cx, 104, &fMedium, TEXT, lgfx::middle_center);
    text("Départs et arrivées des gares SNCF", cx, 128, &fSmall, MUTED, lgfx::middle_center);
    if (credit && *credit) text(credit, cx, 151, &fTiny, MUTED, lgfx::middle_center);
  } else {
    canvas.fillSmoothCircle(cx, cy, 26, TEXT);
    text("M", cx, cy + 3, &fLarge, BG, lgfx::middle_center);
    text("Métro · RER · Tram", cx, 108, &fMedium, TEXT, lgfx::middle_center);
    text("Prochains passages à vos arrêts", cx, 134, &fSmall, MUTED, lgfx::middle_center);
  }
  drawSignature(H - 3, 3);
  present();
}

// ============================================================================
//  Choix libre sur l'écran tactile : accueil, grilles (lignes, initiales) et
//  listes (stations, gares)
// ============================================================================
static constexpr uint32_t C_CARD = 0x141E36;
static constexpr int E_HEADER = 36;  // en-tête 0..35, filet en 36
static constexpr int E_ROW = 26;     // hauteur d'une ligne de liste
static constexpr int E_TOP = E_HEADER + 1;

// En-tête commun : flèche de retour (si back) ...... heure, batterie.
// *x0 / *right : place laissée au pictogramme et au titre.
static void exHeader(bool back, int64_t now, int battery, int* x0, int* rightOut) {
  const int hy = E_HEADER / 2;
  int x = MARGIN;
  if (back) x += drawBack(x, hy, TEXT) + 6;
  int right = W - MARGIN;
  if (battery >= 0) {
    drawBattery(right, hy, battery);
    right -= battery <= 15 ? 70 : 32;
  }
  if (now > 1700000000) {
    char clock[16];
    localHHMM(now, clock, sizeof(clock));
    text(clock, right, hy + 1, &fSmall, MUTED, lgfx::middle_right);
    right -= textW(clock, &fSmall) + 10;
  }
  canvas.drawFastHLine(0, E_HEADER, W, HAIR);
  *x0 = x;
  *rightOut = right;
}

static void exTitle(const char* title, int x, int right) {
  char buf[96];
  const lgfx::IFont* tf = textW(title, &fMedium) <= right - x ? &fMedium : &fCond;
  fitText(buf, sizeof(buf), title, right - x, tf);
  text(buf, x, E_HEADER / 2 + 1, tf, TEXT, lgfx::middle_left);
}

bool uiHitBack(int x, int y) { return x < 56 && y < E_HEADER + 8; }

// Étoile pleine (favoris)
static void drawStar(int cx, int cy, int r, uint16_t color) {
  float px[10], py[10];
  for (int i = 0; i < 10; i++) {
    const float a = -1.5708f + i * 0.6283f;
    const float rr = (i & 1) ? r * 0.45f : r;
    px[i] = cx + rr * cosf(a);
    py[i] = cy + rr * sinf(a);
  }
  for (int i = 0; i < 10; i++) {
    const int j = (i + 1) % 10;
    canvas.fillTriangle(cx, cy, (int)px[i], (int)py[i], (int)px[j], (int)py[j], color);
  }
}

// ---- Accueil -------------------------------------------------------------------
static constexpr int HUB_TOP = E_TOP + 7, HUB_GAP = 8, HUB_COLS = 3;
static int hubRows(int count) { return (count + HUB_COLS - 1) / HUB_COLS; }

static void hubCell(int count, int i, int* x, int* y, int* w, int* h) {
  const int rows = hubRows(count);
  *h = (H - 6 - HUB_TOP - (rows - 1) * HUB_GAP) / (rows < 2 ? 2 : rows);
  *w = (W - 2 * MARGIN - (HUB_COLS - 1) * HUB_GAP) / HUB_COLS;
  const int row = i / HUB_COLS, col = i % HUB_COLS;
  const int inRow = (row == rows - 1) ? count - row * HUB_COLS : HUB_COLS;  // dernière ligne centrée
  const int rowW = inRow * *w + (inRow - 1) * HUB_GAP;
  *x = (W - rowW) / 2 + col * (*w + HUB_GAP);
  const int totalH = rows * *h + (rows - 1) * HUB_GAP;
  *y = HUB_TOP + (H - 6 - HUB_TOP - totalH) / 2 + row * (*h + HUB_GAP);
}

int uiHitHub(int count, int x, int y) {
  for (int i = 0; i < count; i++) {
    int cx, cy, w, h;
    hubCell(count, i, &cx, &cy, &w, &h);
    if (x >= cx - HUB_GAP / 2 && x < cx + w + HUB_GAP / 2 && y >= cy - HUB_GAP / 2 && y < cy + h + HUB_GAP / 2)
      return i;
  }
  return -1;
}

void uiDrawHub(const HubUi& s) {
  canvas.fillScreen(BG);
  int x, right;
  exHeader(false, s.now, s.battery, &x, &right);
  canvas.fillSmoothCircle(x + 11, E_HEADER / 2, 11, TEXT);
  text("M", x + 11, E_HEADER / 2 + 1, &fSmall, BG, lgfx::middle_center);
  x += 30;
  exTitle(s.net == NetState::Connecting ? "Connexion au WiFi…" : "Prochains passages", x, right);

  for (int i = 0; i < s.count; i++) {
    int cx, cy, w, h;
    hubCell(s.count, i, &cx, &cy, &w, &h);
    canvas.fillSmoothRoundRect(cx, cy, w, h, 10, rgb(C_CARD));
    const int ix = cx + w / 2, iy = cy + h / 2 - 9;
    switch (s.tiles[i].kind) {
      case TileKind::Metro:
        canvas.fillSmoothCircle(ix, iy, 13, TEXT);
        text("M", ix, iy + 1, &fMedium, BG, lgfx::middle_center);
        break;
      case TileKind::Tram:
        canvas.fillSmoothCircle(ix, iy, 13, TEXT);
        text("T", ix, iy + 1, &fMedium, BG, lgfx::middle_center);
        break;
      case TileKind::Rer:
      case TileKind::Trains: {
        const char* l = s.tiles[i].kind == TileKind::Rer ? "RER" : "TRAIN";
        const int bw = textW(l, &fSmall) + 12;
        canvas.fillSmoothRoundRect(ix - bw / 2, iy - 12, bw, 24, 6, TEXT);
        text(l, ix, iy + 1, &fSmall, BG, lgfx::middle_center);
        break;
      }
      case TileKind::MyStops:
      case TileKind::MyStations:
        drawStar(ix, iy, 13, AMBER);
        break;
    }
    char buf[32];
    fitText(buf, sizeof(buf), s.tiles[i].label, w - 8, &fSmall);
    text(buf, ix, cy + h - 13, &fSmall, TEXT, lgfx::middle_center);
  }
  present();
}

// ---- Grilles (lignes d'un mode, initiales des gares) -----------------------------
static constexpr int GRID_TOP = E_TOP + 3, GRID_BOTTOM = H - 4;

static void gridCell(int count, int cols, int i, int* x, int* y, int* w, int* h) {
  const int rows = (count + cols - 1) / cols;
  *w = (W - 2 * MARGIN) / cols;
  *h = (GRID_BOTTOM - GRID_TOP) / (rows < 2 ? 2 : rows);
  if (*h > 64) *h = 64;
  const int totalH = rows * *h;
  *x = MARGIN + (i % cols) * *w;
  *y = GRID_TOP + (GRID_BOTTOM - GRID_TOP - totalH) / 2 + (i / cols) * *h;
}

int uiHitGrid(int count, int cols, int x, int y) {
  for (int i = 0; i < count; i++) {
    int cx, cy, w, h;
    gridCell(count, cols, i, &cx, &cy, &w, &h);
    if (x >= cx && x < cx + w && y >= cy && y < cy + h) return i;
  }
  return -1;
}

// Grand pictogramme de ligne, centré
static void bigBadge(int cx, int cy, int cw, int ch, const GridItem& it) {
  const uint16_t bg = rgb(it.color), fg = rgb(it.textColor);
  int sz = (cw < ch ? cw : ch) - 8;
  if (sz > 40) sz = 40;
  const lgfx::IFont* f = sz >= 30 ? &fMedium : &fSmall;
  switch (it.mode) {
    case LineMode::Metro: {
      canvas.fillSmoothCircle(cx, cy, sz / 2, bg);
      if (textW(it.label, f) > sz - 6) f = &fSmall;
      break;
    }
    case LineMode::Rer:
    case LineMode::Train:
      canvas.fillSmoothRoundRect(cx - sz / 2, cy - sz / 2, sz, sz, sz / 5, bg);
      if (sz >= 34) f = &fLarge;
      break;
    default: {
      const int bw = cw - 10, bh = sz * 3 / 4;
      canvas.fillSmoothRoundRect(cx - bw / 2, cy - bh / 2, bw, bh, 5, bg);
      if (textW(it.label, f) > bw - 6) f = &fSmall;
      break;
    }
  }
  text(it.label, cx, cy + 1, f, fg, lgfx::middle_center);
}

void uiDrawGrid(const GridUi& s) {
  canvas.fillScreen(BG);
  int x, right;
  exHeader(true, 0, s.battery, &x, &right);
  x += drawMode(x, E_HEADER / 2, s.titleMode) + 8;
  exTitle(s.title, x, right);
  for (int i = 0; i < s.count; i++) {
    int cx, cy, w, h;
    gridCell(s.count, s.cols, i, &cx, &cy, &w, &h);
    const GridItem& it = s.items[i];
    if (it.badge) {
      bigBadge(cx + w / 2, cy + h / 2, w, h, it);
    } else {
      canvas.fillSmoothRoundRect(cx + 3, cy + 3, w - 6, h - 6, 8, rgb(C_CARD));
      const lgfx::IFont* f = textW(it.label, &fLarge) <= w - 10 ? &fLarge : &fSmall;
      if (f == &fSmall && textW(it.label, f) > w - 8) f = &fTiny;
      text(it.label, cx + w / 2, cy + h / 2 + 1, f, TEXT, lgfx::middle_center);
    }
  }
  present();
}

// ---- Listes ----------------------------------------------------------------------
int uiHitList(int x, int y) {
  (void)x;
  if (y < E_TOP) return -1;
  const int r = (y - E_TOP) / E_ROW;
  return r < LIST_ROWS ? r : -1;
}

void uiDrawList(const ListUi& s) {
  canvas.fillScreen(BG);
  int x, right;
  exHeader(true, 0, s.battery, &x, &right);
  if (s.badge) x += drawBadge(x, E_HEADER / 2, s.badge, s.mode, s.color, s.textColor) + 8;
  else x += drawMode(x, E_HEADER / 2, s.mode) + 8;
  exTitle(s.title, x, right);

  const uint16_t lc = rgb(s.color);
  const int LX = MARGIN + 14;  // axe du trait de la ligne
  const int nameX = s.lineGraphic ? LX + 16 : MARGIN + 4;
  const int nameMax = W - nameX - 14;
  for (int r = 0; r < LIST_ROWS; r++) {
    const int i = s.top + r;
    if (i >= s.count) break;
    const int y0 = E_TOP + r * E_ROW, cy = y0 + E_ROW / 2;
    if (i == s.highlight) canvas.fillSmoothRoundRect(nameX - 6, y0 + 2, W - nameX - 8, E_ROW - 4, 6, tint(s.color, 55));
    if (s.lineGraphic) {
      const uint8_t f = s.flags ? s.flags(i, s.ctx) : 0;
      const bool startsHere = i == 0 || (f & NET_BRANCH);
      const bool endsHere = i == s.count - 1 || (s.flags && (s.flags(i + 1, s.ctx) & NET_BRANCH));
      const int ya = startsHere ? cy : y0, yb = endsHere ? cy : y0 + E_ROW;
      if (yb > ya) canvas.fillRect(LX - 3, ya, 6, yb - ya, lc);
      if (f & NET_BRANCH) {  // début d'une branche : petit raccord en pointillés
        for (int yy = y0; yy < cy - 4; yy += 4) canvas.fillRect(LX - 1, yy, 2, 2, lc);
      }
      const bool term = f & NET_TERMINUS;
      canvas.fillSmoothCircle(LX, cy, term ? 8 : 6, BG);
      canvas.fillSmoothCircle(LX, cy, term ? 6 : 4, TEXT);
    } else if (r > 0) {
      canvas.drawFastHLine(MARGIN, y0, W - 2 * MARGIN - 6, HAIR);
    }
    char buf[96];
    const char* name = s.name(i, s.ctx);
    const lgfx::IFont* f = textW(name, &fCondS) <= nameMax ? &fCondS : &fTiny;
    fitText(buf, sizeof(buf), name, nameMax, f);
    text(buf, nameX, cy + 1, f, TEXT, lgfx::middle_left);
  }
  // Barre de défilement
  if (s.count > LIST_ROWS) {
    const int ty = E_TOP + 4, th = LIST_ROWS * E_ROW - 8;
    canvas.fillSmoothRoundRect(W - 6, ty, 3, th, 1, HAIR);
    int bh = th * LIST_ROWS / s.count;
    if (bh < 12) bh = 12;
    const int by = ty + (th - bh) * s.top / (s.count - LIST_ROWS);
    canvas.fillSmoothRoundRect(W - 6, by, 3, bh, 1, MUTED);
  }
  present();
}
