// types.h — Structures de donnees et enumerations
// Porte depuis struct.h (POC_ATSAMD) — 100% portable
// NE PAS MODIFIER ConfigGenerale_t sans incrementer CONFIG_VERSION

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

// ===== ENUMERATIONS =====

typedef enum
{
  KEY_NONE = 0,
  KEY_1 = 1,
  KEY_2 = 2,
  KEY_3 = 3,
  KEY_4 = 4,
  KEY_5 = 5,
  KEY_INVALID = 255
} key_code_t;

typedef enum { LIST_INPUT_IDLE, LIST_INPUT_ACTIVE, LIST_INPUT_COMPLETED, LIST_INPUT_CANCELLED } listInputState_t;
typedef enum { NUM_INPUT_IDLE, NUM_INPUT_ACTIVE, NUM_INPUT_COMPLETED, NUM_INPUT_CANCELLED } numInputState_t;
typedef enum { STRING_INPUT_IDLE, STRING_INPUT_ACTIVE, STRING_INPUT_COMPLETED, STRING_INPUT_CANCELLED } stringInputState_t;
typedef enum { HEX_INPUT_IDLE, HEX_INPUT_ACTIVE, HEX_INPUT_COMPLETED, HEX_INPUT_CANCELLED } hexInputState_t;
typedef enum { TIME_INPUT_IDLE, TIME_INPUT_ACTIVE, TIME_INPUT_COMPLETED, TIME_INPUT_CANCELLED } timeInputState_t;
typedef enum { DATE_INPUT_IDLE, DATE_INPUT_ACTIVE, DATE_INPUT_COMPLETED, DATE_INPUT_CANCELLED } dateInputState_t;
typedef enum { EMAIL_INPUT_IDLE, EMAIL_INPUT_ACTIVE, EMAIL_INPUT_COMPLETED, EMAIL_INPUT_CANCELLED } emailInputState_t;
typedef enum { IP_INPUT_IDLE, IP_INPUT_ACTIVE, IP_INPUT_COMPLETED, IP_INPUT_CANCELLED } ipInputState_t;
typedef enum { INFO_SCREEN_IDLE, INFO_SCREEN_ACTIVE, INFO_SCREEN_CLOSED } infoScreenState_t;

// ===== STRUCTURES CLAVIER =====

typedef struct {
  key_code_t derniereTouche;
  key_code_t toucheStable;
  int compteStable;
  unsigned long derniereLecture;
  bool toucheDisponible;
} clavier_context_t;

// ===== STRUCTURES SAISIES =====

typedef struct
{
  listInputState_t state;
  uint8_t selectedIndex;
  uint8_t scrollOffset;
  uint8_t maxItems;
  uint8_t lastScrollOffset;
  uint8_t lastSelectedIndex;
  bool lastCursorBlink;
  bool displayRefresh;
  unsigned long lastUpdate;
  bool cursorBlink;
  unsigned long lastBlink;
  unsigned long lastActivity;
  unsigned long timeoutDuration;
  char title[21];
  const char** itemList;
} listInputContext_t;

typedef struct
{
  const char** menuList;
  uint8_t menuSize;
  uint8_t selectedIndex;
  char title[21];
} menuLevel_t;

typedef struct
{
  numInputState_t state;
  uint8_t position;
  uint8_t lastPosition;
  uint8_t length;
  uint8_t lastLength;
  uint8_t maxLength;
  char workingNum[21];
  char lastDisplayedNum[17];
  bool displayRefresh;
  unsigned long lastUpdate;
  bool cursorBlink;
  bool lastCursorBlink;
  unsigned long lastBlink;
  uint8_t displayOffset;
  uint8_t lastDisplayOffset;
  uint8_t lastCursorOffset;
  uint8_t displayWidth;
  unsigned long lastActivity;
  unsigned long timeoutDuration;
  bool allowNegative;
  bool allowDecimal;
  long minValue;
  long maxValue;
  bool firstDisplay;
  uint8_t lastTimeoutValue;
  char title[21];
} numInputContext_t;

