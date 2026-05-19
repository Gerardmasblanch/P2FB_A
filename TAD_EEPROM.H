#ifndef TAD_EEPROM_H
#define TAD_EEPROM_H

#define EEPROM_Init()
#define EEPROM_Busy() EECON1bits.WR
unsigned char EEPROM_Llegeix(unsigned char adreca);
void EEPROM_Escriu(unsigned char adreca, unsigned char valor);

#endif
