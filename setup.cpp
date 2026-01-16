//       1         2         3         4         5         6         7        7
//34567890123456789012345678901234567890123456789012345678901234567890123456789
// IMPRESSION 79 COLONES EN TAILLE 12
//
// ---------------------------------------------------------------------------*
//       _____      _                               
//      / ____|    | |                              
//     | (___   ___| |_ _   _ _ __   ___ _ __  _ __ 
//      \___ \ / _ \ __| | | | '_ \ / __| '_ \| '_ \
//      ____) |  __/ |_| |_| | |_) | (__| |_) | |_) |
//     |_____/ \___|\__|\__,_| .__(_)___| .__/| .__/
//                           | |        | |   | |   
//                           |_|        |_|   |_|    
// ---------------------------------------------------------------------------*
#define __INIT_DONE
#include ".\define.h"
    
// Port Série Debug ----- initialisation ----- infos projet -----
// ---------------------------------------------------------------------------*
// ---------------------------------------------------------------------------*
void initDebugSerial(void)
{
    debugSerial.begin(DEBUG_BAUD);
    while ((!debugSerial) && (millis() < SERIAL_TIMEOUT)) {}
    delay(1000);
    debugSerial.println("Compiled: " __DATE__ ", " __TIME__ ", " VERSION);
    debugSerial.println(PROJECT_NAME);
}

// ---------------------------------------------------------------------------*
// ---------------------------------------------------------------------------*
void softReset() 
{
// Reset immédiat du microcontrôleur
  NVIC_SystemReset();
// Le code ne continue jamais après cette ligne
}

// ---------------------------------------------------------------------------*
//  out : 0 (Error ou non référencé) / n (Num de carte)  
// @brief Find card number
//                Num_Carte (global) dans fonction
// @param void
// @return :  0 error or Card number
// ---------------------------------------------------------------------------*
uint8_t init2483A(uint8_t *HWEUI)  // Init 2483 ---- Init 2483 ---- Init 2483 ---- Init 2483 
{ uint8_t tryy = 3 , rc;

//debugSerial.print("-----------------------------");
//debugSerial.print("INIT 2483... with: ");
//debugSerial.println(config.materiel.Num_Carte);


  loraSerial.begin(LoRaBee.getDefaultBaudRate());
   do
   { rc = Init_2483(HWEUI); // 0 = erreur ou Num de carte
//debugSerial.println(rc ? "rc Init_2483() true":"rc Init_2483() false");
// Reset_LoRa();  // initialise pas sur reset chaud.
/*      
      if (config.materiel.Num_Carte)
      {
        debugSerial.print(" Init 2483 done with card : ");
        debugSerial.println(config.materiel.Num_Carte);
        OLEDDebugDisplay("2483A    Initialized");
      }
      else
      {
        debugSerial.println(" NO 2483 present.");
        OLEDDebugDisplay("2483A   Failed");
      }
*/      
      tryy--;
    }  
    while (!rc && tryy); // erreur et max 4 fois
  /*  
    else 
    {
      debugSerial.println(" Init 2483 failed");    
      OLEDDebugDisplay("2483A   Failed");
    }  
*/
// debugSerialPrintLoRaStatus();
  return(rc);
}


// ---------------------------------------------------------------------------*
// ---------------------------------------------------------------------------*
void initLoRa(void)
{
// INIT LoRa ---- INIT LoRa ---- INIT LoRa ---- INIT LoRa ---- INIT LoRa 
debugSerial.println("------------------------------------------------------------------");
debugSerial.println("INIT LoRa...");
//  Reset_LoRa();  // initialise pas sur reset chaud.

  if (setupLoRa())
  {
debugSerial.println("Init LoRa done.");
debugSerial.println("Test sending LoRa testPayload (7) (Restart)..."); 
    sendLoRaPayload((uint8_t*)testPayload,7);
    OLEDDebugDisplay("LoRa    Initialized");
  }
  else
  {
    OLEDDebugDisplay("LoRa Failed");  
  }

/*
  blink(LED_BUILTIN,200);
// Flash ---- Flash ---- Flash ---- Flash ---- Flash ---- Flash ---- Flash 
// set FLASH to deep sleep & reset SPI pins for min. energy consumption
  DFlashUltraDeepSleep(); 
*/
//  OLEDDebugDisplay("---- SETUP DONE ----");
//delay(1000);
//  display.clearDisplay();
//  display.display();
}