typedef struct
{
  stringInputState_t state;
  uint8_t position;
  uint8_t lastPosition;
  uint8_t maxLength;
  char workingString[21];
  char lastDisplayedString[21];
  bool displayRefresh;
  unsigned long lastUpdate;
  bool cursorBlink;
  bool lastCursorBlink;
  unsigned long lastBlink;
  unsigned long lastActivity;
  unsigned long timeoutDuration;
  bool firstDisplay;
  uint8_t lastTimeoutValue;
  char title[21];
} stringInputContext_t;

typedef struct
{
  hexInputState_t state;
  uint8_t position;
  uint8_t lastPosition;
  uint8_t maxLength;
  char workingHex[41];
  char lastDisplayedHex[17];
  char title[21];
  bool displayRefresh;
  unsigned long lastUpdate;
  bool cursorBlink;
  bool lastCursorBlink;
  unsigned long lastBlink;
  uint8_t displayOffset;
  uint8_t lastDisplayOffset;
  uint8_t lastCursorOffset;
  uint8_t displayWidth;
  unsigned long lastActivity;
  unsigned long timeoutDuration;
  bool lastValidity;
  bool firstDisplay;
  uint8_t lastTimeoutValue;
} hexInputContext_t;

typedef struct
{
  timeInputState_t state;
  uint8_t position;
  uint8_t lastPosition;
  char workingTime[9];
  char lastDisplayedTime[9];
  bool displayRefresh;
  unsigned long lastUpdate;
  bool cursorBlink;
  bool lastCursorBlink;
  unsigned long lastBlink;
  unsigned long lastActivity;
  unsigned long timeoutDuration;
  bool lastValidity;
  bool firstDisplay;
  char title[21];
} timeInputContext_t;

typedef struct
{
  dateInputState_t state;
  uint8_t position;
  uint8_t lastPosition;
  char workingDate[11];
  char lastDisplayedDate[11];
  bool displayRefresh;
  unsigned long lastUpdate;
  bool cursorBlink;
  bool lastCursorBlink;
  unsigned long lastBlink;
  unsigned long lastActivity;
  unsigned long timeoutDuration;
  bool lastValidity;
  bool firstDisplay;
  char title[21];
} dateInputContext_t;

typedef struct
{
  emailInputState_t state;
  uint8_t position;
  uint8_t lastPosition;
  uint8_t length;
  uint8_t lastLength;
  char workingEmail[41];
  char lastDisplayedEmail[17];
  bool displayRefresh;
  unsigned long lastUpdate;
  bool cursorBlink;
  bool lastCursorBlink;
  unsigned long lastBlink;
  uint8_t displayOffset;
  uint8_t lastDisplayOffset;
  uint8_t lastCursorOffset;
  uint8_t displayWidth;
  unsigned long lastActivity;
  unsigned long timeoutDuration;
  bool lastValidity;
  uint8_t lastValidityState;
  bool firstDisplay;
  uint8_t charSetIndex;
  uint8_t lastTimeoutValue;
  char title[21];
} emailInputContext_t;

typedef struct
{
  ipInputState_t state;
  uint8_t position;
  uint8_t lastPosition;
  char workingIP[16];
  char lastDisplayedIP[16];
  bool displayRefresh;
  unsigned long lastUpdate;
  bool cursorBlink;
  bool lastCursorBlink;
  unsigned long lastBlink;
  unsigned long lastActivity;
  unsigned long timeoutDuration;
  bool lastValidity;
  bool firstDisplay;
  char title[21];
} ipInputContext_t;

// ===== STRUCTURES DE DONNEES CAPTEURS =====

