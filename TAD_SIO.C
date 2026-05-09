#include <xc.h>
#include "pic18f4321.h"
#include "TAD_SIO.H"

/*
 * UART hardware amb RC6 com a TX i RC7 com a RX.
 * Fosc = 10 MHz, baudrate aproximat = 9600.
 */

#define CONFIGURACIO_TXSTA 0x24
#define CONFIGURACIO_RCSTA 0x90
#define DIVISOR_BAUDRATE 64

#define MAX_RX 32
#define MASK_RX 0x1F

#define MAX_TX 32
#define MASK_TX 0x1F

static unsigned char CuaRX[MAX_RX];
static unsigned char IniciRX;
static unsigned char FiRX;
static unsigned char QuantsRX;

static unsigned char CuaTX[MAX_TX];
static unsigned char IniciTX;
static unsigned char FiTX;
static unsigned char QuantsTX;

void SIO_Init(void) {
    IniciRX = 0;
    FiRX = 0;
    QuantsRX = 0;

    IniciTX = 0;
    FiTX = 0;
    QuantsTX = 0;

    TRISCbits.TRISC6 = 0;   // TX com a output
    TRISCbits.TRISC7 = 1;   // RX com a input

    BAUDCONbits.BRG16 = 0;
    TXSTA = CONFIGURACIO_TXSTA;
    RCSTA = CONFIGURACIO_RCSTA;
    SPBRG = DIVISOR_BAUDRATE;

    PIE1bits.RCIE = 1;
    PIE1bits.TXIE = 0;
    INTCONbits.PEIE = 1;
}

void SIO_InterrupcioRX(void) {
    unsigned char Valor;

    Valor = RCREG;  // llegir RCREG esborra RCIF

    if(RCSTAbits.OERR) {
        RCSTAbits.CREN = 0;
        RCSTAbits.CREN = 1;
    }

    if(QuantsRX < MAX_RX) {
        CuaRX[IniciRX] = Valor;
        IniciRX++;
        IniciRX &= MASK_RX;
        QuantsRX++;
    }
}

void SIO_InterrupcioTX(void) {
    if(QuantsTX != 0) {
        TXREG = CuaTX[FiTX];
        FiTX++;
        FiTX &= MASK_TX;
        QuantsTX--;
    } else {
        PIE1bits.TXIE = 0;
    }
}

unsigned char SIO_RXAvail(void) {
    return QuantsRX;
}

unsigned char SIO_GetChar(void) {
    unsigned char ElValor;

    di();

    ElValor = CuaRX[FiRX];
    FiRX++;
    FiRX &= MASK_RX;
    QuantsRX--;

    ei();

    return ElValor;
}

unsigned char SIO_TXAvail(void) {
    return MAX_TX - QuantsTX;
}

void SIO_PutChar(unsigned char ElValor) {
    if((PIR1bits.TXIF == 1) && (QuantsTX == 0)) {
        TXREG = ElValor;
        
    } else {
        di();

        if(QuantsTX < MAX_TX) {
            CuaTX[IniciTX] = ElValor;
            IniciTX++;
            IniciTX &= MASK_TX;
            QuantsTX++;
        }

        ei();
        PIE1bits.TXIE = 1;
    }
}

void SIO_PutString(unsigned char *LaFrase) {
    unsigned char Index;

    Index = 0;

    while(LaFrase[Index] != 0x00) {
        SIO_PutChar(LaFrase[Index]);
        Index++;
    }
}

void SIO_End(void) {
}
