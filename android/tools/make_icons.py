#!/usr/bin/env python3
"""Calque avant de l'icône adaptative (mipmap-*/ic_fg.png) à partir de l'icône du projet
(windows/sources/icon-256.png : le « K » et le signal, fond transparent).

Canevas de 108 dp ; le dessin tient dans un carré de 46 dp au centre, pour rester entier
dans la zone sûre (cercle de 66 dp) quelle que soit la forme choisie par le lanceur.
Usage : python3 tools/make_icons.py ../windows/sources/icon-256.png res
"""
import os
import sys

from PIL import Image

src, res = sys.argv[1], sys.argv[2]
im = Image.open(src).convert('RGBA')
im = im.crop(im.getbbox())           # au plus près du dessin
DENSITIES = {'mdpi': 1, 'hdpi': 1.5, 'xhdpi': 2, 'xxhdpi': 3, 'xxxhdpi': 4}
for name, f in DENSITIES.items():
    canvas = round(108 * f)
    box = round(46 * f)
    scale = min(box / im.width, box / im.height)
    w, h = round(im.width * scale), round(im.height * scale)
    layer = Image.new('RGBA', (canvas, canvas), (0, 0, 0, 0))
    layer.paste(im.resize((w, h), Image.LANCZOS), ((canvas - w) // 2, (canvas - h) // 2))
    d = os.path.join(res, 'mipmap-' + name)
    os.makedirs(d, exist_ok=True)
    layer.save(os.path.join(d, 'ic_fg.png'), optimize=True)
    print(d, canvas, 'px')
