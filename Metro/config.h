// =============================================================================
//  Afficheur Métro — configuration matérielle et valeurs par défaut
//  Carte : Waveshare ESP32-S3-Touch-LCD-1.9 (SKU 30939)
//          ESP32-S3R8 (8 Mo PSRAM octale), 16 Mo flash
//          LCD 1,9" ST7789V2 170x320 (SPI), tactile CST816S (I2C)
//          IMU QMI8658 (I2C), chargeur LiPo intégré
// =============================================================================
#pragma once

#define FW_VERSION "1.2.2"

// ---- Écran ST7789V2 (SPI) ---------------------------------------------------
#define PIN_LCD_SCLK 10
#define PIN_LCD_MOSI 13
#define PIN_LCD_CS 12
#define PIN_LCD_DC 11
#define PIN_LCD_RST 9
#define PIN_LCD_BL 14  // rétroéclairage, ACTIF À L'ÉTAT BAS (0 = allumé)

#define LCD_PANEL_W 170  // largeur physique (portrait)
#define LCD_PANEL_H 320
#define LCD_OFFSET_X 35  // la dalle 170 px est centrée dans une RAM de 240 px

// ---- Bus I2C partagé : tactile CST816S (0x15) + IMU QMI8658 (0x6B) ----------
#define PIN_I2C_SDA 47
#define PIN_I2C_SCL 48
#define I2C_PORT 0
#define PIN_TP_INT 21
#define PIN_TP_RST 17
#define PIN_IMU_INT1 8  // broche RTC : sert au réveil "secousse"
#define IMU_ADDR 0x6B

// ---- Divers ------------------------------------------------------------------
#define PIN_BOOT_BTN 0  // bouton BOOT, actif bas, sert aussi au réveil
#define PIN_BAT_ADC 4   // tension batterie via pont diviseur 1/3 (ADC1_CH3)
#define BAT_DIVIDER 3.0f

// ---- Comportement par défaut (modifiable depuis la page de configuration) ---
#define DEFAULT_REFRESH_S 30     // intervalle entre deux appels à l'API
#define DEFAULT_AWAKE_S 60       // durée d'affichage avant mise en veille
#define DEFAULT_DAILY_QUOTA 1000 // quota PRIM par défaut d'un compte gratuit
#define DEFAULT_BRIGHTNESS 180   // 0..255

#define MAX_FAVORITES 4
#define MAX_STATIONS 10           // gares SNCF suivies
#define DEFAULT_SNCF_QUOTA 5000   // quota gratuit de l'API SNCF (requêtes par jour)
#define TRAINS_REFRESH_MIN_S 60   // les tableaux SNCF changent moins vite
#define MAX_WIFI 3
#define CONFIG_AP_PREFIX "Metro-"
#define MDNS_NAME "metro"  // http://metro.local en mode configuration

// Fuseau horaire de Paris (heure d'été automatique)
#define TZ_PARIS "CET-1CEST,M3.5.0,M10.5.0/3"
