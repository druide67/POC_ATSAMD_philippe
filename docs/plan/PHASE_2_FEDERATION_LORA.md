# Phase 2 — Fédération LoRa entre ruches

**Objectif** : permettre aux ruches d'un même rucher de se coordonner pour réduire le nombre de transmissions LoRaWAN payantes.
**Durée estimée** : plusieurs sessions de travail (conception + implémentation + tests terrain)
**Prérequis** : Phase 1 terminée (abstraction radio, payload extensible)
**Risque** : élevé (nouveau protocole, tests terrain indispensables)

-----

## Le problème économique

Avec la topologie actuelle, **chaque ruche envoie individuellement** son payload à la passerelle LoRaWAN. Si un rucher a 5 ruches qui envoient toutes les 5 minutes, ça fait 5 × 288 = **1 440 transmissions/jour**. Sur un réseau payant comme Orange Live Objects, chaque message a un coût.

L'idée : **un seul nœud (le "collecteur") agrège les données des ruches voisines et envoie un seul message LoRaWAN** contenant les données de tout le rucher.

```
AVANT (5 transmissions LoRaWAN) :          APRÈS (1 transmission) :

Ruche A ──LoRaWAN──▶ Passerelle           Ruche A ──P2P──┐
Ruche B ──LoRaWAN──▶ Passerelle           Ruche B ──P2P──┤
Ruche C ──LoRaWAN──▶ Passerelle           Ruche C ──P2P──┼──▶ Collecteur ──LoRaWAN──▶ Passerelle
Ruche D ──LoRaWAN──▶ Passerelle           Ruche D ──P2P──┤
Ruche E ──LoRaWAN──▶ Passerelle           Ruche E ──P2P──┘
```

Économie potentielle : **80% de réduction** des messages LoRaWAN (5 → 1 par cycle).

-----

## Étape 2.1 — Concevoir le protocole P2P

### Topologie en étoile simple

La topologie la plus simple et la plus fiable pour un rucher :

- **1 Collecteur** (élu ou fixe) : écoute les nœuds, agrège, envoie en LoRaWAN
- **N Nœuds** : mesurent, envoient en P2P au collecteur, dorment

Pas de mesh, pas de routage multi-hop — la complexité n'en vaut pas la peine pour des nœuds distants de quelques mètres à dizaines de mètres.

### Séquence temporelle d'un cycle

```
Temps ──────────────────────────────────────────────────────▶

Collecteur:  [SLEEP]──[WAKE]──[RX P2P window]──[AGGREGATE]──[TX LoRaWAN]──[SLEEP]
                              ▲  ▲  ▲  ▲  ▲
Nœud A:      [SLEEP]──[WAKE]──[MESURE]──[TX P2P]──[SLEEP]──────────────────────
Nœud B:      [SLEEP]──────[WAKE]──[MESURE]──[TX P2P]──[SLEEP]──────────────────
Nœud C:      [SLEEP]──────────[WAKE]──[MESURE]──[TX P2P]──[SLEEP]──────────────
Nœud D:      [SLEEP]──────────────[WAKE]──[MESURE]──[TX P2P]──[SLEEP]──────────

                    │◀── fenêtre RX: ~30s ──▶│
```

Chaque nœud a un **slot temporel** décalé pour éviter les collisions P2P. Le collecteur écoute pendant une fenêtre fixe, puis agrège et envoie.

### Commandes RN2483 pour le mode P2P

Le RN2483 supporte le mode radio brut via :

```
mac pause                    // suspend le MAC LoRaWAN
radio set freq 869525000     // fréquence P2P (bande ISM EU)
radio set sf sf7             // SF7 pour courte portée, rapide
radio set bw 125             // bande passante
radio set pwr 2              // puissance réduite (économie)
radio tx <hex_payload>       // envoi P2P
radio rx 0                   // réception continue
radio rx 5000                // réception avec timeout 5s
mac resume                   // reprend le MAC LoRaWAN
```

**Attention** : après `mac pause`, la session OTAA n'est pas perdue, mais le timer de duty cycle continue. Après `mac resume`, le prochain `mac tx` fonctionnera normalement.

