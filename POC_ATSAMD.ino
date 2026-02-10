//       1         2         3         4         5         6         7        7
//34567890123456789012345678901234567890123456789012345678901234567890123456789
// IMPRESSION 79 COLONES EN TAILLE 12
//
// ---------------------------------------------------------------------------*
// https://claude.ai/chat/75e1ba53-8b90-4f09-97a5-cfc366e36a8e
// ---------------------------------------------------------------------------*
//      _____                     _                           _   _            
//     |  __ \               /\  | |                         | | (_)           
//     | |__) |__   ___     /  \ | |_ ___  __ _ _ __ ___   __| |  _ _ __   ___ 
//     |  ___/ _ \ / __|   / /\ \| __/ __|/ _` | '_ ` _ \ / _` | | | '_ \ / _ \
//     | |  | (_) | (__   / ____ \ |_\__ \ (_| | | | | | | (_| |_| | | | | (_) |
//     |_|   \___/ \___| /_/    \_\__|___/\__,_|_| |_| |_|\__,_(_)_|_| |_|\___/
//                   ______                                                    
//                  |______|                                                   
// ---------------------------------------------------------------------------*
// https://patorjk.com/software/taag/#p=display&f=Big&t=&x=cppComment&v=4&h=4&w=80&we=true
// Ascii Art Font : BIG
// C++ style comment
// ---------------------------------------------------------------------------*
//
// @file POC_ATSAMD.ino   
// @brief Programme de démonstration pour SODAQ EXPLORER avec gestion basse consommation
// @author Votre nom
// @version 1.1.1-PL
// @date 2025
// 
// Description : Gestion de deux modes de fonctionnement avec interruptions RTC
// - MODE EXPLOITATION : réveil périodique pour LED rouge
// - MODE PROGRAMMATION : réveil périodique + clignotement LED builtin + interface utilisateur
// ---------------------------------------------------------------------------*

#define __MAIN__
// ===== INCLUDES PROJET =====
#include ".\define.h"

//#define __SerialDebugPoc      // decommenter pour afficher messages debug

