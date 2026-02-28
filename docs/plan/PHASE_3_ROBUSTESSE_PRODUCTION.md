# Phase 3 — Robustesse production, CI/CD et sécurité

**Objectif** : transformer le POC en firmware fiable pour du déploiement terrain long terme.
**Durée estimée** : progression continue, parallélisable avec Phase 2
**Prérequis** : Phase 0 terminée (structure propre), Phase 1 souhaitée
**Risque** : faible à moyen selon les sous-tâches

-----

## Étape 3.1 — CI/CD avec GitHub Actions

### Ce qu'on va faire

Ajouter un workflow GitHub Actions qui s'exécute à chaque push et PR :

```yaml
# .github/workflows/build.yml
name: Build Firmware
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with: { python-version: '3.11' }
      - run: pip install platformio
      - run: pio run -e sodaq_explorer
      - name: Check binary size
        run: |
          SIZE=$(stat -c%s .pio/build/sodaq_explorer/firmware.bin)
          echo "Firmware size: $SIZE bytes"
          if [ $SIZE -gt 245760 ]; then
            echo "ERROR: firmware too large (>240KB)"
            exit 1
          fi
      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: firmware-${{ github.sha }}
          path: .pio/build/sodaq_explorer/firmware.bin
```

### Extensions possibles

- **Analyse statique** avec `cppcheck` ou `clang-tidy`
- **Rapport de taille par section** (`.text`, `.data`, `.bss`) pour détecter les régressions RAM/Flash
- **Compilation multi-cibles** si plusieurs variantes de cartes

### ❓ Questions pour ton cousin

1. **A-t-il un compte GitHub Pro** (ou gratuit) ? Les Actions sont gratuites pour les repos publics, limitées pour les privés. Son repo est public.
1. **Veut-il garder le repo public ?** Les clés LoRa (AppKey) sont en clair dans `var.h`. Si le repo reste public, il faudrait externaliser les clés dans un fichier non versionné (`.gitignore`).

-----

## Étape 3.2 — Watchdog matériel

### Le problème

Le code actuel n'a **pas de watchdog actif**. Si la loop() se bloque (I²C stuck, HX711 non répondant, LoRa timeout infini), le firmware est mort jusqu'au prochain reset manuel — inacceptable pour un dispositif en extérieur sans supervision.

### Ce qu'on va faire

```cpp
// Dans setup() — activer le watchdog Sodaq
#include <Sodaq_wdt.h>
sodaq_wdt_enable(WDT_PERIOD_8X);  // 8 secondes de timeout

// Dans loop() — nourrir le watchdog à chaque cycle
void loop() {
  sodaq_wdt_reset();  // doit être appelé au moins toutes les 8s
  // ... reste du code
}

// Avant deep sleep — le watchdog est automatiquement désactivé par LowPower
// Après réveil — le réactiver si nécessaire
```

### Attention aux opérations longues

Certaines opérations actuelles risquent de déclencher le watchdog :

- **HX711 `read_average(10)`** : peut prendre >1s si le capteur ne répond pas
- **LoRa `send()`** : peut bloquer jusqu'à 10s en SF12
- **DHT22 `read()`** : jusqu'à 2s

Il faut ajouter des `sodaq_wdt_reset()` dans les boucles d'attente, ou utiliser `sodaq_wdt_safe_delay()` au lieu de `delay()`.

### ❓ Questions pour ton cousin

1. **A-t-il déjà eu des blocages en production ?** Si oui, sur quels capteurs/opérations ? Ça guidera le placement des resets watchdog.
1. **Que doit-il se passer après un reset watchdog ?** Redémarrage complet (setup + join LoRa) ou reprise rapide ? La lib Sodaq_wdt permet de distinguer un reset watchdog d'un reset normal.

-----

## Étape 3.3 — Robustesse des bus I²C/SPI

### Les risques connus

Le bus I²C est le maillon faible du système : si un périphérique (OLED, RTC, EEPROM) bloque la ligne SDA en bas, **tout le bus est mort** et Wire.h n'a pas de recovery automatique.

### Ce qu'on va faire

```cpp
// Recovery I²C — à appeler si Wire.endTransmission() retourne une erreur
void i2cRecover(void)
{
  // Forcer 9 clocks sur SCL pour débloquer un esclave stuck
  Wire.end();
  pinMode(SCL, OUTPUT);
  for (int i = 0; i < 9; i++) {
    digitalWrite(SCL, LOW);
    delayMicroseconds(5);
    digitalWrite(SCL, HIGH);
    delayMicroseconds(5);
  }
  // Condition STOP
  pinMode(SDA, OUTPUT);
  digitalWrite(SDA, LOW);
  delayMicroseconds(5);
  digitalWrite(SCL, HIGH);
  delayMicroseconds(5);
  digitalWrite(SDA, HIGH);
  // Réinitialiser Wire
  Wire.begin();
}

// Wrapper pour les lectures I²C avec retry
uint8_t i2cReadWithRetry(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t len, uint8_t retries)
{
  for (uint8_t i = 0; i < retries; i++) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission() == 0) {
      if (Wire.requestFrom(addr, len) == len) {
        for (uint8_t j = 0; j < len; j++) data[j] = Wire.read();
        return 0;  // succès
      }
    }
    i2cRecover();
    delay(10);
  }
  return 1;  // échec après retries
}
```

