#define __INIT_DONE
#include "define.h"



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
