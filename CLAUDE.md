# CLAUDE.md — POC_ATSAMD : Surveillance de Ruches

## Identité du projet

Firmware embarqué C++ (Arduino) pour carte **SODAQ Explorer** (ATSAMD21J18A, Cortex-M0+) destiné à la surveillance de ruches apicoles. Collecte poids (HX711 ×4), température/humidité (DHT22), luminosité (LDR), tensions batterie LiFePO4 et panneau solaire, puis transmet les données via **LoRaWAN** (module Microchip RN2483A) vers une passerelle Orange Live Objects.

## Contraintes fondamentales

**Ce firmware tourne sur un microcontrôleur à ressources limitées.** Chaque décision de code doit être évaluée à travers ces contraintes :

- **RAM** : 32 Ko SRAM — pas d'allocation dynamique en production, pas de `String`, pas de `new`/`malloc`
- **Flash** : 256 Ko — surveiller la taille du binaire après chaque modification
- **Consommation** : objectif < 50 µA en deep sleep, budget énergie batterie+solaire
- **Fiabilité** : fonctionne en extérieur sans supervision humaine pendant des semaines
- **Temps réel** : la loop() ne doit jamais bloquer (pas de `delay()` en production)
- **LoRaWAN** : payload limité à ~51 octets en SF12, duty cycle réglementaire 1%

## Architecture matérielle

```
SODAQ Explorer (ATSAMD21J18A)
├── I²C bus
│   ├── DS3231 RTC + EEPROM 24C32 (0x68 / 0x57)
│   └── OLED SH1106 128×64 (0x3C)
├── Serial2 → RN2483A (LoRa module)
├── GPIO
│   ├── HX711 ×4 (SCK partagé pin 3, DOUT pins 4/6/8/10)
│   ├── DHT22 (pin 5)
│   ├── Pin mode exploitation/programmation (BUTTON)
│   └── Alarme RTC (pin 2, interrupt FALLING)
├── ADC
│   ├── A0 : LDR (luminosité)
│   ├── A1 : VBat (pont diviseur)
│   ├── A2 : VSol (pont diviseur)
│   └── A3 : Clavier analogique 5 touches
└── Alimentation : LiFePO4 + panneau solaire + régulateur
```

## Toolchain

- **IDE** : VS Code + PlatformIO
- **Build** : `pio run -e sodaq_explorer`
- **Upload** : `pio run -e sodaq_explorer -t upload`
- **Monitor série** : `pio device monitor -b 57600`
- **Projets de référence** : voir `docs/REFERENCES_PROJETS.md`

## Organisation du code (état actuel)

Tous les fichiers sont à la racine. Un fichier `define.h` chaîne tous les includes.
Le pattern `#ifdef __MAIN__` / `#else extern` dans `var.h` gère les déclarations/extern.

|Fichier                      |Rôle                                           |Dépendances clés                 |
|-----------------------------|-----------------------------------------------|---------------------------------|
|`POC_ATSAMD.ino`             |setup() + loop()                               |Tout                             |
|`Handle.cpp`                 |Machine à états exploitation/programmation     |ISR flags, Mesures, RN2483A, OLED|
|`RN2483A.cpp`                |Communication LoRaWAN (OTAA, payload, send)    |Sodaq_RN2483, config             |
|`Mesures.cpp`                |Acquisition capteurs (HX711, DHT, ADC)         |HX711, DHT, config               |
|`Power.cpp`                  |Deep sleep / réveil                            |LowPower, LoRaBee, display       |
|`ISR.cpp`                    |Handlers interruption RTC alarmes 1 et 2       |rtc, flags volatile              |
|`DS3231.cpp`                 |Pilote RTC (alarmes, synchro)                  |RTClib                           |
|`24C32.cpp`                  |Config persistante EEPROM                      |Wire, ConfigGenerale_t           |
|`OLED.cpp`                   |Affichage écran                                |Adafruit_SH110X                  |
|`Saisies_NB.cpp`             |Saisies non-bloquantes (num, alpha, hex, date…)|OLED, clavier                    |
|`Menus.cpp` / `MenusFonc.cpp`|Navigation menus + actions associées           |Saisies, OLED, config            |
|`setup.cpp`                  |Initialisations matérielles                    |Tous les drivers                 |

## Conventions de code existantes

- **Style d'accolades** : alignement vertical (Allman)
- **Indentation** : 2 espaces
- **Noms** : fonctions et variables en camelCase, `#define` en MAJUSCULES
- **Préfixes fichier** : chaque fonction est préfixée du nom de son module (ex: `RN2483AsendLoRaPayload`, `E24C32loadConfig`, `DS3231setRTCAlarm1`)
- **Commentaires** : en français avec accents
- **Textes affichés (OLED/série)** : français SANS accents
- **Debug** : `#define debugSerial SerialUSB`, macros `LOG_INFO()`, `LOG_DEBUG()`, etc.
- **Prototypes** : centralisés dans `prototypes.h`

