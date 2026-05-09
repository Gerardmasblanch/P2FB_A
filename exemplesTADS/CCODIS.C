#include "CCODIS.H"
#include "ADC.H"
#include "OUT.H"
#include "E2PROM.H"
#include "SERIAL.H"

#include <xc.h>


static unsigned char flagPols = 0;
static unsigned char flagPols2 = 0;
static unsigned char i = 0;
static unsigned char adc_iniciat = 0;

void RB0_Pols(unsigned char p) {

    if (p) flagPols = 1;
}

void RB1_Pols(unsigned char p) {
    if (p) flagPols2 = 1;
}

void CCODIS_motor(void) {
    static unsigned char estatCC = 0;

    switch (estatCC) {

        case 0:

            if (flagPols) {
                flagPols = 0;
                i = 0;
                estatCC = 1;
            } else if (flagPols2) {
                flagPols2 = 0;
                i = 0;
                estatCC = 2; 
            } else {
                estatCC = 3;
            }
            break;

        case 1: // copiar FLASH ? EEPROM

            if (E2PROM_ReEscriure()) {
                E2PROM_Escriu(i, OUT_getPatro(i));
                i++;

                if (i >= 16) {
                    estatCC = 0;
                    i = 0;
                }
            }
            break;

        case 2: // enviar EEPROM por serie

            if (!SIO_isBusy()) {
                unsigned char data = E2PROM_Llegeix(i);
                SIO_sendChar(data);
                i++;

                if (i >= 16) {
                    estatCC = 0;
                    i = 0;
                }
            }
            break;
        
        case 3:
            
            if (!adc_iniciat) {
                AD_StartConversion();
                adc_iniciat = 1;
            }

            if (AD_HiHaMostra()) {

                unsigned char mostra = AD_GetMostra();
                if (mostra < 16) {
                    Pinta_7Segments(E2PROM_Llegeix(mostra));
                }  

                estatCC = 0;
                adc_iniciat = 0;
            } 

            break;
    }
}