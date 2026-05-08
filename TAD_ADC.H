#ifndef TAD_ADC_H
#define TAD_ADC_H

/*
 * TAD_ADC
 *
 * Multiplexa diversos canals del ADC del PIC18F4321.
 * Cada client (LDR, JOYSTICK...) registra un canal AN i obte un index.
 * El motor recorre els canals registrats fent una conversio a cada un.
 * Resolucio de sortida: 8 bits (0-255), justificacio esquerra.
 *
 * Canals analogics actius: AN0, AN1, AN2 (RA0, RA1, RA2).
 */

#define AD_MAX_CANALS 4

void          AD_Init(void);

// Registra un canal AN i retorna l'index intern (0..AD_MAX_CANALS-1).
// Retorna 0xFF si no queda espai.
unsigned char AD_RegistraCanal(unsigned char canalAN);

// Retorna l'ultima mostra llegida d'aquest index (0-255).
unsigned char AD_GetMostra(unsigned char index);

// Motor cooperatiu, cridar des del bucle principal.
void          AD_Motor(void);

#endif