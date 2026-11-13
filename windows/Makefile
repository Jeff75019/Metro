# KeoLive pour Windows
#   make        : dist/KeoLive.exe (l'application complète, en un seul fichier) et dist/KeoLive.html
#   make html   : dist/KeoLive.html seulement (la même application, qui s'ouvre aussi dans Edge)
#
# Dépendances (Linux) : python3, MinGW-w64 en version « win32 » (paquet g++-mingw-w64-x86-64-win32),
# lld, l'éditeur de liens de LLVM (paquet lld), la bibliothèque webview 0.12
# (https://github.com/webview/webview) et le SDK WebView2 (paquet NuGet Microsoft.Web.WebView2).
#
# Choix de fabrication, pour un exécutable de forme standard :
#   - GCC « win32 » (threads natifs de Windows) plutôt que « posix » : il n'embarque pas winpthreads,
#     dont les appels (SuspendThread, Get/SetThreadContext, OpenProcess) ressemblent pour les
#     antivirus à ceux d'un programme qui s'injecte dans d'autres ;
#   - édition des liens par lld (celui de Chrome, Firefox ou Rust) : en-tête et sections comme les
#     programmes Windows courants (Windows 6.0 minimum, sections .text/.rdata/.data, compatible
#     Terminal Server), au lieu de la disposition propre à l'éditeur de liens GNU ;
#   - la page incluse comme une donnée en lecture seule (native/page.S), pas comme une ressource ;
#   - compilation reproductible : date fixe (SOURCE_DATE_EPOCH), les mêmes sources redonnent
#     exactement le même fichier.

TOOLCHAIN ?= ../../toolchain
WEBVIEW   ?= $(TOOLCHAIN)/webview-src/webview
WV2SDK    ?= $(TOOLCHAIN)/wv2/sdk/build/native/include
MINGW     ?= x86_64-w64-mingw32-
CXX       := $(MINGW)g++-win32
LLD       ?= ld.lld

# Date inscrite dans KeoLive.exe (13/11/2026 12:00 UTC) : à avancer à chaque nouvelle version.
export SOURCE_DATE_EPOCH ?= 1794571200

GCCLIB   := $(patsubst %/,%,$(dir $(shell $(CXX) -print-libgcc-file-name)))
MINGWLIB := $(GCCLIB)/../../../../x86_64-w64-mingw32/lib
LIBS     := -ladvapi32 -lole32 -loleaut32 -lshell32 -lshlwapi -luser32 -lversion -ldwmapi -lcomctl32 -luuid
# Bibliothèques d'exécution, dans l'ordre où GCC les passe lui-même à l'éditeur de liens
RUNTIME  := -lstdc++ -lmingw32 -lgcc -lgcc_eh -lmoldname -lmingwex -lmsvcrt -lkernel32 -lgdi32 -lcomdlg32 \
            -ladvapi32 -lshell32 -luser32 -lkernel32 -lmingw32 -lgcc -lgcc_eh -lmoldname -lmingwex -lmsvcrt -lkernel32

APP := app/index.html app/style.css app/app.js app/plan.js app/data.js app/favicon.png app/keolis.png $(wildcard app/fonts/*.woff2)

all: dist/KeoLive.exe
html: dist/KeoLive.html

dist/KeoLive.html: $(APP) tools/bundle.py
	python3 tools/bundle.py app $@

build/main.o: native/main.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 \
	  -I$(WEBVIEW)/compatibility/mingw/include -I$(WEBVIEW)/core/include -I$(WV2SDK) -c $< -o $@

build/page.o: native/page.S dist/KeoLive.html
	@mkdir -p build
	$(CXX) -c native/page.S -o $@

build/metro_res.o: native/metro.rc native/metro.ico native/metro.manifest
	@mkdir -p build
	cd native && $(MINGW)windres -c 65001 metro.rc -O coff -o ../$@

dist/KeoLive.exe: build/main.o build/page.o build/metro_res.o
	@mkdir -p dist
	$(LLD) -m i386pep --subsystem windows -Bstatic -s -o $@ \
	  $(MINGWLIB)/crt2.o $(GCCLIB)/crtbegin.o -L$(GCCLIB) -L$(MINGWLIB) \
	  build/main.o build/page.o build/metro_res.o $(LIBS) $(RUNTIME) $(GCCLIB)/crtend.o
	cd dist && sha256sum KeoLive.exe > KeoLive.exe.sha256

clean:
	rm -rf build dist/KeoLive.exe dist/KeoLive.html dist/KeoLive.exe.sha256

.PHONY: all html clean
