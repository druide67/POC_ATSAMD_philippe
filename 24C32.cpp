//       1         2         3         4         5         6         7        7
//34567890123456789012345678901234567890123456789012345678901234567890123456789
// IMPRESSION 79 COLONES EN TAILLE 12
//
// ---------------------------------------------------------------------------*
//      ___  _  _    _____ ____ ___                   
//     |__ \| || |  / ____|___ \__ \                  
//        ) | || |_| |      __) | ) |  ___ _ __  _ __ 
//       / /|__   _| |     |__ < / /  / __| '_ \| '_ \
//      / /_   | | | |____ ___) / /_ | (__| |_) | |_) |
//     |____|  |_|  \_____|____/____(_)___| .__/| .__/
//                                        | |   | |   
//                                        |_|   |_|    
// ---------------------------------------------------------------------------*
#define __INIT_DONE
#include "define.h"

// ===========================================================================
// PROJET: Gestion Configuration EEPROM - Balance Connectée
// ===========================================================================
// Board: SODAQ EXPLORER (ATSAMD21J18)
// RTC: DS3231 avec EEPROM AT24C32
// Écran: OLED SH1106 128x64
// LoRa: RN2483
// ===========================================================================



// ---------------------------------------------------------------------------
// ===== FONCTIONS DE GESTION EEPROM BAS NIVEAU =====
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// @brief Écriture d'un octet en EEPROM AT24C32
// @param address Adresse mémoire (0x0000 à 0x0FFF pour 4Ko)
// @param data Octet à écrire
// @return void
// ---------------------------------------------------------------------------
// exemple appel: EPR_24C32writeByte(0x0000, 0xAA);
void EPR_24C32writeByte(uint16_t address, uint8_t data)
{
  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write((uint8_t)(address >> 8));    // MSB de l'adresse
  Wire.write((uint8_t)(address & 0xFF));  // LSB de l'adresse
  Wire.write(data);
  Wire.endTransmission();
  delay(5);  // Délai d'écriture EEPROM (tW = 5ms max pour AT24C32)
}

// ---------------------------------------------------------------------------
// @brief Lecture d'un octet depuis l'EEPROM AT24C32
// @param address Adresse mémoire (0x0000 à 0x0FFF pour 4Ko)
// @return Octet lu depuis l'EEPROM
// ---------------------------------------------------------------------------
// exemple appel: uint8_t val = EPR_24C32readByte(0x0000);
uint8_t EPR_24C32readByte(uint16_t address)
{
  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write((uint8_t)(address >> 8));    // MSB de l'adresse
  Wire.write((uint8_t)(address & 0xFF));  // LSB de l'adresse
  Wire.endTransmission();
  
  Wire.requestFrom(EEPROM_ADDRESS, 1);
  if (Wire.available())
  {
    return Wire.read();
  }
  return 0xFF;  // Valeur par défaut si erreur
}

// ---------------------------------------------------------------------------
// @brief Écriture d'un bloc de données en EEPROM AT24C32
// @param address Adresse de départ
// @param data Pointeur vers les données à écrire
// @param length Nombre d'octets à écrire
// @return void
// ---------------------------------------------------------------------------
// exemple appel: EPR_24C32writeBlock(0x0000, (uint8_t*)&config, sizeof(config));
void EPR_24C32writeBlock(uint16_t address, uint8_t* data, uint16_t length)
{
  for (uint16_t i = 0; i < length; i++)
  {
    EPR_24C32writeByte(address + i, data[i]);
  }
}

// ---------------------------------------------------------------------------
// @brief Lecture d'un bloc de données depuis l'EEPROM AT24C32
// @param address Adresse de départ
// @param data Pointeur vers le buffer de réception
// @param length Nombre d'octets à lire
// @return void
// ---------------------------------------------------------------------------
// exemple appel: EPR_24C32readBlock(0x0000, (uint8_t*)&config, sizeof(config));
void EPR_24C32readBlock(uint16_t address, uint8_t* data, uint16_t length)
{
  for (uint16_t i = 0; i < length; i++)
  {
    data[i] = EPR_24C32readByte(address + i);
  }
}

