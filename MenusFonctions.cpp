#define __INIT_DONE
#include "define.h"

// info afficher NextPayload
// info afficher MODE PE


/*
 * Fonctions lancées par les menus
 * 
 * lancement : menuNNN_FN_WhatToDo()     // ex: menu000_F7_GetTime()
 * Traitement: menuNNN_FN_WhatToDoDone() // ex: menu000_F7_GetTimeDone()    
 * 
 * 
 * 
 * menu000_F6_GetDate()
 * menu000_F7_GetTime() + Done
 * menu000_F8_GetHex()  + Done
 * 
 * prévoir inits param Lora :
//  uint8_t HWEUI [20];       // ID RN2483: "0004A30B00EEEE01"
//  uint8_t AppEUI [10];      // AppEUI: {0x41, 0x42, 0x45, 0x49, 0x4C, 0x4C, 0x45, 0x31, 0x00} menuxxx_xx_GetHex()
//  uint8_t AppKey [18];      // AppKEY: // 5048494C495050454C4F564542454553 - PHILIPPELOVEBEES
//// {0x50, 0x48, 0x49, 0x4C, 0x49, 0x50, 0x50, 0x45, 0x4C, 0x4F, 0x56, 0x45, 0x42, 0x45, 0x45, 0x53, 0x00} 
//  uint8_t SpreadingFactor;  // 7, 9 et 12 echec freudeneck
//  uint8_t SendingPeriod;    // 15 minutes = 500 sans IT
 * 
 * Prévoir inits generaux + Bal
//  Date:  menuxxx_xx_GetDate()
//  Heure: menuxxx_xx_GetTime()
//  uint8_t Balance_ID;           // ID Rucher           xx  uint8_t
//  char    RucherName [20];      // Localisation Rucher (saisir direct ou liste + "autre")
//  version, mail (pour le fun), 
 * 
 * Prévoir Config Hardware
//  uint8_t HX711Clk_0;           // HX711#0 parameters
//  uint8_t HX711Dta_0;
//  float   HX711ZeroValue_0;
//  float   HX711Scaling_0;
//  float   HX711Cor_Temp_0;
//  uint8_t HX711Clk_1;           // HX711#1 parameters
//  uint8_t HX711Dta_1;
//  float   HX711ZeroValue_1;
//  float   HX711Scaling_1;
//  float   HX711Cor_Temp_1;
//  uint8_t HX711Clk_2;           // HX711#2 parameters
//  uint8_t HX711Dta_2;
//  float   HX711ZeroValue_2;
//  float   HX711Scaling_2;
//  float   HX711Cor_Temp_2;
//  uint8_t HX711Clk_3;           // HX711#3 parameters
//  uint8_t HX711Dta_3;
//  float   HX711ZeroValue_3;
//  float   HX711Scaling_3;
//  float   HX711Cor_Temp_3;
//  float   LDRBrightnessScale;   // 
//  float   VSolScale;            //  
//  float   VBatScale;
 * 
 */


void menu000_F6_GetDate()
{
DateTime systemTime;     
debugSerial.println("Lancement saisie DATE");
debugSerial.println("CONFIG. SYSTEME - Demande saisie DATE");
  systemTime = rtc.now();
sprintf(serialbuf, "%02d:%02d:%02d    %02d/%02d/%02d", 
        systemTime.hour(), systemTime.minute(), systemTime.second(),
        systemTime.day(), systemTime.month(), systemTime.year()-2000);
debugSerial.println(serialbuf);
  sprintf(serialbuf, "%02d:%02d:%02d",systemTime.year()-2000, systemTime.month(), systemTime.day());
 // startDateInput(serialbuf); 
}               


void menu000_F7_GetTime()
{
DateTime systemTime;     
debugSerial.println("Lancement saisie TIME");
debugSerial.println("CONFIG. SYSTEME - Demande saisie TIME");
  systemTime = rtc.now();
sprintf(serialbuf, "%02d:%02d:%02d    %02d/%02d/%02d", 
        systemTime.hour(), systemTime.minute(), systemTime.second(),
        systemTime.day(), systemTime.month(), systemTime.year()-2000);
debugSerial.println(serialbuf);
  sprintf(serialbuf, "%02d:%02d:%02d",systemTime.hour(), systemTime.minute(), systemTime.second());
  startTimeInput(serialbuf); 
}           

void menu000_F7_GetTimeDone()    
{ DateTime systemTime;
  byte hour, minute, second;
  static char timeBuffer[9] = ""; // Buffer pour l'heure

  finalizeTimeInput(timeBuffer); // Récupérer l'heure finale
debugSerial.print("Nouvelle heure: ");
debugSerial.println(timeBuffer);        // Ici vous pouvez traiter l'heure et revenir au menu
// hh:mm:ss
  hour = ((byte)timeBuffer[0] -48)*10;
  hour += (byte)timeBuffer[1] -48;
  minute = ((byte)timeBuffer[3] -48)*10;
  minute += (byte)timeBuffer[4] -48;
  second = ((byte)timeBuffer[6] -48)*10;
  second += (byte)timeBuffer[7] -48;
  systemTime = rtc.now();
  rtc.adjust(DateTime(systemTime.year()-2000, systemTime.month(), systemTime.day(), hour, minute, second));  
debugSerial.println("mise à l'heure DS3231");
  copyDS3231TimeToMicro(1);
  synchronizeDS3231TimeToMicro();
debugSerial.println("Reprogramme IRQ2");  
  DS3231setRTCAlarm2(); // Reprogrammer prochaine alarme dans n min
// Activer la liste de démarrage quand fin saisie TIME : void finalizeTimeInput(char* outputTime)
  if (currentMenuDepth > 0)
  {
    menuLevel_t* currentMenu = &menuStack[currentMenuDepth - 1];
    startListInputWithTimeout(currentMenu->title, currentMenu->menuList, currentMenu->menuSize, currentMenu->selectedIndex, 0);
  }
}


void menu000_F8_GetHex()
{ static char hexBuffer[41] = "0123456789ABCDEF0123456789ABCDEF01234567"; // Buffer pour l'hexa

debugSerial.println("Lancement saisie HEXA");
debugSerial.println("CONFIG. SYSTEME - Demande saisie HEXA");
  startHexInput(hexBuffer); 
}           

void menu000_F8_GetHexDone()    
{ static char hexBuffer[41] = "0123456789ABCDEF0123456789ABCDEF01234567"; // Buffer pour l'hexa

  finalizeHexInput(hexBuffer); // Récupérer chaine HEXA
debugSerial.print("Nouvelle chaine: ");
debugSerial.println(hexBuffer);        // Ici vous pouvez traiter l'heure et revenir au menu
  if (currentMenuDepth > 0)
  {
    menuLevel_t* currentMenu = &menuStack[currentMenuDepth - 1];
    startListInputWithTimeout(currentMenu->title, currentMenu->menuList, currentMenu->menuSize, currentMenu->selectedIndex, 0);
  }
}
