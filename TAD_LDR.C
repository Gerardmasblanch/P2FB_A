#include "TAD_ADC.H"
#include "TAD_LDR.H"

/*
 * Llindar per considerar "esta tapat".
 *
 * Amb el divisor LDR(dalt) + 10k(baix):
 *   - Llum d'habitacio normal: ~150-200
 *   - Llum directa intensa:    ~230
 *   - Tapat amb la ma:         ~30-50
 *
 * Si veus que es dispara massa facilment o massa dificil,
 * ajusta aquest valor segons les mostres reals que vegis al main de prova.
 */
#define LLINDAR_FOSC 60

#define CANAL_LDR_AN 2   // RA2 / AN2

static unsigned char indexLdr;

void LDR_Init(void)
{
    indexLdr = AD_RegistraCanal(CANAL_LDR_AN);
}

unsigned char LDR_GetLlum(void)
{
    return AD_GetMostra(indexLdr);
}

unsigned char LDR_HiHaPocaLlum(void)
{
    return (AD_GetMostra(indexLdr) < LLINDAR_FOSC) ? 1 : 0;
}