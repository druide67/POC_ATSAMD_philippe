# Plan de travail — Ruches Connectees ESP32-S3

## MISE A JOUR 01/03/2026 — Migration ATSAMD21 -> ESP32-S3 Master/Slaves

Toutes les decisions architecturales ont ete tranchees avec Philippe le 01/03/2026.
Voir `docs/plan/IMPACT_MIGRATION_ESP32.md` pour le detail des 29 decisions.

## Architecture cible

```
1 Master ESP32-S3 (N16R8)          3 Slaves ESP32-S3
WiFi + BLE + LoRaWAN + Web         BLE + HX711 + OLED debug
BME280 + BH1750 + INA219           Deep sleep entre cycles
OLED + clavier + menus             Poids + VBat + timestamp
           |                              |
           +--- BLE GATT (15m) -----------+
           |
     LoRaWAN (SX1262)
           |
   Orange Live Objects
           |
   Decodeur Go (Scalingo)
           |
   Prometheus -> Grafana
```

## Vue d'ensemble des phases

```
Phase 0 (EN COURS)             Phase 1                    Phase 2
MIGRATION DE BASE              CAPTEURS + OLED            BLE MASTER/SLAVES

+- Structure PlatformIO [OK]   +- HX711 manager           +- Slave GATT server
+- types.h porte [OK]          +- DS3231 + EEPROM         +- Master GATT client
+- config.h (en cours)         +- OLED SSD1309 (U8g2)     +- Pairing passkey
+- Porter code portable        +- Clavier analogique      +- Cycle 3 slaves
+- Adapter drivers             +- BME280/BH1750/INA219    +- Deep sleep slaves
+- Power management            +- Menus calibration       +- Affichage OLED

Phase 3 (quand module recu)    Phase 4
LORAWAN                        WEB + ROBUSTESSE

+- RadioLib + SX1262           +- ESPAsyncWebServer
+- OTAA Orange Live Objects    +- API REST + WebSocket
+- Payload V2 (24 octets)     +- OTA WiFi
+- Persistance deep sleep      +- Watchdog
+- ADR + forcer SF             +- Logs LittleFS
```

**Backend : Orange Live Objects** (pas de TTN)
**Module LoRa : E22-868M22S (SPI)** — a commander (le E22-900T30D UART est incompatible RadioLib)

## Phase 0 — Migration de base

**Objectif** : repo PlatformIO fonctionnel, code portable porte, compilation OK.

### Fait

- [x] Structure repo (`src/common/`, `src/master/`, `src/slave/`, `include/`)
- [x] `platformio.ini` multi-env (master + slave compilent)
- [x] `.gitignore` (credentials, .pio, binaires)
- [x] `credentials.h` + `credentials_example.h`
- [x] `include/types.h` (porte depuis struct.h + SlaveReading_t)
- [x] `src/master/main.cpp` et `src/slave/main.cpp` (minimaux)

### A faire

- [ ] `include/config.h` — constantes et pins adaptes ESP32-S3
- [ ] `src/common/convert.cpp/h` — copie directe de Convert.cpp
- [ ] `src/common/saisies_nb.cpp/h` — copie + remap Serial
- [ ] `src/common/eeprom_manager.cpp/h` — copie + remap Serial
- [ ] `src/common/rtc_manager.cpp/h` — adapter ISR (IRAM_ATTR, flag only)
- [ ] Ajouter les lib_deps incrementalement dans platformio.ini
- [ ] Valider compilation des deux envs avec le code porte

### Validation

```bash
pio run -e master   # compile sans erreur
pio run -e slave    # compile sans erreur
```

## Phase 1 — Capteurs + OLED + menus

**Objectif** : lecture de tous les capteurs, affichage OLED, navigation menus.

### Master

- [ ] `src/common/hx711_manager.cpp/h` — 1 HX711, coefficients Jauge[]
- [ ] `src/common/rtc_manager.cpp/h` — alarmes DS3231 + reveil deep sleep
- [ ] `src/common/eeprom_manager.cpp/h` — config persistante AT24C32
- [ ] `src/common/display_manager.cpp/h` — U8g2 + SSD1309 (reecriture API)
- [ ] `src/common/keypad.cpp/h` — clavier ADC 12 bits
- [ ] `src/common/power_manager.cpp/h` — esp_sleep (reecriture complete)
- [ ] `src/master/sensor_manager.cpp/h` — BME280, BH1750, INA219
- [ ] `src/common/menus_common.cpp/h` — calibration, info
- [ ] `src/master/menus_master.cpp/h` — menus specifiques master