// INIT DHT22 ---- INIT DHT22 ---- INIT DHT22 ---- INIT DHT22 ---- 
// ---------------------------------------------------------------------------*
// ---------------------------------------------------------------------------*
void DHTInit(void)
{
debugSerial.println("------------------------------------------------------------------");
debugSerial.println("INIT DHT22...");
  dht.begin(); // temperature
  debugSerial.println("done.");
OLEDDebugDisplay("DHT22   Initialized");
}


// ---------------------------------------------------------------------------
// @brief Initialise la configuration avec des valeurs par défaut
// @param void
// @return void
// ---------------------------------------------------------------------------
// exemple appel: setStructDefaultValues();
void setStructDefaultValues()
{
debugSerial.println("////////////////////////////// setStructDefaultValues() ////////////////////////////////////");  

// ---------------------------------------------------------------------------
// ========================= Configuration Applicatif ========================
// ---------------------------------------------------------------------------
  config.applicatif.version = CONFIG_VERSION; 
  
// Paramètres LED
  config.applicatif.redLedDuration = RED_LED_DURATION;;
  config.applicatif.greenLedDuration = GREEN_LED_DURATION;;
  config.applicatif.blueLedDuration = BLUE_LED_DURATION;;
  config.applicatif.builtinLedDuration = BUILTIN_LED_DURATION;;
  
// Paramètres Rucher
  config.applicatif.RucherID = 11;
  strcpy(config.applicatif.RucherName, "Rucher Test");
  
// Paramètres LoRa - AppEUI
  uint8_t defaultAppEUI[] = {0x41, 0x42, 0x45, 0x49, 0x4C, 0x4C, 0x45, 0x31, 0x00};
  memcpy(config.applicatif.AppEUI, defaultAppEUI, 9);
  
// Paramètres LoRa - AppKey: PHILIPPELOVEBEES
  uint8_t defaultAppKey[] = {0x50, 0x48, 0x49, 0x4C, 0x49, 0x50, 0x50, 0x45, 
                             0x4C, 0x4F, 0x56, 0x45, 0x42, 0x45, 0x45, 0x53, 0x00};
  memcpy(config.applicatif.AppKey, defaultAppKey, 17);
  
  config.applicatif.SpreadingFactor = DEFAULT_SF;
  config.applicatif.SendingPeriod = WAKEUP_INTERVAL_PAYLOAD;
  config.applicatif.OLEDRefreshPeriod = 1000; //INTERVAL_1SEC;
  
// ---------------------------------------------------------------------------
// ========================== Configuration Matériel =========================
// ---------------------------------------------------------------------------
  config.materiel.version = 3;  // PCB2
  config.materiel.adresseRTC = DS3231_ADDRESS;
  config.materiel.adresseOLED = OLED_ADDRESS;
  config.materiel.adresseEEPROM = EEPROM_ADDRESS;

//Conditionner que pas déjà lu  
  config.materiel.Num_Carte = init2483A(config.materiel.DevEUI);   //1;

debugSerial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ setStructDefaultValues() \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");  
//debugSerial.print("config.materiel.Num_Carte <= "); debugSerial.println(config.materiel.Num_Carte);  // OK
  
// DevEUI par défaut
  uint8_t defaultDevEUI[] = {0x00, 0x04, 0xA3, 0x0B, 0x00, 0xEE, 0xEE, 0x01, 0x00};
  memcpy(config.materiel.DevEUI, defaultDevEUI, 9);
  
// Détection périphériques
  config.materiel.Rtc = true;
  config.materiel.KBD_Ana = true;
  config.materiel.Oled = 96;
  config.materiel.SDHC = false;
  config.materiel.LiPo = true;
  config.materiel.Solaire = true;
  
// Facteurs d'échelle analogiques
  config.materiel.LDRBrightnessScale = 1.0;
  config.materiel.VSolScale = 1.0;
  config.materiel.VBatScale = 1.0;
  
// Initialisation HX711#0
  config.materiel.Peson_0 = Peson[config.materiel.Num_Carte][0];
  config.materiel.HX711Clk_0 = HX711_SENSOR_SCK;
  config.materiel.HX711Dta_0 = HX711_ASENSOR_DOUT;
// les données sont extraites de la base de données des pesons
  config.materiel.HX711NoloadValue_0 = Jauge[config.materiel.Peson_0][0];
  config.materiel.HX711Tare_Temp_0 = Jauge[config.materiel.Peson_0][2];
  config.materiel.HX711Scaling_0 = Jauge[config.materiel.Peson_0][1];
  config.materiel.HX711Cor_Temp_0 = Jauge[config.materiel.Peson_0][3];

//    Jauge[19] =>   {7929.70,97.49,20,0},    // J19 proto1  20kg (OK à 1 et 5kg) + DHT22

  
// Initialisation HX711#1
  config.materiel.Peson_1 = Peson[config.materiel.Num_Carte][1];
  config.materiel.HX711Clk_1 = HX711_SENSOR_SCK;
  config.materiel.HX711Dta_1 = HX711_BSENSOR_DOUT;
// les données sont extraites de la base de données des pesons
  config.materiel.HX711NoloadValue_1 = Jauge[config.materiel.Peson_1][0];
  config.materiel.HX711Tare_Temp_1 = Jauge[config.materiel.Peson_1][2];
  config.materiel.HX711Scaling_1 = Jauge[config.materiel.Peson_1][1];
  config.materiel.HX711Cor_Temp_1 = Jauge[config.materiel.Peson_1][3];
  
// Initialisation HX711#2
  config.materiel.Peson_2 = Peson[config.materiel.Num_Carte][2];
  config.materiel.HX711Clk_2 = HX711_SENSOR_SCK;
  config.materiel.HX711Dta_2 = HX711_CSENSOR_DOUT;
// les données sont extraites de la base de données des pesons
  config.materiel.HX711NoloadValue_2 = Jauge[config.materiel.Peson_2][0];
  config.materiel.HX711Tare_Temp_2 = Jauge[config.materiel.Peson_2][2];
  config.materiel.HX711Scaling_2 = Jauge[config.materiel.Peson_2][1];
  config.materiel.HX711Cor_Temp_2 = Jauge[config.materiel.Peson_2][3];
  
// Initialisation HX711#3
  config.materiel.Peson_3 = Peson[config.materiel.Num_Carte][3];
  config.materiel.HX711Clk_3 = HX711_SENSOR_SCK;
  config.materiel.HX711Dta_3 = HX711_DSENSOR_DOUT;
// les données sont extraites de la base de données des pesons
  config.materiel.HX711NoloadValue_3 = Jauge[config.materiel.Peson_3][0];
  config.materiel.HX711Tare_Temp_3 = Jauge[config.materiel.Peson_3][2];
  config.materiel.HX711Scaling_3 = Jauge[config.materiel.Peson_3][1];
  config.materiel.HX711Cor_Temp_3 = Jauge[config.materiel.Peson_3][3];

// ---------------------------------------------------------------------------
// ========================== Configuration du reste =========================
// ---------------------------------------------------------------------------
// Initialisation magicNumber
  config.magicNumber = CONFIG_MAGIC_NUMBER;  
// Calcul et stockage du checksum
  config.checksum = calculateChecksum(&config);
debugSerial.println(F("Config par defaut initialisee"));
}
