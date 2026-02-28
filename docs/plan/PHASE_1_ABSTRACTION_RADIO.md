# Phase 1 — Abstraction radio et payload extensible

**Objectif** : découpler le code applicatif du matériel radio pour pouvoir ajouter le mode P2P sans tout casser.
**Durée estimée** : 4-6h à deux
**Prérequis** : Phase 0 terminée
**Risque** : moyen (refactoring du chemin critique mesures → envoi)

-----

## Pourquoi cette phase est indispensable

Aujourd'hui le code fait :

```
Handle.cpp → buildLoraPayload() → RN2483AsendLoRaPayload() → LoRaBee.send()
```

Tout est câblé en dur : le format du payload (19 octets bruts), le mode d'envoi (LoRaWAN uniquement), la destination (passerelle uniquement). Pour fédérer les envois entre ruches, il faut pouvoir :

- Envoyer un message **à un autre nœud** (P2P) en plus de la passerelle (LoRaWAN)
- Construire des payloads **de tailles variables** (données agrégées de plusieurs ruches)
- **Stocker** les données non envoyées pour retry ou bulk upload

-----

## Étape 1.1 — Définir le format de payload extensible

### Le problème actuel

Le payload est un tableau de 19 octets rempli manuellement par copie de floats :

```cpp
payload[indice++] = ((uint8_t*)&temperature)[0];  // little-endian brut
payload[indice++] = ((uint8_t*)&temperature)[1];
```

Pas de header, pas de version, pas de type de message, pas de CRC. Le décodeur côté serveur doit connaître exactement le format.

### La proposition : format binaire compact versionné

```
┌─────────┬──────┬───────┬────────┬──────────────────┬─────┐
│ Version │ Type │ Flags │ SrcID  │ Payload data     │ CRC │
│ 1 octet │ 1 o  │ 1 o   │ 1 o    │ N octets         │ 1 o │
└─────────┴──────┴───────┴────────┴──────────────────┴─────┘
```

- **Version** (4 bits haut) + **longueur data** (4 bits bas, ×2 → max 30 octets data)
- **Type** : 0x01 = mesures individuelles, 0x02 = mesures agrégées, 0x03 = ACK/status, 0x04 = config
- **Flags** : bits pour delta-encoding, batterie faible, erreur capteur, demande de relais
- **SrcID** : identifiant du nœud source (Num_Carte)
- **CRC8** : intégrité du message

### Contrainte LoRaWAN

En SF12 (portée maximale), le payload LoRaWAN est limité à **51 octets**. Avec le header de 5 octets, il reste 46 octets pour les données — largement suffisant pour 1 ruche (14 octets de données utiles) et même 3 ruches agrégées.

### ❓ Questions pour ton cousin

1. **Quel format côté serveur ?** Orange Live Objects attend-il un format spécifique ? Ou le décodage est fait par un script custom (Grafana, Node-RED) ? Si c'est custom, on est libre sur le format.
1. **CayenneLPP** est inclus dans les dépendances mais semble peu utilisé. Est-ce qu'il a été envisagé comme format standard ? Il est compatible TTN/Chirpstack mais consomme plus d'octets (type+canal+valeur pour chaque mesure).
1. **La résolution des données est-elle correcte ?** Actuellement les poids sont en `float` (4 octets). Un `int16_t` en hectogrammes (0.1 kg) couvrirait 0-3276 kg en seulement 2 octets. Est-ce que la précision à 100g suffit pour la pesée de ruche ? Même question pour la température (demi-degrés ?).
1. **Le `RucherID` dans le payload** — est-il vraiment nécessaire si le DevEUI identifie déjà le nœud côté LoRaWAN ?

-----

## Étape 1.2 — Créer la couche d'abstraction radio

### Architecture cible

```
Code applicatif (Handle.cpp)
         │
         ▼
┌─────────────────────┐
│   RadioManager      │  ← nouveau module
│  - sendToGateway()  │
│  - sendToNode()     │  ← Phase 2
│  - listenForNodes() │  ← Phase 2
│  - getStatus()      │
└────────┬────────────┘
         │
         ▼
┌─────────────────────┐
│   RN2483 Driver     │  ← RN2483A.cpp simplifié
│  - initOTAA()       │
│  - sendLoRaWAN()    │
│  - sendP2P()        │  ← Phase 2
│  - receiveP2P()     │  ← Phase 2
│  - sleep() / wake() │
└─────────────────────┘
```

### Interface proposée

