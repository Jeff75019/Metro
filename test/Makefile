# Tests sur PC (Linux/macOS) — aucun matériel nécessaire.
#
#   make test   : tests du décodage des réponses PRIM (siri.cpp) et SNCF (sncf.cpp),
#                 et du réseau embarqué (network.cpp)
#   make ui     : rendu des écrans en images (out/*.ppm puis out/*.png)
#
# Dépendances : g++, et les sources de ArduinoJson et LovyanGFX
# (chemins à adapter ci-dessous ou en ligne de commande :
#   make ARDUINOJSON=~/Arduino/libraries/ArduinoJson LOVYANGFX=~/Arduino/libraries/LovyanGFX ui)

ARDUINOJSON ?= ../../toolchain/ArduinoJson
LOVYANGFX   ?= ../../toolchain/LovyanGFX
BUILD       ?= build

CXX      ?= g++
CXXFLAGS ?= -std=gnu++17 -O1 -g -Wall -Wno-maybe-uninitialized
SAN      ?= -fsanitize=address,undefined

LGFX_SRC := $(wildcard $(LOVYANGFX)/src/lgfx/v1/*.cpp) \
            $(wildcard $(LOVYANGFX)/src/lgfx/v1/misc/*.cpp) \
            $(wildcard $(LOVYANGFX)/src/lgfx/v1/panel/*.cpp) \
            $(wildcard $(LOVYANGFX)/src/lgfx/v1/touch/*.cpp) \
            $(wildcard $(LOVYANGFX)/src/lgfx/v1/platforms/framebuffer/*.cpp)
LGFX_C   := $(wildcard $(LOVYANGFX)/src/lgfx/utility/*.c) \
            $(wildcard $(LOVYANGFX)/src/lgfx/Fonts/efont/*.c) \
            $(wildcard $(LOVYANGFX)/src/lgfx/Fonts/IPA/*.c) \
            $(wildcard $(LOVYANGFX)/src/lgfx/Fonts/lvgl/*.c) \
            $(wildcard $(LOVYANGFX)/src/lgfx/v1/lv_font/*.c)
LGFX_OBJ := $(patsubst $(LOVYANGFX)/%,$(BUILD)/lgfx/%.o,$(LGFX_SRC) $(LGFX_C))

.PHONY: test ui clean

test: $(BUILD)/test_siri $(BUILD)/test_sncf $(BUILD)/test_network
	$(BUILD)/test_siri
	$(BUILD)/test_sncf
	$(BUILD)/test_network

$(BUILD)/test_network: test_network.cpp ../Metro/network.cpp ../Metro/network_data.h ../Metro/sncf_list_data.h ../Metro/siri.cpp ../Metro/lines.cpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(SAN) -I$(ARDUINOJSON)/src test_network.cpp ../Metro/network.cpp ../Metro/siri.cpp ../Metro/lines.cpp -o $@

$(BUILD)/test_sncf: test_sncf.cpp ../Metro/sncf.cpp ../Metro/siri.cpp ../Metro/lines.cpp ../Metro/sncf.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(SAN) -I$(ARDUINOJSON)/src test_sncf.cpp ../Metro/sncf.cpp ../Metro/siri.cpp ../Metro/lines.cpp -o $@

$(BUILD)/test_siri: test_siri.cpp ../Metro/siri.cpp ../Metro/lines.cpp ../Metro/siri.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(SAN) -I$(ARDUINOJSON)/src test_siri.cpp ../Metro/siri.cpp ../Metro/lines.cpp -o $@

$(BUILD)/lgfx/%.cpp.o: $(LOVYANGFX)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -std=gnu++17 -O1 -w -DLGFX_LINUX_FB -I$(LOVYANGFX)/src -c $< -o $@

$(BUILD)/lgfx/%.c.o: $(LOVYANGFX)/%.c
	@mkdir -p $(dir $@)
	$(CC) -O1 -w -DLGFX_LINUX_FB -I$(LOVYANGFX)/src -c $< -o $@

$(BUILD)/test_ui: test_ui.cpp ../Metro/ui.cpp ../Metro/siri.cpp ../Metro/sncf.cpp ../Metro/lines.cpp ../Metro/network.cpp ../Metro/fonts_metro.h $(LGFX_OBJ)
	$(CXX) $(CXXFLAGS) -DLGFX_LINUX_FB -I$(ARDUINOJSON)/src -I$(LOVYANGFX)/src \
	  test_ui.cpp ../Metro/ui.cpp ../Metro/siri.cpp ../Metro/sncf.cpp ../Metro/lines.cpp ../Metro/network.cpp $(LGFX_OBJ) -lpthread -o $@

ui: $(BUILD)/test_ui
	$(BUILD)/test_ui
	@python3 -c "import glob; from PIL import Image; [Image.open(p).resize((640,340), Image.NEAREST).save(p[:-4]+'.png') for p in glob.glob('out/*.ppm')]" \
	  && echo "PNG agrandis x2 dans out/" || echo "(installez Pillow pour obtenir des PNG)"

clean:
	rm -rf $(BUILD) out
