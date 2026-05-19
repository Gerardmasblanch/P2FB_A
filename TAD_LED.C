#include <xc.h>
#include "pic18f4321.h"
#include "TAD_TIMER.H"
#include "TAD_LED.H"

#define LED_UN_SEGON 2400

static unsigned char timerLed;
static unsigned char ledBaixant;
static unsigned char ledActiu;

void LED_Init(void) {
    TRISAbits.TRISA4 = 0;
    LATAbits.LATA4 = 0;
    ledBaixant = 0;
    ledActiu = 0;
    TI_NewTimer(&timerLed);
    TI_ResetTics(timerLed);
}

void LED_Actiu(unsigned char actiu) {
    ledActiu = actiu;
}

void LED_Motor(void) {
    unsigned int tics;
    unsigned int intensitat;

    if(!ledActiu) {
        LATAbits.LATA4 = 0;
        return;
    }

    tics = TI_GetTics(timerLed);
    if(tics >= LED_UN_SEGON) {
        TI_ResetTics(timerLed);
        ledBaixant = !ledBaixant;
        tics = 0;
    }

    if(!ledBaixant) {
        intensitat = tics >> 7;
    } else {
        intensitat = (LED_UN_SEGON - tics) >> 7;
    }

    if((tics & 0x0F) < intensitat) {
        LATAbits.LATA4 = 1;
    } else {
        LATAbits.LATA4 = 0;
    }
}
