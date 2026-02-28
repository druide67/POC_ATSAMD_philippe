# Phase 0 — Assainissement du repo et du code

**Objectif** : rendre le projet maintenable, compilable par n'importe qui, et prêt pour les évolutions.
**Durée estimée** : 2-3h à deux
**Prérequis** : aucun
**Risque** : faible (pas de changement fonctionnel)

-----

## Étape 0.1 — Structurer le repository

### Ce qu'on va faire

Réorganiser les fichiers depuis la racine plate actuelle vers une structure PlatformIO standard :

```
POC_ATSAMD/
├── CLAUDE.md                  ← contexte pour Claude Code
├── README.md                  ← documentation projet
├── platformio.ini             ← configuration build
├── .gitignore                 ← exclure binaires, .pio/, etc.
├── src/
│   ├── main.cpp               ← ex POC_ATSAMD.ino (renommé)
│   ├── handle.cpp
│   ├── isr.cpp
│   ├── mesures.cpp
│   ├── power.cpp
│   ├── rn2483a.cpp
│   ├── setup_hw.cpp           ← ex setup.cpp (renommé pour éviter conflit)
│   ├── ds3231.cpp
│   ├── eeprom_24c32.cpp       ← ex 24C32.cpp (renommé, nom invalide)
│   ├── oled.cpp
│   ├── menus.cpp
│   ├── menus_fonc.cpp
│   ├── saisies_nb.cpp
│   ├── keypad.cpp
│   ├── led_nb.cpp
│   ├── serial_debug.cpp
│   ├── dht22.cpp
│   └── convert.cpp
├── include/
│   ├── define.h
│   ├── config.h               ← vrai fichier config, pas le fichier notes actuel
│   ├── struct.h
│   ├── var.h
│   ├── prototypes.h
│   ├── menu.h
│   └── state.h
├── docs/
│   ├── plan/                  ← ces fichiers .md
│   └── hardware/              ← schémas, photos PCB
└── test/                      ← futur : tests unitaires
```

### ❓ Questions pour ton cousin

1. **Le fichier `config.h` actuel n'est PAS un header** — c'est un fichier de notes avec du code commenté (WDT, sleep examples). On le renomme en `docs/notes_config.md` et on crée un vrai `config.h` avec les paramètres projet ? Ou il veut garder ses notes dans le code ?
1. **Le fichier `Claude.cpp`** contient uniquement les spécifications données à Claude AI. On le déplace en `docs/` ou on le supprime ?
1. **Les fichiers .txt à la racine** (`SW_CCM 4.txt`, `ecranInfo_CCM.txt`, `m01_2F_GetNumRucher.txt`, `m04_1F_PoidsTare.txt`, `Saisies.cpp.txt`) — ce sont des notes de travail ? On les archive dans `docs/` ou on supprime ?
1. **Les fichiers .xlsx** (`menus.xlsx`, `jauges.xlsx`) — ils sont référence ? Ils vont dans `docs/` ?
1. **Le dossier `positionnement OLED/`** avec son fichier HTML — on le garde dans `docs/` ?
1. **Est-ce qu'il utilise Arduino IDE ou PlatformIO ?** La migration vers PlatformIO est fortement recommandée (reproductibilité du build, gestion des libs, CI possible). S'il est sur Arduino IDE, on peut garder la structure `.ino` et juste organiser en sous-dossiers, mais c'est moins propre.

### Arbitrage : Arduino IDE vs PlatformIO

|                          |Arduino IDE          |PlatformIO               |
|--------------------------|---------------------|-------------------------|
|Familiarité cousin        |✅ Probable           |❓ À vérifier             |
|Reproductibilité build    |❌ Dépend de l'install|✅ `platformio.ini` suffit|
|Gestion libs avec versions|❌ Manuelle           |✅ `lib_deps`             |
|CI/CD GitHub Actions      |⚠ Complexe           |✅ Natif                  |
|Migration                 |❌ Rien à faire       |⚠ 30 min de travail      |

-----

## Étape 0.2 — .gitignore et nettoyage

### Ce qu'on va faire

Créer un `.gitignore` et supprimer les fichiers qui n'ont pas leur place dans le repo.

```gitignore
# Build
.pio/
build/
*.bin
*.elf
*.hex

# IDE
.vscode/
*.code-workspace

# OS
.DS_Store
Thumbs.db

# Temp
*.bak
*.tmp
```

### ❓ Questions pour ton cousin