```cpp
// radio_manager.h — nouvelle interface

typedef enum {
  RADIO_OK = 0,
  RADIO_ERR_NOT_CONNECTED,
  RADIO_ERR_PAYLOAD_TOO_BIG,
  RADIO_ERR_TIMEOUT,
  RADIO_ERR_BUSY,
  RADIO_ERR_NO_ACK
} RadioResult_t;

typedef enum {
  MSG_TYPE_MEASURES = 0x01,
  MSG_TYPE_AGGREGATED = 0x02,
  MSG_TYPE_STATUS = 0x03,
  MSG_TYPE_CONFIG = 0x04
} MessageType_t;

// Construire un message à partir des mesures
uint8_t radioBuildMessage(uint8_t* buffer, uint8_t maxLen,
                          MessageType_t type,
                          const HiveSensor_Data_t* data);

// Envoyer vers la passerelle LoRaWAN
RadioResult_t radioSendToGateway(const uint8_t* data, uint8_t len);

// Statut de la connexion
bool radioIsConnected(void);

// Gestion énergie
void radioSleep(void);
void radioWake(void);
```

### ❓ Questions pour ton cousin

1. **Le RN2483A peut-il commuter entre LoRaWAN et mode radio P2P ?** Techniquement oui (`mac pause` + `radio tx/rx`), mais est-ce qu'il a déjà testé cette commande sur ses modules ? Il y a un risque que la session OTAA soit perdue et nécessite un re-join.
1. **Fréquence et SF pour le P2P** : le P2P entre nœuds devrait utiliser une fréquence/SF différente du LoRaWAN pour ne pas interférer. A-t-il des contraintes sur les fréquences utilisables (plan de fréquences EU868) ?
1. **Portée nécessaire entre ruches** : quelle est la distance typique entre les ruches d'un même rucher ? Si c'est < 100m, un SF7 en P2P suffit (faible consommation, rapide). Si c'est > 1km, il faut du SF12 P2P.

-----

## Étape 1.3 — File d'attente d'envoi

### Le problème actuel

Si `RN2483AsendLoRaPayload()` échoue, les données sont perdues. Le code affiche l'erreur et passe à autre chose.

### La proposition

Stocker les messages non envoyés dans un buffer circulaire en RAM (et optionnellement en EEPROM pour survivre aux resets).

```cpp
// send_queue.h

#define QUEUE_MAX_MESSAGES  8
#define QUEUE_MAX_MSG_SIZE  51  // max payload SF12

typedef struct {
  uint8_t data[QUEUE_MAX_MSG_SIZE];
  uint8_t len;
  uint8_t retryCount;
  uint32_t timestamp;  // epoch ou uptime
} QueuedMessage_t;

typedef struct {
  QueuedMessage_t messages[QUEUE_MAX_MESSAGES];
  uint8_t head;
  uint8_t tail;
  uint8_t count;
} SendQueue_t;

bool queuePush(SendQueue_t* q, const uint8_t* data, uint8_t len);
bool queuePop(SendQueue_t* q, QueuedMessage_t* msg);
uint8_t queueCount(const SendQueue_t* q);
void queueFlush(SendQueue_t* q);  // envoyer tout ce qui est en attente
```

### Budget RAM

`QUEUE_MAX_MESSAGES × (51 + 1 + 1 + 4) = 8 × 57 = 456 octets` — environ 1.4% de la RAM, acceptable.

### ❓ Questions pour ton cousin

1. **Combien de messages non envoyés veut-il conserver ?** 8 messages = ~40 minutes de données à 5 min d'intervalle. Est-ce suffisant ? Pour un stockage plus long, il faudrait utiliser l'EEPROM ou une carte SD.
1. **La carte SD** : le code a un `#define SD_CS` et la lib SD est mentionnée dans le README. Est-ce que le slot SD est câblé et fonctionnel ? Si oui, c'est un excellent support pour le log local.
1. **Stratégie de retry** : combien de tentatives avant d'abandonner un message ? Faut-il un backoff exponentiel ? Ou on préfère juste stocker et faire un bulk upload quand la connexion revient ?

-----

## Étape 1.4 — Encodage compact des données

### Optimisation de la taille payload

Actuellement 19 octets pour des floats bruts. On peut faire beaucoup mieux :

|Donnée           |Actuel      |Proposé                    |Plage couverte|
|-----------------|------------|---------------------------|--------------|
|Température      |float 4o    |int8_t 1o (×0.5°C)         |-64°C à +63°C |
|Humidité         |float 4o    |uint8_t 1o (×0.5%)         |0-127%        |
|Luminosité       |float 4o    |uint8_t 1o (%)             |0-255%        |
|VBat             |float 4o    |uint8_t 1o (×0.02V + 2V)   |2V-7.1V       |
|VSol             |float 4o    |uint8_t 1o (×0.04V)        |0-10.2V       |
|Poids ×4         |float×4 16o |int16_t×4 8o (×0.01kg)     |-327-327 kg   |
|**Total data**   |**36o**     |**13o**                    |              |
|+ Header         |0           |5o (ver+type+flags+src+crc)|              |
|**Total message**|**19o brut**|**18o avec header**        |              |

