# Analyse du Repository POC_ATSAMD

**Repo** : github.com/pleuh67/POC_ATSAMD
**Date d'analyse** : 26 février 2026
**Objectif** : état des lieux avant évolution (fédération des envois LoRa)

-----

## 1. Vue d'ensemble

Le projet est un firmware Arduino (.ino) pour carte **SODAQ Explorer** (ATSAMD21J18) destiné à la surveillance de ruches. Il collecte poids (HX711 × 4), température/humidité (DHT22), luminosité (LDR), tensions batterie/solaire, et transmet les données via un module LoRa **RN2483A**.

|Métrique                      |Valeur             |
|------------------------------|-------------------|
|Lignes de code (C++/H)        |~16 200            |
|Fichiers source (.cpp)        |19                 |
|Headers (.h)                  |7                  |
|Commits                       |29 (au 26/02/2026) |
|Branches                      |1 (main uniquement)|
|Fichiers binaires dans le repo|2 (.bin)           |

### Carte des fichiers et leurs rôles

|Fichier          |Lignes|Rôle                                                                   |
|-----------------|------|-----------------------------------------------------------------------|
|`POC_ATSAMD.ino` |447   |Point d'entrée : setup() + loop() principal                            |
|`Saisies_NB.cpp` |4 364 |Saisies non-bloquantes (numérique, alpha, hexa, date, heure, email, IP)|
|`Convert.cpp`    |1 907 |Conversions de formats (hex, byte arrays, etc.)                        |
|`MenusFonc.cpp`  |1 141 |Fonctions associées aux menus (actions)                                |
|`OLED.cpp`       |947   |Pilote écran OLED SH110X / SSD1306                                     |
|`Handle.cpp`     |906   |Machine à états : modes exploitation / programmation                   |
|`RN2483A.cpp`    |636   |Driver LoRa : init OTAA, build payload, send                           |
|`Menus.cpp`      |533   |Navigation dans les menus                                              |
|`24C32.cpp`      |529   |Gestion EEPROM (config persistante)                                    |
|`Mesures.cpp`    |506   |Acquisition capteurs (HX711, DHT, analog)                              |
|`serialDebug.cpp`|477   |Fonctions de debug série                                               |
|`setup.cpp`      |429   |Initialisations matérielles                                            |
|`DS3231.cpp`     |319   |Pilote RTC (alarmes, heure)                                            |
|`LED_NB.cpp`     |309   |Gestion LEDs non-bloquante                                             |
|`KEYPAD.cpp`     |158   |Clavier analogique 5 touches                                           |
|`Power.cpp`      |110   |Gestion veille / réveil                                                |
|`ISR.cpp`        |68    |Handlers d'interruption RTC                                            |
|`DHT22.cpp`      |53    |Lecture capteur DHT22                                                  |
|`Claude.cpp`     |136   |Notes / spécifications (pas de code actif)                             |

-----

## 2. Architecture actuelle

### 2.1 Flux de données

```
Capteurs               Traitement              Communication
─────────              ──────────              ─────────────
HX711 ×4 ─┐
DHT22 ────┤            ┌──────────┐            ┌────────────┐
LDR ──────┼──────────▶ │ HiveSensor│──────────▶│ RN2483A    │──▶ LoRaWAN
VBat ─────┤            │ _Data     │  payload  │ sendPayload│    (Passerelle)
VSol ─────┘            └──────────┘  19 bytes  └────────────┘
                            │
                    ┌───────┴───────┐
                    │ OLED Display  │
                    │ (écran local) │
                    └───────────────┘
```

### 2.2 Modes de fonctionnement

Le firmware possède deux modes sélectionnés par un pin physique (PIN_PE) :

**Mode EXPLOITATION** : cycle veille → réveil par alarme RTC → mesures → build payload → envoi LoRa → retour veille. C'est le mode terrain, optimisé consommation.

**Mode PROGRAMMATION** : boucle active avec interface utilisateur (clavier 5 touches + OLED), navigation dans les menus pour configurer le matériel, calibrer les balances, ajuster les paramètres LoRa, etc.

### 2.3 Structure de la configuration