1. **Les deux fichiers `.bin` (104K + 112K)** dans le repo — est-ce qu'il en a besoin comme référence "dernier firmware qui marchait" ? Si oui, on peut les tagger dans une release GitHub plutôt que de les versionner. Si non, on supprime. **Git garde l'historique**, il pourra toujours les retrouver.
1. **Y a-t-il d'autres fichiers générés** qu'il a l'habitude de committer ?

-----

## Étape 0.3 — Supprimer le code mort

### Ce qu'on va faire

Supprimer tous les blocs de code commentés de plus de 5 lignes, les fonctions préfixées `non_appelle_`, et le code de test orphelin. L'historique Git conserve tout.

### Inventaire du code mort identifié

|Fichier         |Lignes environ|Description                                                                                                    |
|----------------|--------------|---------------------------------------------------------------------------------------------------------------|
|`RN2483A.cpp`   |~150 lignes   |Fonctions `non_appelle_readLoRaResponse`, `non_appelle_RN2483Version`, blocs `sleep_LoRa`/`wake_LoRa` commentés|
|`Mesures.cpp`   |~100 lignes   |Ancien code de compensation température commenté, fonctions test `affiche_nombre`, `uniontest`                 |
|`config.h`      |~130 lignes   |Exemples WDT et sleep commentés (tout le fichier est du commentaire)                                           |
|`POC_ATSAMD.ino`|~20 lignes    |Blocs debug commentés                                                                                          |
|`var.h`         |~30 lignes    |Variables commentées                                                                                           |
|`state.h`       |~40 lignes    |Notes de debug mélangées avec un header                                                                        |

### ❓ Questions pour ton cousin

1. **La fonction `fonctiondemerdetoggle()`** dans Mesures.cpp — est-elle utilisée quelque part ? (grep dit non). On supprime ?
1. **La fonction `TODOHX711Set_Scale_Bal()`** — le préfixe TODO indique qu'elle est en travaux. Est-ce qu'il veut la garder comme base de travail ou elle est remplacée par les menus de calibration ?
1. **Les commentaires de debug** avec `?????` et `!!!!!` — est-ce qu'il y a des bugs connus derrière ces marqueurs qu'il faut noter dans des issues GitHub avant de nettoyer ?

-----

## Étape 0.4 — Factoriser les 4 balances

### Le problème

Le même switch/case est répété 8+ fois dans le code :

```cpp
// Ce pattern apparaît dans : setup, loop, Handle.cpp, Mesures.cpp...
case 0 : Contrainte_List[0] = MESURESHX711GetStrainGauge(0, scaleA, AVR_10); break;
case 1 : Contrainte_List[1] = MESURESHX711GetStrainGauge(1, scaleB, AVR_10); break;
case 2 : Contrainte_List[2] = MESURESHX711GetStrainGauge(2, scaleC, AVR_10); break;
case 3 : Contrainte_List[3] = MESURESHX711GetStrainGauge(3, scaleD, AVR_10); break;
```

### La solution

```cpp
// Dans var.h — remplacer les 4 variables par un tableau
HX711 scales[4];  // au lieu de scaleA, scaleB, scaleC, scaleD

// Dans setup — initialisation par boucle
for (int i = 0; i < 4; i++) {
  if (Peson[config.materiel.Num_Carte][i]) {
    Contrainte_List[i] = MESURESHX711GetStrainGauge(i, scales[i], AVR_10);
    HiveSensor_Data.HX711Weight[i] = poidsBal_kg(i);
  }
}
```

### ❓ Questions pour ton cousin

1. **Les 4 HX711 partagent-ils vraiment le même SCK (pin 3) ?** C'est ce que dit le define. Si oui, le `power_down()` de scaleA éteint-il tous les autres ? C'est important pour le refactoring.
1. **Les DOUT sont sur les pins 4, 6, 8, 10** — c'est bien ça ? Pas de cas où une carte a une config différente ?

-----

## Étape 0.5 — Réduire les variables globales (premier passage)

### Le problème

`var.h` déclare ~194 variables globales. Toute modification a des effets de bord imprévisibles. Les flags ISR sont mélangés avec les contextes de saisie, les objets hardware, les buffers.

### Ce qu'on va faire (premier passage conservateur)

Ne PAS tout refactorer d'un coup. Juste regrouper logiquement en structs :

```cpp
// Regrouper les flags ISR
typedef struct {
  volatile bool wakeup1Sec;
  volatile bool wakeupPayload;
  volatile bool alarm1_enabled;
  volatile bool alarm2_enabled;
  volatile bool displayNextPayload;
  volatile bool modeExploitation;
} IsrFlags_t;

// Regrouper les flags d'affichage
typedef struct {
  bool infoScreenTime;
  bool loraScreenTime;
  bool loraNextPayload;
  bool balScreenTime;
  bool balRefresh[4];    // au lieu de InfoBalScreenRefreshBal_1..4
  bool vbatTime, vbatRefresh;
  bool vsolTime, vsolRefresh;
  bool vlumTime, vlumRefresh;
} DisplayFlags_t;
```

