#include "board.h"

#include <WiFi.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

LGFX lcd;
static bool displayReady = false;

LGFX::LGFX() {
  {
    auto cfg = _bus.config();
    cfg.spi_host = SPI2_HOST;
    cfg.spi_mode = 0;
    cfg.freq_write = 40000000;
    cfg.freq_read = 16000000;
    cfg.spi_3wire = true;
    cfg.use_lock = true;
    cfg.dma_channel = SPI_DMA_CH_AUTO;
    cfg.pin_sclk = PIN_LCD_SCLK;
    cfg.pin_mosi = PIN_LCD_MOSI;
    cfg.pin_miso = -1;
    cfg.pin_dc = PIN_LCD_DC;
    _bus.config(cfg);
    _panel.setBus(&_bus);
  }
  {
    auto cfg = _panel.config();
    cfg.pin_cs = PIN_LCD_CS;
    cfg.pin_rst = PIN_LCD_RST;
    cfg.pin_busy = -1;
    cfg.memory_width = 240;
    cfg.memory_height = 320;
    cfg.panel_width = LCD_PANEL_W;
    cfg.panel_height = LCD_PANEL_H;
    cfg.offset_x = LCD_OFFSET_X;
    cfg.offset_y = 0;
    cfg.offset_rotation = 0;
    cfg.readable = false;
    cfg.invert = true;      // dalle IPS : couleurs inversées
    cfg.rgb_order = false;
    cfg.dlen_16bit = false;
    cfg.bus_shared = false;
    _panel.config(cfg);
  }
  {
    auto cfg = _light.config();
    cfg.pin_bl = PIN_LCD_BL;
    cfg.invert = true;  // rétroéclairage actif à l'état bas
    cfg.freq = 12000;
    cfg.pwm_channel = 7;
    _light.config(cfg);
    _panel.setLight(&_light);
  }
  {
    auto cfg = _touch.config();
    cfg.x_min = 0;
    cfg.x_max = LCD_PANEL_W - 1;
    cfg.y_min = 0;
    cfg.y_max = LCD_PANEL_H - 1;
    cfg.pin_int = PIN_TP_INT;
    cfg.pin_rst = PIN_TP_RST;
    cfg.bus_shared = false;
    cfg.offset_rotation = 0;
    cfg.i2c_port = I2C_PORT;
    cfg.i2c_addr = 0x15;
    cfg.pin_sda = PIN_I2C_SDA;
    cfg.pin_scl = PIN_I2C_SCL;
    cfg.freq = 400000;
    _touch.config(cfg);
    _panel.setTouch(&_touch);
  }
  setPanel(&_panel);
}

// ----------------------------------------------------------------------------
void boardEarlyInit() {
  // Le rétroéclairage a pu être maintenu éteint pendant la veille : on relâche
  // le maintien en gardant le niveau haut (= éteint) pour éviter un flash.
  gpio_set_direction((gpio_num_t)PIN_LCD_BL, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)PIN_LCD_BL, 1);
  gpio_hold_dis((gpio_num_t)PIN_LCD_BL);
  gpio_deep_sleep_hold_dis();
  if (rtc_gpio_is_valid_gpio((gpio_num_t)PIN_BOOT_BTN)) rtc_gpio_deinit((gpio_num_t)PIN_BOOT_BTN);
  pinMode(PIN_BOOT_BTN, INPUT_PULLUP);
  analogReadResolution(12);
}

WakeReason boardWakeReason() {
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT0: return WakeReason::Button;
    case ESP_SLEEP_WAKEUP_EXT1: return WakeReason::Motion;
    case ESP_SLEEP_WAKEUP_TIMER: return WakeReason::Timer;
    case ESP_SLEEP_WAKEUP_UNDEFINED: return WakeReason::PowerOn;
    default: return WakeReason::Other;
  }
}

void boardDisplayInit(bool flip, uint8_t brightness) {
  lcd.init();
  displayReady = true;
  lcd.setRotation(flip ? 3 : 1);  // paysage 320 x 170
  lcd.fillScreen(TFT_BLACK);
  lcd.setBrightness(brightness);
}

bool bootButtonPressed() { return digitalRead(PIN_BOOT_BTN) == LOW; }

// ----------------------------------------------------------------------------
//  Batterie (pont diviseur 1/3 sur GPIO4)
// ----------------------------------------------------------------------------
int batteryMilliVolts() {
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) sum += analogReadMilliVolts(PIN_BAT_ADC);
  return (int)((sum / 8) * BAT_DIVIDER);
}

int batteryPercent(int mv) {
  // Courbe de décharge typique d'un élément LiPo 3,7 V (au repos, approximative)
  static const int16_t curve[][2] = {{4180, 100}, {4100, 92}, {4000, 80}, {3920, 68}, {3860, 56},
                                     {3820, 45}, {3790, 34}, {3760, 24}, {3730, 15}, {3690, 8},
                                     {3600, 3},  {3450, 0}};
  const int n = sizeof(curve) / sizeof(curve[0]);
  if (mv >= curve[0][0]) return 100;
  for (int i = 1; i < n; i++) {
    if (mv >= curve[i][0]) {
      int v0 = curve[i][0], v1 = curve[i - 1][0], p0 = curve[i][1], p1 = curve[i - 1][1];
      return p0 + (mv - v0) * (p1 - p0) / (v1 - v0);
    }
  }
  return 0;
}

