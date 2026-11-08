#!/usr/bin/env python3
"""Génère Metro/reseau_data.h : les lignes de métro, RER et tramway d'Île-de-France
avec leurs stations dans l'ordre de la ligne, pour le choix « ligne → station »
sur l'afficheur.

Sources (open data Île-de-France Mobilités, licence ODbL) :
  - emplacement-des-gares-idf    : stations de chaque ligne, avec l'identifiant de
                                   la zone d'arrêt (ZdA) utilisé par l'API PRIM
                                   (MonitoringRef « STIF:StopArea:SP:<zda>: ») ;
  - traces-du-reseau-ferre-idf   : tracé des lignes, en tronçons d'une station à
                                   l'autre : sert à retrouver l'ordre des stations
                                   (branches comprises) ;
  - projets_arrets_idf / projets_lignes_idf : stations et dates d'ouverture prévues
                                   des lignes en construction (Grand Paris Express).

Utilisation :
  python3 tools/make_reseau.py                  # télécharge les données
  python3 tools/make_reseau.py gares.json traces.geojson projets_arrets.json projets_lignes.json
"""
import json
import math
import os
import sys
import urllib.request
from collections import defaultdict

API = "https://data.iledefrance-mobilites.fr/api/explore/v2.1/catalog/datasets"
URL_GARES = (API + "/emplacement-des-gares-idf/exports/json?select=nom_gares,id_ref_zda,idrefligc,"
             "res_com,mode,termetro,terrer,tertram,geo_point_2d")
URL_TRACES = API + "/traces-du-reseau-ferre-idf/exports/geojson?select=idrefligc,res_com,mode,geo_shape"
URL_PROJ_ARRETS = API + "/projets_arrets_idf/exports/json"
URL_PROJ_LIGNES = API + "/projets_lignes_idf/exports/json?select=operation,mes_off_tx"

# Lignes en construction : identifiant (IDFM s'il existe déjà, sinon provisoire,
# à remplacer quand la ligne apparaîtra dans le référentiel) -> nom dans les
# données des projets. Leurs stations pas encore référencées (zone d'arrêt 0)
# s'affichent avec l'année d'ouverture prévue.
FUTURE = {"C02833": "METRO 18", "M16": "METRO 16", "M17": "METRO 17"}

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Metro", "reseau_data.h")

MODES = {"METRO": "Metro", "RER": "Rer", "TRAMWAY": "Tram"}
# Lignes retenues et ordre d'affichage (identifiants IDFM, nom court)
LINES = [
    ("C01371", "1"), ("C01372", "2"), ("C01373", "3"), ("C01386", "3B"), ("C01374", "4"),
    ("C01375", "5"), ("C01376", "6"), ("C01377", "7"), ("C01387", "7B"), ("C01378", "8"),
    ("C01379", "9"), ("C01380", "10"), ("C01381", "11"), ("C01382", "12"), ("C01383", "13"),
    ("C01384", "14"), ("M16", "16"), ("M17", "17"), ("C02833", "18"),
    ("C01742", "A"), ("C01743", "B"), ("C01727", "C"), ("C01728", "D"), ("C01729", "E"),
    ("C01389", "T1"), ("C01390", "T2"), ("C01391", "T3a"), ("C01679", "T3b"), ("C01843", "T4"),
    ("C01684", "T5"), ("C01794", "T6"), ("C01774", "T7"), ("C01795", "T8"), ("C02317", "T9"),
    ("C02528", "T10"), ("C01999", "T11"), ("C02529", "T12"), ("C02344", "T13"), ("C02732", "T14"),
]
# Caractères présents dans les polices de l'afficheur (tools/make_fonts.py)
FONT_CHARS = set(range(0x20, 0x7F)) | set(range(0xA0, 0x100)) | {0x152, 0x153, 0x2013, 0x2014, 0x2019,
                                                                  0x2026, 0x20AC}


def load(path_or_url, cache):
    if path_or_url:
        with open(path_or_url, encoding="utf-8") as f:
            return json.load(f)
    print("Téléchargement", cache, file=sys.stderr)
    with urllib.request.urlopen(cache, timeout=120) as r:
        return json.loads(r.read().decode("utf-8"))


def dist(a, b):  # mètres (approximation locale, suffisante en Île-de-France)
    return math.hypot((a[0] - b[0]) * 73000, (a[1] - b[1]) * 111000)