typedef struct
{
  float DHT_Temp;          // Temperature en degC (BME280 sur ESP32)
  float DHT_Hum;           // Humidite en %
  float Brightness;        // Luminosite en lux (BH1750 sur ESP32)
  float Bat_Voltage;       // Tension batterie en V
  float Solar_Voltage;     // Tension panneau solaire en V
  float HX711Weight[4];    // Masse 4 pesons en g (1 seul utilise par noeud sur ESP32)
  float ProcessorTemp;     // Temperature processeur
  float Pressure;          // Pression atmospherique hPa (BME280, nouveau)
  float SolarCurrent;      // Courant solaire mA (INA219, nouveau)
} HiveSensor_Data_t;

// ===== STRUCTURES DE CONFIGURATION PERSISTANTE (EEPROM AT24C32) =====
// ATTENTION : ne JAMAIS modifier sans incrementer CONFIG_VERSION

typedef struct __attribute__((packed))
{
  uint16_t version;
  uint8_t adresseRTC;
  uint8_t adresseOLED;
  uint8_t adresseEEPROM;
  uint16_t poidsTare;
  uint8_t Num_Carte;
  uint8_t DevEUI[9];
  uint8_t Rtc;
  uint8_t KBD_Ana;
  uint8_t Oled;
  uint8_t SDHC;
  uint8_t LiPo;
  uint8_t Solaire;
  // HX711#0
  uint8_t Peson_0;
  uint8_t HX711Clk_0;
  uint8_t HX711Dta_0;
  float   HX711NoloadValue_0;
  float   HX711Tare_Temp_0;
  float   HX711Scaling_0;
  float   HX711Cor_Temp_0;
  // HX711#1
  uint8_t Peson_1;
  uint8_t HX711Clk_1;
  uint8_t HX711Dta_1;
  float   HX711NoloadValue_1;
  float   HX711Tare_Temp_1;
  float   HX711Scaling_1;
  float   HX711Cor_Temp_1;
  // HX711#2
  uint8_t Peson_2;
  uint8_t HX711Clk_2;
  uint8_t HX711Dta_2;
  float   HX711NoloadValue_2;
  float   HX711Tare_Temp_2;
  float   HX711Scaling_2;
  float   HX711Cor_Temp_2;
  // HX711#3
  uint8_t Peson_3;
  uint8_t HX711Clk_3;
  uint8_t HX711Dta_3;
  float   HX711NoloadValue_3;
  float   HX711Tare_Temp_3;
  float   HX711Scaling_3;
  float   HX711Cor_Temp_3;
  // Calibration analogique
  float   LDRBrightnessScale;
  float   VSolScale;
  float   VBatScale;
} ConfigMateriel_t;

typedef struct __attribute__((packed))
{
  uint16_t version;
  uint16_t redLedDuration;
  uint16_t greenLedDuration;
  uint16_t blueLedDuration;
  uint16_t builtinLedDuration;
  uint8_t RucherID;
  char    RucherName[21];
  uint8_t AppEUI[9];
  uint8_t AppKey[17];
  uint8_t SpreadingFactor;
  uint16_t SendingPeriod;
  uint16_t OLEDRefreshPeriod;
} ConfigApplicatif_t;

typedef struct __attribute__((packed))
{
  uint16_t magicNumber;
  ConfigApplicatif_t applicatif;
  ConfigMateriel_t materiel;
  uint16_t checksum;
} ConfigGenerale_t;

// ===== STRUCTURES SPECIFIQUES ESP32 (nouvelles) =====

// Donnees lues depuis un slave BLE
typedef struct
{
  char address[18];      // Adresse MAC BLE "aa:bb:cc:dd:ee:ff"
  int16_t weight;        // Poids en g (x 0.01 kg)
  uint8_t vbat;          // Tension batterie (x 0.1V, offset 2.0V)
  uint32_t timestamp;    // Epoch Unix (depuis DS3231)
  bool valid;            // Lecture reussie
} SlaveReading_t;

#endif // TYPES_H