// ----------------------------------------------------------------------------
//  IMU QMI8658 : "Wake on Motion"
// ----------------------------------------------------------------------------
static constexpr uint8_t QMI_WHO_AM_I = 0x00, QMI_CTRL1 = 0x02, QMI_CTRL2 = 0x03, QMI_CTRL7 = 0x08,
                         QMI_CTRL8 = 0x09, QMI_CTRL9 = 0x0A, QMI_CAL1_L = 0x0B, QMI_CAL1_H = 0x0C,
                         QMI_STATUSINT = 0x2D, QMI_STATUS1 = 0x2F;

static bool imuWrite(uint8_t reg, uint8_t val) {
  return lgfx::i2c::writeRegister8(I2C_PORT, IMU_ADDR, reg, val, 0, 400000).has_value();
}
static int imuRead(uint8_t reg) {
  auto r = lgfx::i2c::readRegister8(I2C_PORT, IMU_ADDR, reg, 400000);
  return r.has_value() ? r.value() : -1;
}

bool imuPresent() {
  lgfx::i2c::init(I2C_PORT, PIN_I2C_SDA, PIN_I2C_SCL);
  return imuRead(QMI_WHO_AM_I) == 0x05;
}

void imuDisableWakeOnMotion() {
  if (!imuPresent()) return;
  imuWrite(QMI_CTRL7, 0x00);  // capteurs coupés
  imuRead(QMI_STATUS1);       // acquitte une éventuelle interruption
}

static bool imuEnableWakeOnMotion() {
  if (!imuPresent()) return false;
  imuWrite(QMI_CTRL7, 0x00);  // 1. tout désactiver
  imuWrite(QMI_CTRL1, 0x08);  // 2. sortie d'interruption active (push-pull)
  imuWrite(QMI_CTRL8, 0x80);  // 3. poignée de main CTRL9 via STATUSINT.bit7
  imuWrite(QMI_CTRL2, 0x27);  // 4. accéléromètre ±8 g, basse consommation
  imuWrite(QMI_CAL1_L, 0xC0); // 5. seuil ~192 mg : il faut vraiment secouer
  imuWrite(QMI_CAL1_H, 0x20); // 6. niveau initial bas, filtrage ~250 ms
  imuWrite(QMI_CTRL9, 0x08);  // 7. commande "configurer Wake on Motion"
  bool done = false;
  for (int i = 0; i < 50 && !done; i++) {
    int st = imuRead(QMI_STATUSINT);
    done = st >= 0 && (st & 0x80);
    if (!done) delay(2);
  }
  imuWrite(QMI_CTRL9, 0x00);  // 8. acquittement
  imuWrite(QMI_CTRL7, 0x01);  // 9. accéléromètre en marche
  delay(5);
  imuRead(QMI_STATUS1);       // 10. on efface l'interruption en attente
  return done;
}

// ----------------------------------------------------------------------------
//  Veille profonde
// ----------------------------------------------------------------------------
void boardDeepSleep(bool motion, uint32_t timerS) {
  if (displayReady) {
    lcd.sleep();  // rétroéclairage à 0 + écran en sommeil (SLPIN)
    delay(10);
  }

  // Maintient le rétroéclairage éteint (niveau haut) pendant la veille
  gpio_reset_pin((gpio_num_t)PIN_LCD_BL);
  gpio_set_direction((gpio_num_t)PIN_LCD_BL, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)PIN_LCD_BL, 1);
  gpio_hold_en((gpio_num_t)PIN_LCD_BL);
  gpio_deep_sleep_hold_en();

  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_OFF);

  // Réveil par le bouton BOOT (niveau bas)
  rtc_gpio_pullup_en((gpio_num_t)PIN_BOOT_BTN);
  rtc_gpio_pulldown_dis((gpio_num_t)PIN_BOOT_BTN);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BOOT_BTN, 0);

  // Réveil par secousse : la sortie d'interruption de l'IMU passe à l'état haut.
  // Selon le câblage, elle arrive sur GPIO8 ou GPIO7 : on surveille les deux,
  // sauf une broche déjà haute (qui réveillerait la carte tout de suite).
  if (motion && imuEnableWakeOnMotion()) {
    uint64_t mask = 0;
    for (int pin : {PIN_IMU_INT1, 7}) {
      pinMode(pin, INPUT_PULLDOWN);
      delay(2);
      if (digitalRead(pin) == LOW) mask |= (1ULL << pin);
    }
    if (mask) {
      for (int pin : {PIN_IMU_INT1, 7}) {
        if (!(mask & (1ULL << pin))) continue;
        rtc_gpio_pulldown_en((gpio_num_t)pin);
        rtc_gpio_pullup_dis((gpio_num_t)pin);
      }
      esp_sleep_enable_ext1_wakeup_io(mask, ESP_EXT1_WAKEUP_ANY_HIGH);
    }
  } else {
    imuDisableWakeOnMotion();
  }

  if (timerS) esp_sleep_enable_timer_wakeup((uint64_t)timerS * 1000000ULL);

  // On attend que le bouton soit relâché (sinon réveil immédiat)
  uint32_t t0 = millis();
  while (bootButtonPressed() && millis() - t0 < 5000) delay(10);
  Serial.flush();
  esp_deep_sleep_start();
}
