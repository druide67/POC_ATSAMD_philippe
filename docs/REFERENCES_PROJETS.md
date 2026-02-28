# Projets open-source de référence — Monitoring de ruches

Ce document recense les projets open-source matures dont l'architecture, le code ou les choix techniques sont directement exploitables pour l'évolution du POC_ATSAMD.

-----

## Hiveeyes — Le plus pertinent

|                |                                                  |
|----------------|--------------------------------------------------|
|**Site**        |https://hiveeyes.org                              |
|**Firmware**    |https://github.com/hiveeyes/arduino               |
|**Doc firmware**|https://hiveeyes.org/docs/arduino/                |
|**Communauté**  |https://community.hiveeyes.org                    |
|**Licence**     |AGPL-3.0                                          |
|**Actif depuis**|2011                                              |
|**MCU**         |ATmega328P, ESP8266, ESP32, Heltec CubeCell       |
|**Radio**       |RFM69, RFM95 (LoRa), LoRaWAN, GSM, WiFi           |
|**Capteurs**    |HX711, DS18B20, DHT22, BME280 — les mêmes que nous|
|**Backend**     |Kotori (MQTT) → InfluxDB → Grafana                |

### Pourquoi c'est la référence n°1

Le firmware "generic" de Hiveeyes implémente exactement l'architecture cible de notre Phase 2 : **un même firmware configurable en "sensor node", "telemetry relay node" ou "network gateway node"**. Les trois rôles coexistent dans le même binaire, activés par configuration.

### Ce qu'on peut en tirer

- **Architecture multi-rôle** : leur code sépare proprement la collecte de données, l'encodage (protocole BERadio avec sérialisation Bencode), et le transport radio. C'est le modèle à suivre pour notre refactoring Phase 1.
- **Relais multi-hop** : le relay node reçoit en RFM69 et réémet en RFM95 (LoRa) de manière opaque, sans décoder le message. Pattern à adapter pour notre relais P2P → LoRaWAN.
- **Structure PlatformIO** : projet bien structuré avec `platformio.ini` multi-environnements, bon modèle pour notre restructuration Phase 0.
- **Calibration HX711** : sketches dédiés pour la calibration des cellules de charge, directement réutilisables.

### Différences avec notre projet

- Ils utilisent des modules radio RFM69/RFM95 (SPI direct), nous utilisons un RN2483A (commandes AT via UART). Le protocole bas niveau est différent.
- Pas de mode LoRaWAN natif dans leur firmware generic (ils passent par un gateway qui fait le pont).
- Pas d'interface utilisateur embarquée (pas d'OLED, pas de clavier) — notre système de menus et saisies est unique.

-----

## BEEP — Le plus industrialisé

|            |                                                         |
|------------|---------------------------------------------------------|
|**Site**    |https://beep.nl                                          |
|**App/API** |https://github.com/beepnl/BEEP                           |
|**Firmware**|https://github.com/beepnl/beep-base-firmware             |
|**Hardware**|https://github.com/beepnl/measurement-system-v3          |
|**Licence** |AGPL-3.0 (API/app), hardware open                        |
|**MCU**     |Nordic NRF52832 (Cortex-M4F)                             |
|**Radio**   |LoRaWAN (RN2903/RN2483) + BLE                            |
|**Capteurs**|HX711 (poids), DS18B20 ×N (température), microphone (son)|
|**Backend** |Laravel PHP API + InfluxDB + VUE.js app                  |

### Pourquoi c'est intéressant

BEEP est le projet le plus abouti en termes de **produit fini** : ils produisent et vendent des cartes, ont une app mobile (Android/iOS), une plateforme web avec ~5000 utilisateurs dans 9 langues. C'est la référence pour l'industrialisation.

### Ce qu'on peut en tirer

- **Plateforme gratuite** : la plateforme https://app.beep.nl est ouverte et gratuite. Notre cousin pourrait y envoyer ses données au lieu de développer un backend custom. L'API accepte les données via HTTP POST ou via TTN webhook.
- **Décodeur LoRaWAN** : leur payload formatter pour TTN/Chirpstack est open-source et bien documenté, utile comme référence pour notre encodage Phase 1.
- **Configuration par BLE** : leur app mobile configure le firmware par Bluetooth. Pas prioritaire pour nous, mais intéressant si on veut éviter le clavier/OLED pour la configuration terrain.
- **Ultra low power** : leur design est optimisé pour la consommation, bon benchmark pour notre Phase 3.

