#ifndef TAD_SIOFARM_H
#define TAD_SIOFARM_H

#define MIDA_CUA_SIOFARM 16

extern unsigned char estatTxSioFarm;
#define SIOFARM_TxLliure() (estatTxSioFarm == 0)

void SIOFARM_Init(void);

unsigned char SIOFARM_HiHaCaracter(void);
char SIOFARM_LlegeixCaracter(void);
unsigned char SIOFARM_EnviaCaracter(char caracter);

void SIOFARM_Motor(void);

#endif
