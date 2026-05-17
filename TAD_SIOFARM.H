#ifndef TAD_SIOFARM_H
#define TAD_SIOFARM_H

void SIOFARM_Init(void);

char SIOFARM_LlegeixCaracter(void);
unsigned char SIOFARM_EnviaCaracter(char caracter);
unsigned char SIOFARM_TxBuida(void);

void SIOFARM_Motor(void);

#endif
