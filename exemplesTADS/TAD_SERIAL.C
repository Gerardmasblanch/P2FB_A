#include "TAD_SERIAL.H"
#include <xc.h>

#define CONFIGURACIO_TXSTA 0x24
#define CONFIGURACIO_RCSTA 0x90
#define DIVISOR_BAUDRATE   64


void Serial_Init(void) {
    TRISCbits.TRISC6 = 1; // TX --> transmetre
    TRISCbits.TRISC7 = 1; // RX --> rebre

    BAUDCONbits.BRG16 = 0;
    TXSTA = CONFIGURACIO_TXSTA;
    RCSTA = CONFIGURACIO_RCSTA;
    SPBRG = DIVISOR_BAUDRATE;

}

unsigned char Serial_CharAvail(void) {
    return RCIF;
}

unsigned char Serial_GetChar(void) {
    return RCREG;
}

unsigned char Serial_TxAvail(void) {
    return TXIF;
}

void Serial_PutChar(unsigned char c) {
    TXREG = c;
}

