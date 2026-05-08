#include "ADC.H"
#include "OUT.H"
#include <xc.h>



void AD_Init(unsigned char canal) {
    
    ADCON1 = 0x0E; 
    ADCON0 = ((canal << 2) & 0x3C) | 0x01; // Configura el canal i engega el conversor
    TRISAbits.RA3 = 0;
    LATAbits.LATA3 = 0;

}


void AD_StartConversion(void) {
    ADCON0bits.GO = 1; // inici de conversio
}

unsigned char AD_HiHaMostra(void) {
    return !ADCON0bits.GO;  // fi de la conversio
}

unsigned char AD_GetMostra(void) {

    return ((ADRESH >> 4) & 0x0F);
}



