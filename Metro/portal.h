// Mode configuration : point d'accès WiFi "Metro-XXXX" + portail captif +
// page web pour régler le WiFi, la clé PRIM et les arrêts.
#pragma once
#include <Arduino.h>

void portalStart();
void portalLoop();       // à appeler en boucle tant que le portail est actif
bool portalActive();
const char* portalApName();