### ❓ Questions pour ton cousin

1. **Combien de ruches par rucher en moyenne ?** Si c'est 2-3, la fédération est moins rentable. Si c'est 5-10+, l'économie est significative.
1. **La topologie collecteur fixe est-elle acceptable ?** Ça veut dire qu'une ruche spécifique a un rôle différent (plus de consommation car elle écoute en P2P + envoie en LoRaWAN). Ou préfère-t-il un système rotatif (chaque ruche est collecteur à tour de rôle) ?
1. **Si le collecteur tombe en panne** (batterie vide, perte de connexion), les autres nœuds doivent-ils revenir automatiquement en mode individuel LoRaWAN ? C'est le fallback le plus sûr.
1. **Quelle est la disposition physique des ruches ?** En ligne, en groupe serré ? La distance max entre deux ruches du même rucher ?
1. **Les ruches ont-elles accès au même réseau LoRaWAN** (même passerelle Orange visible) ? Sinon la fédération n'a pas de sens.

-----

## Étape 2.2 — Format de message P2P

### Messages inter-nœuds

```cpp
// Message P2P : nœud → collecteur
typedef struct __attribute__((packed)) {
  uint8_t version_len;    // version (4 bits) + longueur data (4 bits)
  uint8_t type;           // MSG_TYPE_MEASURES = 0x01
  uint8_t srcId;          // Num_Carte du nœud
  uint8_t seqNum;         // numéro de séquence (détection perte)
  // --- données compactes ---
  int8_t  temperature;    // ×0.5°C
  uint8_t humidity;       // ×0.5%
  uint8_t vbat;           // ×0.02V + offset 2V
  int16_t weight[4];      // ×0.01 kg, little-endian
  // --- intégrité ---
  uint8_t crc8;
} P2PNodeMessage_t;       // 14 octets
```

### Message agrégé vers passerelle

```cpp
// Message LoRaWAN : collecteur → passerelle
// Header (5 octets) + N × données nœud (11 octets chacun)
// Max avec 51 octets SF12 : header + 4 nœuds = 5 + 4×11 = 49 octets ✓

typedef struct __attribute__((packed)) {
  uint8_t version_len;
  uint8_t type;           // MSG_TYPE_AGGREGATED = 0x02
  uint8_t flags;          // bitmap : quels nœuds présents
  uint8_t collectorId;
  uint8_t nodeCount;
  // --- données par nœud (répétées N fois) ---
  struct {
    uint8_t srcId;
    int8_t  temperature;
    uint8_t humidity;
    uint8_t vbat;
    int16_t weight[4];    // 8 octets
  } nodes[];              // 11 octets × N
  uint8_t crc8;
} AggregatedMessage_t;
```

### Limites par SF

|SF  |Max payload LoRaWAN|Header|Par nœud|Max nœuds agrégés|
|----|-------------------|------|--------|-----------------|
|SF7 |242 octets         |5     |11      |21               |
|SF9 |115 octets         |5     |11      |10               |
|SF12|51 octets          |5     |11      |**4**            |

En SF12 (pire cas), on peut agréger **4 nœuds** dans un seul message. Pour 5+ ruches en SF12, il faudrait 2 messages. En SF7/SF9, on peut agréger tout un rucher facilement.

### ❓ Questions pour ton cousin

1. **Le SF est-il négociable ?** S'il peut passer en SF7 ou SF9 (meilleure portée passerelle disponible), la fédération est beaucoup plus efficace. Son expérience : SF7 et SF9 passaient-ils vers la passerelle Orange ?
1. **Faut-il transmettre la luminosité et VSol dans le message agrégé ?** Ce sont des mesures identiques pour tout le rucher (même exposition). On pourrait les envoyer uniquement depuis le collecteur pour économiser des octets.

-----

## Étape 2.3 — Synchronisation temporelle

### Le problème

Pour que les nœuds envoient dans leurs slots et que le collecteur écoute au bon moment, il faut une **base de temps commune**. Chaque nœud a un DS3231 (précision ±2 ppm, soit ~0.17s/jour de dérive), ce qui est largement suffisant.

### Approche simple : slots fixes par configuration