// ---------------------------------------------------------------------------
// ===== FONCTIONS DE CALCUL ET VALIDATION =====
// ---------------------------------------------------------------------------
//pas 2 fois le même resultat?????
// ---------------------------------------------------------------------------
// @brief Calcule le checksum de la configuration (CRC16)
// @param cfg Pointeur vers la structure de configuration
// @return Valeur du checksum calculé (uint16_t)
/* 
typedef struct 
{
  uint16_t magicNumber;         // Nombre magique pour validation
  ConfigApplicatif_t applicatif;
  ConfigMateriel_t materiel;
  uint16_t checksum;
} ConfigGenerale_t;
*/
// ---------------------------------------------------------------------------
// exemple appel: uint16_t crc = EPR_24C32calcChecksum(&config);
uint16_t EPR_24C32calcChecksum(ConfigGenerale_t* cfg)
{
  uint16_t crc = 0xFFFF;
  uint8_t* data = (uint8_t*)cfg;
  // Calcul sur toute la structure sauf le champ checksum lui-même
  uint16_t length = sizeof(ConfigGenerale_t) - sizeof(uint16_t);
  
  for (uint16_t i = 0; i < length; i++)  // sans 
  {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++)
    {
      if (crc & 0x0001)
      {
        crc = (crc >> 1) ^ 0xA001;  // Polynôme CRC16
      }
      else
      {
        crc = crc >> 1;
      }
    }
  }
/**/  
// Affichage du checksum
debugSerial.print(F("\n********************** EPR_24C32calcChecksum() ==>> calculé: 0x"));
debugSerial.print(crc, HEX);
debugSerial.print(F(" en structure: 0x"));
debugSerial.println(config.checksum, HEX);
/**/
   return crc;
}

// ---------------------------------------------------------------------------
// ===== FONCTIONS DE GESTION CONFIGURATION =====
// ---------------------------------------------------------------------------



// ---------------------------------------------------------------------------
// @brief Charge la configuration depuis l'EEPROM avec validation
// @param void
// @return void
// ---------------------------------------------------------------------------
// exemple appel: EPR_24C32loadConfig();
void EPR_24C32loadConfig(void)
{
  // Lecture de la configuration
  EPR_24C32readConfig();                           // lecture et copie des 176 octets de l'eeprom dans structure
//debugSerial.println(F("FIN : EPR_24C32readConfig()"));
EPR_24C32DumpConfigToJSON(); 
  // Vérification du nombre magique
  if (config.magicNumber != CONFIG_MAGIC_NUMBER)
  {
    debugSerial.print(F("ERREUR: Magic number invalide! Attendu: 0x"));
    debugSerial.print(CONFIG_MAGIC_NUMBER, HEX);
    debugSerial.print(F(" / Lu: 0x"));
    debugSerial.print(config.magicNumber, HEX);
    debugSerial.println(F(" => Chargement config par defaut..."));
    //initDefaultConfig();
    SETUPSetStructDefaultValues();
    debugSerial.println(F("FIN : SETUPSetStructDefaultValues() cas erreur CONFIG_MAGIC_NUMBER"));
    EPR_24C32saveConfig();
EPR_24C32DumpConfigToJSON();    
    return;
  }

  // Calcul et vérification du checksum
  uint16_t calculatedChecksum = EPR_24C32calcChecksum(&config);
debugSerial.println(calculatedChecksum, HEX);
  if (calculatedChecksum != config.checksum)
  {
    debugSerial.print(F("ERREUR: Checksum invalide! Attendu: 0x"));
    debugSerial.print(config.checksum, HEX);
    debugSerial.print(F(" / Calcule: 0x"));
    debugSerial.print(calculatedChecksum, HEX);
    debugSerial.println(F(" => Chargement config par defaut..."));
    //initDefaultConfig();
    SETUPSetStructDefaultValues();
debugSerial.println(F("FIN : SETUPSetStructDefaultValues() cas erreur chksum"));
    EPR_24C32saveConfig();
EPR_24C32DumpConfigToJSON();    
    return;
  }
  
  debugSerial.println(F("Config chargee avec succes"));
  debugSerial.print(F("Version materiel: "));
  debugSerial.println(config.materiel.version);
  debugSerial.print(F("Version applicatif: "));
  debugSerial.println(config.applicatif.version);
  debugSerial.print(F("Checksum: 0x"));
  debugSerial.println(config.checksum, HEX);

EPR_24C32DumpConfigToJSON();


  debugSerial.println("______________________________________________________________________________________________________________________");

  
}

