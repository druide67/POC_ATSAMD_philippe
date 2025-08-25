// ===== PROTOTYPES DE FONCTIONS =====

// ===== PROTOTYPES DE FONCTIONS DE SETUP.CPP=====
void initDebugSerial(void);


// ===== PROTOTYPES DE FONCTIONS DE POC_ATSAMD.INO=====
// Gestion RTC et temps
void initRTC(void);
DateTime getSystemTime(void);
void setRTCAlarms(void);
void clearRTCAlarms(void);
void printTimeComparison(void);
void printTimeOndebugSerial(void);
void print2digits(int number);
void forcerSynchronisationDS3231(void);
void synchronizeDS3231TimeToMicro(void);
void copyDS3231TimeToMicro(bool forcer);
void checkRTCStatus(void);

// Gestion Config et EEPROM
void initConfig(void);
void loadConfigFromEEPROM(void);
void readConfigFromEEPROM(void);
void saveConfigToEEPROM(void);
uint16_t calculateChecksum(ConfigGenerale_t* cfg);
void setDefaultConfig(void);

// Gestion LEDs
void initLEDs(void);
void turnONRedLED(void);   // sort vert sur PCB
void turnOffRedLED(void);  // sort vert sur PCB
void blinkRedLED(void); // 300ms; sort vert sur PCB
void turnOnGreenLED(void);   // sort rouge sur PCB
void turnOffGreenLED(void);  // sort rouge sur PCB
void blinkGreenLED(void); // 300ms; sort rouge sur PCB
void turnOnBlueLED(void); 
void turnOffBlueLED(void);
void blinkBlueLED(void); // 300ms
void turnOnBuiltinLED(void);
void turnOffBuiltinLED(void);
void blinkBuiltinLED(void);

// Gestion clavier
key_code_t readKeyOnce(void);
key_code_t readKey(void);
const char* keyToString(key_code_t key);
key_code_t readKeyNonBlocking(void);
void processContinuousKeyboard(void);

// Gestion OLED
void OLEDInit(void);
void OLEDInit096(void);
void OLEDInit130(void);
void OLEDInit154(void);
void OLEDClear(void);
void nonprintOLED(uint8_t ligne, uint8_t colonne, const char *message);
void OLEDClearLine(uint8_t ligne);
void OLEDPrintChar(uint8_t ligne, uint8_t colonne, char c);
void OLEDPrintVar(uint8_t ligne, uint8_t col, const void *valeur, char type);
void OLEDPrintFormatted(uint8_t ligne, uint8_t col, const void *valeur, char type, const char *unite, int precision, char align);
void OLEDDisplayMessageL8(const char* message, bool defilant, bool inverse);
void OLEDDebugDisplay(char* message);
void OLEDDebugDisplayReset(void);
void OLEDSetDebug(bool actif);
void OLEDDrawScreenNextPayload(uint8_t ligne, uint8_t colonne, DateTime nextPayload );
void OLEDDrawScreenTime(uint8_t ligne, uint8_t colonne);        // sous format: hh:mm:ss - dd/mm/yyyy
void OLEDDrawScreenRefreshTime(uint8_t ligne, uint8_t colonne); // que les valeurs modifiées
void OLEDDrawText(int8_t Txt_Size, uint8_t ligne, uint8_t colonne,const char *text);
void OLEDEraseText(int16_t col, int16_t lig, int16_t Ncar);
void OLEDDisplayDate(char *d, uint8_t pos);
void OLEDDisplayTime(char *h, uint8_t pos);

// Gestion saisies
bool isDateValid(const char *d);
void modifyDateDigit(char *d, uint8_t pos, int delta);
void inputDate(char *d);
bool isTimeValid(const char *h);
void modifyTimeDigit(char *h, uint8_t pos, int delta);
void inputTime(char *h);
void inputListValue(const char *label, const int *liste, uint8_t nbValeurs, int *valeurSelectionnee);
void inputListValueLibelle(const char *label, const int *valeurs, const char **libelles, uint8_t nbValeurs, int *valeurSelectionnee, bool *valide);
uint64_t inputHex(const char* variable, uint64_t valeurInitiale);
uint32_t inputDecimal(const char* variable, uint32_t valeurInitiale);

// Gestion serialDebug 
void debugSerialPrintLoraPayload(uint8_t *payload, uint8_t len);
void debugSerialPrintText(char *txt, char len);
void debugSerialPrintLoRaStatus();
void debugSerialPrintNextAlarm(DateTime nextPayload, int IRQ);
void debugSerialPrintKbdKey(void);

// Gestion modes et interruptions
void handleOperationMode(void);
void executeOperationMode(void);
void handleProgrammingMode(void);
void executeProgrammingMode(void);
void testConnexionDS3231(void);
void forceTestAlarm(void);

void onRTCAlarm(void);

// Gestion modes basse consommation
void sleep(void);
void configureLowPowerMode(void);

// Gestion RN2483 LoRa
void init2483A(void);
void initLoRa(void);
void clearLoRaBuffer(void);
String readLoRaResponse(int timeoutMs);
void getHWEUI(char *); 
bool setupLoRa(void);
bool setupLoRaOTAA(void);
void Send_DATA_LoRa(void);
void Send_LoRa_Mess(uint8_t *, uint8_t);
uint8_t Init_2483(void);
void Reset_LoRa(void);
void BuildLoraPayload(void);
void sleep_LoRa(void);
void wake_LoRa(void);

// Gestion menus
void displaySystemInfo(void);
void handleConfigurationMenu(void);
void handleTimeDateRegisterMenu(void);
void handleDebugMenu(void);
