#ifndef TAD_LDR_H
#define TAD_LDR_H

/*
 * TAD_LDR
 *
 * Llegeix un sensor de llum LDR connectat al canal AN2 (RA2).
 * Esquema de muntatge:
 *
 *       +5V
 *        |
 *       LDR
 *        |
 *        +------> RA2 / AN2
 *        |
 *       10k
 *        |
 *       GND
 *
 * Es recolza en el TAD_ADC. Cal cridar AD_Init() abans de LDR_Init().
 */

void          LDR_Init(void);

unsigned char LDR_GetLlum(void);          // 0 = fosc total, 255 = molt clar

unsigned char LDR_HiHaPocaLlum(void);     // 1 si esta tapat (sota llindar), 0 si no

#endif