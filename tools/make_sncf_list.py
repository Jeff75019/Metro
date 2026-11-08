#!/usr/bin/env python3
"""Génère Metro/sncf_list_data.h : les gares proposées sur l'afficheur (programme
Trains, choix « gare → départs / arrivées »), à partir de Metro/sncf_stations.h
(les 145 grandes gares de Train Nomade, nom en commentaire) et des gares
parisiennes de Metro/settings.cpp (tous leurs codes UIC).

Les gares de Paris viennent en premier, les autres par ordre alphabétique."""
import os
import re
import unicodedata

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
src = open(os.path.join(ROOT, "Metro", "sncf_stations.h"), encoding="utf-8").read()
cfg = open(os.path.join(ROOT, "Metro", "settings.cpp"), encoding="utf-8").read()

stations = {}  # slug -> [nom, [uic...]]
for uic, slug, name in re.findall(r'\{"(\d+)", "([^"]+)"\},\s*//\s*(.+)', src):
    stations.setdefault(slug, [name.strip(), []])[1].append(uic)

# Gares de Paris : tous leurs codes UIC et leur nom court (« Gare de Lyon »)
paris_def = re.findall(r'\{"([\d,]+)", "([^"]+)"\}', cfg.split("PARIS_STATIONS[] = {")[1].split("};")[0])
paris_uic = [u for u, _ in paris_def]
paris_name = {u: n for u, n in paris_def}


def key(n):
    return unicodedata.normalize("NFKD", n).encode("ascii", "ignore").decode().lower()


paris, others = [], []
for name, uics in stations.values():
    full = next((p for p in paris_uic if uics[0] in p.split(",")), None)
    if full:
        paris.append((paris_name[full], full))
    else:
        others.append((name, ",".join(uics)))
order = [p.split(",")[0] for p in paris_uic]
paris.sort(key=lambda e: order.index(e[1].split(",")[0]))
others.sort(key=lambda e: key(e[0]))

with open(os.path.join(ROOT, "Metro", "sncf_list_data.h"), "w", encoding="utf-8") as f:
    f.write("// Généré par tools/make_sncf_list.py — ne pas modifier à la main.\n")
    f.write("// Gares proposées sur l'afficheur : Paris d'abord, puis ordre alphabétique.\n")
    f.write("#pragma once\n\n")
    f.write("static const SncfPick SNCF_PICKS[] = {\n")
    for name, uic in paris + others:
        f.write('    {"%s", "%s"},\n' % (name.replace('"', '\\"'), uic))
    f.write("};\n")
    f.write("static const int SNCF_PARIS_COUNT = %d;\n" % len(paris))
print(len(paris), "gares de Paris,", len(others), "autres")