```cpp
// Configuration par nœud (stockée en EEPROM)
typedef struct {
  uint8_t role;           // 0 = nœud simple, 1 = collecteur
  uint8_t slotIndex;      // 0..N, position dans le cycle P2P
  uint8_t totalSlots;     // nombre total de nœuds
  uint16_t slotDuration;  // durée d'un slot en ms (ex: 3000 = 3s)
  uint16_t cycleMinutes;  // période du cycle (ex: 5 min)
  uint8_t p2pFreqIndex;   // index dans la table de fréquences P2P
  uint8_t p2pSF;          // SF pour P2P (7 recommandé)
} FederationConfig_t;
```

Le cycle est ancré sur l'alarme 2 du DS3231. Chaque nœud sait quand réveiller :

- **Nœud** : réveil à `alarme2 + slotIndex × slotDuration`, mesure, envoi P2P, dodo
- **Collecteur** : réveil à `alarme2`, écoute pendant `totalSlots × slotDuration`, agrège, envoi LoRaWAN, dodo

### ❓ Questions pour ton cousin

1. **La synchronisation initiale** : comment les DS3231 sont-ils mis à l'heure ? Par USB lors de la programmation ? Si les horloges sont synchronisées à ±1 seconde, c'est suffisant pour des slots de 3 secondes.
1. **Que faire si un nœud ne répond pas dans son slot ?** Le collecteur envoie quand même les données qu'il a reçues ? Ou il attend le cycle suivant ?

-----

## Étape 2.4 — Gestion de l'énergie en mode fédéré

### Impact sur la consommation

Le collecteur consomme **plus** qu'un nœud simple car il doit :