La configuration est stockée en EEPROM 24C32 via une structure `ConfigGenerale_t` (packed) qui contient un magic number, un bloc applicatif (rucher ID, AppEUI, AppKey, SF, période d'envoi) et un bloc matériel (identifiants I²C, paramètres des 4 jauges HX711, coefficients de calibration). Un checksum protège l'intégrité.

### 2.4 Transmission LoRa

La topologie actuelle est simple : **chaque nœud envoie individuellement** vers la passerelle via LoRaWAN OTAA (join via RN2483A → Sodaq_RN2483). Le payload binaire fait 19 octets en little-endian contenant : ID rucher, température, humidité, luminosité, VBat, VSol, et 4 poids. L'envoi est déclenché par l'alarme 2 du DS3231 selon `SendingPeriod` (défaut 5 min).

-----

## 3. Points forts

**Le prototype fonctionne.** C'est l'essentiel pour un POC — les capteurs lisent, le payload part, le deep sleep est implémenté, et l'interface de calibration terrain existe. Quelques aspects méritent d'être soulignés :

**Gestion non-bloquante aboutie.** Les saisies (numérique, alpha, hexa, date, heure, email, IP) sont toutes implémentées en machines à états non-bloquantes avec timeout, ce qui est solide pour un système embarqué monothread. Le clavier analogique avec anti-rebond logiciel est bien fait.

**Séparation des deux modes.** La distinction exploitation / programmation via un pin physique est pragmatique : le mode terrain n'embarque que le strict nécessaire (mesure → envoi → dodo).

**Configuration persistante.** Le système EEPROM avec magic number, versioning et checksum permet de survivre aux mises à jour firmware et aux corruptions.

**Deep sleep fonctionnel.** La séquence veille/réveil (extinction OLED, HX711 power_down, LoRa sleep, USB off → LowPower.sleep() → restauration) est correcte.

-----

## 4. Points faibles et dettes techniques

### 4.1 Organisation du projet

**Pas de structure de projet standard.** Tous les fichiers sont à la racine — pas de `src/`, `include/`, `lib/`, `doc/`. Il manque `.gitignore` (les .bin compilés sont versionnés), `platformio.ini` ou tout fichier de build. Impossible de compiler sans connaître l'environnement exact de l'auteur.

**Pas de CI/CD.** Aucun GitHub Actions, aucun test automatisé, aucune vérification de taille de binaire.

**Binaires dans le repo.** Deux fichiers .bin (104K + 112K) n'ont rien à faire dans le contrôle de version.

### 4.2 Architecture logicielle

**Couplage fort via variables globales.** Le fichier `var.h` déclare **~97 variables globales uniques** (dans le bloc `#ifdef __MAIN__`), incluant tous les contextes de saisie, flags d'interruption, états d'écran, objets hardware (rtc, scales, display). Toute modification a des effets de bord imprévisibles. Note : 3 variables sont déclarées `extern` sans définition correspondante (`OLED`, `PvageInfosLoRaRefresh`, `PvageInfosSystRefresh`).

**Pas d'abstraction entre couches.** Le code LoRa (`RN2483A.cpp`) accède directement aux variables globales `HiveSensor_Data`, `config`, `payload`. Le code de mesures accède aux objets hardware globaux (`scaleA`, `scaleB`…). Il n'y a pas d'interface entre "acquisition", "traitement" et "communication".

**Include chain fragile.** Le fichier `define.h` inclut tout : Wire, RTClib, Sodaq, Adafruit OLED, HX711, DHT, CayenneLPP, puis les headers du projet (struct.h → var.h → prototypes.h). Un seul fichier `.h` cassé recompile tout.

**Le switch/case sur 4 balances est répété partout.** Le pattern `case 0: scaleA... case 1: scaleB... case 2: scaleC... case 3: scaleD` apparaît dans le setup, la loop, Handle.cpp, Mesures.cpp. Un tableau de pointeurs `HX711* scales[4] = {&scaleA, &scaleB, &scaleC, &scaleD}` éliminerait cette duplication.

### 4.3 Qualité du code

**Code mort abondant.** De larges blocs commentés (`/* ... */`) parsèment le code — fonctions entières commentées dans RN2483A.cpp (`non_appelle_readLoRaResponse`, `non_appelle_RN2483Version`, `sleep_LoRa`, `wake_LoRa`), code de test dans Mesures.cpp, exemples dans Claude.cpp.

**Marqueurs d'incertitude.** On trouve beaucoup de `?????`, `!!!!!`, `WTF`, `fonctiondemerdetoggle`, `// ici pas bon`, `// 2 fois???`, `// pas terrible!!!` — signes d'un code qui a évolué par essais successifs sans consolidation.

**Pas de gestion d'erreurs structurée.** Les lectures capteurs ne gèrent pas les timeouts de façon uniforme. Le DHT est noté "bloquant en erreur". Les envois LoRa retournent un code d'erreur qui est affiché mais pas traité (pas de retry automatique, pas de log local).

**Incohérence d'indexation.** Un commentaire dans Mesures.cpp le dit explicitement : "indices des macros de 0 à 3 ou de 1 à 4 ????? manque de cohérence !". Cela se confirme dans `Build_Lora_String` où `poidsBal_g(4)` est appelé (index hors bornes pour un tableau de 4).

**Utilisation de `String` Arduino dans var.h.** Les variables `readingL` et `readingT` (var.h) sont déclarées comme `String` Arduino — en violation directe de la contrainte mémoire (fragmentation heap sur 32 Ko de RAM).

**Bug potentiel memcpy dans buildLoraPayload().** Dans RN2483A.cpp, `memcpy(&payload[indice], &Masse, sizeof(Masse))` copie `sizeof(int)` = 4 octets sur ARM 32 bits, mais le code n'incrémente l'indice que de 2 — risque d'écrasement des octets adjacents dans le payload.

**delay(10000-20000) dans RN2483AsendLoRaPayload().** En cas d'erreur LoRa, le code bloque pendant 10 à 20 secondes avec des `delay()` — bloquant en mode exploitation et incompatible avec un watchdog de 8 secondes.

**`__INIT_DONE` défini partout, utilisé nulle part.** Chaque fichier .cpp définit `#define __INIT_DONE` avant l'include de `define.h`, mais ce symbole n'est jamais testé dans aucun header. Code résiduel mort.

### 4.4 Communication LoRa

**Aucune optimisation de la bande passante.** Chaque nœud envoie son payload complet à chaque cycle, même si les valeurs n'ont pas changé. Pas de delta encoding, pas de batching.

**Payload en format propriétaire brut.** Le payload de 19 octets est construit "à la main" par copie d'octets de floats en little-endian. Pas de versioning du format, pas de CRC du payload, pas de possibilité d'extension propre.

**Pas de retransmission.** Si l'envoi échoue, c'est perdu. Pas de file d'attente, pas de log local pour upload ultérieur.

**Topologie point-à-point uniquement.** Aucun mécanisme pour la communication inter-nœuds (pas de mode LoRa P2P, pas de relais, pas de table de routage).

### 4.5 Gestion de l'énergie

**Deep sleep incomplet.** Le `configureLowPowerMode()` est quasi vide. Il manque la désactivation des périphériques SAMD non utilisés (ADC, TC, SERCOM inutilisés), la configuration des pins en mode basse conso, le suivi de la consommation.

**delay(5000) dans sleep().** Il y a un `delay(5000)` "pour le temps de lire l'info" dans la fonction de mise en veille — inacceptable en production, ça consomme ~10mA pendant 5 secondes à chaque cycle.

-----

## 5. Évaluation de la faisabilité : fédération des envois LoRa

L'objectif est de permettre à des ruches proches de se coordonner pour réduire les transmissions LoRaWAN payantes. Voici l'analyse de faisabilité sur le code actuel :

### Ce qui bloque directement

**Le RN2483A ne supporte que LoRaWAN.** Le module Microchip RN2483 est un modem LoRaWAN — il gère l'OTAA, les sessions, le MAC layer. Pour la communication inter-nœuds (P2P), il faudrait utiliser le mode "radio" du RN2483 (`mac pause` puis `radio tx/rx`), ce qui est possible mais nécessite de gérer manuellement le timing, les collisions, et la commutation entre mode P2P et mode LoRaWAN.

**Pas d'abstraction de la couche comms.** Le code d'envoi est câblé directement sur `LoRaBee.send()`. Pour ajouter un mode relais P2P, il faudrait d'abord créer une couche d'abstraction : interface `IRadio` avec `sendToGateway()` et `sendToRelay()`.

**Un seul payload fixe de 19 octets.** La structure payload actuelle est rigide. La fédération nécessite un format extensible pouvant inclure des headers (source, type de message, séquence) et des données agrégées de plusieurs nœuds.

### Ce qui peut être réutilisé

La structure `HiveSensor_Data_t` est propre et contient toutes les mesures nécessaires. Le mécanisme d'alarme RTC pour les réveils périodiques est fonctionnel. Le système de configuration EEPROM peut être étendu pour stocker les paramètres de fédération (rôle : nœud/relais, voisins connus, fenêtres de réception).

### Effort estimé

Avant d'ajouter la fédération, il faut refactorer la couche communication. L'estimation est de l'ordre de 3 à 4 phases itératives : (1) abstraction radio + payload extensible, (2) protocole P2P minimal entre nœuds, (3) logique d'agrégation et de relais, (4) optimisations (delta, batching, scheduling).

-----

## 6. Recommandations prioritaires

### Phase 0 — Assainissement (préalable indispensable)

1. **Structurer le repo** : créer `src/`, `include/`, `doc/`, ajouter `.gitignore`, supprimer les .bin, ajouter `platformio.ini` pour rendre le build reproductible.
1. **Éliminer le code mort** : supprimer les fonctions commentées, les blocs `/* */` de 100+ lignes, les fichiers .txt orphelins. Git conserve l'historique.
1. **Réduire les globales** : regrouper les variables par module dans des structs passées par pointeur. Commencer par les flags ISR (les regrouper dans une struct `volatile`).
1. **Factoriser les 4 balances** : remplacer les switch/case par un tableau `HX711* scales[4]` et une boucle.

### Phase 1 — Abstraction radio (fondation pour la fédération)

1. **Créer une couche comms** : abstraire `RN2483AsendLoRaPayload()` derrière une interface qui permettra d'ajouter le mode P2P sans toucher au code applicatif.
1. **Payload extensible** : remplacer le format brut par un format versionné avec header (type, source ID, séquence, longueur). Envisager CBOR ou un format binaire compact maison.
1. **File d'attente d'envoi** : stocker les payloads non envoyés en EEPROM ou RAM pour retry/bulk upload.

### Phase 2 — Fédération LoRa

1. **Mode P2P** : implémenter la commutation `mac pause` / `radio tx` / `radio rx` pour la communication inter-nœuds sur une fréquence/SF dédiés.
1. **Protocole d'agrégation** : définir les rôles (collecteur/relais/gateway), les fenêtres de réception, et le format de messages agrégés.
1. **Scheduling** : coordonner les fenêtres d'écoute P2P et les envois LoRaWAN pour minimiser la consommation.

-----

## 7. Résumé visuel de l'état du code

```
Domaine                    État          Priorité pour fédération LoRa
──────────────────────     ──────────    ─────────────────────────────
Structure du projet        ⚠ Faible     ★★☆ Moyenne (maintenabilité)
Architecture logicielle    ⚠ Faible     ★★★ Haute (abstraction nécessaire)
Couche capteurs            ✅ OK         ★☆☆ Faible (réutilisable tel quel)
Couche communication       ⚠ Limitée    ★★★ Haute (cœur du chantier)
Format payload             ❌ Rigide     ★★★ Haute (doit être extensible)
Gestion énergie            ⚠ Partielle  ★★☆ Moyenne (impacte le P2P)
Interface utilisateur      ✅ Aboutie    ★☆☆ Faible (ne change pas)
Config / EEPROM            ✅ OK         ★★☆ Moyenne (étendre pour fédération)
Tests / CI                 ❌ Absent     ★★☆ Moyenne (filet de sécurité)
Documentation              ❌ Minimale   ★★☆ Moyenne (onboarding)
```

-----

*Ce rapport a été établi par analyse statique du code source. Une analyse dynamique (traces série, consommation mesurée, couverture radio) serait le complément nécessaire avant de démarrer les développements.*
