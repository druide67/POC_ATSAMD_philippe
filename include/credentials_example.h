// credentials_example.h — Copier vers credentials.h et remplir les valeurs
// credentials.h est dans .gitignore — JAMAIS versionner les cles !

#ifndef CREDENTIALS_H
#define CREDENTIALS_H

// --- WiFi (master uniquement) ---
#define WIFI_SSID     "MonReseau"
#define WIFI_PASSWORD "MonMotDePasse"

// --- LoRaWAN OTAA (master uniquement) ---
// Recuperer ces valeurs depuis la console Orange Live Objects
#define LORA_DEV_EUI  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define LORA_JOIN_EUI { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define LORA_APP_KEY  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define LORA_NWK_KEY  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

// --- BLE Passkey (6 chiffres, identique sur master et slaves) ---
#define BLE_PASSKEY   123456

#endif // CREDENTIALS_H
