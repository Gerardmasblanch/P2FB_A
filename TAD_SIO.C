#include <xc.h>
#include "pic18f4321.h"
#include "TAD_SIO.H"

// -----------------------------------------------------------------------------
// CONFIGURACIO UART
// Fosc = 10 MHz
// Baudrate aproximat = 9600
// BRG16 = 0
// BRGH = 1
// SPBRG = 64
// -----------------------------------------------------------------------------

#define CONFIGURACIO_TXSTA 0x24
#define CONFIGURACIO_RCSTA 0x90
#define DIVISOR_BAUDRATE   64

// -----------------------------------------------------------------------------
// CONSTANTS CUES CIRCULARS
// -----------------------------------------------------------------------------

#define MAX_RX   32
#define MASK_RX  0x1F

#define MAX_TX   32
#define MASK_TX  0x1F

// -----------------------------------------------------------------------------
// VARIABLES PRIVADES DEL TAD
// -----------------------------------------------------------------------------

static unsigned char CuaRX[MAX_RX];
static unsigned char IniciRX;
static unsigned char FiRX;
static unsigned char QuantsRX; 

static unsigned char CuaTX[MAX_TX];
static unsigned char IniciTX;
static unsigned char FiTX;
static unsigned char QuantsTX;

// -----------------------------------------------------------------------------
// CONSTRUCTOR DEL TAD
// -----------------------------------------------------------------------------

void SIO_Init(void) {
    
    // CUES
    IniciRX = 0;
    FiRX = 0;
    QuantsRX = 0; //Nombre de caracters que hi ha a la cua

    IniciTX = 0;
    FiTX = 0;
    QuantsTX = 0;

    TRISCbits.TRISC6 = 0;   // TX --> sortida
    TRISCbits.TRISC7 = 1;   // RX --> entrada

    // Configuracio UART
    BAUDCONbits.BRG16 = 0;
    TXSTA = CONFIGURACIO_TXSTA;
    RCSTA = CONFIGURACIO_RCSTA;
    SPBRG = DIVISOR_BAUDRATE;

    // Interrupcio recepcio activada
    PIE1bits.RCIE = 1;

    // Interrupcio transmissio inicialment desactivada
    PIE1bits.TXIE = 0;

    // Activem interrupcions de periferics
    INTCONbits.PEIE = 1;
}

// -----------------------------------------------------------------------------
// INTERRUPCIO RX
// Cal cridar aquesta funcio des de la RSI si:
// PIR1bits.RCIF == 1 && PIE1bits.RCIE == 1
// -----------------------------------------------------------------------------

void SIO_InterrupcioRX(void) {
    unsigned char Valor;

    // Llegir RCREG esborra RCIF automaticament
    Valor = RCREG;

    // Si hi ha error d'overrun, reiniciem recepcio
    if (RCSTAbits.OERR) {
        RCSTAbits.CREN = 0;
        RCSTAbits.CREN = 1;
    }

    // Guardem el caracter a la cua si hi ha espai
    if (QuantsRX < MAX_RX) {
        CuaRX[IniciRX++] = Valor;
        IniciRX &= MASK_RX;
        QuantsRX++;
    }
}

// -----------------------------------------------------------------------------
// INTERRUPCIO TX
// Cal cridar aquesta funcio des de la RSI si:
// PIR1bits.TXIF == 1 && PIE1bits.TXIE == 1
// -----------------------------------------------------------------------------

void SIO_InterrupcioTX(void) {
    // Si queda alguna cosa en cua, enviem el seguent caracter
    if (QuantsTX != 0) {
        TXREG = CuaTX[FiTX++];
        FiTX &= MASK_TX;
        QuantsTX--;
    }

    // Si ja no queda res pendent, desactivem interrupcio TX
    else {
        PIE1bits.TXIE = 0;
    }
}

// -----------------------------------------------------------------------------
// RX AVAILABLE
// Retorna quants caracters hi ha disponibles a la cua RX
// -----------------------------------------------------------------------------

unsigned char SIO_RXAvail(void) {
    return QuantsRX;
}

// -----------------------------------------------------------------------------
// GET CHAR
// Lectura destructiva d'un caracter rebut
// Pre: SIO_RXAvail() ha retornat un valor superior a 0
// -----------------------------------------------------------------------------

unsigned char SIO_GetChar(void) {
    unsigned char ElValor;

    di();

    ElValor = CuaRX[FiRX++];
    FiRX &= MASK_RX;
    QuantsRX--;

    ei();

    return ElValor;
}

// -----------------------------------------------------------------------------
// TX AVAILABLE
// Retorna quants espais lliures queden a la cua TX
// -----------------------------------------------------------------------------

unsigned char SIO_TXAvail(void) {
    return MAX_TX - QuantsTX;
}

// -----------------------------------------------------------------------------
// PUT CHAR
// Posa un caracter a enviar
// Pre: SIO_TXAvail() ha retornat un valor superior a 0
// -----------------------------------------------------------------------------

void SIO_PutChar(unsigned char ElValor) {
    // Si TXREG esta buit i no hi ha cua pendent, enviem directe
    if ((PIR1bits.TXIF == 1) && (QuantsTX == 0)) {
        TXREG = ElValor;
    }

    // Si no, guardem a la cua de transmissio
    else {
        di();

        if (QuantsTX < MAX_TX) {
            CuaTX[IniciTX++] = ElValor;
            IniciTX &= MASK_TX;
            QuantsTX++;
        }

        ei();

        // Activem interrupcio TX per buidar la cua
        PIE1bits.TXIE = 1;
    }
}

// -----------------------------------------------------------------------------
// PUT STRING
// Posa una cadena sencera a la cua d'enviament
// Pre: SIO_TXAvail() >= longitud de LaFrase
// -----------------------------------------------------------------------------

void SIO_PutString(unsigned char *LaFrase) {
    unsigned char Index = 0;

    while (LaFrase[Index] != 0x00) {
        SIO_PutChar(LaFrase[Index]);
        Index++;
    }
}

// -----------------------------------------------------------------------------
// DESTRUCTOR DEL TAD
// -----------------------------------------------------------------------------

void SIO_End(void) {
    // No fa res
}
