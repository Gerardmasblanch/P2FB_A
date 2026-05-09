#ifndef TAD_SIOFARM_H
#define TAD_SIOFARM_H

#define MIDA_CUA_SIOFARM 16

void SIOFARM_Init(void);

unsigned char SIOFARM_EnviaCaracter(char caracter);
unsigned char SIOFARM_HiHaCaracter(void);
char SIOFARM_LlegeixCaracter(void);

void SIOFARM_Motor(void);

#endif