def clean_name(s):
    for z in ("\u200b", "\u200c", "\u200d", "\ufeff"):  # caractères invisibles
        s = s.replace(z, "")
    s = " ".join(s.split())
    s = s.replace("\u2010", "-").replace("\u2011", "-").replace("\u2012", "-")
    s = s.replace("\u00a0", " ").replace("\u2018", "\u2019").replace("'", "\u2019")
    out = "".join(c if ord(c) in FONT_CHARS else "?" for c in s)
    if "?" in out and "?" not in s:
        print("  caractère absent de la police :", s, file=sys.stderr)
    return out


def order_line(stations, segments):
    """stations : {zda: (nom, (lon, lat), terminus)} ; segments : listes de points.
    Renvoie [(zda, branche)] : branche = True quand la station ne suit pas la
    précédente sur la ligne (début d'une autre branche)."""
    coords = {z: s[1] for z, s in stations.items()}

    # Graphe : sommets = stations ou extrémités de tronçons hors station (aiguillages)
    nodes = {}  # clé -> zda ou point

    def node_for(p):
        best, bd = None, 1e9
        for z, c in coords.items():
            d = dist(p, c)
            if d < bd:
                best, bd = z, d
        if bd <= 60:
            return best
        key = (round(p[0], 4), round(p[1], 4))  # ~10 m
        return nodes.setdefault(key, "J%d" % len(nodes))

    adj = defaultdict(set)
    for pts in segments:
        a, b = node_for(pts[0]), node_for(pts[-1])
        if a != b:
            adj[a].add(b)
            adj[b].add(a)

    # Voisins « station » de chaque station, en traversant les aiguillages
    sadj = defaultdict(set)
    for z in stations:
        seen, stack = {z}, list(adj[z])
        while stack:
            n = stack.pop()
            if n in seen:
                continue
            seen.add(n)
            if n in stations:
                sadj[z].add(n)
            else:
                stack.extend(adj[n])
    for z in stations:
        for n in sadj[z]:
            sadj[n].add(z)

    # Tracé incomplet (prolongement récent…) : on relie les morceaux isolés par
    # leurs stations les plus proches
    def components():
        comp, out = {}, []
        for z in stations:
            if z in comp:
                continue
            cur, stack = [], [z]
            comp[z] = len(out)
            while stack:
                n = stack.pop()
                cur.append(n)
                for m in sadj[n]:
                    if m not in comp:
                        comp[m] = len(out)
                        stack.append(m)
            out.append(cur)
        return out

    def farthest(start, allowed):
        """Chemin le plus long (en mètres) depuis start, parmi les stations allowed (arbre ou presque)."""
        best = (0.0, [start])
        stack = [(start, [start], 0.0)]
        while stack:
            n, path, d = stack.pop()
            if d > best[0]:
                best = (d, path)
            for m in sadj[n]:
                if m in allowed and m not in path:
                    stack.append((m, path + [m], d + dist(coords[n], coords[m])))
        return best[1]

    # Itinéraire principal : d'un terminus à l'autre (le plus long), sur la plus
    # grande partie connexe du tracé
    comps = sorted(components(), key=len, reverse=True)
    core = set(comps[0])
    ends = [z for z in core if len(sadj[z]) <= 1] or list(core)
    a = farthest(ends[0], core)[-1]
    main = farthest(a, core)
    # Sens de lecture : d'ouest en est si la ligne est plutôt horizontale, sinon du nord au sud
    s, e = coords[main[0]], coords[main[-1]]
    if abs(s[0] - e[0]) * 73 >= abs(s[1] - e[1]) * 111:
        if s[0] > e[0]:
            main.reverse()
    elif s[1] < e[1]:
        main.reverse()

    # Stations hors du tracé connu (prolongement récent, tracé incomplet) : insérées
    # entre deux stations de l'itinéraire si elles se trouvent sur le chemin
    orphans = [z for c in comps[1:] for z in c]
    progress = True
    while orphans and progress:
        progress = False
        for x in list(orphans):
            best = None
            for i in range(len(main) - 1):
                pa, pb = coords[main[i]], coords[main[i + 1]]
                seg = dist(pa, pb)
                detour = dist(pa, coords[x]) + dist(coords[x], pb) - seg
                if detour <= 0.35 * seg + 200 and (best is None or detour < best[0]):
                    best = (detour, i)
            if best:
                i = best[1]
                sadj[main[i]].discard(main[i + 1])
                sadj[main[i + 1]].discard(main[i])
                for y in (main[i], main[i + 1]):
                    sadj[x].add(y)
                    sadj[y].add(x)
                main.insert(i + 1, x)
                orphans.remove(x)
                progress = True
    # Les autres morceaux isolés sont reliés par leurs stations les plus proches
    comps = components()
    while len(comps) > 1:
        best = None
        for i, ca in enumerate(comps):
            for cb in comps[i + 1:]:
                for x in ca:
                    for y in cb:
                        d = dist(coords[x], coords[y])
                        if best is None or d < best[0]:
                            best = (d, x, y)
        sadj[best[1]].add(best[2])
        sadj[best[2]].add(best[1])
        comps = components()

    # pos : place le long de la ligne (0 au début de l'itinéraire principal) ;
    # une branche est comptée depuis son embranchement, vers le côté de la ligne
    # où elle se dirige. Sert à ranger les destinations d'un bout à l'autre.
    remaining = set(stations)
    result = []
    pos = {}
    for i, z in enumerate(main):
        result.append((z, False))
        pos[z] = i
        remaining.discard(z)
    # Branches restantes, depuis leur point d'attache, dans l'ordre de la ligne
    while remaining:
        attach, base = None, None
        for z, _ in result:
            nxt = [m for m in sadj[z] if m in remaining]
            if nxt:
                attach, base = nxt[0], z
                break
        if attach is None:  # station isolée (tracé manquant)
            attach = min(remaining, key=lambda z: stations[z][0])
        path = farthest(attach, remaining)
        far = coords[path[-1]]
        towards_start = dist(far, coords[main[0]]) < dist(far, coords[main[-1]])
        p0 = pos[base] if base is not None else len(main)
        for i, z in enumerate(path):
            result.append((z, i == 0))
            pos[z] = p0 - (i + 1) if towards_start else p0 + (i + 1)
            remaining.discard(z)
    return [(z, b, pos[z]) for z, b in result]


