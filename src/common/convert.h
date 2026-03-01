// ---------------------------------------------------------------------------*
// convert.h — Fonctions de conversion hexadecimale et decimale
// Projet POC_ATSAMD : Surveillance de Ruches
// ---------------------------------------------------------------------------*
#ifndef CONVERT_H
#define CONVERT_H

#include <Arduino.h>
#include <stdint.h>

// ===== Conversions hexadecimales =====

// Convertit un caractère hexadécimal en valeur numérique (nibble)
uint8_t hexCharToNibble(char c);

// Convertit une valeur numérique (nibble) en caractère hexadécimal
char nibbleToHexChar(uint8_t nibble);

// Convertit une chaîne hexadécimale en tableau d'octets (version avec macros)
void CONVERTfconvertByteArray(const char *source, uint8_t *destination, uint8_t len);

// Convertit une chaîne hexadécimale en tableau d'octets (version avec validation)
bool hexStringToByteArray(const char* hexString, uint8_t* byteArray, uint8_t maxBytes);

// Convertit un tableau d'octets en chaîne hexadécimale
bool byteArrayToHexString(const uint8_t* byteArray, uint8_t numBytes, char* hexString, uint8_t maxChars);

// Convertit une chaîne hexadécimale en tableau d'octets (version simple)
void convertByteArray(const char *source, uint8_t *destination, uint8_t len);

// Convertit un tableau d'octets en chaîne hexadécimale (version simple)
void convertToHexString(const uint8_t *source, char *destination, uint8_t len);

// ===== Conversions decimales (uint8_t <-> char buffer) =====

// Convertit un uint8_t en chaîne décimale
bool uint8ToDecimalString(uint8_t value, char* buffer, uint8_t maxChars);

// Convertit une chaîne décimale en uint8_t
bool decimalStringToUint8(const char* buffer, uint8_t* value);

// ===== Validation Spreading Factor LoRaWAN =====

// Vérifie si un Spreading Factor est valide pour LoRaWAN
bool isValidLoRaWanSF(uint8_t sf);

// Convertit et valide un Spreading Factor LoRaWAN depuis une chaîne
bool validateLoRaWanSF(const char* sfString, uint8_t* sfValue);

// ===== Affichage debug =====

// Affiche un tableau d'octets en hexadécimal sur le port série
void printByteArray(const uint8_t* byteArray, uint8_t length);

// Affiche une chaîne hexadécimale sur le port série
void printHexString(const char* hexString);

// ===== Tests =====

// Fonction de test complète des conversions
void TestConvert(void);

#endif // CONVERT_H