## Structures de données critiques

```cpp
// Mesures capteurs — rempli par acquisition, lu par build payload
HiveSensor_Data_t HiveSensor_Data;

// Configuration persistante (EEPROM) — NE JAMAIS modifier la struct sans incrémenter CONFIG_VERSION
ConfigGenerale_t config; // contient .magicNumber, .applicatif, .materiel, .checksum

// Identifiants des cartes physiques — tables de lookup par Num_Carte
HWEUI_List[], SN2483_List[], AppKey_List[], Peson[][], Jauge[][]
```

## Flags d'interruption (ISR ↔ loop)

```cpp
volatile bool wakeup1Sec;        // Alarme 1 : tick 1s (mode programmation)
volatile bool wakeupPayload;     // Alarme 2 : cycle mesure+envoi
volatile bool alarm1_enabled;    // Verrou : désactivé pendant traitement payload
volatile bool alarm2_enabled;    // Verrou alarme 2
volatile bool modeExploitation;  // État du pin physique mode
```

## Règles pour Claude Code

### Ce qu'il FAUT faire

- Toujours vérifier la taille du binaire après modification (`pio run` → section .text + .data)
- Tester la compilation avant de committer (`pio run -e sodaq_explorer`)
- Documenter chaque fonction : description, paramètres, retour (en français)
- Utiliser des types à taille fixe (`uint8_t`, `int16_t`, `float`) — jamais `int` seul
- Préférer `snprintf` à `sprintf` (buffer overflow protection)
- Marquer `volatile` toute variable partagée ISR ↔ loop
- Mettre les constantes en `PROGMEM` / `const` quand possible

### Ce qu'il NE FAUT PAS faire

- **Pas de `String` Arduino** (fragmentation heap) — utiliser `char[]` + `snprintf`
- **Pas de `delay()` dans loop()** — tout doit être non-bloquant
- **Pas d'allocation dynamique** (`new`, `malloc`, `std::vector`) — mémoire statique uniquement
- **Pas de `Serial.print` dans les ISR** — les macros LOG_* actuelles dans ISR.cpp sont un bug connu à corriger
- **Ne pas modifier `ConfigGenerale_t` sans incrémenter `CONFIG_VERSION`** et gérer la migration EEPROM
- **Ne pas toucher aux clés LoRa (AppKey, AppEUI)** — ce sont des secrets même si en clair dans le code

### Workflow de modification

1. Créer une branche depuis `main`
1. Modifier le code
1. Compiler : `pio run -e sodaq_explorer`
1. Vérifier taille : la flash ne doit pas dépasser 240 Ko (marge bootloader)
1. Si possible, tester sur carte physique
1. Commit avec message descriptif en français
1. PR vers main

## Objectifs d'évolution (par priorité)

1. **Phase 0** — Assainissement : structuration repo, suppression code mort, réduction globales
1. **Phase 1** — Abstraction radio : couche comms, payload extensible, file d'attente
1. **Phase 2** — Fédération LoRa : mode P2P inter-nœuds, agrégation, scheduling
1. **Phase 3** — Robustesse production : CI/CD, watchdog, OTA, logs locaux

## Bibliothèques utilisées (versions figées)

|Lib               |Version              |Usage                       |
|------------------|---------------------|----------------------------|
|RTClib            |2.1.4 (Adafruit)     |DS3231 RTC                  |
|ArduinoLowPower   |1.2.1                |Deep sleep SAMD             |
|Adafruit_SH110X   |2.1.13               |OLED 1.3"                   |
|Adafruit_SSD1306  |2.5.15               |OLED 0.96" (alternatif)     |
|Sodaq_RN2483      |1.1.0                |Module LoRa RN2483A         |
|Sodaq_wdt         |1.0.2                |Watchdog timer              |
|CayenneLPP        |1.4.0                |Encodage LoRa (peu utilisé) |
|DHT sensor library|1.3.7 (Adafruit)     |DHT22                       |
|OneWire           |2.3.5                |DS18B20 (prévu, pas utilisé)|
|HX711             |0.7.2 (Bogdan Necula)|Cellules de charge          |

## Carte des prototypes physiques

|Num_Carte|DevEUI          |Localisation       |Pesons (A,B,C,D)|
|---------|----------------|-------------------|----------------|
|1        |0004A30B0020300A|HS / récupéré      |0,0,0,J17       |
|2        |0004A30B0024BF45|Non connecté Orange|J13,J08,J09,0   |
|3        |0004A30B00EEEE01|Ruches Loess       |J06,J09,J03,J08 |
|4        |0004A30B00EEA5D5|Ruches Verger      |0,J18,0,0       |
|5        |0004A30B00F547CF|Cave               |J19,J21,J14,J17 |
