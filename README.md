# TALENT-EdgeAI-Weather-STM32

**Edge-AI Weather Prediction for Mountain Safety — STM32 N657X0-Q + NPU Neural-ART**

> Stage L3 — Université Savoie Mont Blanc / CROMA (Grenoble INP – CNRS)
> Projet de recherche **TALENT**: *Intelligent Management of Mobility and Parking in Mountain Low-Density Heritage Cities*

---

## 📖 Description

Ce dépôt contient les travaux réalisés dans le cadre du projet **TALENT**, visant à concevoir et déployer une solution d'**Intelligence Artificielle embarquée (Edge-AI)** pour la prédiction météorologique en environnements de montagne à haute altitude. L'objectif est d'améliorer la sécurité des sports de plein air (parapente, randonnée, ski) grâce à une inférence locale ultra-basse consommation, sans dépendance au Cloud.

Le cœur du système est **METEONET v2**, un réseau de neurones léger entraîné sur ~87 700 observations météo réelles filtrées en haute altitude, atteignant **99,8% d'exactitude** sur la classification pluie/beau temps à partir de seulement 3 variables (Température, Humidité, Pression), avec une empreinte mémoire de **384 octets de RAM** après quantification INT8.

## ✨ Points clés

- 🧠 **Modèle léger** : Dense(3→64→32→1), Adam, RMS loss, ReLU, quantifié INT8
- 🎯 **99,8% d'exactitude** (pluie/beau temps) — modèle multi-sortie étendu à 82,6% avec prédiction de luminosité
- 🔌 **Cible matérielle** : STM32 N657X0-Q (ARM Cortex-M55 + NPU ATON Neural-ART)
- 📡 **Transmission** : LoRaWAN via module Seeed LoRa-E5
- 🌡️ **Capteurs embarqués** : HTS221 (T°/Humidité), LPS22HH (Pression), TSL230BR (Luminosité)
- ⚡ **Latence d'inférence** : 0,21 ms (INT8) vs 1,42 ms (Float32) — gain de 85,2%
- 🏔️ **Validation in-situ** au Mont Revard (1500 m) sur deux scénarios (événement transitoire, environnement saturé)

## 🏗️ Architecture du pipeline

```
main.c (3 appels)
 ├── periph_init.c/.h   → GPIO, UART, I2C, capteurs
 ├── sensors.c/.h       → Acquisition (g_sensors)
 └── display.c/.h       → Formatage UART

Drivers capteurs :
 ├── hts221_polling.c   → Température + Humidité   → hts221_reg.c (I2C)
 └── pressure_polling.c → Pression atmosphérique    → lps22hh_reg.c (I2C)
```

**Boucle applicative (période 2 s)** :
1. Acquisition capteurs (`sensors_read()`)
2. Affichage UART (`display_data()`)
3. Quantification INT8 (`acquire_and_process_data()`)
4. Inférence NPU ATON (`aiRun()`)
5. Post-traitement, calcul altitude, envoi LoRa (`post_process()`)

## 📊 Résultats

| Précision du modèle | Latence d'inférence | RAM utilisée | Flash |
|---|---|---|---|
| Float32 (non compressé) | 1,42 ms | 1,51 Ko | 12,4 Ko |
| **INT8 quantifié** | **0,21 ms** | **384 o** | **2,9 Ko** |
| Gain obtenu | −85,2% | −74,5% | −76,6% |

- **Modèle n°82** (référence, sans altitude) : Train acc = Test acc = **99,8%**
- **Modèle étendu** (+ sortie luminosité) : Train acc = 82,7% / Test acc = 82,6%

## 🗂️ Structure du dépôt

```
├── README.md
├── data/
│   └── base_universelle_meteo.csv       # (non inclus - données sources)
├── training/
│   ├── train_meteonet.py                # Script d'entraînement (Annexe A du rapport)
│   └── scaler.pkl                        # Normalisation Z-score sauvegardée
├── firmware/                             # Code embarqué STM32 (C)
│   ├── main.c
│   ├── periph_init.c/.h
│   ├── sensors.c/.h
│   ├── display.c/.h
│   ├── hts221_polling.c / hts221_reg.c
│   └── pressure_polling.c / lps22hh_reg.c
├── lorawan/
│   └── ttn_payload_decoder.js            # Décodeur de payload TTN (Annexe B)
├── docs/
│   └── rapport_stage_TALENT.pdf          # Rapport de stage complet
└── LICENSE
```

## ⚙️ Installation & Utilisation

### Prérequis
```bash
pip install pandas numpy scikit-learn tensorflow matplotlib
```

### Entraînement du modèle
```bash
python training/train_meteonet.py
```

### Décodeur LoRaWAN (TTN)
Copier le contenu de `lorawan/ttn_payload_decoder.js` dans l'onglet **Payload Formatters** de votre application The Things Network (fonction `decodeUplink`).

### Format de la trame LoRa
```
T:29.1,H:76.0,P:988.8,A:241,R:33,B:67,D:1,L:92
```
`T`=température, `H`=humidité, `P`=pression, `A`=altitude(m), `R`=proba pluie(%), `B`=proba beau(%), `D`=jour/nuit, `L`=luminosité(%)

## 🔬 Contexte scientifique

Ce travail s'inscrit dans le continuum **6G IoT-Edge-Cloud**, où l'intelligence est déportée à l'extrême périphérie du réseau pour pallier l'absence de connectivité dans les zones blanches de montagne (cas d'étude : Covilhã, Portugal et Chambéry/Mont Revard, France).

## 📚 Références

- M. Mezlini, T. Lacrevaz, F. J. Velez, P. M. Ferreira, *"Sustainable Goal-Oriented Weather Prediction in the 6G IoT-Edge AI-Cloud Mountain Sports Continuum"*, Proc. of IEEE, 2026.
- M. Mezlini, T. Lacrevaz, F. J. Velez, P. M. Ferreira, *"TALENT: Intelligent Management of Mobility and Parking in Mountain Low-Density Heritage Cities"*, Proc. of IEEE, 2026.

*(Liste complète des références dans le rapport de stage, section Bibliographie)*

## 👤 Auteure

**MEZLINI Maram** — Stage L3, Électronique, Systèmes Embarqués, Télécommunication (ESET)
Sous la direction de : Prof. Pietro M. FERREIRA, Prof. Thierry LACREVAZ (USMB/CROMA), Prof. Vigneron Pierre-Baptiste

Laboratoire **CROMA** (Centre de Radiofréquences, Optique et Micro-nanoélectronique des Alpes) — Grenoble INP / CNRS / Université Savoie Mont Blanc

## 📄 Licence

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.21457136.svg)](https://doi.org/10.5281/zenodo.21457136)


Ce projet est distribué sous licence MIT (voir `LICENSE`) — à adapter selon les règles de votre laboratoire/université avant publication publique.
