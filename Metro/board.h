// Tout ce qui touche au matériel de la carte Waveshare ESP32-S3-Touch-LCD-1.9 :
// écran + tactile (LovyanGFX), rétroéclairage, batterie, IMU, veille profonde.
#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>

#include "config.h"

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Light_PWM _light;
  lgfx::Touch_CST816S _touch;

 public:
  LGFX();
};

extern LGFX lcd;

enum class WakeReason : uint8_t { PowerOn, Button, Motion, Timer, Other };

void boardEarlyInit();          // à appeler en tout premier dans setup()
WakeReason boardWakeReason();
void boardDisplayInit(bool flip, uint8_t brightness);

int batteryMilliVolts();        // tension de la batterie (mV), moyenne de 8 mesures
int batteryPercent(int mv);     // estimation 0..100 (courbe LiPo)

bool bootButtonPressed();

// Réveil par secousse (IMU QMI8658 en mode "Wake on Motion")
bool imuPresent();
void imuDisableWakeOnMotion();

// Coupe l'écran et passe en veille profonde. Réveil : bouton BOOT, et
// secousse si motion = true, et minuterie si timerS > 0.
[[noreturn]] void boardDeepSleep(bool motion, uint32_t timerS);