// ---------------------------------------------------------------------------
// @brief Lit la configuration brute depuis l'EEPROM
// @param void
// @return void
// ---------------------------------------------------------------------------
// exemple appel: EPR_24C32readConfig();
void EPR_24C32readConfig(void)
{
  debugSerial.println(F("Lecture EEPROM..."));
  EPR_24C32readBlock(CONFIG_EEPROM_START, (uint8_t*)&config, sizeof(ConfigGenerale_t));
  debugSerial.print(F("Octets lus: "));
  debugSerial.println(sizeof(ConfigGenerale_t));
}

//Sauvegarde marche pas
//controler json avant écriture et contrôler le mécanisme CHKSUM


// ---------------------------------------------------------------------------
// @brief Sauvegarde la configuration en EEPROM avec calcul du checksum
// @param void
// @return void
// ---------------------------------------------------------------------------
// exemple appel: EPR_24C32saveConfig();
void EPR_24C32saveConfig(void)
{
  debugSerial.println(F("Sauvegarde config en EEPROM..."));
  
  // S'assurer que le nombre magique est présent
  config.magicNumber = CONFIG_MAGIC_NUMBER;
  
  // Recalculer le checksum avant sauvegarde
  config.checksum = EPR_24C32calcChecksum(&config);
  
  debugSerial.print(F("Checksum calcule: 0x"));
  debugSerial.println(config.checksum, HEX);
  
  // Écriture en EEPROM
  EPR_24C32writeBlock(CONFIG_EEPROM_START, (uint8_t*)&config, sizeof(ConfigGenerale_t));
  
  debugSerial.print(F("Octets ecrits: "));
  debugSerial.println(sizeof(ConfigGenerale_t));
  debugSerial.println(F("Config sauvegardee avec succes"));
}

// ---------------------------------------------------------------------------
// ===== FONCTIONS D'AFFICHAGE ET DEBUG =====
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// @brief Affiche un tableau d'octets au format JSON hexadécimal
// @param array Pointeur vers le tableau d'octets
// @param length Nombre d'octets à afficher
// @return void
// ---------------------------------------------------------------------------
// exemple appel: EPR_24C32printJSON(config.materiel.DevEUI, 8);
void EPR_24C32printJSON(uint8_t* array, uint8_t length)
{
  debugSerial.print("\"");
  for (uint8_t i = 0; i < length; i++)
  {
    if (array[i] < 0x10)
    {
      debugSerial.print("0");
    }
    debugSerial.print(array[i], HEX);
  }
  debugSerial.print("\"");
}