### ❓ Questions pour ton cousin

1. **Les flags `switchToProgrammingMode` et `switchToOperationMode`** sont déclarés `int` mais utilisés comme `bool`. C'est voulu ? Ils servent de "one-shot" pour afficher un message au changement de mode ?
1. **Le buffer `OLEDbuf[OLEDBUFLEN]`** fait `128 * 21 = 2688 octets` — c'est 8% de la RAM totale pour un buffer d'affichage. Est-ce que cette taille est vraiment nécessaire ? Un buffer de 21 caractères (1 ligne OLED) suffirait-il ?
1. **Le buffer `serialbuf[256]`** est partagé par tout le code pour le debug série. Est-ce acceptable ? (risque de corruption si une ISR l'utilise pendant que loop() écrit dedans)

-----

## Étape 0.6 — Corriger les problèmes de sécurité ISR

### Le problème

`ISR.cpp` contient des appels `LOG_DEBUG()` qui font `debugSerial.print()` — c'est interdit dans une ISR (risque de deadlock série, corruption de buffer).

### Ce qu'on va faire

```cpp
// AVANT (dangereux)
void ISRonRTCAlarm(void) {
  LOG_DEBUG("€ISR€ ");  // ❌ Serial dans ISR
  if (rtc.alarmFired(1)) { ... }
}

// APRÈS (safe)
void ISRonRTCAlarm(void) {
  // Pas de Serial.print dans ISR — juste positionner les flags
  if (rtc.alarmFired(1)) {
    rtc.clearAlarm(1);
    if (alarm1_enabled) wakeup1Sec = true;
  }
  if (rtc.alarmFired(2)) {
    alarm1_enabled = false;
    rtc.clearAlarm(1);
    rtc.clearAlarm(2);
    if (alarm2_enabled) wakeupPayload = true;
  }
}
```

### ❓ Question pour ton cousin

1. **Le "watchdog" dans l'ISR** (`if millis() - loopWDT > 100000`) — est-ce qu'il a déjà observé des blocages de la loop() en production ? Ce mécanisme ne fait rien actuellement (pas de reset), c'est juste du logging. Veut-il implémenter un vrai watchdog (Sodaq_wdt) ?

-----

## Checklist de validation Phase 0

- [ ] Structure de dossiers créée
- [ ] `.gitignore` ajouté
- [ ] Binaires supprimés du repo (optionnel : tagués en release)
- [ ] Code mort supprimé
- [ ] 4 balances factorisées en tableau
- [ ] Flags ISR regroupés en struct
- [ ] Flags d'affichage regroupés en struct
- [ ] Serial.print supprimé de l'ISR
- [ ] Compilation OK (même binaire fonctionnellement)
- [ ] Taille binaire vérifiée (doit baisser après suppression code mort)
- [ ] Commit + push

-----

## Récapitulatif des questions Phase 0

|# |Question                                           |Impact                   |
|--|---------------------------------------------------|-------------------------|
|1 |`config.h` : garder notes ou faire un vrai header ?|Structure                |
|2 |`Claude.cpp` : archiver ou supprimer ?             |Nettoyage                |
|3 |Fichiers .txt : archiver ou supprimer ?            |Nettoyage                |
|4 |Fichiers .xlsx : garder dans docs/ ?               |Structure                |
|5 |Dossier OLED HTML : garder ?                       |Structure                |
|6 |Arduino IDE ou PlatformIO ?                        |**Décision structurante**|
|7 |Fichiers .bin : release tag ou supprimer ?         |Git                      |
|8 |Autres fichiers générés ?                          |.gitignore               |
|9 |`fonctiondemerdetoggle()` : utilisée ?             |Code mort                |
|10|`TODOHX711Set_Scale_Bal()` : garder ?              |Code mort                |
|11|Bugs derrière les marqueurs `?????` ?              |Issues                   |
|12|HX711 SCK partagé — power_down affecte tous ?      |Factorisation            |
|13|Config pins DOUT identique sur toutes les cartes ? |Factorisation            |
|14|`switchTo*Mode` : int ou bool ?                    |Types                    |
|15|`OLEDbuf` 2688 octets : nécessaire ?               |RAM                      |
|16|`serialbuf` partagé : risque ISR ?                 |Sécurité                 |
|17|Watchdog : implémenter un vrai reset ?             |Robustesse               |
