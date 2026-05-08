#ifndef TAD_SIOFARM_H
#define TAD_SIOFARM_H

/*
 * TAD_SIOFARM
 *
 * UART per software (bit-banging).
 * Pins: TX = RD1, RX = RD0.
 * Velocitat: 1200 bauds (Timer0 a 417 us, 2 tics = 1 bit).
 *
 * IMPORTANT: SIOFARM_Motor() s'ha de cridar des del bucle principal.
 * Internament avanca nomes quan ha passat un tic de Timer0.
 */

#define MIDA_CUA_SIOFARM 16

void          SIOFARM_Init(void);

unsigned char SIOFARM_EnviaCaracter(char caracter);
unsigned char SIOFARM_HiHaCaracter(void);
char          SIOFARM_LlegeixCaracter(void);

void          SIOFARM_Motor(void);

#endif
