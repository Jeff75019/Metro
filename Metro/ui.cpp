#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "fonts_metro.h"

// ----------------------------------------------------------------------------
//  Palette (inspirée des écrans de quai IDFM : fond bleu nuit, minutes ambre)
// ----------------------------------------------------------------------------
static constexpr uint16_t rgb(uint32_t c) {
  return (uint16_t)((((c >> 16) & 0xF8) << 8) | (((c >> 8) & 0xFC) << 3) | ((c & 0xFF) >> 3));
}
static const uint16_t COL_BG = rgb(0x06101F);
static const uint16_t COL_PANEL = rgb(0x0E1E38);
static const uint16_t COL_LINE = rgb(0x1E3357);
static const uint16_t COL_TEXT = rgb(0xFFFFFF);
static const uint16_t COL_MUTED = rgb(0x8A9BBC);
static const uint16_t COL_AMBER = rgb(0xFFC629);
static const uint16_t COL_AMBER_DIM = rgb(0xB89130);
static const uint16_t COL_GREEN = rgb(0x48D597);
static const uint16_t COL_RED = rgb(0xFF5F5F);
static const uint16_t COL_ORANGE = rgb(0xFF9A3C);

static constexpr int W = 320;
static constexpr int H = 170;

static LGFX_Sprite canvas;
static lgfx::LovyanGFX* lcd = nullptr;

static lgfx::PointerWrapper wSmall(font_small, sizeof(font_small));
static lgfx::PointerWrapper wMedium(font_medium, sizeof(font_medium));
static lgfx::PointerWrapper wLarge(font_large, sizeof(font_large));
static lgfx::PointerWrapper wBig(font_big, sizeof(font_big));
static lgfx::VLWfont fSmall, fMedium, fLarge, fBig;

LGFX_Sprite& uiCanvas() { return canvas; }