// =====  PROGRAMME =====
// ===== SETUP =====
void setup() 
{ // Initialisation de debugSerial et envoi infos compil.
  SETUPinitDebugSerial(); 

  // Initialisation de LoraSerial
  SETUPinitLoRaSerial();

  // Initialisation des LEDs
  initLEDs();
    
  // Initialisation du mode selon PIN_PE
  pinMode(PIN_PE, INPUT_PULLUP);
  modeExploitation = digitalRead(PIN_PE);
  debugSerial.print("Mode détecté: ");
  debugSerial.println(modeExploitation ? "EXPLOITATION" : "PROGRAMMATION");
   
  // Initialisation I2C
  Wire.begin();
  debugSerial.println(F("Bus I2C initialise"));
// scanI2C???

  // Initialisation OLED
  debugOLEDDrawText = false;
  OLEDInit();

/*
// Après display.begin() si décallage de 2 lignes
display.ssd1306_command(0x21);  // Set column address
display.ssd1306_command(2);      // Column start address (2 au lieu de 0)
display.ssd1306_command(129);    // Column end address (128 + 2 - 1)
*/
  
  OLEDDebugDisplay("OLEDInit OK"); // scrolling lors du setup(); 

debugSerial.println(F("-------------------------------- SETUP - Fin OLED et avant-----------------------"));

  // Initialisation configuration
  initConfig();     // appel EPR_24C32loadConfig();
  OLEDDebugDisplay("initConfig OK");
delay(1000);

debugSerial.println(F("-------------------------------- SETUP - Fin initConfig() -----------------------"));

// initialiser les Structures, lecture EEPROM I2C@0x57,  si echec val par défaut
// Si echec tester autre adresse I2C si existe
// initialiser par lecture EEPROM I2C


SETUPSetStructDefaultValues();

debugSerial.println(F("-------------------------------- SETUP - Fin SETUPSetStructDefaultValues() -----------"));

  sprintf(OLEDbuf,"ID: %s",config.materiel.DevEUI);
  OLEDDebugDisplay(OLEDbuf);



// config balance connue 10 lecture + moyenne

debugSerial.println(F("-------------------------------- SETUP - GetStrainGaugeAverage -------------------------------"));





  for ( int z=0;z<4;z++)                                 // Z  .. 3
  {     
    if (Peson[config.materiel.Num_Carte][z])
    {  

SETUPinitHX711WithWatchdog(z);

//#define poidsBal_kg(num)  abs((Contrainte_List[num]-pesonTare(num))/pesonScale(num)/1000) // kg
      
      Contrainte_List[z]=GetStrainGaugeAverage(z,10);
      HiveSensor_Data.HX711Weight[z] = poidsBal_kg(z); //calculePoids(z);
      snprintf(OLEDbuf, 21,"Bal. %c: %8.2f kg",z+65,poidsBal_kg(z));  //calculePoids(z));
      OLEDDebugDisplay(OLEDbuf);
    }
    else 
    {
      sprintf(OLEDbuf,"Bal. %d: NONE ",z);
     OLEDDebugDisplay(OLEDbuf);
    }
  }
 
// INIT LoRa ---- INIT LoRa ---- INIT LoRa ---- INIT LoRa ---- INIT LoRa 
debugSerial.println("--------------------------------- SETUP - INIT LoRa ---------------------------------");
  if (initLoRa())
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
         OLEDDebugDisplay("2483A    Initialized");


// void DS3231CompleteReset() si DS3231 out!

debugSerial.println(F("-------------------------------- SETUP - Fin initLoRa()+ Send Payload -----------"));

  initRTC();

debugSerial.println(F("-------------------------------- SETUP - Fin initRTC() --------------------------"));

//Affiche heure DS3231
static DateTime oldSystemTime(1692712245);  //1692712245);  // timestamp Unix
char localbuf[21] = "00:00:00    00/00/00";  
  DateTime systemTime = rtc.now();

  snprintf(localbuf, 21, "%02d:%02d:%02d    %02d/%02d/%02d", 
          systemTime.hour(), systemTime.minute(), systemTime.second(),
          systemTime.day(), systemTime.month(), systemTime.year()-2000);
debugSerial.println(localbuf);
// Affiche heure µC
    OLEDDebugDisplay("Set Time OK");
  
// Désactiver TOUTES les interruptions temporairement
    noInterrupts();
  
// Configuration DS3231 AVANT interruption
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

// Configuration des alarmes RTC 1 et 2
if (DEBUG_INTERVAL_1SEC)
{
  debugSerial.println("Initialisation IRQ1");
  DS3231setRTCAlarm1();
}
if (DEBUG_WAKEUP_PAYLOAD)
{
//debugSerial.println("Initialisation IRQ2");
  DS3231setRTCAlarm2();
}
  OLEDDebugDisplay("Set RTC Alarms OK");
// Configuration interruption externe
  pinMode(RTC_INTERRUPT_PIN, INPUT_PULLUP);
// AJOUTÉ: Debug configuration
debugSerial.print("Configuration interruption sur pin ");
debugSerial.println(RTC_INTERRUPT_PIN);
  
  LowPower.attachInterruptWakeup(RTC_INTERRUPT_PIN, onRTCAlarm, FALLING);
debugSerial.println("Initialisation terminee");

//  DS3231forcerSynchronisation();
debugSerial.println("Mise à l'heure");

debugSerial.println(F("-------------------------------- SETUP - Fin irq DS3231 -------------------------"));
 
  dht.begin();
  if  (!read_DHT(dht))  // initialise HiveSensor_Data.DHT_Hum et HiveSensor_Data.DHT_Temp
  {
    OLEDDebugDisplay("DHT Done");
    debugSerial.println("DHT Done");
  }
  else
  {
    OLEDDebugDisplay("DHT: Error");
    debugSerial.println("DHT: Error"); 
  } 

debugSerial.println(F("-------------------------------- SETUP - Fin DHT --------------------------------"));

  for ( int z=0;z<10;z++)
  {
    HiveSensor_Data.Bat_Voltage=getVBatMoy();   // calcul moyenne de 10 lectures
    HiveSensor_Data.Solar_Voltage=getVSolMoy();   // calcul moyenne de 10 lectures
  }

debugSerial.println(F("-------------------------------- SETUP - Fin init tensions Moyennes Bat/Sol -----"));

  OLEDDebugDisplayReset();
  buildLoraPayload();
  sendLoRaPayload((uint8_t*)payload,sizeof(payload)); //19);   // hex
debugSerial.println(F("-------------------------------- SETUP - Send Payload Done ----------------------"));

// Activer les interruptions
  rtc.writeSqwPinMode(DS3231_OFF); 
  
// Réactiver les interruptions
    interrupts();
debugSerial.println("Start loop(); =====================================");
}