- Rester éveillé plus longtemps (fenêtre d'écoute P2P)
- Faire sa propre mesure + l'envoi LoRaWAN
- Le module RN2483 en réception consomme ~10 mA

Les nœuds simples consomment **moins** qu'avant :

- Envoi P2P en SF7 : ~50 ms à ~30 mA
- Envoi LoRaWAN en SF12 : ~1.5s à ~40 mA
- Économie par nœud : significative sur la durée

### Budget énergie du collecteur (estimation)

|Phase               |Durée |Courant|Énergie/cycle|
|--------------------|------|-------|-------------|
|Réveil + init       |500 ms|15 mA  |7.5 µAh      |
|Écoute P2P (4 slots)|12 s  |12 mA  |40 µAh       |
|Mesures propres     |2 s   |20 mA  |11 µAh       |
|Envoi LoRaWAN SF12  |2 s   |40 mA  |22 µAh       |
|**Total par cycle** |~17 s |—      |**~80 µAh**  |
|Sleep (5 min - 17s) |283 s |20 µA  |1.6 µAh      |
|**Total 5 min**     |—     |—      |**~82 µAh**  |

Sur 24h : 82 × 288 = ~23.6 mAh/jour. Avec une batterie LiFePO4 de 3000 mAh, ça tient ~127 jours sans solaire. Avec le panneau solaire, c'est quasi infini.

### ❓ Questions pour ton cousin

1. **Quelle est la capacité de sa batterie LiFePO4 ?** Et la puissance du panneau solaire ?
1. **Accepte-t-il qu'une ruche consomme plus que les autres** (le collecteur) ? Ou faut-il une rotation automatique du rôle ?
1. **En hiver**, avec peu de soleil, est-ce que les batteries tiennent ? Le collecteur devrait-il augmenter la période d'envoi automatiquement quand la batterie baisse ?

-----

## Étape 2.5 — Fallback et robustesse

### Scénarios de panne

|Scénario                  |Comportement attendu                                                                         |
|--------------------------|---------------------------------------------------------------------------------------------|
|Collecteur en panne       |Les nœuds détectent l'absence d'ACK P2P → basculent en mode LoRaWAN individuel               |
|Un nœud ne répond pas     |Le collecteur envoie les données qu'il a (bitmap dans le header indique quels nœuds présents)|
|Batterie collecteur faible|Le collecteur envoie un message de status et le rôle bascule au nœud suivant                 |
|Passerelle LoRaWAN down   |Le collecteur stocke en file d'attente (Phase 1), retry au cycle suivant                     |
|Collision P2P             |Slot manqué → le collecteur continue sans cette donnée                                       |

### Règle fondamentale : le fallback est toujours l'envoi individuel

Si la fédération échoue (pas de collecteur, pas de réponse P2P), chaque nœud doit pouvoir **revenir automatiquement** en mode LoRaWAN direct. La fédération est une optimisation, pas une dépendance.

```cpp
// Logique de décision dans le nœud
if (federationEnabled && collecteurDétecté) {
  // Mode fédéré : envoi P2P au collecteur
  radioSendP2P(message);
} else {
  // Fallback : envoi LoRaWAN direct (comme avant)
  radioSendToGateway(message);
}
```

### ❓ Questions pour ton cousin

1. **Combien de cycles P2P ratés avant de basculer en fallback ?** 1 ? 3 ? 5 ?
1. **Le mode fallback doit-il être permanent** jusqu'au prochain reboot, ou tenter de revenir en mode fédéré après un certain temps ?
1. **Y a-t-il des cas où il veut forcer le mode individuel** même si un collecteur est disponible ? (ex: déplacement d'une ruche, maintenance)

-----

## Étape 2.6 — Configuration terrain

### Comment configurer la fédération sur le terrain ?

Options possibles :

- **Via le menu OLED existant** : ajouter un sous-menu "Fédération" pour définir rôle, slot, voisins
- **Via la configuration EEPROM** : étendre `ConfigGenerale_t` avec `FederationConfig_t`
- **Par message LoRaWAN descendant** : le serveur envoie la config de fédération (plus complexe)
- **Auto-découverte** : les nœuds s'annoncent en P2P et le collecteur construit la table (encore plus complexe)

### ❓ Questions pour ton cousin

1. **Configuration manuelle ou automatique ?** Le plus simple et le plus fiable est la configuration manuelle par le menu OLED. L'auto-découverte est sexy mais fragile. Qu'est-ce qu'il préfère ?
1. **Combien de ruchers différents a-t-il ?** Si c'est 2-3, la configuration manuelle est triviale. Si c'est 20+, il faut automatiser.

-----

## Checklist de validation Phase 2

- [ ] Protocole P2P défini et documenté
- [ ] Mode `mac pause` / `radio tx/rx` testé sur RN2483 physique
- [ ] Slots temporels fonctionnels entre 2 nœuds minimum
- [ ] Message agrégé correctement construit et décodé côté serveur
- [ ] Fallback individuel fonctionnel si collecteur absent
- [ ] Consommation mesurée sur les deux rôles (nœud et collecteur)
- [ ] Configuration stockée en EEPROM et éditable via menu OLED
- [ ] Test terrain : 3+ ruches en fédération pendant 24h

-----

## Récapitulatif des questions Phase 2

|# |Question                                   |Impact                   |
|--|-------------------------------------------|-------------------------|
|1 |Combien de ruches par rucher ?             |Rentabilité              |
|2 |Collecteur fixe ou rotatif ?               |Architecture             |
|3 |Fallback automatique si collecteur HS ?    |Robustesse               |
|4 |Disposition physique des ruches ?          |Portée P2P               |
|5 |Même passerelle visible pour toutes ?      |Prérequis                |
|6 |SF négociable (SF7/SF9 possible) ?         |Capacité agrégation      |
|7 |Luminosité/VSol par ruche ou par rucher ?  |Taille payload           |
|8 |Synchro initiale des horloges ?            |Timing slots             |
|9 |Comportement si nœud absent ?              |Robustesse               |
|10|Capacité batterie et panneau solaire ?     |Budget énergie           |
|11|Collecteur consomme plus — acceptable ?    |Énergie                  |
|12|Comportement hivernal (peu de solaire) ?   |Autonomie                |
|13|Nb cycles ratés avant fallback ?           |Robustesse               |
|14|Fallback permanent ou temporaire ?         |Robustesse               |
|15|Forçage mode individuel possible ?         |Flexibilité              |
|16|Config manuelle (menu) ou auto-découverte ?|**Décision structurante**|
|17|Combien de ruchers à gérer ?               |Scalabilité              |