def order_geometric(stations):
    """Lignes en construction (pas de tracé complet) : chemin le plus court passant
    par toutes les stations, d'un terminus à l'autre (plus proche voisin + 2-opt)."""
    coords = {z: s[1] for z, s in stations.items()}
    zs = list(stations)
    if len(zs) < 3:
        return [(z, False, i) for i, z in enumerate(zs)]
    # Extrémités : les deux terminus annoncés les plus éloignés (à défaut, les deux
    # stations les plus éloignées)
    ends = [z for z in zs if stations[z][2]]
    if len(ends) < 2:
        ends = zs
    a, b = max(((x, y) for x in ends for y in ends), key=lambda p: dist(coords[p[0]], coords[p[1]]))
    path, left = [a], set(zs) - {a, b}
    while left:
        n = min(left, key=lambda z: dist(coords[path[-1]], coords[z]))
        path.append(n)
        left.discard(n)
    path.append(b)

    def length(p):
        return sum(dist(coords[p[i]], coords[p[i + 1]]) for i in range(len(p) - 1))

    improved = True
    while improved:
        improved = False
        for i in range(1, len(path) - 2):  # extrémités fixes
            for j in range(i + 1, len(path) - 1):
                cand = path[:i] + path[i:j + 1][::-1] + path[j + 1:]
                if length(cand) + 1 < length(path):
                    path, improved = cand, True
    s, e = coords[path[0]], coords[path[-1]]
    if abs(s[0] - e[0]) * 73 >= abs(s[1] - e[1]) * 111:
        if s[0] > e[0]:
            path.reverse()
    elif s[1] < e[1]:
        path.reverse()
    return [(z, False, i) for i, z in enumerate(path)]


def year_of(text):
    digits = "".join(c for c in str(text) if c.isdigit())
    return int(digits[-4:]) if len(digits) >= 4 else 0