### Slave

- [ ] HX711 via hx711_manager (1 cellule 200 kg)
- [ ] VBat via ADC
- [ ] Affichage OLED simple (poids, VBat, horodatage)
- [ ] Menus calibration basiques

### Validation

- [ ] Master affiche T/HR/P/Lux/Poids/VBat/VSol sur OLED
- [ ] Slave affiche Poids/VBat sur OLED
- [ ] Clavier navigue dans les menus
- [ ] Deep sleep < 20 uA

## Phase 2 — BLE Master/Slaves

**Objectif** : communication BLE entre master et 3 slaves.

- [ ] `src/slave/ble_slave.cpp/h` — GATT server, 3 caracteristiques (poids, VBat, timestamp)
- [ ] `src/master/ble_master.cpp/h` — scan, connect, read, disconnect
- [ ] Pairing passkey statique
- [ ] Cycle : slave mesure -> BLE advertising 30s -> deep sleep 15 min
- [ ] Master agrege dans `SlaveReading_t slaveReadings[3]`

### Validation

- [ ] Master trouve et lit les 3 slaves
- [ ] Pairing persiste apres deep sleep
- [ ] Donnees slaves affichees sur OLED master

## Phase 3 — LoRaWAN (quand module E22-868M22S recu)

**Objectif** : envoi des donnees agregees vers Orange Live Objects.

- [ ] `src/master/lora_manager.cpp/h` — RadioLib + SX1262 via SPI
- [ ] OTAA join vers Orange Live Objects
- [ ] Construction payload V2 (24 octets : master + 3 slaves)
- [ ] Persistance session LoRaWAN apres deep sleep (NVS + RTC RAM)
- [ ] ADR active, option forcer SF via menu

### Validation

- [ ] Join OTAA reussi
- [ ] Payload decode correctement cote serveur
- [ ] Session survit au deep sleep
- [ ] ADR fonctionne

## Phase 4 — Serveur web + robustesse

**Objectif** : interface web, OTA, watchdog.

- [ ] `src/master/web_server.cpp/h` — ESPAsyncWebServer
- [ ] API REST : `/api/data`, `/api/config`, `/api/status`
- [ ] WebSocket temps reel
- [ ] Pages HTML/CSS/JS dans `data-master/` (LittleFS)
- [ ] OTA WiFi (ArduinoOTA ou AsyncElegantOTA)
- [ ] Watchdog ESP32
- [ ] Recovery I2C

### Validation

- [ ] Interface web accessible depuis smartphone
- [ ] OTA fonctionne
- [ ] Watchdog redemarre en cas de blocage

## Ordre de developpement

Le module LoRa SPI n'est pas encore recu. Ordre optimise :

```
IMMEDIAT (materiel disponible)          QUAND MODULE LORA RECU
------------------------------          ----------------------
Phase 0 : Setup + portage              Phase 3 : LoRaWAN
Phase 1 : Capteurs + OLED              Phase 4 : Web + robustesse
Phase 2 : BLE Master/Slaves
```

## Portabilite du code ATSAMD

| Categorie | % | Fichiers |
|-----------|---|----------|
| Reutilisable tel quel | ~40% | struct.h, Convert.cpp, Saisies_NB.cpp, Menus.cpp, 24C32.cpp, DS3231.cpp |
| Adaptable | ~30% | define.h, var.h, Mesures.cpp, Handle.cpp, MenusFonc.cpp, OLED.cpp, KEYPAD.cpp |
| A reecrire | ~30% | RN2483A.cpp, Power.cpp, ISR.cpp |

## Documentation associee

| Fichier | Description |
|---------|-------------|
| `CLAUDE.md` | Contexte projet pour Claude Code |
| `docs/plan/IMPACT_MIGRATION_ESP32.md` | Analyse d'impact + 29 decisions |
| `docs/20260220_memo_ruches_connectees_ESP.md` | Memo technique ESP32 (Philippe) |
| `docs/ANALYSE_BEEP.md` | Etude plateforme BEEP |
| `docs/plan/ANALYSE_REPO.md` | Audit code ATSAMD |
