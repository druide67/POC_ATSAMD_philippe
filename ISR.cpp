#define __INIT_DONE
#include "define.h"

/**
 * @brief Fonction d'interruption RTC
 * @param Aucun
 * @return void
 */
void onRTCAlarm(void) // $ ou £
{
// ALARME 1 : Toutes les secondes (mode programmation)
  if (rtc.alarmFired(1)) 
  {
    rtc.clearAlarm(1);
//    if (!alarm1_enabled) 
//      return;     // interruption execution code de IRQ1 pendant IRQ2
    wakeup1Sec = alarm1_enabled;  // True si autorisé par IRQ2
//debugSerial.print("$");
/*
// en mode  DS3231_A1_PerSecond, ne pas reprogrammer l'alarme
// L'alarme DS3231_A1_PerSecond se répète automatiquement
// Seulement reprogrammer si le mode change
// reprogramer si DS3231_A1_Second, faire :  
// Reprogrammer l'alarme 1 seconde si en mode programmation
// ici toutes les minutes quand la seconded programmée survient
    if (DEBUG_INTERVAL_1SEC && !modeExploitation) 
    {
//      DateTime nextSecond = rtc.now() + TimeSpan(0, 0, 0, 1);
 //     rtc.setAlarm1(nextSecond, DS3231_A1_Second);
//     rtc.setAlarm1(nextSecond, DS3231_A1_PerSecond);  
// DS3231setRTCAlarm1();
     
// debugSerialPrintNextAlarm(nextSecond, 1); 
    }
*/
  }
  
  // ALARME 2 : Payload périodique    
  if (rtc.alarmFired(2)) 
  {
    alarm1_enabled = false;  // Bloquer alarme 1
    wakeupPayload = true;
    rtc.clearAlarm(1);       
    rtc.clearAlarm(2);      
    DS3231setRTCAlarm2(); // Reprogrammer prochaine alarme
//debugSerial.print("£");
  }
}
