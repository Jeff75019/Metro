#!/usr/bin/env python3
"""
Aperçu de la page de configuration sur PC, sans l'afficheur.

Sert la page contenue dans Metro/portal_html.h et répond aux appels /api/...
avec des données fictives (rien n'est envoyé à l'afficheur ni à PRIM).

    python3 tools/portal_preview.py        puis ouvrir http://localhost:8080
"""
import json
import os
import re
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlparse

HERE = os.path.dirname(os.path.abspath(__file__))
HEADER = os.path.join(HERE, "..", "Metro", "portal_html.h")

CONFIG = {
    "wifi": [{"ssid": "Freebox-1A2B3C", "hasPass": True}],
    "apiKeySet": True,
    "apiKeyHint": "…9f3c",
    "favorites": [
        {"label": "", "stopRef": "STIF:StopArea:SP:71517:", "stopName": "Bastille", "line": "C01371",
         "badge": "", "filter": "Château de Vincennes",
         "_line": {"name": "1", "mode": "metro", "color": "#FFBE00", "text": "#000000"}},
        {"label": "Boulot", "stopRef": "STIF:StopArea:SP:47900:", "stopName": "Châtelet - Les Halles",
         "line": "C01743", "badge": "", "filter": "",
         "_line": {"name": "B", "mode": "rer", "color": "#5091CB", "text": "#FFFFFF"}},
    ],
    "refreshS": 30, "awakeS": 60, "dailyQuota": 1000, "brightness": 180,
    "sleepMode": 0, "motionWake": False, "flip": False,
    "sncfKeySet": True, "sncfKeyHint": "…b21e", "mainlineOnly": True, "sncfQuota": 5000,
    "stations": [{"id": "stop_area:SNCF:87686006", "name": "Gare de Lyon"},
                 {"id": "stop_area:SNCF:87271007", "name": "Gare du Nord"},
                 {"id": "stop_area:SNCF:87391003", "name": "Montparnasse"},
                 {"id": "stop_area:SNCF:87111849", "name": "Marne-la-Vallée Chessy"}],
}
STATUS = {"sta": True, "ssid": "Freebox-1A2B3C", "ip": "192.168.1.42", "rssi": -58, "ap": "Metro-3F2A",
          "battery": 87, "quotaUsed": 42, "quota": 1000, "sncfUsed": 118, "sncfQuota": 5000,
          "version": "1.1.0"}
SCAN = {"networks": [{"ssid": "Freebox-1A2B3C", "rssi": -48, "secure": True},
                     {"ssid": "Livebox-77F0", "rssi": -67, "secure": True},
                     {"ssid": "iPhone de Jeff", "rssi": -74, "secure": True},
                     {"ssid": "FreeWifi_secure", "rssi": -81, "secure": False}]}
SEARCH = {"stops": [{"ref": "STIF:StopArea:SP:71517:", "name": "Bastille", "town": "Paris", "type": "metro"},
                    {"ref": "STIF:StopArea:SP:71518:", "name": "Bastille", "town": "Paris", "type": "bus"},
                    {"ref": "STIF:StopArea:SP:71600:", "name": "Bastille - Rue Saint-Antoine", "town": "Paris",
                     "type": "bus"}]}
LINES = {"ref": "STIF:StopArea:SP:71517:", "stopName": "Bastille", "lines": [
    {"id": "C01371", "name": "1", "mode": "metro", "color": "#FFBE00", "text": "#000000",
     "dests": ["Château de Vincennes", "La Défense"]},
    {"id": "C01375", "name": "5", "mode": "metro", "color": "#FF5A00", "text": "#000000",
     "dests": ["Bobigny - Pablo Picasso", "Place d'Italie"]},
    {"id": "C01378", "name": "8", "mode": "metro", "color": "#D282BE", "text": "#000000",
     "dests": ["Balard", "Pointe du Lac"]}]}


SNCF_SEARCH = {"stations": [{"id": "stop_area:SNCF:87393009", "name": "Versailles Chantiers"},
                            {"id": "stop_area:SNCF:87393843", "name": "Versailles Rive Droite"},
                            {"id": "stop_area:SNCF:87393579", "name": "Versailles Château Rive Gauche"}]}


def load_page():
    src = open(HEADER, encoding="utf-8").read()
    m = re.search(r'R"HTML\((.*)\)HTML"', src, re.S)
    if not m:
        sys.exit("page introuvable dans portal_html.h")
    return m.group(1).encode("utf-8")


class Handler(BaseHTTPRequestHandler):
    def _send(self, code, body, ctype="application/json; charset=utf-8"):
        if not isinstance(body, bytes):
            body = json.dumps(body).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = urlparse(self.path).path
        if path == "/":
            return self._send(200, load_page(), "text/html; charset=utf-8")
        routes = {"/api/config": CONFIG, "/api/status": STATUS, "/api/scan": SCAN,
                  "/api/search": SEARCH, "/api/lines": LINES, "/api/sncf/search": SNCF_SEARCH}
        if path in routes:
            return self._send(200, routes[path])
        self._send(404, {"error": "inconnu"})

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8") if length else ""
        if urlparse(self.path).path == "/api/config":
            print("Configuration reçue :", body)
            return self._send(200, {"ok": True, "ready": True, "missing": ""})
        self._send(200, {"ok": True})

    def log_message(self, *args):
        pass


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    print(f"Aperçu : http://localhost:{port}")
    HTTPServer(("127.0.0.1", port), Handler).serve_forever()
