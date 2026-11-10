# Métro pour Windows
#   make        : dist/Metro.html (application en un seul fichier) et dist/Metro.exe
#   make html   : dist/Metro.html seulement
#
# Dépendances (Linux) : python3, MinGW-w64 (paquet g++-mingw-w64-x86-64-posix),
# la bibliothèque webview 0.12 (https://github.com/webview/webview) et le SDK
# WebView2 (paquet NuGet Microsoft.Web.WebView2), chemins ci-dessous.

TOOLCHAIN ?= ../../toolchain
WEBVIEW   ?= $(TOOLCHAIN)/webview-src/webview
WV2SDK    ?= $(TOOLCHAIN)/wv2/sdk/build/native/include
MINGW     ?= x86_64-w64-mingw32-

APP := app/index.html app/style.css app/app.js app/data.js app/icon.png $(wildcard app/fonts/*.woff2)

all: dist/Metro.exe
html: dist/Metro.html

dist/Metro.html: $(APP) tools/bundle.py
	python3 tools/bundle.py app $@

build/metro_res.o: native/metro.rc native/metro.ico native/metro.manifest dist/Metro.html
	@mkdir -p build
	cd native && $(MINGW)windres metro.rc -O coff -o ../$@

dist/Metro.exe: native/main.cpp build/metro_res.o
	$(MINGW)g++-posix -std=c++17 -O2 -Wall -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 \
	  -I$(WEBVIEW)/compatibility/mingw/include -I$(WEBVIEW)/core/include -I$(WV2SDK) \
	  native/main.cpp build/metro_res.o -o $@ -mwindows -static \
	  -ladvapi32 -lole32 -loleaut32 -lshell32 -lshlwapi -luser32 -lversion -ldwmapi -lcomctl32
	$(MINGW)strip $@

clean:
	rm -rf build dist

.PHONY: all html clean
