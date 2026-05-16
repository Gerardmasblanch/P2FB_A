#ifndef TAD_SIO_H
#define TAD_SIO_H

/*
 * TAD_SIO
 *
 * UART hardware del PIC18F4321.
 * Pins: TX = RC6, RX = RC7.
 * Baudrate aproximat: 9600 amb Fosc = 10 MHz.
 */

void SIO_Init(void);

unsigned char SIO_PutChar(unsigned char ElValor);
unsigned char SIO_PutString(const char *text);
char *SIO_GetLine(void);
void SIO_Motor(void);

void SIO_InterrupcioRX(void);
void SIO_InterrupcioTX(void);

#endif