def main():
    gares = load(sys.argv[1] if len(sys.argv) > 1 else None, URL_GARES)
    traces = load(sys.argv[2] if len(sys.argv) > 2 else None, URL_TRACES)
    proj_arrets = load(sys.argv[3] if len(sys.argv) > 3 else None, URL_PROJ_ARRETS)
    proj_lignes = load(sys.argv[4] if len(sys.argv) > 4 else None, URL_PROJ_LIGNES)

    by_line = defaultdict(dict)
    for r in gares:
        if r["mode"] not in MODES or not r.get("id_ref_zda"):
            continue
        p = r["geo_point_2d"]
        # termetro / terrer / tertram : nom de la ligne dont la station est le terminus, sinon "0"
        term = any(str(r.get(k) or "0") not in ("0", "") for k in ("termetro", "terrer", "tertram"))
        zda = int(r["id_ref_zda"])
        if zda in by_line[r["idrefligc"]]:
            continue
        by_line[r["idrefligc"]][zda] = (clean_name(r["nom_gares"]), (p["lon"], p["lat"]), term)
    years = defaultdict(int)  # (ligne, clé de station) -> année d'ouverture prévue (0 : en service)

    # Lignes en construction : stations des projets, sauf celles déjà référencées
    op_year = {r["operation"]: year_of(r.get("mes_off_tx")) for r in proj_lignes}
    line_year = {}
    for lid, res in FUTURE.items():
        st = by_line[lid]
        known = list(st.values())
        added = {}
        for r in proj_arrets:
            if r.get("res_com") != res or not r.get("nom_arret"):
                continue
            p = r["geo_point_2d"]
            c = (p["lon"], p["lat"])
            y = op_year.get(r.get("operation"), 0)
            line_year[lid] = min(line_year.get(lid, 9999), y or 9999)
            if any(dist(c, k[1]) < 400 for k in known):
                continue  # déjà dans les gares référencées (zone d'arrêt connue)
            name = clean_name(r["nom_arret"])
            if name in added:  # même station dans deux phases : la plus proche ouverture
                old = added[name]
                years[(lid, old)] = min(years[(lid, old)], y) if years[(lid, old)] else y
                continue
            key = -(len(st) + 1)
            st[key] = (name, c, str(r.get("terminus")) == "1")
            years[(lid, key)] = y
            added[name] = key

    segs = defaultdict(list)
    for f in traces["features"]:
        g = f["geometry"]
        parts = g["coordinates"] if g["type"] == "MultiLineString" else [g["coordinates"]]
        segs[f["properties"]["idrefligc"]].extend(parts)

    names, name_off = [], {}
    pool_len = 0
    lines_out, stops_out = [], []
    for lid, short in LINES:
        st = by_line.get(lid)
        if not st:
            print("Ligne absente des données :", lid, short, file=sys.stderr)
            continue
        order = order_geometric(st) if lid in FUTURE else order_line(st, segs[lid])
        first = len(stops_out)
        for i, (zda, branch, p) in enumerate(order):
            name, _, term = st[zda]
            if lid in FUTURE:  # terminus : les deux bouts de la ligne complète
                term = i in (0, len(order) - 1)
            if name not in name_off:
                name_off[name] = pool_len
                names.append(name)
                pool_len += len(name.encode("utf-8")) + 1
            flags = (1 if branch else 0) | (2 if term else 0)
            stops_out.append((max(zda, 0), name_off[name], flags, p, years[(lid, zda)], name))
        ly = line_year.get(lid, 0)
        lines_out.append((lid, short, first, len(order), 0 if ly == 9999 else ly))
        nb = sum(1 for _, b, _ in order if b)
        print("%-4s %-7s %2d stations%s : %s … %s" % (short, lid, len(order),
              (", %d branche(s)" % nb) if nb else "", st[order[0][0]][0], st[order[-1][0]][0]),
              file=sys.stderr)

    assert pool_len < 65536
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("// Généré par tools/make_reseau.py — ne pas modifier à la main.\n")
        f.write("// Lignes de métro, RER et tramway d'Île-de-France, stations dans l'ordre de la ligne.\n")
        f.write("// Source : open data Île-de-France Mobilités (emplacement-des-gares-idf,\n")
        f.write("// traces-du-reseau-ferre-idf, projets_arrets_idf, projets_lignes_idf), licence ODbL.\n")
        f.write("#pragma once\n#include <stdint.h>\n\n")
        f.write("// Noms des stations, chacun terminé par \\0 (une seule fois par nom)\n")
        f.write("static const char NET_NAMES[] =\n")
        for n in names:
            f.write("    %s \"\\0\"\n" % json.dumps(n, ensure_ascii=False))
        f.write("    ;\n\n")
        f.write("// zda : zone d'arrêt (MonitoringRef PRIM « STIF:StopArea:SP:<zda>: »)\n")
        f.write("// name : position du nom dans NET_NAMES ; flags : NET_BRANCH | NET_TERMINUS ;\n")
        f.write("// pos : place le long de la ligne (branches : depuis leur embranchement) ;\n")
        f.write("// year : année d'ouverture prévue (0 : en service ; zda 0 : pas encore référencée)\n")
        f.write("static const NetStop NET_STOPS[] = {\n")
        for zda, off, flags, p, y, name in stops_out:
            f.write("    {%d, %d, %d, %d, %d},  // %s\n" % (zda, off, flags, p, y, name))
        f.write("};\n\n")
        f.write("// year : ligne pas encore en service, ouverture prévue cette année-là (0 : en service)\n")
        f.write("static const NetLine NET_LINES[] = {\n")
        for lid, short, first, count, y in lines_out:
            f.write("    {\"%s\", %d, %d, %d},  // %s\n" % (lid, first, count, y, short))
        f.write("};\n")
    print("%d lignes, %d stations, %d noms (%d octets) -> %s" % (len(lines_out), len(stops_out), len(names),
                                                                 pool_len, OUT), file=sys.stderr)


if __name__ == "__main__":
    main()
