# Analyse de la plateforme BEEP comme backend complémentaire

**Date** : 28 février 2026
**Contexte** : le projet dispose déjà d'un Prometheus + Grafana hébergé. L'idée est d'évaluer BEEP (beep.nl) comme complément gratuit avec une interface apicole spécialisée.

-----

## Qu'est-ce que BEEP ?

Plateforme open source (AGPLv3) de monitoring de ruches, développée par la fondation BEEP (Pays-Bas). 5000+ utilisateurs, 9 langues dont le français. Gratuite, auto-hébergeable.

- **Site** : https://beep.nl
- **App** : https://app.beep.nl
- **API** : https://api.beep.nl/docs/
- **Code** : https://github.com/beepnl/BEEP

-----

## Intérêts

| Intérêt | Détail |
|---|---|
| **Gratuit et illimité** | Pas de quota, pas d'abonnement, fondation à but non lucratif |
| **Interface apicole spécialisée** | Gestion ruchers/ruches, inspections, checklists — ce que Grafana ne fait pas |
| **API simple** | Un POST avec paires clé/valeur suffit |
| **Données compatibles** | Toutes nos mesures ont un mapping direct (voir ci-dessous) |
| **Communauté** | 5000+ apiculteurs, partage de données pour la recherche |
| **Open source** | Auto-hébergement possible (Docker) si le service s'arrête |
| **Backup gratuit** | Deuxième copie des données sans coût |

-----

## Mapping des données POC_ATSAMD → BEEP

| Donnée projet | Champ BEEP | Compatible |
|---|---|---|
| `DHT_Temp` | `t_i` (température intérieure) | Oui |
| `DHT_Hum` | `h` (humidité) | Oui |
| `Brightness` (LDR) | `l` (luminosité) | Oui |
| `Bat_Voltage` | `bv` (tension batterie) | Oui |
| `Solar_Voltage` | Pas de champ standard | Utiliser un champ custom |
| `HX711Weight[0]` | `w_fl` (avant-gauche) | Oui |
| `HX711Weight[1]` | `w_fr` (avant-droite) | Oui |
| `HX711Weight[2]` | `w_bl` (arrière-gauche) | Oui |
| `HX711Weight[3]` | `w_br` (arrière-droite) | Oui |

### Exemple d'envoi

```bash
curl -X POST "https://api.beep.nl/api/sensors?key=<DEVICE_KEY>&t_i=25.3&h=65&bv=3.85&w_fl=12.5&w_fr=11.8&w_bl=13.1&w_br=12.9&l=500"
```

-----

## Risques

| Risque | Gravité | Probabilité | Mitigation |
|---|---|---|---|
| Arrêt du service BEEP | Élevée | Faible | Auto-hébergement Docker possible ; Prometheus reste la source primaire |
| Panne temporaire | Faible | Moyenne | File d'attente dans le pont ; données non perdues (Prometheus les a) |
| Changement de l'API | Moyenne | Moyenne | Adapter le pont ; API versionnée |
| Propriété des données | Faible | Très faible | Fondation NL, conforme RGPD, données restent propriété de l'utilisateur |
| Dépendance à un tiers | Moyenne | — | Utiliser BEEP en **complément**, jamais comme source unique |

-----

## Solution technique : pont Live Objects → BEEP

### Le problème

BEEP a une intégration native avec TTN (The Things Network), mais **pas avec Orange Live Objects**. Il faut un intermédiaire.

### Architecture recommandée

```
ATSAMD21 + RN2483A
    │
    │ LoRaWAN (payload 19 octets)
    ▼
Orange Live Objects
    │
    │ MQTT (FIFO)
    ▼
Node-RED (sur le serveur hébergeur, à côté de Prometheus)
    │
    ├──→ Prometheus (pushgateway) ← existant, source primaire
    │
    └──→ BEEP (POST api.beep.nl/api/sensors) ← complément gratuit
```

### Décodeur de payload (à intégrer dans Node-RED ou script Python)

```python
import struct

def decode_poc_payload(hex_payload):
    """Décode le payload 19 octets du firmware POC_ATSAMD"""
    buf = bytes.fromhex(hex_payload)
    return {
        'rucher_id': buf[0],
        't_i':  struct.unpack_from('<h', buf, 1)[0] / 100.0,   # temperature
        'h':    struct.unpack_from('<h', buf, 3)[0] / 100.0,   # humidite
        'l':    struct.unpack_from('<h', buf, 5)[0],            # luminosite
        'bv':   struct.unpack_from('<h', buf, 7)[0] / 100.0,   # vbat
        'vsol': struct.unpack_from('<h', buf, 9)[0] / 100.0,   # vsol
        'w_fl': struct.unpack_from('<h', buf, 11)[0] / 100.0,  # poids A
        'w_fr': struct.unpack_from('<h', buf, 13)[0] / 100.0,  # poids B
        'w_bl': struct.unpack_from('<h', buf, 15)[0] / 100.0,  # poids C
        'w_br': struct.unpack_from('<h', buf, 17)[0] / 100.0,  # poids D
    }
```

### Alternatives au pont

| Option | Effort | Avantage | Inconvénient |
|---|---|---|---|
| **Node-RED sur le serveur** | 1-2 jours | Flexible, visuel, facile à maintenir | Un composant de plus |
| **Script Python cron** | 1 jour | Simple | Moins réactif (polling vs push) |
| **Migrer vers TTN** | 2-3 jours | Intégration BEEP native | Perte du réseau Orange, couverture différente |
| **Envoyer depuis le firmware** | Non viable | — | Le RN2483 ne fait pas de HTTP, et le payload LoRaWAN ne va pas directement à BEEP |

-----

## BEEP vs Prometheus+Grafana

| Critère | BEEP | Prometheus + Grafana |
|---|---|---|
| **Coût** | Gratuit (hébergé par BEEP) | Coût serveur (déjà payé) |
| **Spécialisation apicole** | Oui (inspections, checklists, ruchers) | Non, tout est à configurer |
| **Personnalisation dashboards** | Faible (interface fixe) | Illimitée |
| **Alertes** | Basiques | Avancées (AlertManager) |
| **Contrôle des données** | Chez un tiers (NL, RGPD) | Total |
| **Communauté apicole** | Oui (5000+ membres) | Non |
| **Pérennité** | Dépend de la fondation | Vous-même |

### Verdict

**Ils ne sont pas en concurrence, ils sont complémentaires.**

- **Prometheus/Grafana** = source de vérité, alertes, contrôle total
- **BEEP** = interface apicole gratuite, backup, communauté

-----

## Plan d'action si on décide d'y aller

1. **Tester manuellement** : créer un compte sur app.beep.nl, créer un rucher/ruche, envoyer des données test avec `curl`
2. **Évaluer la valeur ajoutée** : est-ce que l'interface apicole apporte quelque chose par rapport à Grafana ?
3. **Si oui** : mettre en place le pont Node-RED sur le serveur hébergeur
4. **Monitorer** : vérifier que les données arrivent bien des deux côtés

**Décision à prendre avec Philippe** : est-ce que l'interface apicole l'intéresse, ou est-ce que Grafana lui suffit ?