### ❓ Question pour ton cousin

1. **A-t-il déjà eu des problèmes I²C** (OLED figé, RTC non lu) ? Surtout par temps froid ou après un réveil deep sleep ?

-----

## Étape 3.4 — Logs locaux et diagnostics

### Le problème

Actuellement, le debug passe exclusivement par `SerialUSB`. En production terrain, il n'y a pas de port série connecté. Si quelque chose ne marche pas, c'est invisible.

### Options de stockage

|Support                |Capacité|Survie reset|Complexité |Conso        |
|-----------------------|--------|------------|-----------|-------------|
|EEPROM 24C32 (existant)|4 Ko    |✅           |Faible     |Négligeable  |
|RAM circular buffer    |2-4 Ko  |❌           |Très faible|Aucune       |
|Carte SD (si câblée)   |Illimité|✅           |Moyenne    |~100 µA actif|
|Flash SPI externe      |2-16 Mo |✅           |Moyenne    |~1 µA standby|

### Proposition : log minimal en EEPROM

Utiliser les derniers Ko de l'EEPROM 24C32 (4 Ko total, la config utilise ~200 octets) pour un log circulaire de diagnostics :

```cpp
typedef struct __attribute__((packed)) {
  uint32_t timestamp;     // epoch ou uptime
  uint8_t  eventType;     // BOOT, LORA_FAIL, I2C_ERR, WDT_RESET, LOW_BAT...
  uint8_t  detail;        // code d'erreur spécifique
} LogEntry_t;             // 6 octets

// ~600 entrées dans 3.6 Ko → ~2 jours de log à 5 min/cycle
```

Ces logs seraient lisibles via le menu OLED ("Système > Logs") ou via un upload bulk quand un technicien connecte le port série.

### ❓ Questions pour ton cousin

1. **Espace EEPROM disponible** : quelle taille fait sa config en EEPROM actuellement ? Il reste combien de Ko libres ?
1. **La carte SD est-elle câblée ?** Si oui, c'est la meilleure option pour le logging (illimité, format CSV lisible).
1. **Quels événements veut-il logger ?** Suggestions : boot/reset, échec LoRa, erreur I²C, erreur capteur, batterie faible, changement de mode, watchdog trigger.

-----

## Étape 3.5 — Sécurité minimale

### Problèmes actuels

- **Clés LoRa en clair dans le code source** (et dans un repo public)
- **Pas de chiffrement des messages P2P** (Phase 2)
- **Pas de mécanisme OTA sécurisé**
- **Pas d'authentification** pour la configuration via menu

### Priorités réalistes pour un POC

|Mesure                              |Priorité|Effort            |Impact                      |
|------------------------------------|--------|------------------|----------------------------|
|Externaliser les clés du code source|★★★     |30 min            |Empêche le vol de clés      |
|CRC/intégrité messages P2P          |★★★     |Déjà prévu Phase 1|Détecte corruption          |
|Chiffrement P2P (AES-128)           |★★☆     |2h                |Empêche l'écoute entre nœuds|
|Vérification intégrité firmware     |★★☆     |1h                |Détecte corruption flash    |
|OTA sécurisé                        |★☆☆     |Complexe          |Mise à jour à distance      |

### Externalisation des clés

```cpp
// credentials.h — ce fichier est dans .gitignore et n'est JAMAIS commité
#ifndef CREDENTIALS_H
#define CREDENTIALS_H

const uint8_t MY_APP_EUI[8] = { 0x41, 0x42, ... };
const uint8_t MY_APP_KEY[16] = { 0x48, 0x4F, ... };

#endif
```

```cpp
// credentials_template.h — ce fichier EST commité comme exemple
#ifndef CREDENTIALS_H
#define CREDENTIALS_H

// Remplacer par vos vraies clés
const uint8_t MY_APP_EUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t MY_APP_KEY[16] = { 0x00, 0x00, ... };

#endif
```

### ❓ Questions pour ton cousin

