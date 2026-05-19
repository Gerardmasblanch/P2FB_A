#ifndef TAD_SIO_H
#define TAD_SIO_H

#define PRODUCTES "P:"
#define ANIMALS "A:"
#define LINIA_NOVA "\r\n"
#define FINISH "F\r\n"
#define SLEEP_OK "Y\r\n"
#define SLEEP_NO "N\r\n"

void SIO_Init(void);

unsigned char SIO_PutChar(unsigned char ElValor);
unsigned char SIO_PutString(const char *text);
char *SIO_GetLine(void);
void SIO_Motor(void);

void SIO_InterrupcioRX(void);
void SIO_InterrupcioTX(void);

#endif
