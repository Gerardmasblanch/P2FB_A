#include <xc.h>
#include "pic18f4321.h"
#include "TAD_TIMER.H"
#include "TAD_ADC.H"

/*
 * Estats del motor:
 *   ESTAT_CANVI_CANAL: configura ADCON0 amb el canal actual i prepara conversio
 *   ESTAT_TACQ:        espera Tacq abans de llancar la conversio
 *   ESTAT_CONVERTIR:   GO=1 i espera que GO baixi
 */
#define ESTAT_CANVI_CANAL  0
#define ESTAT_TACQ         1
#define ESTAT_CONVERTIR    2

// Tics d'espera per Tacq (temps d'adquisicio).
// Amb tic de 417us, 1 tic ja son ~417us (mes que suficient per al Tacq de ~20us).
#define TICS_TACQ 1

static unsigned char canals[AD_MAX_CANALS];        // num AN registrat per a cada index
static unsigned char mostres[AD_MAX_CANALS];       // ultima mostra llegida per index
static unsigned char numCanals;                    // quants canals registrats

static unsigned char canalActual;                  // index del canal en curs
static unsigned char estat;
static unsigned char timerAdc;

void AD_Init(void)
{
    // ADCON0: canal seleccionat = 0, ADON desactivat de moment
    ADCON0 = 0x00;

    // ADCON1: PCFG[3:0] = 1100 -> AN0..AN2 analogics, AN3..AN12 digitals.
    //   Aixo deixa RA0, RA1 i RA2 com a entrades analogiques i la resta
    //   (RA3, RA4, RA5, RB, RD, RE) com a digitals purs.
    //   Important: NO afecta al SIOFARM perque ara va a RD0/RD1.
    ADCON1 = 0x0C;

    // ADCON2:
    //   ADFM    = 0    -> left justify (els 8 bits alts a ADRESH)
    //   ACQT2:0 = 010  -> 4 Tad de adquisicio
    //   ADCS2:0 = 110  -> Fosc/64 (Fosc=10MHz -> Tad=6.4us)
    ADCON2 = 0b00010110;

    // desactiva comparadors analogics (poden agafar control de RA0-RA3)
    CMCON = 0x07;

    // pins d'entrada analogica
    TRISAbits.TRISA0 = 1;   // JoyX (AN0)
    TRISAbits.TRISA1 = 1;   // JoyY (AN1)
    TRISAbits.TRISA2 = 1;   // LDR  (AN2)

    // engega el ADC
    ADCON0bits.ADON = 1;

    // estat inicial
    numCanals = 0;
    canalActual = 0;
    estat = ESTAT_CANVI_CANAL;

    for(unsigned char i = 0; i < AD_MAX_CANALS; i++) {
        canals[i] = 0;
        mostres[i] = 0;
    }

    TI_NewTimer(&timerAdc);
    TI_ResetTics(timerAdc);
}

unsigned char AD_RegistraCanal(unsigned char canalAN)
{
    if(numCanals >= AD_MAX_CANALS) {
        return 0xFF;
    }

    canals[numCanals] = canalAN;
    mostres[numCanals] = 0;
    numCanals++;

    return numCanals - 1;
}

unsigned char AD_GetMostra(unsigned char index)
{
    if(index >= numCanals) {
        return 0;
    }
    return mostres[index];
}

void AD_Motor(void)
{
    if(numCanals == 0) {
        return;
    }

    switch(estat)
    {
        case ESTAT_CANVI_CANAL:

            // posa el canal actiu al ADCON0 (CHS<3:0> als bits 5..2)
            ADCON0 = (ADCON0 & 0xC3) | ((canals[canalActual] & 0x0F) << 2);

            // arrenca el comptador per esperar Tacq
            TI_ResetTics(timerAdc);
            estat = ESTAT_TACQ;

            break;

        case ESTAT_TACQ:

            // espera el temps d'adquisicio
            if(TI_GetTics(timerAdc) < TICS_TACQ) {
                return;
            }

            // llanca la conversio
            ADCON0bits.GO = 1;
            estat = ESTAT_CONVERTIR;

            break;

        case ESTAT_CONVERTIR:

            // espera que GO baixi sol (fi de conversio)
            if(ADCON0bits.GO == 1) {
                return;
            }

            // guarda la mostra (8 bits alts, left-justify)
            mostres[canalActual] = ADRESH;

            // passa al seguent canal
            canalActual++;
            if(canalActual >= numCanals) {
                canalActual = 0;
            }

            estat = ESTAT_CANVI_CANAL;

            break;
    }
}