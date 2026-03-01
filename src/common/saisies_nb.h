// ---------------------------------------------------------------------------*
// saisies_nb.h — Machines a etats pour saisies non-bloquantes
// Projet POC_ATSAMD : Surveillance de Ruches
//
// Saisies : liste, numerique, alphanumerique, hexadecimale,
//           heure, date, adresse IP, email.
// Chaque saisie fonctionne comme une machine a etats appelee
// depuis la loop() principale (non-bloquant).
// ---------------------------------------------------------------------------*
#ifndef SAISIES_NB_H
#define SAISIES_NB_H

#include <Arduino.h>
#include <stdint.h>
#include "types.h"
#include "config.h"

// ===== VARIABLES EXTERNES NECESSAIRES =====
// Ces variables doivent etre definies par l'appelant :
//
// listInputContext_t   listInputCtx;       // Contexte saisie liste
// numInputContext_t    numInputCtx;        // Contexte saisie numerique
// stringInputContext_t stringInputCtx;     // Contexte saisie alphanum
// hexInputContext_t    hexInputCtx;        // Contexte saisie hexa
// timeInputContext_t   timeInputCtx;       // Contexte saisie heure
// dateInputContext_t   dateInputCtx;       // Contexte saisie date
// emailInputContext_t  emailInputCtx;      // Contexte saisie email
// ipInputContext_t     ipInputCtx;         // Contexte saisie IP
// key_code_t           touche;             // Touche clavier en cours
// bool                 startupListActivated; // Flag menu demarrage
// uint8_t              currentMenuDepth;   // Profondeur menu actuel
// bool                 displayStringDebug; // Affichage debug string
// char                 emailCharSet[];     // Jeu de caracteres email
// uint8_t              emailCharSetSize;   // Taille jeu email
// menuLevel_t          menuStack[];        // Pile de menus

// ===== FONCTIONS DE SELECTION DANS UNE LISTE =====

// Initialise l'affichage de la liste au demarrage
void initStartupList(void);

// Demarre la selection dans une liste avec timeout
void startListInput(const char* title, const char** itemList,
                    uint8_t numItems, uint8_t initialIndex,
                    unsigned long timeoutMs = 0);

// Traite la selection dans la liste (appel depuis loop)
listInputState_t processListInput(void);

// Finalise la selection et recupere l'item choisi
uint8_t finalizeListInput(char* outputItem);

// Annule la selection dans la liste
void cancelListInput(void);

// Verifie si une selection est en cours
bool isListInputActive(void);

// Rafraichit l'affichage de la liste avec curseur et defilement
void refreshListDisplay(void);

// Gere le clignotement du curseur pour la liste
void updateListInputCursorBlink(void);

// ===== FONCTIONS DE SAISIE NUMERIQUE =====

// Demarre la saisie numerique non-bloquante
void startNumInput(const char* title, const char* initialNum,
                   uint8_t maxLen, bool allowNeg, bool allowDec,
                   long minVal, long maxVal);

// Traite la saisie numerique (appel depuis loop)
numInputState_t processNumInput(void);

// Finalise la saisie et recupere le nombre
void finalizeNumInput(char* outputNum);

// Annule la saisie numerique
void cancelNumInput(void);

// Verifie si une saisie numerique est active
bool isNumInputActive(void);

// Rafraichit l'affichage du nombre avec curseur
void refreshNumDisplay(void);

// Verifie si un nombre est valide
bool isNumValid(const char *num, bool allowNeg, bool allowDec,
                long minVal, long maxVal);

// Obtient le caractere numerique suivant/precedent
char getNextNumChar(char current, int delta, bool allowNeg, bool allowDec);

// Insere un caractere a la position donnee
void insertNumCharAtPosition(char *num, uint8_t *length,
                              uint8_t pos, char c);

// Supprime un caractere a la position donnee
void deleteCharAtPosition(char *num, uint8_t *length, uint8_t pos);

// Met a jour le decalage d'affichage (scroll horizontal)
void updateNumDisplayOffset(void);

// Gere le clignotement du curseur numerique
void updateNumInputCursorBlink(void);

// ===== FONCTIONS DE SAISIE ALPHANUMERIQUE =====

// Obtient le caractere alphanum suivant/precedent
char getNextAlphaNumChar(char current, int delta);

// Modifie un caractere de la chaine
void modifyStringChar(char* str, uint8_t pos, int delta);

// Demarre la saisie alphanum non-bloquante
void startStringInput(const char* title, const char* initialString,
                      uint8_t maxLength);

// Traite la saisie alphanum (appel depuis loop)
stringInputState_t processStringInput(void);

// Finalise la saisie et recupere la chaine
void finalizeStringInput(char* outputString);

// Annule la saisie alphanum
void cancelStringInput(void);

// Verifie si une saisie alphanum est active
bool isStringInputActive(void);

// Gere le clignotement du curseur alphanum
void updateStringInputCursorBlink(void);

// Rafraichit l'affichage de la chaine avec curseur
void refreshStringDisplay(void);

// ===== FONCTIONS DE SAISIE HEXADECIMALE =====

// Verifie si une chaine hexa est valide
bool isHexStringValid(const char *hex, uint8_t expectedLength);