On garde la même taille mais on ajoute un header structuré et on a de la marge pour des données supplémentaires (timestamp delta, identifiants de nœuds relayés).

### Delta encoding (optionnel, Phase 2+)

Si les valeurs changent peu entre deux envois (typique pour une ruche stable), on peut n'envoyer que les différences. Le flag `DELTA` dans le header indique que les valeurs sont des offsets signés par rapport au dernier envoi complet.

### ❓ Questions pour ton cousin

1. **Quelles précisions sont acceptables ?** Température à 0.5°C, poids à 10g — est-ce suffisant pour ses besoins ? Ou il a besoin de plus de résolution pour certains capteurs ?
1. **Le décodeur côté serveur** : qui le maintient ? Est-ce un script dans Live Objects, un décodeur TTN, ou un programme custom ? Il faudra le mettre à jour en même temps que le format payload.

-----

## Étape 1.5 — Refactoring de buildLoraPayload()

### Ce qu'on va faire

Remplacer l'assemblage manuel par une fonction propre qui utilise le nouveau format :

```cpp
// AVANT
void buildLoraPayload() {
  int indice = 0;
  payload[indice++] = config.applicatif.RucherID;
  payload[indice++] = ((uint8_t*)&temperature)[0];
  // ... 30 lignes de copie octet par octet
}

// APRÈS
uint8_t radioBuildMessage(uint8_t* buffer, uint8_t maxLen,
                          MessageType_t type,
                          const HiveSensor_Data_t* data)
{
  if (maxLen < MSG_HEADER_SIZE + MSG_MEASURES_SIZE) return 0;

  uint8_t pos = 0;

  // Header
  buffer[pos++] = (MSG_VERSION << 4) | (MSG_MEASURES_SIZE / 2);
  buffer[pos++] = type;
  buffer[pos++] = buildFlags(data);  // batterie faible, erreur capteur, etc.
  buffer[pos++] = config.materiel.Num_Carte;

  // Data — encodage compact
  buffer[pos++] = encodeTemp(data->DHT_Temp);
  buffer[pos++] = encodeHum(data->DHT_Hum);
  buffer[pos++] = encodeLum(data->Brightness);
  buffer[pos++] = encodeVoltage(data->Bat_Voltage, 2.0, 0.02);
  buffer[pos++] = encodeVoltage(data->Solar_Voltage, 0.0, 0.04);

  for (int i = 0; i < 4; i++) {
    int16_t w = (int16_t)(data->HX711Weight[i] * 100);  // kg → hectogrammes
    buffer[pos++] = w & 0xFF;
    buffer[pos++] = (w >> 8) & 0xFF;
  }

  // CRC
  buffer[pos] = crc8(buffer, pos);
  pos++;

  return pos;
}
```

-----

## Checklist de validation Phase 1

- [ ] `radio_manager.h/.cpp` créé avec l'interface propre
- [ ] `RN2483A.cpp` simplifié (ne fait plus que le bas niveau)
- [ ] Format payload versionné implémenté
- [ ] File d'attente en RAM fonctionnelle
- [ ] `buildLoraPayload()` remplacé par `radioBuildMessage()`
- [ ] Décodeur serveur mis à jour
- [ ] Test : envoi payload nouveau format → réception correcte sur le serveur
- [ ] Test : échec d'envoi → message en file d'attente → retry réussi
- [ ] Taille binaire vérifiée (doit rester dans les limites)
- [ ] Pas de régression sur les mesures

-----

## Récapitulatif des questions Phase 1

|# |Question                                        |Impact             |
|--|------------------------------------------------|-------------------|
|1 |Format attendu côté serveur (Live Objects) ?    |Format payload     |
|2 |CayenneLPP envisagé ou format custom ?          |Format payload     |
|3 |Résolution acceptable (poids 100g, temp 0.5°C) ?|Encodage compact   |
|4 |RucherID nécessaire dans le payload ?           |Taille             |
|5 |Mode radio P2P déjà testé sur RN2483 ?          |Faisabilité Phase 2|
|6 |Fréquences disponibles pour P2P ?               |Fédération         |
|7 |Distance entre ruches d'un même rucher ?        |SF pour P2P        |
|8 |Nb messages en file d'attente souhaité ?        |RAM budget         |
|9 |Carte SD câblée et fonctionnelle ?              |Stockage local     |
|10|Stratégie de retry ?                            |Robustesse         |
|11|Précision capteurs nécessaire ?                 |Encodage           |
|12|Qui maintient le décodeur serveur ?             |Déploiement        |
