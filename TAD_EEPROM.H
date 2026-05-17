#ifndef TAD_EEPROM_H
#define TAD_EEPROM_H

void EEPROM_Init(void);
unsigned char EEPROM_Llegeix(unsigned char adreca);
void EEPROM_Escriu(unsigned char adreca, unsigned char valor);

#endif
