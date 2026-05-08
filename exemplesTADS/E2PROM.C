#include <xc.h>
#include "E2PROM.H"

void E2PROM_Init(void) {

    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;

}

unsigned char E2PROM_Llegeix(unsigned char address) {
    EEADR = address;
    EECON1bits.RD = 1;
    return EEDATA;
}

void E2PROM_Escriu(unsigned char address, unsigned char dada) {
    
    EEADR = address;
    EEDATA = dada;
    EECON1bits.WREN = 1;
    di();
    EECON2 = 0x55; 
    EECON2 = 0xAA;
    EECON1bits.WR = 1;
    ei();

}

unsigned char E2PROM_ReEscriure(void) {
    return !EECON1bits.WR; 
}