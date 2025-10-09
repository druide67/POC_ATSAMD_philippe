#ifdef __MAIN__
// Penser à les déclarer en extern


// Exemple de liste de valeurs alphanumériques
// Définition des menus
const char* menu000Demarrage[] = {
  "LISTE_MENU0       ",    // 0 : Libre
  "Page INFOS     (P)",    // 1 : Infos produit => OLEDdisplayInfoScreen();
  "CONFIG. SYSTEME(F)",    // 2 : Date, Time, N° LoRa
  "CONNEX. RESEAU (F)",    // 3 : Lora: DevEUI, AppEUI, SF, Délai Payload
  "CALIB. Volt (M040)",    // 4 : VBat, Vsol, Lum => menu040CalibTensions
  "CALIB. BALANCES(F)",    // 5 : Bal1, Bal2, Bal3, Bal4    puis sous menu ou fonction Tare, Echelle, Comp T° par Bal
  "SAISIE DATE    (S)",    // 6 : test rapide date
  "SAISIE HEURE   (S)",    // 7 : Test rapide Time
  "SAISIE HEXA    (S)"     // 8 : Libre
};

const char* menu040CalibTensions[] = {
  "Calib. VBAT    (F)",      // Mise à Echelle VBat
  "Calib. VSOL    (F)",      // Mise à Echelle VSol
  "Calib. LUM     (F)",      // Mise à Echelle VLum
  "Reserve     (M043)",    // Libre
  "RET  popMenu(M000)"     // Rertour menu principal
};


const char* menu043Reserve040[] = {
  "menu043-0       (F)",      // Mise à Echelle VBat
  "menu043-1       (F)",      // Mise à Echelle VSol
  "menu043-2       (F)",      // Mise à Echelle VLum
  "menu043-3       (F)",    // Libre
  "RET   popMenu(M040)"     // Rertour menu principal
};



#else



// Exemple de liste de valeurs alphanumériques
extern const char* menu000Demarrage[];
extern const char* menu040CalibTensions[];
extern const char* menu043Reserve040[];

// Exemple de Menus
/*
extern const char* menu00ListeValeurs[];
extern const char* menu0ListeValeurs[];
extern const char* menu11ListeValeurs[];
extern const char* menu21ListeValeurs[];
*/

#endif
