#ifndef TAD_E2PROM_H
#define TAD_E2PROM_H

void E2PROM_Init();

unsigned char E2PROM_Llegeix(unsigned char address);

unsigned char E2PROM_ReEscriure();
    //CERT si E2PROM no ocupada
    //FALS si E2PROM està ocupada

void E2PROM_Escriu(unsigned char address, unsigned char dada);

#endif