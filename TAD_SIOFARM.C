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

#define MASK_CUA_SIOFARM 0x0F

#define TICS_PER_BIT      2
#define TICS_AL_CENTRE_0  2

#define TX_REPOS    0
#define TX_ENVIANT  1

#define RX_ESPERA_START   0
#define RX_ESPERA_BIT     1
#define RX_ESPERA_STOP    2

/* ---------------------- Cua TX ---------------------- */
static volatile char          cuaTx[MIDA_CUA_SIOFARM];
static volatile unsigned char escriureTx;
static volatile unsigned char llegirTx;
static volatile unsigned char quantsTx;

/* ---------------------- Cua RX ---------------------- */
static volatile char          cuaRx[MIDA_CUA_SIOFARM];
static volatile unsigned char escriureRx;
static volatile unsigned char llegirRx;
static volatile unsigned char quantsRx;

/* ---------------------- Variables TX ---------------------- */
static unsigned char estatTx;
static unsigned char caracterTx;
static unsigned char bitTx;
static unsigned char ticsTx;

/* ---------------------- Variables RX ---------------------- */
static unsigned char estatRx;
static unsigned char caracterRx;
static unsigned char bitRx;
static unsigned char ticsRx;

/* Timer per fer avancar el bit-banging des del bucle principal */
static unsigned char timerSioFarm;

/* ---------------------- Init ---------------------- */

void SIOFARM_Init(void)
{
    /* TX (RD1) com a sortida en idle alt */
    LATDbits.LATD1 = 1;
    TRISDbits.TRISD1 = 0;

    /* RX (RD0) com a entrada */
    TRISDbits.TRISD0 = 1;

    /* cues */
    escriureTx = 0;
    llegirTx = 0;
    quantsTx = 0;

    escriureRx = 0;
    llegirRx = 0;
    quantsRx = 0;

    /* estats */
    estatTx = TX_REPOS;
    estatRx = RX_ESPERA_START;

    TI_NewTimer(&timerSioFarm);
    TI_ResetTics(timerSioFarm);
}

/* ---------------------- API publica ---------------------- */

unsigned char SIOFARM_EnviaCaracter(char caracter)
{
    unsigned char enviat;

    enviat = 0;

    di();

    if(quantsTx >= MIDA_CUA_SIOFARM) {
        ei();
        return 0;
    }

    cuaTx[escriureTx] = caracter;
    escriureTx++;
    escriureTx &= MASK_CUA_SIOFARM;
    quantsTx++;

    ei();

    enviat = 1;
    return enviat;
}

unsigned char SIOFARM_HiHaCaracter(void)
{
    unsigned char quants;

    di();
    quants = quantsRx;
    ei();

    return quants;
}

char SIOFARM_LlegeixCaracter(void)
{
    char c;

    di();

    if(quantsRx == 0) {
        ei();
        return 0;
    }

    c = cuaRx[llegirRx];
    llegirRx++;
    llegirRx &= MASK_CUA_SIOFARM;
    quantsRx--;

    ei();

    return c;
}

/* ---------------------- Motor TX ---------------------- */

static void MotorTx(void)
{
    switch(estatTx)
    {
        case TX_REPOS:

            PIN_TX = 1;

            if(quantsTx > 0) {
                caracterTx = cuaTx[llegirTx];
                llegirTx++;
                llegirTx &= MASK_CUA_SIOFARM;
                quantsTx--;

                PIN_TX = 0;     /* start bit */
                bitTx = 0;
                ticsTx = TICS_PER_BIT;
                estatTx = TX_ENVIANT;
            }
            break;

        case TX_ENVIANT:

            ticsTx--;

            if(ticsTx == 0) {
                bitTx++;

                if(bitTx <= 8) {
                    PIN_TX = caracterTx & 0x01;
                    caracterTx >>= 1;
                }
                else if(bitTx == 9) {
                    PIN_TX = 1;     /* stop bit */
                }
                else {
                    PIN_TX = 1;
                    estatTx = TX_REPOS;
                    return;
                }

                ticsTx = TICS_PER_BIT;
            }
            break;
    }
}

/* ---------------------- Motor RX ---------------------- */

static void MotorRx(void)
{
    switch(estatRx)
    {
        case RX_ESPERA_START:

            if(PIN_RX == 0) {
                caracterRx = 0;
                bitRx = 0;
                ticsRx = TICS_AL_CENTRE_0;
                estatRx = RX_ESPERA_BIT;
            }
            break;

        case RX_ESPERA_BIT:

            ticsRx--;

            if(ticsRx == 0) {
                if(PIN_RX == 1) {
                    caracterRx |= (1 << bitRx);
                }

                bitRx++;

                if(bitRx >= 8) {
                    ticsRx = TICS_PER_BIT;
                    estatRx = RX_ESPERA_STOP;
                }
                else {
                    ticsRx = TICS_PER_BIT;
                }
            }
            break;

        case RX_ESPERA_STOP:

            ticsRx--;

            if(ticsRx == 0) {
                if(PIN_RX == 1) {
                    if(quantsRx < MIDA_CUA_SIOFARM) {
                        cuaRx[escriureRx] = caracterRx;
                        escriureRx++;
                        escriureRx &= MASK_CUA_SIOFARM;
                        quantsRx++;
                    }
                }
                estatRx = RX_ESPERA_START;
            }
            break;
    }
}

/* ---------------------- Motor unic ---------------------- */

void SIOFARM_Motor(void)
{
    if(INTCONbits.GIE == 0) {
        return;
    }

    if(TI_GetTics(timerSioFarm) == 0) {
        return;
    }

    TI_ResetTics(timerSioFarm);
    MotorTx();
    MotorRx();
}