1. **Est-il conscient que ses AppKey sont publiques ?** Quelqu'un pourrait usurper un de ses nœuds sur Orange Live Objects. Veut-il régénérer les clés après la migration ?
1. **A-t-il besoin d'OTA** (mise à jour firmware à distance) ? Sur le RN2483, il n'y a pas de mécanisme OTA natif. Il faudrait passer par le bootloader USB ou un mécanisme custom via LoRaWAN downlink (très complexe, probablement hors scope POC).

-----

## Étape 3.6 — Gestion adaptative de l'énergie

### Améliorer le deep sleep actuel

```cpp
// AVANT (Power.cpp actuel)
void sleep(void) {
  delay(5000);  // ❌ 5 secondes de gaspillage !
  scaleA.power_down();
  display.oled_command(SH110X_DISPLAYOFF);
  LoRaBee.sleep();
  USB->DEVICE.CTRLA.bit.ENABLE = 0;
  LowPower.sleep();
  // ... réveil
}

// APRÈS — sleep optimisé
void enterDeepSleep(void) {
  // 1. Éteindre les périphériques externes
  for (int i = 0; i < 4; i++) scales[i].power_down();
  display.oled_command(SH110X_DISPLAYOFF);
  LoRaBee.sleep();

  // 2. Configurer les pins inutilisées en INPUT_PULLUP (réduit la conso)
  configurePinsForSleep();

  // 3. Désactiver les périphériques SAMD inutilisés
  PM->APBCMASK.reg &= ~PM_APBCMASK_ADC;     // ADC off
  PM->APBCMASK.reg &= ~PM_APBCMASK_TC3;     // Timer off
  // ... autres périphériques

  // 4. Désactiver USB
  USB->DEVICE.CTRLA.bit.ENABLE = 0;

  // 5. Debug flush (sans delay !)
  debugSerial.flush();

  // 6. Dormir
  LowPower.sleep();

  // === RÉVEIL ===

  // 7. Réactiver les périphériques
  PM->APBCMASK.reg |= PM_APBCMASK_ADC;
  USB->DEVICE.CTRLA.bit.ENABLE = 1;
  LoRaBee.wakeUp();
  display.oled_command(SH110X_DISPLAYON);
  delay(100);  // stabilisation hardware
}
```

### Mode adaptatif selon la batterie

```cpp
// Ajuster la période d'envoi selon le niveau de batterie
uint16_t getAdaptiveSendPeriod(float vbat) {
  if (vbat > 3.4)  return config.applicatif.SendingPeriod;     // normal
  if (vbat > 3.2)  return config.applicatif.SendingPeriod * 2;  // économie
  if (vbat > 3.0)  return config.applicatif.SendingPeriod * 4;  // survie
  return 0;  // batterie critique → arrêt des envois, sleep maximum
}
```

### ❓ Questions pour ton cousin

1. **Quel est le seuil de tension critique pour sa batterie LiFePO4 ?** LiFePO4 a une courbe de décharge très plate autour de 3.2V, puis chute rapidement sous 3.0V. À quel voltage considère-t-il que la batterie est vide ?
1. **Veut-il un mode "hibernation"** où le firmware arrête tout sauf les mesures de tension pour protéger la batterie, et redémarre automatiquement quand le solaire recharge suffisamment ?

-----

## Checklist de validation Phase 3

- [ ] GitHub Actions : build automatique à chaque push
- [ ] GitHub Actions : vérification taille binaire
- [ ] Watchdog activé et testé (vérifier qu'il ne se déclenche pas en fonctionnement normal)
- [ ] Recovery I²C implémenté
- [ ] Clés LoRa externalisées du code source
- [ ] `.gitignore` mis à jour pour exclure `credentials.h`
- [ ] Log diagnostic minimal en EEPROM (ou SD)
- [ ] Deep sleep optimisé (suppression delay(5000), pins configurées)
- [ ] Consommation mesurée en deep sleep (objectif < 50 µA)
- [ ] Mode adaptatif batterie implémenté

-----

## Récapitulatif des questions Phase 3

|# |Question                                 |Impact               |
|--|-----------------------------------------|---------------------|
|1 |Compte GitHub Pro ou gratuit ?           |CI/CD                |
|2 |Repo public → clés exposées, acceptable ?|**Sécurité critique**|
|3 |Blocages déjà observés en production ?   |Watchdog             |
|4 |Comportement après reset watchdog ?      |Recovery             |
|5 |Problèmes I²C observés ?                 |Robustesse bus       |
|6 |Espace EEPROM libre ?                    |Logs                 |
|7 |Carte SD câblée et fonctionnelle ?       |Logs                 |
|8 |Événements à logger ?                    |Diagnostics          |
|9 |Conscient que les AppKey sont publiques ?|Sécurité             |
|10|Besoin d'OTA ?                           |Complexité           |
|11|Seuil tension batterie critique ?        |Énergie              |
|12|Mode hibernation souhaité ?              |Autonomie            |
