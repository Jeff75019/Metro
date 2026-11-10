#!/usr/bin/env python3
"""Regroupe l'application (app/) en un seul fichier HTML autonome : feuille de
style, scripts, polices et icône inclus. Ce fichier est embarqué dans
Metro.exe, et s'ouvre aussi tel quel dans Edge ou Chrome.
Usage : python3 bundle.py ../app ../dist/Metro.html"""
import base64, os, re, sys

src, out = sys.argv[1], sys.argv[2]
read = lambda f: open(os.path.join(src, f), encoding='utf-8').read()
b64 = lambda f: base64.b64encode(open(os.path.join(src, f), 'rb').read()).decode()

css = read('style.css')
css = re.sub(r'url\((fonts/[^)]+\.woff2)\)', lambda m: f'url(data:font/woff2;base64,{b64(m.group(1))})', css)
html = read('index.html')

def inline_script(name):
    js = read(name)
    assert '</script' not in js.lower(), name
    return f'<script>\n{js}\n</script>'

html = html.replace('<link rel="stylesheet" href="style.css">', f'<style>\n{css}\n</style>')
html = html.replace('<link rel="icon" href="icon.png">', f'<link rel="icon" href="data:image/png;base64,{b64("icon.png")}">')
html = html.replace('<script src="data.js"></script>', inline_script('data.js'))
html = html.replace('<script src="app.js"></script>', inline_script('app.js'))
assert 'src="' not in html and 'href="style' not in html, 'ressource non incluse'
os.makedirs(os.path.dirname(out) or '.', exist_ok=True)
open(out, 'w', encoding='utf-8').write(html)
print(f'{out} : {len(html.encode()) // 1024} Ko')
