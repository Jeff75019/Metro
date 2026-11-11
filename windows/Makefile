# KeoLive pour Windows
#   make        : dist/KeoLive.html (l'application, en un seul fichier), dist/KeoLive.exe (le lanceur)
#                 et dist/KeoLive.zip (les deux, à distribuer)
#   make html   : dist/KeoLive.html seulement
#
# Le lanceur ne contient pas la page : il lit KeoLive.html posé à côté de lui. Modifier l'interface
# (dossier app/) ne recompile donc pas KeoLive.exe, qui garde la même empreinte d'une version de
# l'interface à l'autre. Il n'est reconstruit que si native/ change, et la compilation est
# reproductible (pas d'horodatage) : les mêmes sources redonnent exactement le même fichier.
#
# Dépendances (Linux) : python3, MinGW-w64 (paquet g++-mingw-w64-x86-64-win32),
# la bibliothèque webview 0.12 (https://github.com/webview/webview) et le SDK
# WebView2 (paquet NuGet Microsoft.Web.WebView2), chemins ci-dessous.

TOOLCHAIN ?= ../../toolchain
WEBVIEW   ?= $(TOOLCHAIN)/webview-src/webview
WV2SDK    ?= $(TOOLCHAIN)/wv2/sdk/build/native/include
MINGW     ?= x86_64-w64-mingw32-

# Date inscrite dans KeoLive.exe : fixe (celle du lanceur 2.0.0), pour une compilation reproductible.
# À changer seulement quand le lanceur lui-même change.
export SOURCE_DATE_EPOCH ?= 1794398400

# Compilateur « win32 » (threads natifs de Windows) plutôt que « posix » : il n'embarque pas
# winpthreads, dont les appels (SuspendThread, Get/SetThreadContext, OpenProcess) ressemblent
# pour les antivirus à ceux d'un programme qui s'injecte dans d'autres.

APP := app/index.html app/style.css app/app.js app/data.js app/icon.png app/keolis.png $(wildcard app/fonts/*.woff2)

all: dist/KeoLive.zip
html: dist/KeoLive.html

dist/KeoLive.html: $(APP) tools/bundle.py
	python3 tools/bundle.py app $@

build/metro_res.o: native/metro.rc native/metro.ico native/metro.manifest
	@mkdir -p build
	cd native && $(MINGW)windres -c 65001 metro.rc -O coff -o ../$@

dist/KeoLive.exe: native/main.cpp build/metro_res.o
	@mkdir -p dist
	$(MINGW)g++-win32 -std=c++17 -O2 -Wall -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 \
	  -I$(WEBVIEW)/compatibility/mingw/include -I$(WEBVIEW)/core/include -I$(WV2SDK) \
	  native/main.cpp build/metro_res.o -o $@ -mwindows -static -s \
	  -ladvapi32 -lole32 -loleaut32 -lshell32 -lshlwapi -luser32 -lversion -ldwmapi -lcomctl32 -luuid
	cd dist && sha256sum KeoLive.exe > KeoLive.exe.sha256

dist/KeoLive.zip: dist/KeoLive.exe dist/KeoLive.html
	python3 tools/make_zip.py $@ dist/KeoLive.exe dist/KeoLive.html

clean:
	rm -rf build dist

.PHONY: all html clean