void uiBegin(lgfx::LovyanGFX* display) {
  lcd = display;
  fSmall.loadFont(&wSmall);
  fMedium.loadFont(&wMedium);
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

// ----------------------------------------------------------------------------
//  Texte
// ----------------------------------------------------------------------------
// Copie s dans out en le raccourcissant (avec « … ») pour tenir dans maxW pixels
// avec la police courante.
static void fitText(char* out, size_t n, const char* s, int maxW) {
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
static bool wrap2(const char* s, int maxW, char* l1, char* l2, size_t n) {
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
  fitText(l2, n, rest, maxW);
  return true;
}

static void drawText(const char* s, int x, int y, const lgfx::IFont* f, uint16_t color,
                     lgfx::textdatum_t datum) {
  canvas.setFont(f);
  canvas.setTextColor(color);
  canvas.setTextDatum(datum);
  canvas.drawString(s, x, y);
}

// ----------------------------------------------------------------------------
//  Éléments graphiques
// ----------------------------------------------------------------------------
// Pictogramme de ligne : rond (métro), carré arrondi (RER/Transilien),
// rectangle (tram, bus, navette). Renvoie la largeur dessinée.
static int drawBadge(int x, int cy, const char* name, LineMode mode, uint32_t color, uint32_t text) {
  const uint16_t bg = rgb(color), fg = rgb(text);
  const int h = 28;
  canvas.setFont(&fMedium);
  int tw = canvas.textWidth(name);
  const lgfx::IFont* f = &fMedium;
  if ((mode == LineMode::Metro && tw > 22) || mode == LineMode::Tram || tw > 52) {
    f = &fSmall;
    canvas.setFont(f);
    tw = canvas.textWidth(name);
  }
  int w;
  switch (mode) {
    case LineMode::Metro:
      w = h;
      canvas.fillCircle(x + h / 2, cy, h / 2, bg);
      break;
    case LineMode::Rer:
    case LineMode::Train:
      w = h;
      canvas.fillRoundRect(x, cy - h / 2, w, h, 6, bg);
      break;
    default:
      w = tw + 14;
      if (w < h) w = h;
      canvas.fillRoundRect(x, cy - h / 2, w, h, 4, bg);
      if (mode == LineMode::Tram) {  // filets haut et bas comme les pictos tram
        canvas.fillRect(x + 3, cy - h / 2 + 2, w - 6, 2, fg);
        canvas.fillRect(x + 3, cy + h / 2 - 4, w - 6, 2, fg);
      }
      break;
  }
  drawText(name, x + w / 2, cy + 1, f, fg, lgfx::middle_center);
  return w;
}

static void drawBattery(int xRight, int cy, int pct) {
  if (pct < 0) return;
  const int w = 22, h = 11;
  int x = xRight - w - 2;
  uint16_t c = pct > 50 ? COL_GREEN : (pct > 20 ? COL_AMBER : COL_RED);
  canvas.drawRoundRect(x, cy - h / 2, w, h, 2, COL_MUTED);
  canvas.fillRect(x + w, cy - 2, 2, 5, COL_MUTED);
  int fw = (w - 4) * pct / 100;
  if (fw > 0) canvas.fillRect(x + 2, cy - h / 2 + 2, fw, h - 4, c);
  char buf[16];
  snprintf(buf, sizeof(buf), "%d%%", pct);
  drawText(buf, x - 4, cy + 1, &fSmall, COL_MUTED, lgfx::middle_right);
}

static void drawDots(int xRight, int cy, int index, int count) {
  if (count < 2) return;
  const int step = 10;
  int x = xRight - (count - 1) * step - 3;
  for (int i = 0; i < count; i++) {
    if (i == index) canvas.fillCircle(x + i * step, cy, 3, COL_TEXT);
    else canvas.drawCircle(x + i * step, cy, 3, COL_MUTED);
  }
}

static void localHHMM(int64_t t, char* out, size_t n) {
  time_t tt = (time_t)t;
  struct tm lt;
  localtime_r(&tt, &lt);
  snprintf(out, n, "%02d:%02d", lt.tm_hour, lt.tm_min);
}

// Une ligne de départ : destination à gauche, attente à droite.
static void drawRow(int y, const Row* r, int64_t now) {
  const int x = 4, w = W - 8, h = 56;
  canvas.fillRoundRect(x, y, w, h, 8, COL_PANEL);
  if (!r) return;
  const int cy = y + h / 2;
  const int64_t diff = r->when - now;

  // --- partie droite : attente
  int rightW;  // largeur réservée à droite
  auto status = [&](const char* txt, const lgfx::IFont* f, uint16_t c) {
    drawText(txt, x + w - 10, cy + 1, f, c, lgfx::middle_right);
    canvas.setFont(f);
    return (int)canvas.textWidth(txt) + 16;
  };
  if (r->flags & DEP_CANCELLED) {
    rightW = status("Supprimé", &fMedium, COL_RED);
  } else if ((r->flags & DEP_AT_STOP) || diff < 0) {
    rightW = status("À quai", &fLarge, COL_GREEN);
  } else if (diff < 45) {
    // clignote comme sur les quais
    rightW = status("À l'approche", &fMedium, (now & 1) ? COL_AMBER : COL_AMBER_DIM);
  } else if (diff >= 100 * 60) {
    char hhmm[16];
    localHHMM(r->when, hhmm, sizeof(hhmm));
    rightW = status(hhmm, &fLarge, COL_AMBER);
  } else {
    char num[16];
    snprintf(num, sizeof(num), "%d", (int)(diff / 60));
    const int base = y + 45;
    uint16_t c = (r->flags & DEP_REALTIME) ? COL_AMBER : COL_AMBER_DIM;
    drawText("min", x + w - 10, base, &fSmall, c, lgfx::baseline_right);
    canvas.setFont(&fSmall);
    int minW = canvas.textWidth("min");
    drawText(num, x + w - 14 - minW, base, &fBig, c, lgfx::baseline_right);
    canvas.setFont(&fBig);
    rightW = 22 + minW + canvas.textWidth(num);
    if (rightW < 70) rightW = 70;
  }

  // --- partie gauche : destination (+ mention éventuelle)
  const int tx = x + 10;
  const int maxW = w - 20 - rightW;
  char extra[24] = "";
  uint16_t extraCol = COL_MUTED;
  if (r->flags & DEP_DELAYED) {
    copyStr(extra, "Retardé", sizeof(extra));
    extraCol = COL_ORANGE;
  } else if (r->platform[0]) {
    snprintf(extra, sizeof(extra), "Voie %s", r->platform);
  } else if (!(r->flags & DEP_REALTIME) && !(r->flags & DEP_CANCELLED)) {
    copyStr(extra, "Horaire théorique", sizeof(extra));
  }

  char l1[64], l2[64];
  canvas.setFont(&fMedium);
  if (canvas.textWidth(r->dest) <= maxW) {
    if (extra[0]) {
      drawText(r->dest, tx, cy - 8, &fMedium, COL_TEXT, lgfx::middle_left);
      drawText(extra, tx, cy + 15, &fSmall, extraCol, lgfx::middle_left);
    } else {
      drawText(r->dest, tx, cy + 1, &fMedium, COL_TEXT, lgfx::middle_left);
    }
    return;
  }
  canvas.setFont(&fSmall);
  if (canvas.textWidth(r->dest) <= maxW) {
    drawText(r->dest, tx, extra[0] ? cy - 8 : cy + 1, &fSmall, COL_TEXT, lgfx::middle_left);
    if (extra[0]) drawText(extra, tx, cy + 12, &fSmall, extraCol, lgfx::middle_left);
  } else if (wrap2(r->dest, maxW, l1, l2, sizeof(l1))) {
    drawText(l1, tx, cy - 9, &fSmall, COL_TEXT, lgfx::middle_left);
    drawText(l2, tx, cy + 10, &fSmall, COL_TEXT, lgfx::middle_left);
  } else {
    fitText(l1, sizeof(l1), r->dest, maxW);
    drawText(l1, tx, cy + 1, &fSmall, COL_TEXT, lgfx::middle_left);
  }
}

static void drawCenterMessage(int y, int h, const char* a, const char* b, uint16_t color) {
  canvas.fillRoundRect(4, y, W - 8, h, 8, COL_PANEL);
  char buf[64];
  canvas.setFont(&fMedium);
  fitText(buf, sizeof(buf), a, W - 24);
  int cy = y + h / 2;
  if (b && *b) {
    drawText(buf, W / 2, cy - 12, &fMedium, color, lgfx::middle_center);
    canvas.setFont(&fSmall);
    fitText(buf, sizeof(buf), b, W - 24);
    drawText(buf, W / 2, cy + 14, &fSmall, COL_MUTED, lgfx::middle_center);
  } else {
    drawText(buf, W / 2, cy, &fMedium, color, lgfx::middle_center);
  }
}

// ----------------------------------------------------------------------------
//  Écrans
// ----------------------------------------------------------------------------
void uiDrawMain(const UiState& s) {
  canvas.fillScreen(COL_BG);

  // En-tête : pictogramme + nom de l'arrêt + batterie
  const int hy = 18;
  int bx = 6;
  int bw = drawBadge(bx, hy, s.badge && *s.badge ? s.badge : "?", s.mode, s.badgeColor, s.badgeText);
  int titleX = bx + bw + 9;
  int titleMax = W - titleX - 70;
  char title[64];
  const lgfx::IFont* tf = &fMedium;
  canvas.setFont(tf);
  if (canvas.textWidth(s.title ? s.title : "") > titleMax) {
    tf = &fSmall;
    canvas.setFont(tf);
  }
  fitText(title, sizeof(title), s.title ? s.title : "", titleMax);
  drawText(title, titleX, hy + 1, tf, COL_TEXT, lgfx::middle_left);
  drawBattery(W - 6, hy, s.battery);

  // Corps : deux prochains passages
  const int y1 = 36, y2 = 96;
  const View* v = s.view;
  if (v && v->rows > 0) {
    drawRow(y1, &v->row[0], s.now);
    drawRow(y2, v->rows > 1 ? &v->row[1] : nullptr, s.now);
  } else if (!s.hasData) {
    if (s.status && *s.status)
      drawCenterMessage(y1, 116, s.status, "Nouvel essai bientôt", COL_ORANGE);
    else if (s.net == NetState::Connecting)
      drawCenterMessage(y1, 116, "Connexion au WiFi…", nullptr, COL_MUTED);
    else
      drawCenterMessage(y1, 116, "Chargement…", nullptr, COL_MUTED);
  } else {
    drawCenterMessage(y1, 116, "Aucun passage prévu", "Service terminé ou info indisponible", COL_TEXT);
  }

  // Pied de page : état de la mise à jour, sens, favoris
  const int fy = 162;
  char foot[64] = "";
  uint16_t fc = COL_MUTED;
  if (s.busy) {
    copyStr(foot, "Mise à jour…", sizeof(foot));
  } else if (s.status && *s.status && s.hasData) {
    copyStr(foot, s.status, sizeof(foot));
    fc = COL_ORANGE;
  } else if (s.net == NetState::Connecting) {
    copyStr(foot, "Connexion au WiFi…", sizeof(foot));
  } else if (s.ageS >= 0) {
    if (s.ageS < 60) snprintf(foot, sizeof(foot), "Mis à jour il y a %d s", s.ageS);
    else snprintf(foot, sizeof(foot), "Mis à jour il y a %d min", s.ageS / 60);
    if (s.ageS > 300) fc = COL_ORANGE;
  }
  canvas.setFont(&fSmall);
  char tmp[64];
  fitText(tmp, sizeof(tmp), foot, 190);
  drawText(tmp, 8, fy, &fSmall, fc, lgfx::middle_left);
  int dotsW = s.favCount > 1 ? s.favCount * 10 + 6 : 0;
  if (v && v->groupCount > 1) {
    char g[16];
    snprintf(g, sizeof(g), "sens %d/%d", v->groupIndex + 1, v->groupCount);
    drawText(g, W - 8 - dotsW, fy, &fSmall, COL_MUTED, lgfx::middle_right);
  }
  drawDots(W - 6, fy, s.favIndex, s.favCount);

  present();
}

void uiDrawMessage(const char* title, const char* line1, const char* line2, uint32_t accent) {
  canvas.fillScreen(COL_BG);
  canvas.fillRect(0, 0, W, 4, rgb(accent));
  char buf[96];
  canvas.setFont(&fLarge);
  fitText(buf, sizeof(buf), title ? title : "", W - 20);
  drawText(buf, W / 2, 44, &fLarge, rgb(accent), lgfx::middle_center);
  int y = 94;
  for (const char* l : {line1, line2}) {
    if (!l || !*l) continue;
    canvas.setFont(&fMedium);
    if (canvas.textWidth(l) <= W - 20) {
      drawText(l, W / 2, y, &fMedium, COL_TEXT, lgfx::middle_center);
    } else {
      char a[64], b[64];
      canvas.setFont(&fSmall);
      if (wrap2(l, W - 20, a, b, sizeof(a))) {
        drawText(a, W / 2, y - 9, &fSmall, COL_TEXT, lgfx::middle_center);
        drawText(b, W / 2, y + 10, &fSmall, COL_TEXT, lgfx::middle_center);
      } else {
        fitText(a, sizeof(a), l, W - 20);
        drawText(a, W / 2, y, &fSmall, COL_TEXT, lgfx::middle_center);
      }
    }
    y += 42;
  }
  present();
}

void uiDrawPortal(const char* apName, const char* qrPayload, const char* url, const char* url2) {
  canvas.fillScreen(COL_BG);
  // QR code (fond blanc) pour rejoindre le point d'accès d'un coup d'appareil photo
  const int q = 132, qx = 8, qy = (H - q) / 2;
  canvas.fillRoundRect(qx - 4, qy - 4, q + 8, q + 8, 6, COL_TEXT);
  canvas.qrcode(qrPayload, qx, qy, q, 2);

  const int tx = qx + q + 14;
  const int tw = W - tx - 6;
  char buf[64];
  drawText("Configuration", tx, 18, &fMedium, COL_AMBER, lgfx::middle_left);
  drawText("1. Rejoignez le WiFi", tx, 46, &fSmall, COL_MUTED, lgfx::middle_left);
  canvas.setFont(&fMedium);
  fitText(buf, sizeof(buf), apName, tw);
  drawText(buf, tx, 67, &fMedium, COL_TEXT, lgfx::middle_left);
  drawText("2. Ouvrez", tx, 95, &fSmall, COL_MUTED, lgfx::middle_left);
  canvas.setFont(&fMedium);
  fitText(buf, sizeof(buf), url, tw);
  drawText(buf, tx, 116, &fMedium, COL_TEXT, lgfx::middle_left);
  if (url2 && *url2) {
    canvas.setFont(&fSmall);
    fitText(buf, sizeof(buf), url2, tw);
    drawText(buf, tx, 140, &fSmall, COL_MUTED, lgfx::middle_left);
  }
  drawText("BOOT : quitter", tx, 160, &fSmall, COL_MUTED, lgfx::middle_left);
  present();
}

void uiDrawSplash(const char* version) {
  canvas.fillScreen(COL_BG);
  canvas.fillCircle(W / 2, 62, 34, COL_TEXT);
  drawText("M", W / 2, 64, &fBig, COL_BG, lgfx::middle_center);
  drawText("Afficheur Métro", W / 2, 122, &fMedium, COL_TEXT, lgfx::middle_center);
  char buf[32];
  snprintf(buf, sizeof(buf), "v%s", version);
  drawText(buf, W / 2, 150, &fSmall, COL_MUTED, lgfx::middle_center);
  present();
}
