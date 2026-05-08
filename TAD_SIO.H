#ifndef TAD_SIO_H
#define TAD_SIO_H

/*
 * TAD_SIO
 *
 * Driver UART hardware del PIC18F4321.
 * Pins: TX=RC6, RX=RC7. Baudrate ~9600 amb Fosc=10 MHz.
 * Funciona per interrupcions (RCIE/TXIE).
 */

void          SIO_Init(void);                          // inicialitza UART i cues
void          SIO_End(void);                           // destructor (buit)

unsigned char SIO_RXAvail(void);                       // caracters disponibles a RX
unsigned char SIO_GetChar(void);                       // llegeix un byte de la cua RX

unsigned char SIO_TXAvail(void);                       // espais lliures a la cua TX
void          SIO_PutChar(unsigned char ElValor);      // posa un byte a enviar
void          SIO_PutString(unsigned char *LaFrase);   // posa una cadena sencera

void          SIO_InterrupcioRX(void);                 // cridar des de la RSI si RCIF
void          SIO_InterrupcioTX(void);                 // cridar des de la RSI si TXIF

#endif