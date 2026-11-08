#!/bin/bash
# Compilation de vérification avec ESP-IDF (Arduino utilisé comme composant IDF).
# Prérequis : ESP-IDF 5.5 installé et activé (. $IDF_PATH/export.sh), puis dans
# components/ : arduino (arduino-esp32 3.3.12), LovyanGFX (1.2.36), ArduinoJson (7.4.x)
# et mdns (esp-protocols), ou bien laisser le gestionnaire de composants les récupérer.
set -e
cd "$(dirname "$0")"
rm -f main/*.cpp main/*.h
cp ../../Metro/*.cpp ../../Metro/*.h main/
cp ../../Metro/Metro.ino main/Metro.cpp
idf.py set-target esp32s3 build
cd build && python -m esptool --chip esp32s3 merge_bin -o metro-merged.bin @flash_args
echo "Image complète : $(pwd)/metro-merged.bin (à flasher en 0x0)"