// ---------------------------------------------------------------------------
// @brief Dump complet de la configuration au format JSON sur debugSerial
// @param void
// @return void
// ---------------------------------------------------------------------------
// exemple appel: EPR_24C32DumpConfigToJSON();
void EPR_24C32DumpConfigToJSON(void)
{
  debugSerial.println(F("{"));
  debugSerial.println(F("  \"configuration\": {"));
  
  // ===== En-tête de configuration =====
  debugSerial.println(F("    \"header\": {"));
  debugSerial.print(F("      \"magicNumber\": \"0x"));
  debugSerial.print(config.magicNumber, HEX);
  debugSerial.println(F("\","));
  debugSerial.print(F("      \"checksum\": \"0x"));
  debugSerial.print(config.checksum, HEX);
  debugSerial.println(F("\","));
  debugSerial.print(F("      \"checksumCalcule\": \"0x"));
  debugSerial.print(EPR_24C32calcChecksum(&config), HEX);
  debugSerial.println(F("\","));
  debugSerial.print(F("      \"checksumValide\": "));
  debugSerial.print((EPR_24C32calcChecksum(&config) == config.checksum) ? "true" : "false");
  debugSerial.println(F(","));
  debugSerial.print(F("      \"tailleStructure\": "));
  debugSerial.println(sizeof(ConfigGenerale_t));
  debugSerial.println(F("    },"));
  
  // ===== Configuration Applicatif =====
  debugSerial.println(F("    \"applicatif\": {"));
  debugSerial.print(F("      \"version\": "));
  debugSerial.print(config.applicatif.version);
  debugSerial.println(F(","));
  
  // LED
  debugSerial.println(F("      \"led\": {"));
  debugSerial.print(F("        \"redDuration\": "));
  debugSerial.print(config.applicatif.redLedDuration);
  debugSerial.println(F(","));
  debugSerial.print(F("        \"greenDuration\": "));
  debugSerial.print(config.applicatif.greenLedDuration);
  debugSerial.println(F(","));
  debugSerial.print(F("        \"blueDuration\": "));
  debugSerial.print(config.applicatif.blueLedDuration);
  debugSerial.println(F(","));
  debugSerial.print(F("        \"builtinDuration\": "));
  debugSerial.println(config.applicatif.builtinLedDuration);
  debugSerial.println(F("      },"));
  
  // Rucher
  debugSerial.println(F("      \"rucher\": {"));
  debugSerial.print(F("        \"id\": "));
  debugSerial.print(config.applicatif.RucherID);
  debugSerial.println(F(","));
  debugSerial.print(F("        \"nom\": \""));
  debugSerial.print(config.applicatif.RucherName);
  debugSerial.println(F("\""));
  debugSerial.println(F("      },"));
  
  // LoRa
  debugSerial.println(F("      \"lora\": {"));
  debugSerial.print(F("        \"appEUI\": "));
  EPR_24C32printJSON(config.applicatif.AppEUI, 8);
  debugSerial.println(F(","));
  debugSerial.print(F("        \"appKey\": "));
  EPR_24C32printJSON(config.applicatif.AppKey, 16);
  debugSerial.println(F(","));
  debugSerial.print(F("        \"spreadingFactor\": "));
  debugSerial.print(config.applicatif.SpreadingFactor);
  debugSerial.println(F(","));
  debugSerial.print(F("        \"sendingPeriod\": "));
  debugSerial.print(config.applicatif.SendingPeriod);
  debugSerial.println(F(","));
  debugSerial.print(F("        \"oledRefreshPeriod\": "));
  debugSerial.println(config.applicatif.OLEDRefreshPeriod);
  debugSerial.println(F("      }"));
  debugSerial.println(F("    },"));
  
  // ===== Configuration Matériel =====
  debugSerial.println(F("    \"materiel\": {"));
  debugSerial.print(F("      \"version\": "));
  debugSerial.print(config.materiel.version);
  debugSerial.println(F(","));
  
  // Adresses I2C
  debugSerial.println(F("      \"adressesI2C\": {"));
  debugSerial.print(F("        \"rtc\": \"0x"));
  debugSerial.print(config.materiel.adresseRTC, HEX);
  debugSerial.println(F("\","));
  debugSerial.print(F("        \"oled\": \"0x"));
  debugSerial.print(config.materiel.adresseOLED, HEX);
  debugSerial.println(F("\","));
  debugSerial.print(F("        \"eeprom\": \"0x"));
  debugSerial.print(config.materiel.adresseEEPROM, HEX);
  debugSerial.println(F("\",")); 
  debugSerial.print(F("        \"poidsTare\": "));
  debugSerial.println(config.materiel.poidsTare);
  debugSerial.println(F("      },"));

  // Identification
  debugSerial.println(F("      \"identification\": {"));
  debugSerial.print(F("        \"numCarte\": "));
  debugSerial.print(config.materiel.Num_Carte);
  debugSerial.println(F(","));
  debugSerial.print(F("        \"devEUI\": "));
  EPR_24C32printJSON(config.materiel.DevEUI, 8);
  debugSerial.println();
  debugSerial.println(F("      },"));
  
  // Périphériques présents
  debugSerial.println(F("      \"peripheriques\": {"));
  debugSerial.print(F("        \"rtc\": "));
  debugSerial.print(config.materiel.Rtc ? "true" : "false");
  debugSerial.println(F(","));
  debugSerial.print(F("        \"kbdAnalogique\": "));
  debugSerial.print(config.materiel.KBD_Ana ? "true" : "false");
  debugSerial.println(F(","));
  debugSerial.print(F("        \"oled\": "));
  debugSerial.print(config.materiel.Oled ? "true" : "false");
  debugSerial.println(F(","));
  debugSerial.print(F("        \"sdhc\": "));
  debugSerial.print(config.materiel.SDHC ? "true" : "false");
  debugSerial.println(F(","));
  debugSerial.print(F("        \"lipo\": "));
  debugSerial.print(config.materiel.LiPo ? "true" : "false");
  debugSerial.println(F(","));
  debugSerial.print(F("        \"solaire\": "));
  debugSerial.print(config.materiel.Solaire ? "true" : "false");
  debugSerial.println();
  debugSerial.println(F("      },"));
  
  // Facteurs d'échelle analogiques
  debugSerial.println(F("      \"analogScale\": {"));
  debugSerial.print(F("        \"ldrBrightness\": "));
  debugSerial.print(config.materiel.LDRBrightnessScale, 4);
  debugSerial.println(F(","));
  debugSerial.print(F("        \"vSolaire\": "));
  debugSerial.print(config.materiel.VSolScale, 4);
  debugSerial.println(F(","));
  debugSerial.print(F("        \"vBatterie\": "));
  debugSerial.print(config.materiel.VBatScale, 4);
  debugSerial.println();
  debugSerial.println(F("      },"));
  
  // HX711 Pesons
  debugSerial.println(F("      \"pesons\": ["));
  
  // Peson #0
  debugSerial.println(F("        {"));
  debugSerial.print(F("          \"numero\": "));
  debugSerial.print(config.materiel.Peson_0);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"pinClk\": "));
  debugSerial.print(config.materiel.HX711Clk_0);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"pinData\": "));
  debugSerial.print(config.materiel.HX711Dta_0);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"noloadValue\": "));
  debugSerial.print(config.materiel.HX711NoloadValue_0, 2);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"tareTemp\": "));
  debugSerial.print(config.materiel.HX711Tare_Temp_0, 2);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"scaling\": "));
  debugSerial.print(config.materiel.HX711Scaling_0, 4);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"corTemp\": "));
  debugSerial.println(config.materiel.HX711Cor_Temp_0, 4);
  debugSerial.println(F("        },"));
  
  // Peson #1
  debugSerial.println(F("        {"));
  debugSerial.print(F("          \"numero\": "));
  debugSerial.print(config.materiel.Peson_1);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"pinClk\": "));
  debugSerial.print(config.materiel.HX711Clk_1);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"pinData\": "));
  debugSerial.print(config.materiel.HX711Dta_1);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"noloadValue\": "));
  debugSerial.print(config.materiel.HX711NoloadValue_1, 2);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"tareTemp\": "));
  debugSerial.print(config.materiel.HX711Tare_Temp_1, 2);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"scaling\": "));
  debugSerial.print(config.materiel.HX711Scaling_1, 4);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"corTemp\": "));
  debugSerial.println(config.materiel.HX711Cor_Temp_1, 4);
  debugSerial.println(F("        },"));
  
  // Peson #2
  debugSerial.println(F("        {"));
  debugSerial.print(F("          \"numero\": "));
  debugSerial.print(config.materiel.Peson_2);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"pinClk\": "));
  debugSerial.print(config.materiel.HX711Clk_2);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"pinData\": "));
  debugSerial.print(config.materiel.HX711Dta_2);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"noloadValue\": "));
  debugSerial.print(config.materiel.HX711NoloadValue_2, 2);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"tareTemp\": "));
  debugSerial.print(config.materiel.HX711Tare_Temp_2, 2);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"scaling\": "));
  debugSerial.print(config.materiel.HX711Scaling_2, 4);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"corTemp\": "));
  debugSerial.println(config.materiel.HX711Cor_Temp_2, 4);
  debugSerial.println(F("        },"));
  
  // Peson #3
  debugSerial.println(F("        {"));
  debugSerial.print(F("          \"numero\": "));
  debugSerial.print(config.materiel.Peson_3);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"pinClk\": "));
  debugSerial.print(config.materiel.HX711Clk_3);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"pinData\": "));
  debugSerial.print(config.materiel.HX711Dta_3);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"noloadValue\": "));
  debugSerial.print(config.materiel.HX711NoloadValue_3, 2);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"tareTemp\": "));
  debugSerial.print(config.materiel.HX711Tare_Temp_3, 2);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"scaling\": "));
  debugSerial.print(config.materiel.HX711Scaling_3, 4);
  debugSerial.println(F(","));
  debugSerial.print(F("          \"corTemp\": "));
  debugSerial.println(config.materiel.HX711Cor_Temp_3, 4);
  debugSerial.println(F("        }"));
  
  debugSerial.println(F("      ]"));
  debugSerial.println(F("    }"));
  debugSerial.println(F("  }"));
  debugSerial.println(F("}"));
}


