#!/usr/bin/env python3
"""lines.json pour la partie native (notifications) : les lignes de windows/app/data.js,
dans l'ordre du réseau (identifiant, nom, mode, couleurs).
Usage : python3 tools/lines_json.py ../windows/app/data.js build/assets/lines.json"""
import json
import re
import sys

src, out = sys.argv[1], sys.argv[2]
js = open(src, encoding='utf-8').read()
lines = json.loads(re.search(r'const LINES = (\{.*?\});\n', js, re.S).group(1))
arr = [{'id': k, 'n': v['n'], 'm': v['m'], 'c': v['c'], 't': v['t']} for k, v in lines.items()]
with open(out, 'w', encoding='utf-8') as f:
    json.dump(arr, f, ensure_ascii=False, separators=(',', ':'))
print(out, ':', len(arr), 'lignes')
