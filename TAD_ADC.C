#include <xc.h>
#include "pic18f4321.h"
#include "TAD_TIMER.H"
#include "TAD_ADC.H"

static unsigned char mostres[3];
static unsigned char canalActual;
static unsigned char estat;
static unsigned char timerAdc;

void AD_Init(void) {

    ADCON1 = 0x0C;       
    ADCON2 = 0b00010110; 
    CMCON = 0x07;        

    TRISAbits.TRISA0 = 1; // X
    TRISAbits.TRISA1 = 1; // Y
    TRISAbits.TRISA2 = 1; // LDR 

    ADCON0bits.ADON = 1;

    canalActual = 0;  // sera el JoyX
    estat = 0;

    mostres[0] = 0;
    mostres[1] = 0;
    mostres[2] = 0;

    TI_NewTimer(&timerAdc);
    TI_ResetTics(timerAdc);
}

unsigned char AD_GetMostra(unsigned char index) {
    return mostres[index];

}

void AD_Motor(void) {
    switch(estat){
        case 0: // canvi de canal

            ADCON0bits.CHS = canalActual;

            TI_ResetTics(timerAdc);
            estat = 1;

            break;  // aixp fara xx00 00xx --> xx00 01xx --> xx00 10xx i bucle al ADCON0

        case 1: // espera lo del condensador

            if(TI_GetTics(timerAdc) < 1) {
                return;
            }

            ADCON0bits.GO = 1;
            estat = 2;

            break;

        case 2: // espera conversio

            if(ADCON0bits.GO == 1) {
                return;
            }

            mostres[canalActual] = ADRESH;

            canalActual++;
            if(canalActual >= 3) {
                canalActual = 0;
            }

            estat = 0;

            break;
    }
}