// OLD CODE

// ---------------------------------------------------------------------------*
// @brief Définit la configuration par défaut
// @param Aucun
// @return void
// ---------------------------------------------------------------------------*
void non_setDefaultConfig(void) 
{
  config.materiel.version = CONFIG_VERSION;
  config.materiel.adresseRTC = DS3231_ADDRESS;    // Adresse RTC Module DS3231
  config.materiel.adresseOLED = OLED_ADDRESS;     // Adresse écran OLED
  config.materiel.adresseEEPROM = EEPROM_ADDRESS; // Adresse EEPROM Module DS3231
  config.materiel.poidsTare = TARE;              // valeur de la tare en grammes

   
  config.applicatif.redLedDuration = RED_LED_DURATION;
  config.applicatif.greenLedDuration = GREEN_LED_DURATION;
  config.applicatif.blueLedDuration = BLUE_LED_DURATION;
  config.applicatif.builtinLedDuration = BUILTIN_LED_DURATION;
  config.applicatif.SendingPeriod = WAKEUP_INTERVAL_PAYLOAD;    // mieux: wakeupIntervalPayload
  config.applicatif.OLEDRefreshPeriod = INTERVAL_1SEC;               //
    
  config.checksum = EPR_24C32calcChecksum(&config);
}


// ---------------------------------------------------------------------------*
// @brief Initialise la configuration par défaut
// @param Aucun
// @return void
// ---------------------------------------------------------------------------*
void initConfig(void) 
{
  debugSerial.println("Début lecture configuration EEPROM");
  EPR_24C32loadConfig();
  debugSerial.println("Lecture configuration EEPROM executée");
}

