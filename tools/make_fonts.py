#!/usr/bin/env python3
"""
Génère les polices anti-aliasées (format VLW, lu par LovyanGFX) utilisées par
le firmware, à partir des fichiers .woff/.ttf de tools/fonts/.

Pourquoi ? Les polices intégrées aux bibliothèques graphiques Arduino n'ont
souvent pas les accents (« Château », « Défense », « Église »...). Ici on
rasterise une vraie police (Barlow Semi Condensed, licence OFL) en niveaux de
gris, avec tout le jeu Latin-1 + quelques signes typographiques.

Usage :
    pip install pillow
    python3 tools/make_fonts.py

Sortie : Metro/fonts_metro.h (tableaux d'octets const, en flash).
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONT_DIR = os.path.join(HERE, "fonts")
OUT = os.path.join(HERE, "..", "Metro", "fonts_metro.h")

# Jeu de caractères "texte" : ASCII + Latin-1 + Œ œ ’ – — … €
TEXT_CHARS = (
    [c for c in range(0x20, 0x7F)]
    + [c for c in range(0xA0, 0x100)]
    + [0x152, 0x153, 0x2013, 0x2014, 0x2019, 0x2026, 0x20AC]
)
# Jeu "grands chiffres" pour les minutes d'attente
BIG_CHARS = [ord(c) for c in " 0123456789:+-<>'M"]

# (nom C, fichier, taille en pixels, jeu de caractères)
FONTS = [
    ("font_small", "barlow-semi-condensed-latin-600-normal.woff", 16, TEXT_CHARS),
    ("font_medium", "barlow-semi-condensed-latin-600-normal.woff", 23, TEXT_CHARS),
    ("font_large", "barlow-semi-condensed-latin-700-normal.woff", 30, TEXT_CHARS),
    ("font_big", "barlow-semi-condensed-latin-700-normal.woff", 52, BIG_CHARS),
]


def be32(v):
    return (v & 0xFFFFFFFF).to_bytes(4, "big")


def make_vlw(path, size, chars):
    font = ImageFont.truetype(path, size)
    ascent, descent = font.getmetrics()
    headers = bytearray()
    bitmaps = bytearray()
    count = 0
    for code in sorted(set(chars)):
        ch = chr(code)
        adv = int(round(font.getlength(ch)))
        x0, y0, x1, y1 = font.getbbox(ch, anchor="ls")
        w, h = max(0, x1 - x0), max(0, y1 - y0)
        if code in (0x20, 0xA0) or w == 0 or h == 0:
            w = h = 0
            dx = dy = 0
            bmp = b""
        else:
            img = Image.new("L", (w, h), 0)
            ImageDraw.Draw(img).text((-x0, -y0), ch, font=font, fill=255, anchor="ls")
            bmp = img.tobytes()
            dx, dy = x0, -y0
        if w > 255 or adv > 255 or not -128 <= dx <= 127:
            sys.exit(f"glyph {ch!r} too large for VLW ({w}x{h})")
        headers += be32(code) + be32(h) + be32(w) + be32(adv) + be32(dy) + be32(dx) + be32(0)
        bitmaps += bmp
        count += 1
    head = be32(count) + be32(11) + be32(size) + be32(0) + be32(ascent) + be32(descent)
    return bytes(head + headers + bitmaps)


def main():
    lines = [
        "// Fichier généré par tools/make_fonts.py — ne pas modifier à la main.",
        "// Police : Barlow Semi Condensed (SIL Open Font License 1.1, voir tools/fonts/OFL.txt)",
        "#pragma once",
        "#include <stdint.h>",
        "",
    ]
    for name, file, size, chars in FONTS:
        data = make_vlw(os.path.join(FONT_DIR, file), size, chars)
        lines.append(f"// {file} @ {size}px, {len(data)} octets")
        lines.append(f"static const uint8_t {name}[] = {{")
        for i in range(0, len(data), 24):
            lines.append("  " + ",".join(f"0x{b:02x}" for b in data[i:i + 24]) + ",")
        lines.append("};")
        lines.append("")
        print(f"{name}: {size}px, {len(data)} octets")
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print("->", os.path.normpath(OUT))


if __name__ == "__main__":
    main()
