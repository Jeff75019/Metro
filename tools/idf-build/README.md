# Build ESP-IDF (vérification)

Configuration utilisée pour compiler le firmware hors de l'IDE Arduino :
ESP-IDF 5.5.6, cœur Arduino ESP32 3.3.12 en composant IDF, LovyanGFX 1.2.36,
ArduinoJson 7.4.3. Elle produit aussi une image unique `metro-merged.bin`
à flasher à l'adresse 0x0.

Pour un usage normal, l'IDE Arduino ou PlatformIO suffisent (voir le README principal).

Notes :
- `ARDUINO_USB_CDC_ON_BOOT=1` / `ARDUINO_USB_MODE=1` : le port série passe par l'USB-C.
- `CONFIG_MBEDTLS_PSK_MODES` est requis par `NetworkClientSecure` (comme dans le cœur précompilé).
- `AsyncUDP` est requis par `DNSServer`.
- Sans le gestionnaire de composants (`IDF_COMPONENT_MANAGER=0`), il faut ajouter `mdns`
  aux dépendances du composant `arduino` et fournir `components/mdns`.
