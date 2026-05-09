#include <xc.h>
#include "pic18f4321.h"
#include "TAD_TIMER.H"
#include "TAD_SIOFARM.H"

/*
 * RD1 = TX, RD0 = RX
 * 2 tics = 1 bit a 1200 bauds (Timer0 a 417 us)
 * Amb 2 tics/bit la deteccio del start pot arribar fins a 1 tic tard.
 * Per aixo el primer bit es mostra al cap de 2 tics i no de 3.
 */

#define PIN_TX LATDbits.LATD1
#define PIN_RX PORTDbits.RD0

#define MIDA_CUA 16
#define MASK_CUA 0x0F

static volatile char cuaTx[MIDA_CUA];
static volatile unsigned char escriureTx;
static volatile unsigned char llegirTx;
static volatile unsigned char quantsTx;

static volatile char cuaRx[MIDA_CUA];
static volatile unsigned char escriureRx;
static volatile unsigned char llegirRx;
static volatile unsigned char quantsRx;

static unsigned char estatTx;
static unsigned char caracterTx;
static unsigned char bitTx;
static unsigned char ticsTx;

static unsigned char estatRx;
static unsigned char caracterRx;
static unsigned char bitRx;
static unsigned char ticsRx;

static unsigned char timerSioFarm;
static unsigned char quantsAux;
static unsigned char enviatAux;
static char caracterAux;

static void MotorTx(void);
static void MotorRx(void);

void SIOFARM_Init(void) {
    LATDbits.LATD1 = 1;    // deixa TX en repos alt
    TRISDbits.TRISD1 = 0;  // com a output

    TRISDbits.TRISD0 = 1;  // com a input

    escriureTx = 0;
    llegirTx = 0;
    quantsTx = 0;

    escriureRx = 0;
    llegirRx = 0;
    quantsRx = 0;

    estatTx = 0;
    estatRx = 0;

    TI_NewTimer(&timerSioFarm);
    TI_ResetTics(timerSioFarm);
}

unsigned char SIOFARM_EnviaCaracter(char caracter) {
    enviatAux = 0;

    di();

    if(quantsTx >= MIDA_CUA) {
        ei();
        return 0;
    }

    cuaTx[escriureTx] = caracter;
    escriureTx++;
    escriureTx &= MASK_CUA;
    quantsTx++;

    ei();

    enviatAux = 1;
    return enviatAux;
}

unsigned char SIOFARM_HiHaCaracter(void) {
    di();
    quantsAux = quantsRx;
    ei();

    return quantsAux;
}

char SIOFARM_LlegeixCaracter(void) {
    di();

    if(quantsRx == 0) {
        ei();
        return 0;
    }

    caracterAux = cuaRx[llegirRx];
    llegirRx++;
    llegirRx &= MASK_CUA;
    quantsRx--;

    ei();

    return caracterAux;
}

void SIOFARM_Motor(void) {
    if(INTCONbits.GIE == 0) {  // evita avancar el motor si encara som dins una interrupcio
        return;
    }

    if(TI_GetTics(timerSioFarm) == 0) {
        return;
    }

    TI_ResetTics(timerSioFarm);
    MotorTx();
    MotorRx();
}

static void MotorTx(void) {
    switch(estatTx) {
        case 0: // TX repos

            PIN_TX = 1;

            if(quantsTx > 0) {
                caracterTx = cuaTx[llegirTx];
                llegirTx++;
                llegirTx &= MASK_CUA;
                quantsTx--;

                PIN_TX = 0;     // start bit
                bitTx = 0;
                ticsTx = 2;
                estatTx = 1;
            }
            break;

        case 1: // TX enviant

            ticsTx--;

            if(ticsTx == 0) {
                bitTx++;

                if(bitTx <= 8) {
                    PIN_TX = caracterTx & 0x01;
                    caracterTx >>= 1;
                } else if(bitTx == 9) {
                    PIN_TX = 1;     // stop bit
                } else {
                    PIN_TX = 1;
                    estatTx = 0;
                    return;
                }

                ticsTx = 2;
            }
            break;
    }
}

static void MotorRx(void) {
    switch(estatRx) {
        case 0: // RX espera start

            if(PIN_RX == 0) {
                caracterRx = 0;
                bitRx = 0;
                ticsRx = 2;
                estatRx = 1;
            }
            break;

        case 1: // RX espera bit

            ticsRx--;

            if(ticsRx == 0) {
                if(PIN_RX == 1) {
                    caracterRx |= (1 << bitRx);
                }

                bitRx++;

                if(bitRx >= 8) {
                    ticsRx = 2;
                    estatRx = 2;
                } else {
                    ticsRx = 2;
                }
            }
            break;

        case 2: // RX espera stop

            ticsRx--;

            if(ticsRx == 0) {
                if(PIN_RX == 1) {
                    if(quantsRx < MIDA_CUA) {
                        cuaRx[escriureRx] = caracterRx;
                        escriureRx++;
                        escriureRx &= MASK_CUA;
                        quantsRx++;
                    }
                }
                estatRx = 0;
            }
            break;
    }
}

/*
 * Cicle de funcionament:
 * - Per enviar, SIOFARM_EnviaCaracter() posa el byte a cuaTx.
 *   El motor TX el treu de la cua i el passa per RD1: start, 8 bits i stop.
 * - Per rebre, el motor RX vigila RD0. Quan veu el start, llegeix els 8 bits,
 *   comprova el stop i guarda el byte a cuaRx.
 * - El TAD que ho necessiti consulta SIOFARM_HiHaCaracter() i extreu el byte
 *   amb SIOFARM_LlegeixCaracter().
 */