/*
 * 

// ---------------------------------------------------------------------------*
// @brief Charge la configuration depuis l'EEPROM
// @param Aucun
// @return void
// ---------------------------------------------------------------------------*
void EPR_24C32loadConfig(void) 
{
  // Simulation de lecture EEPROM - à implémenter avec bibliothèque I2C EEPROM
  setDefaultConfig();
}

// ---------------------------------------------------------------------------*
// @brief Sauvegarde la configuration dans l'EEPROM
// @param Aucun
// @return void
// ---------------------------------------------------------------------------*
void EPR_24C32saveConfig(void) 
{
  config.checksum = EPR_24C32calcChecksum(&config);
  // Simulation de sauvegarde EEPROM - à implémenter avec bibliothèque I2C EEPROM
  debugSerial.println("Configuration sauvegardée");
}

// ---------------------------------------------------------------------------*
// @brief Calcule le checksum de la configuration
// @param cfg Pointeur vers la configuration
// @return uint16_t Checksum calculé
// ---------------------------------------------------------------------------*
uint16_t EPR_24C32calcChecksum(ConfigGenerale_t* cfg) 
{
  uint16_t sum = 0;
  uint8_t* data = (uint8_t*)cfg;
  for (int i = 0; i < sizeof(ConfigGenerale_t) - sizeof(uint16_t); i++) 
  {
      sum += data[i];
  }
  return sum;
}
*/
