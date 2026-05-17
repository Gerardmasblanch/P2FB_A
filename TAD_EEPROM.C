#include <xc.h>
#include "pic18f4321.h"
#include "TAD_EEPROM.H"

void EEPROM_Init(void) {
    EECON1bits.WREN = 0;
}

unsigned char EEPROM_Llegeix(unsigned char adreca) {
    EEADR = adreca;
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.RD = 1;
    return EEDATA;
}

void EEPROM_Escriu(unsigned char adreca, unsigned char valor) {
    EEADR = adreca;
    EEDATA = valor;
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.WREN = 1;
    di();
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;
    ei();
    while(EECON1bits.WR);
    EECON1bits.WREN = 0;
}
