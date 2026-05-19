#ifndef TAD_SIOFARM_H
#define TAD_SIOFARM_H

#define BORRAR "\b \b"
#define DATA_CORRECTA "\n\rDate and time correct\n\r"
#define DATA_INCORRECTA "\n\rPlease input a correct date\n\r"

#define MISSATGE_DATA_CORRECTA DATA_CORRECTA
#define MISSATGE_DATA_INCORRECTA DATA_INCORRECTA
#define MISSATGE_BORRAR BORRAR

void SIOFARM_Init(void);

char SIOFARM_LlegeixCaracter(void);
unsigned char SIOFARM_EnviaCaracter(char caracter);
void SIOFARM_IniciaMissatge(const char *missatge);

void SIOFARM_Motor(void);

#endif
