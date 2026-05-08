#ifndef TAD_SERIAL_H
#define TAD_SERIAL_H

void Serial_Init(void);
unsigned char Serial_CharAvail(void);
void Serial_PutChar(unsigned char c);
unsigned char Serial_TxAvail(void);
unsigned char Serial_GetChar(void);

#endif