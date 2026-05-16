#include <xc.h>
#include "pic18f4321.h"
#include "TAD_TIMER.H"
#include "TAD_SIOFARM.H"

#define PIN_TX LATDbits.LATD1
#define PIN_RX PORTDbits.RD0

#define MIDA_CUA 16
#define MASK_CUA 0x0F

static volatile char cuaRx[MIDA_CUA];
static volatile unsigned char escriureRx;
static volatile unsigned char llegirRx;
static volatile unsigned char quantsRx;

static unsigned char estatRx;
static unsigned char caracterRx;
static unsigned char bitRx;
static unsigned char ticsRx;
unsigned char estatTxSioFarm;
static unsigned char caracterTx;
static unsigned char bitTx;
static unsigned char ticsTx;

static unsigned char timerSioFarm;
static char caracterAux;

static void MotorRx(void);
static void MotorTx(void);

void SIOFARM_Init(void) {
    PIN_TX = 1;
    TRISDbits.TRISD1 = 0;  // com a output
    TRISDbits.TRISD0 = 1;  // com a input

    escriureRx = 0;
    llegirRx = 0;
    quantsRx = 0;

    estatRx = 0;
    estatTxSioFarm = 0;

    TI_NewTimer(&timerSioFarm);
    TI_ResetTics(timerSioFarm);
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

unsigned char SIOFARM_EnviaCaracter(char caracter) {
    if(estatTxSioFarm != 0) return 0;

    caracterTx = caracter;
    bitTx = 0;
    ticsTx = 2;
    PIN_TX = 0;
    estatTxSioFarm = 1;

    return 1;
}

void SIOFARM_Motor(void) {
    if(INTCONbits.GIE == 0) {  // evita avancar el motor si encara som dins una interrupcio
        return;
    }

    if(TI_GetTics(timerSioFarm) == 0) {
        return;
    }

    TI_ResetTics(timerSioFarm);
    MotorRx();
    MotorTx();
}

static void MotorTx(void) {
    if(estatTxSioFarm == 0) return;

    ticsTx--;
    if(ticsTx != 0) return;

    ticsTx = 2;

    switch(estatTxSioFarm) {
        case 1: // TX dades
            if((caracterTx & (1 << bitTx)) != 0) {
                PIN_TX = 1;
            } else {
                PIN_TX = 0;
            }

            bitTx++;
            if(bitTx >= 8) estatTxSioFarm = 2;
            break;

        case 2: // TX stop
            PIN_TX = 1;
            estatTxSioFarm = 3;
            break;

        case 3:
            estatTxSioFarm = 0;
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
 *
 * repos:  1 1 1 1 1
 * start:  0
 * dades:  b0 b1 b2 b3 b4 b5 b6 b7
 * stop:   1
 * repos:  1 1 1...
 *
 * caracterRx es un unsigned char, per tant te 8 bits i no cal cap array.
 *
 * Exemple rebent dades: 0 1 1 0 1 0 1 0
 *
 * caracterRx inicial = 00000000
 *
 * bitRx = 1, dada = 1 -> caracterRx |= (1 << 1) -> 00000010
 * bitRx = 2, dada = 1 -> caracterRx |= (1 << 2) -> 00000110
 * bitRx = 4, dada = 1 -> caracterRx |= (1 << 4) -> 00010110
 * bitRx = 6, dada = 1 -> caracterRx |= (1 << 6) -> 01010110
 *
 * Els bits que arriben a 0 no es toquen, perque caracterRx ja comenca a 0.
 * Al final caracterRx conte el byte complet i es guarda a cuaRx.
 * El bit de stop es comprova al final i no es guarda com una dada mes.
 */
