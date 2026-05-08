#ifndef TAD_ADC_H
#define TAD_ADC_H

void AD_Init(unsigned char canal);
void AD_StartConversion(void);
unsigned char AD_HiHaMostra(void);
unsigned char AD_GetMostra(void);


#endif