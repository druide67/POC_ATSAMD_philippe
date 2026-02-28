# Plan de travail — Évolution POC_ATSAMD

## Vue d'ensemble des phases

```
Phase 0                Phase 1                Phase 2               Phase 3
ASSAINISSEMENT         ABSTRACTION RADIO      FÉDÉRATION LoRa       ROBUSTESSE
(2-3h)                 (4-6h)                 (multi-sessions)      (continu)

├ Structurer repo      ├ Payload extensible   ├ Protocole P2P       ├ CI/CD GitHub
├ .gitignore           ├ Couche RadioManager  ├ Slots temporels     ├ Watchdog HW
├ Supprimer code mort  ├ File d'attente       ├ Message agrégé      ├ Recovery I²C
├ Factoriser balances  ├ Encodage compact     ├ Fallback individuel ├ Logs locaux
├ Regrouper globales   └ Refactor buildPayload├ Config terrain      ├ Sécurité clés
└ Corriger ISR                                └ Tests terrain       └ Énergie adaptive

▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼
DIMANCHE: Phase 0 + début Phase 1 (objectif réaliste pour une journée)
```

-----

## Fichiers livrés

|Fichier                                     |Description                             |
|--------------------------------------------|----------------------------------------|
|`CLAUDE.md`                                 |Contexte projet complet pour Claude Code|
|`docs/plan/PHASE_0_ASSAINISSEMENT.md`       |Plan détaillé + 17 questions            |
|`docs/plan/PHASE_1_ABSTRACTION_RADIO.md`    |Plan détaillé + 12 questions            |
|`docs/plan/PHASE_2_FEDERATION_LORA.md`      |Plan détaillé + 17 questions            |
|`docs/plan/PHASE_3_ROBUSTESSE_PRODUCTION.md`|Plan détaillé + 12 questions            |
|`docs/plan/PLAN_MASTER.md`                  |Ce fichier (vue d'ensemble)             |
|`docs/plan/ANALYSE_REPO.md`                 |Audit complet du code source            |
|`docs/REFERENCES_PROJETS.md`                |Projets open-source de référence        |
|`docs/ANALYSE_BEEP.md`                      |Étude plateforme BEEP comme backend     |

-----

## Toutes les questions à trancher avec ton cousin

### Décisions structurantes (à trancher en premier)

|#    |Phase|Question                                |Options                                                                   |
|-----|-----|----------------------------------------|--------------------------------------------------------------------------|
|P0-6 |0    |**Arduino IDE ou PlatformIO ?**         |Arduino = pas de migration / PlatformIO = build reproductible, CI possible|
|P1-1 |1    |**Format côté serveur ?**               |Custom script → libre sur le format / CayenneLPP → format imposé          |
|P2-2 |2    |**Collecteur fixe ou rotatif ?**        |Fixe = simple / Rotatif = plus robuste mais complexe                      |
|P2-16|2    |**Config manuelle ou auto-découverte ?**|Menu OLED = fiable / Auto = fragile mais scalable                         |
|P3-2 |3    |**Repo public avec clés exposées ?**    |Acceptable pour POC / Passer en privé ou externaliser les clés            |

### Questions matériel (seul le cousin peut répondre)

|#    |Phase|Question                                                                      |
|-----|-----|------------------------------------------------------------------------------|
|P0-12|0    |Les 4 HX711 partagent-ils le SCK ? power_down affecte-t-il tous les capteurs ?|
|P0-13|0    |Config pins DOUT identique sur toutes les cartes ?                            |
|P0-15|0    |OLEDbuf de 2688 octets : nécessaire ou réductible ?                           |
|P1-5 |1    |Mode `mac pause` / `radio tx` déjà testé sur RN2483 ?                         |
|P1-7 |1    |Distance typique entre ruches d'un même rucher ?                              |
|P1-9 |1    |Carte SD câblée et fonctionnelle ?                                            |
|P2-1 |2    |Combien de ruches par rucher ?                                                |
|P2-4 |2    |Disposition physique des ruches ?                                             |
|P2-6 |2    |SF7/SF9 passent-ils vers la passerelle Orange ?                               |
|P2-10|2    |Capacité batterie et puissance panneau solaire ?                              |
|P3-3 |3    |Blocages déjà observés en production ?                                        |
|P3-5 |3    |Problèmes I²C observés ?                                                      |
|P3-11|3    |Seuil tension critique batterie LiFePO4 ?                                     |

### Questions logicielles (arbitrages à faire ensemble)

|#    |Phase|Question                                                    |
|-----|-----|------------------------------------------------------------|
|P0-1 |0    |`config.h` : garder les notes ou faire un vrai header ?     |
|P0-7 |0    |Fichiers .bin : release tag ou supprimer ?                  |
|P0-9 |0    |`fonctiondemerdetoggle()` : utilisée quelque part ?         |
|P0-10|0    |`TODOHX711Set_Scale_Bal()` : garder ou supprimer ?          |
|P0-11|0    |Bugs derrière les marqueurs `?????` à documenter en issues ?|
|P0-17|0    |Implémenter un vrai watchdog (Sodaq_wdt) ?                  |
|P1-3 |1    |Résolution acceptable : poids 100g, temp 0.5°C ?            |
|P1-4 |1    |RucherID nécessaire dans le payload ?                       |
|P1-8 |1    |Combien de messages en file d'attente (RAM budget) ?        |
|P1-10|1    |Stratégie de retry (nb tentatives, backoff) ?               |
|P2-3 |2    |Fallback auto si collecteur HS ?                            |
|P2-9 |2    |Que faire si un nœud ne répond pas dans son slot ?          |
|P2-13|2    |Nb cycles ratés avant basculer en fallback ?                |
|P3-4 |3    |Comportement après reset watchdog ?                         |
|P3-8 |3    |Quels événements logger ?                                   |
|P3-12|3    |Mode hibernation batterie basse souhaité ?                  |

-----

## Suggestion de déroulement pour dimanche

### Matin (2-3h) — Cadrage + Phase 0

1. **30 min** : parcourir les questions ensemble, trancher les décisions structurantes
1. **30 min** : installer PlatformIO (si choisi), créer le fork, pousser les .md
1. **1-2h** : exécuter Phase 0 avec Claude Code (restructuration, nettoyage, factorisation)

### Après-midi (3-4h) — Phase 1

1. **30 min** : valider le format payload ensemble (taille, résolution, header)
1. **2-3h** : implémenter l'abstraction radio et le nouveau payload avec Claude Code
1. **30 min** : tester la compilation et vérifier la taille binaire

### Si le temps le permet

1. Commencer les tests de `mac pause` / `radio tx` sur le RN2483 physique (prépare Phase 2)

-----

## Comment utiliser ces documents avec Claude Code

1. **Forker le repo** sur GitHub
1. **Cloner** le fork localement
1. **Copier** le `CLAUDE.md` à la racine et les docs dans `docs/plan/`
1. **Ouvrir** Claude Code dans le dossier du projet
1. Claude Code lira automatiquement `CLAUDE.md` et connaîtra le contexte
1. Pour chaque étape, référencer le fichier de phase : "On attaque l'étape 0.3 du plan Phase 0"
1. Claude Code proposera le code, vous validez et testez

Le `CLAUDE.md` est le document le plus important : il donne à Claude Code toute la connaissance du projet, les conventions, les interdits, et les objectifs. Il remplace des heures d'explications.
