#include <xc.h>
#include "pic18f4321.h"
#include "TAD_LED.H"

#define LED_TICS_SEGON 2400

static unsigned char ledBaixant;

void LED_Init(void) {
    TRISAbits.TRISA4 = 0;
    LATAbits.LATA4 = 0;
}

void LED_CanviaSentit(void) {
    ledBaixant = !ledBaixant;
}

void LED_Motor(unsigned int tics, unsigned char actiu) {
    unsigned char intensitat;

    if(!actiu) {
        LATAbits.LATA4 = 0;
        return;
    }

    if(!ledBaixant) {
        intensitat = tics >> 7;
    } else {
        intensitat = (LED_TICS_SEGON - tics) >> 7;
    }

    LATAbits.LATA4 = ((tics & 0x0F) < intensitat) ? 1 : 0;
}
