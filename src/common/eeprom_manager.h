// eeprom_manager.h — Gestion configuration EEPROM AT24C32
// Porte depuis 24C32.cpp (POC_ATSAMD) — 95% portable
// Utilise Wire (I2C standard)

#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <Arduino.h>
#include <stdint.h>
#include "types.h"

// ===== FONCTIONS BAS NIVEAU EEPROM =====

// Ecrit un octet en EEPROM AT24C32
void EPR_24C32writeByte(uint16_t address, uint8_t data);

// Lit un octet depuis l'EEPROM AT24C32
uint8_t EPR_24C32readByte(uint16_t address);

// Ecrit un bloc de donnees en EEPROM
void EPR_24C32writeBlock(uint16_t address, uint8_t* data, uint16_t length);

// Lit un bloc de donnees depuis l'EEPROM
void EPR_24C32readBlock(uint16_t address, uint8_t* data, uint16_t length);

// ===== FONCTIONS DE CALCUL =====

// Calcule le checksum CRC16 de la configuration
uint16_t EPR_24C32calcChecksum(ConfigGenerale_t* cfg);

// ===== FONCTIONS DE GESTION CONFIGURATION =====

// Charge la configuration depuis l'EEPROM avec validation
void E24C32loadConfig(void);

// Lit la configuration brute depuis l'EEPROM
void E24C32readConfig(void);

// Sauvegarde la configuration en EEPROM avec checksum
void E24C32saveConfig(void);

// Initialise la configuration (lecture + validation)
void E24C32initConfig(void);

// ===== FONCTIONS DEBUG =====

// Affiche un tableau d'octets au format JSON hexadecimal
void E24C32printJSON(uint8_t* array, uint8_t length);

// Dump complet de la configuration au format JSON sur Serial
void E24C32DumpConfigToJSON(void);

#endif // EEPROM_MANAGER_H