### Différences avec notre projet

- MCU Nordic NRF52 (pas SAMD21) — le code firmware n'est pas directement portable.
- Pas de fédération inter-nœuds : chaque BEEP base envoie individuellement en LoRaWAN.
- L'app mobile (Flutter) est un gros investissement qu'on ne fera pas.

### Note importante

Ils utilisent aussi le **RN2483** pour le LoRa, comme nous. Leur code d'initialisation et de gestion du module peut être une bonne référence.

-----

## WaggleNet — La recherche la plus récente

|            |                                 |
|------------|---------------------------------|
|**Paper**   |https://arxiv.org/pdf/2512.07408 |
|**Licence** |Académique                       |
|**MCU**     |Heltec WiFi LoRa 32 V3 (ESP32-S3)|
|**Radio**   |LoRa P2P + WiFi/MQTT             |
|**Capteurs**|DHT22, LDR, GPS                  |

### Pourquoi c'est intéressant

WaggleNet implémente exactement la topologie **master/worker** qu'on vise pour la Phase 2, avec des résultats terrain documentés.

### Ce qu'on peut en tirer

- **Validation terrain** : 100% de livraison de paquets sur 110m à travers des ruches en bois. Confirme que le P2P courte portée entre ruches est fiable.
- **Architecture master/worker** : le master est alimenté en continu (USB/5V), les workers sont sur batterie avec deep sleep. C'est le pattern "collecteur fixe" de notre Phase 2.
- **Budget coût** : <30$/nœud comme objectif, utile pour positionner notre projet.
- **Gap analysis** : le papier identifie les lacunes des systèmes existants (pas de poids, pas de son dans leur implémentation). Utile pour comprendre le positionnement.

### Différences avec notre projet

- ESP32 (WiFi+LoRa intégré), pas SAMD21 + RN2483 séparé.
- Le master se connecte en WiFi au cloud (MQTT), pas en LoRaWAN. Pas de contrainte de duty cycle LoRa pour le master.
- Pas de pesée (HX711) dans leur implémentation actuelle.

-----

## Autres projets utiles

### joergkeller/beehive-sensor

|           |                                                                                                                             |
|-----------|-----------------------------------------------------------------------------------------------------------------------------|
|**Repo**   |https://github.com/joergkeller/beehive-sensor                                                                                |
|**MCU**    |Heltec CubeCell (ASR6502)                                                                                                    |
|**Intérêt**|Benchmark autonomie : 1W solaire + 230 mAh LiPo = 2 semaines sans soleil. Encodage payload en int16 (×100) pour la compacité.|

### DanNduati/IoT-beehive-monitoring-system

|           |                                                                                                                                              |
|-----------|----------------------------------------------------------------------------------------------------------------------------------------------|
|**Repo**   |https://github.com/DanNduati/IoT-beehive-monitoring-system                                                                                    |
|**Intérêt**|Architecture double carte (main board + aux board LoRa) avec audio FFT. Firmware en C Arduino et MicroPython, bon pour comparer les approches.|

### Projet OpenRuche (français)

|           |                                                                                                                          |
|-----------|--------------------------------------------------------------------------------------------------------------------------|
|**Repo**   |https://github.com/InGELLIS/Projet_OpenRuche                                                                              |
|**Intérêt**|Projet français similaire utilisant Arduino + LoRaWAN + BEEP. Intégration webhook TTN → BEEP documentée. Code en français.|

-----

## Synthèse : quoi prendre où

|Besoin                                   |Meilleure source                      |
|-----------------------------------------|--------------------------------------|
|Structure PlatformIO multi-environnements|Hiveeyes/arduino                      |
|Architecture nœud/relais/gateway         |Hiveeyes generic firmware             |
|Encodage payload LoRaWAN compact         |joergkeller (int16 ×100) + BEEP       |
|Calibration HX711                        |Hiveeyes (sketches dédiés)            |
|Plateforme de visualisation gratuite     |BEEP (app.beep.nl)                    |
|Validation portée P2P entre ruches       |WaggleNet (110m, 100% delivery)       |
|Budget énergie solaire/batterie          |joergkeller (1W + 230mAh = 2 semaines)|
|Gestion RN2483 (commandes AT)            |BEEP base firmware                    |
|Intégration TTN → dashboard              |OpenRuche                             |