// ---------------------------------------------------------------------------*
// ===== LOOP PRINCIPAL =====
// ---------------------------------------------------------------------------*
void loop() 
{  static int index=0; 
   static int counter1s=0;   

      loopWDT  = millis();
//debugSerial.println("M");  // MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  gererLEDsNonBloquant();        // *** APPEL OBLIGATOIRE À CHAQUE CYCLE ***

// Vérification du mode
  modeExploitation = digitalRead(PIN_PE);  
  if (modeExploitation)         // OK, validé, GreenLED => couleur Red
  {
    delay(10);  // anti rebond ILS/Switch
    handleOperationMode();    // normalement rien à faire dans ce mode.
// Envoi du payLoad si ISR2 

// ---------------------------------------------------------------------------*
// Gestion Clignotement LED Rouge non bloquant
// ---------------------------------------------------------------------------*
    if (wakeup1Sec) 
    {  
      wakeup1Sec = false; 
// attention     if (wakeup1Sec)   retseté plus bas!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      
      config.applicatif.redLedDuration = 100;  // Clignotement Rouge = 100 ms
      LEDStartRed();                           //Clignotement 100ms
//debugSerial.println("BlinkRED");   
// FIN Gestion Clignotement LED non bloquant 
    
// Gestion des affichages de pages rafraichies    
// FIN Gestion des affichages de pages rafraichies        
    }  
  } 
  else                          // OK, validé, BlueLED modeProgrammation
  {
// seulement en PROG  
// ---------------------------------------------------------------------------*
// Gestion clavier non bloquant
// ---------------------------------------------------------------------------*
  processContinuousKeyboard();      // *** TRAITEMENT CLAVIER NON-BLOQUANT ***
  touche = readKeyNonBlocking();  // lecture du clavier
  if (touche != KEY_NONE)         // Affiche touche pressée (KEY1..5, KEY_INVALID)
  {
    // Traiter la touche
    sprintf(serialbuf,"Touche pressée: %s",keyToString(touche));
    debugSerial.println(serialbuf);
  }
#ifdef __SerialDebugPoc
debugSerial.print("2");   // 22222222222222222222222
#endif  
// seulement en PROG  sans envoi Payload, contrôle process
// Affiche l'heure du prochain PayLoad si displayNextPayload == true     
 if (displayNextPayload)
 {
   displayNextPayload = false;
   debugSerialPrintNextAlarm(nextPayload,2);
 }
#ifdef __SerialDebugPoc
debugSerial.print("3");   // 333333333333333333333333333
#endif  

// seulement en PROG  
// ---------------------------------------------------------------------------*
// ISR1 => Lecture Cyclique des paramètres répartis à chaque seconde
// ---------------------------------------------------------------------------*
  if (wakeup1Sec) // && !modeExploitation)            // màj heure, 
  {    
    loopWDT  = millis();
//#ifdef __SerialDebugPoc     
//debugSerial.print("I1$ WDT:");   //   I1$ I1$ I1$ I1$ I1$ I1$ I1$ I1$
//debugSerial.println(loopWDT);
//#endif
    wakeup1Sec = false;   
    counter1s++;
    
// rappel toutes les minutes
    if (!(counter1s %60)) 
    {     
      debugSerialPrintNextAlarm(nextPayload,2);
//debugSerial.print("menuDeep: ");    // MMMMMMMMMMMMMMMMMMMMMMMMMMM
//debugSerial.println(currentMenuDepth);
    }
    switch (counter1s %10)
    {
      case 0 :    // Rafraichir OLED
  //             OLEDDisplayHivesDatas();  // pas quand saisies en cours....
//debugSerial.println("Case0");
               break;
      case 1 :   //  read_DHT(dht);  
// Reading temperature or humidity takes about 250 milliseconds!
//debugSerial.println("Case1");
               read_DHT(dht); // init: Data_LoRa.DHT_Temp, Data_LoRa.DHT_Hum
               break;
     case 2 :
//debugSerial.println("Case2");
               HiveSensor_Data.Brightness = getLuminance();
//  HiveSensor_Data.Lux = ???();
//  HiveSensor_Data.ProcessorTemp = getTemperature(); // lecture Temp en String
/*               HiveSensor_Data.Bat_Voltage=getVBatMoy();
*/              HiveSensor_Data.Solar_Voltage=getVSolMoy();
               break;
     case 3 :
//debugSerial.println("Case3"); 
//logPeson = true;
              if (Peson[config.materiel.Num_Carte][0])
              {
                Contrainte_List[0] = GetStrainGaugeAverage(0,1);
                HiveSensor_Data.HX711Weight[0] = calculePoids(0);
              }
// afficher poids bal(1) sur fenêtre OLEDdisplayWeightBal(void)
logPeson = false;
              break;
     case 4 :
//debugSerial.println("Case4"); 
//logPeson = true;
              if (Peson[config.materiel.Num_Carte][1])
              {
                Contrainte_List[1] = GetStrainGaugeAverage(1,1);
                HiveSensor_Data.HX711Weight[1] = calculePoids(1);
              }               
logPeson = false;
              break;
     case 5 :
//debugSerial.println("Case5");
//logPeson = true;
              if (Peson[config.materiel.Num_Carte][2])
              {
                Contrainte_List[2] = GetStrainGaugeAverage(2,1);
                HiveSensor_Data.HX711Weight[2] = calculePoids(2);
              }               
logPeson = false;
              break;
     case 6 :
//debugSerial.println("Case6");
logPeson = true;
              if (Peson[config.materiel.Num_Carte][3])
              {
                Contrainte_List[3] = GetStrainGaugeAverage(3,1);
                HiveSensor_Data.HX711Weight[3] = calculePoids(3);
              }               
logPeson = false;
              break;
     case 7 :
//debugSerial.println("Case7");
              readingT=getTemperature();
               break;
      case 8 :
//debugSerial.println("Case8");
              break;
      case 9 :      // Alive: '.' sur OLED
              debugSerial.print(".");
              break;
      default : // WTF
               debugSerial.print("WTF");  
               break;
    }
#ifdef __SerialDebugPoc      
 debugSerial.print("5");    // 55555555555555555555555555555555555
#endif 
// FIN ISR1 => Lecture Cyclique des paramètres répartis à chaque seconde

// ---------------------------------------------------------------------------*
// Gestion Clignotement LED Bleue non bloquant
// ---------------------------------------------------------------------------*
    config.applicatif.blueLedDuration = 100;  // Clignotement Blue = 100 ms
    LEDStartBlue();                           //Clignotement 100ms
// FIN Gestion Clignotement LED non bloquant



// mode rafraichissement rapide des balances
// attention le scale.begin  dure > 400 ms
// les 10 lectures 900 ms
debugSerial.println("BalRap A");
    if (InfoBalScreenRefreshBal_1)
    {
      Contrainte_List[0] = GetStrainGaugeFast(0);
      HiveSensor_Data.HX711Weight[0] = calculePoids(0);
    }
debugSerial.println("BalRap B");   
    if (InfoBalScreenRefreshBal_2)
    {
      Contrainte_List[1] = GetStrainGaugeFast(1);
      HiveSensor_Data.HX711Weight[1] = calculePoids(1);
    }   
debugSerial.println("BalRap C");            
    if (InfoBalScreenRefreshBal_3)
    {
      Contrainte_List[2] = GetStrainGaugeFast(2);
      HiveSensor_Data.HX711Weight[2] = calculePoids(2);
    }    
debugSerial.println("BalRap D");               
    if (InfoBalScreenRefreshBal_4)
    {
      Contrainte_List[3] = GetStrainGaugeFast(3);
      HiveSensor_Data.HX711Weight[3] = calculePoids(3);
    }  
 
 
// ---------------------------------------------------------------------------*
// Gestion rafraichissement écrans (System\INFOS, )
// ---------------------------------------------------------------------------*
    OLEDRefreshDisplay();
  }
// fin de ISR1
#ifdef __SerialDebugPoc  
debugSerial.print("6");   // 666666666666666666666666666666666666666666666
#endif

// ---------------------------------------------------------------------------*
// traite : 
// ---------------------------------------------------------------------------*
    handleProgrammingMode();  // faire gestion Clavier et actions associées
  }

#ifdef __SerialDebugPoc  
debugSerial.print("4");   // 444444444444444444444444444444
#endif

  if (!DEBUG_INTERVAL_1SEC)
  { static unsigned long noIRQSecLast=0;
    if (millis() - noIRQSecLast > 1000) // Clignotement toutes les 1s
    {
      noIRQSecLast = millis();
      wakeup1Sec = true;
    } 
//debugSerial.println("dans if (!DEBUG_INTERVAL_1SEC) /////////////////////////////////////////////////////////////////////////////////");    
  }
  
}
