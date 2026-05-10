#ifndef TAD_EEPROM_H
#define TAD_EEPROM_H

#define EEPROM_MAGIC 0xA5
#define EEPROM_RECORDS 2
#define RECORD_SIZE 8

unsigned char EEPROM_Read(unsigned char adreca);
void EEPROM_Write(unsigned char adreca, unsigned char valor);

#endif
