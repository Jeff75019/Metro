"""KeoLive.zip : le lanceur et la page, côte à côte, à distribuer.

usage : python3 tools/make_zip.py sortie.zip fichier...
Les dates sont fixées : la même entrée redonne exactement la même archive.
"""
import os
import sys
import zipfile

out, files = sys.argv[1], sys.argv[2:]
with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for f in files:
        info = zipfile.ZipInfo(os.path.basename(f), date_time=(2026, 1, 1, 0, 0, 0))
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = 0o644 << 16
        with open(f, 'rb') as src:
            z.writestr(info, src.read())
