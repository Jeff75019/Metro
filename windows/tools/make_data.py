#!/usr/bin/env python3
"""Données embarquées de l'application Windows (app/data.js), tirées du projet :
   - lignes (couleurs, noms) : Metro/lines.cpp ;
   - stations de métro, RER et tram, dans l'ordre des lignes : réseau embarqué
     de l'afficheur, exporté par tools/dump_reseau.cpp (JSON sur l'entrée) ;
   - gares SNCF : même export.
Usage : ./dump_reseau | python3 make_data.py ../../Metro/lines.cpp > ../app/data.js"""
import json, re, sys

lines_cpp = open(sys.argv[1], encoding='utf-8').read()
modes = {'Metro': 'metro', 'Rer': 'rer', 'Tram': 'tram', 'Train': 'train', 'Shuttle': 'navette'}
lines = {}
for m in re.finditer(r'\{"(\w+)", "([^"]+)", LineMode::(\w+), 0x([0-9A-F]{6}), 0x([0-9A-F]{6})\}', lines_cpp):
    lid, name, mode, col, txt = m.groups()
    lines[lid] = {'n': name, 'm': modes.get(mode, 'bus'), 'c': '#' + col.lower(), 't': '#' + txt.lower()}

net = json.load(sys.stdin)
network = [{'id': l['id'], 'year': l['year'], 'stops': [[s[0], s[1], s[2], s[3]] for s in l['stops']]} for l in net['lines']]
sncf = net['sncf']

out = sys.stdout
out.write('// Généré par tools/make_data.py — ne pas modifier à la main.\n')
out.write('// Sources : open data Île-de-France Mobilités et SNCF (licence ODbL).\n')
out.write('const LINES = ' + json.dumps(lines, ensure_ascii=False, separators=(',', ':')) + ';\n')
out.write('// Lignes de métro, RER et tram : arrêts [zone d\'arrêt PRIM, nom, drapeaux (1 branche, 2 terminus), année d\'ouverture]\n')
out.write('const NETWORK = ' + json.dumps(network, ensure_ascii=False, separators=(',', ':')) + ';\n')
out.write('// Gares SNCF : [nom, codes UIC]\n')
out.write('const SNCF_STATIONS = ' + json.dumps(sncf, ensure_ascii=False, separators=(',', ':')) + ';\n')