// Obtient le caractere hexa suivant/precedent
char getNextHexChar(char current, int delta);

// Modifie un caractere hexadecimal
void modifyHexDigit(char *hex, uint8_t pos, int delta);

// Demarre la saisie hexadecimale non-bloquante
void startHexInput(const char* title, const char* initialHex,
                   uint8_t maxLen);

// Traite la saisie hexadecimale (appel depuis loop)
hexInputState_t processHexInput(void);

// Rafraichit l'affichage de la chaine hexa avec curseur
void refreshHexDisplay(void);

// Finalise la saisie et recupere la chaine hexa
void finalizeHexInput(char* outputHex);

// Annule la saisie hexadecimale
void cancelHexInput(void);

// Verifie si une saisie hexa est en cours
bool isHexInputActive(void);

// Met a jour le decalage d'affichage hexa
void updateHexDisplayOffset(void);

// Gere le clignotement du curseur hexa
void updateHexInputCursorBlink(void);

// ===== FONCTIONS DE SAISIE HEURE =====

// Verifie si une heure est valide (format HH:MM:SS)
bool isTimeValid(const char *t);

// Demarre la saisie d'heure non-bloquante
void startTimeInput(const char* title, const char* initialTime);

// Traite la saisie d'heure (appel depuis loop)
timeInputState_t processTimeInput(void);

// Finalise la saisie et recupere l'heure
void finalizeTimeInput(char* outputTime);

// Annule la saisie d'heure
void cancelTimeInput(void);

// Verifie si une saisie d'heure est en cours
bool isTimeInputActive(void);

// Rafraichit l'affichage de l'heure avec curseur
void refreshTimeDisplay(void);

// Gere le clignotement du curseur heure
void updateTimeInputCursorBlink(void);

// Calcule la prochaine position valide du curseur heure
uint8_t getNextValidTimePosition(uint8_t currentPos, bool forward);

// Modifie un chiffre d'une heure avec gestion des limites
void modifyTimeDigit(char *t, uint8_t pos, int delta);

// ===== FONCTIONS DE SAISIE DATE =====

// Verifie si une date est valide (format JJ/MM/AAAA)
bool isDateValid(const char *d);

// Demarre la saisie de date non-bloquante
void startDateInput(const char* title, const char* initialDate);

// Traite la saisie de date (appel depuis loop)
dateInputState_t processDateInput(void);

// Finalise la saisie et recupere la date
void finalizeDateInput(char* outputDate);

// Annule la saisie de date
void cancelDateInput(void);

// Verifie si une saisie de date est en cours
bool isDateInputActive(void);

// Rafraichit l'affichage de la date avec curseur
void refreshDateDisplay(void);

// Gere le clignotement du curseur date
void updateDateInputCursorBlink(void);

// Calcule la prochaine position valide du curseur date
uint8_t getNextValidDatePosition(uint8_t currentPos, bool forward);

// Modifie un chiffre d'une date avec gestion des limites
void modifyDateDigit(char *d, uint8_t pos, int delta);

// ===== FONCTIONS DE SAISIE IP =====

// Verifie si une adresse IP est valide (format XXX.XXX.XXX.XXX)
bool isIPValid(const char *ip);

// Calcule la prochaine position valide du curseur IP
uint8_t getNextValidIPPosition(uint8_t currentPos, bool forward);

// Modifie un chiffre d'une adresse IP avec gestion des limites
void modifyIPDigit(char *ip, uint8_t pos, int delta);

// Demarre la saisie IP non-bloquante
void startIPInput(const char* initialIP);

// Traite la saisie IP (appel depuis loop)
ipInputState_t processIPInput(void);

// Finalise la saisie et recupere l'adresse IP
void finalizeIPInput(char* outputIP);

// Annule la saisie IP
void cancelIPInput(void);

// Verifie si une saisie IP est en cours
bool isIPInputActive(void);

// Rafraichit l'affichage de l'IP avec curseur
void refreshIPDisplay(void);

// Gere le clignotement du curseur IP
void updateIPInputCursorBlink(void);

// ===== FONCTIONS DE SAISIE EMAIL =====

// Verifie si une adresse email est valide (basique)
bool isEmailValid(const char *email);

// Obtient le caractere email suivant/precedent
char getNextEmailChar(char current, int delta);

// Insere un caractere email a la position donnee
void insertEmailCharAtPosition(char *email, uint8_t *length,
                                uint8_t pos, char c);

// Supprime un caractere email a la position donnee
void deleteEmailCharAtPosition(char *email, uint8_t *length,
                                uint8_t pos);

// Demarre la saisie email non-bloquante
void startEmailInput(const char* initialEmail);

// Traite la saisie email (appel depuis loop)
emailInputState_t processEmailInput(void);

// Finalise la saisie et recupere l'adresse email
void finalizeEmailInput(char* outputEmail);

// Annule la saisie email
void cancelEmailInput(void);

// Verifie si une saisie email est en cours
bool isEmailInputActive(void);

// Rafraichit l'affichage de l'email avec curseur
void refreshEmailDisplay(void);

// Gere le clignotement du curseur email
void updateEmailInputCursorBlink(void);

// Met a jour le decalage d'affichage email
void updateEmailDisplayOffset(void);

#endif // SAISIES_NB_H
