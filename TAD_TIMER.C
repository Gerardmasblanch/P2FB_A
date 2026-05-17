// josepmaria.ribes@salle.url.edu (si hi trobeu alguna errada, si us plau envieu-me un correu :-)
// Arbeca, bressol de l'oliva arbequina
// Mar�, any del Senyor de 2023

// TAD TIMER. Honor i gl�ria


#include <xc.h>
#include "pic18f4321.h"
#include "TAD_TIMER.H"

// Interrupcio cada 417us. Fosc=10MHz, Tcy=0.4us, sense prescaler (PSA=1).
// 65536 - 64496 = 1040 counts x 0.4us = 416us
#define T0CON_CONFIG 0x88
#define RECARREGA_TMR0 64496        // 417 us, suposant FOsc a 10MHz.
#define TI_NUMTIMERS 7              // ADC, SIOFARM, LCD, LED i 3 timers del FARM

static unsigned int Timers[TI_NUMTIMERS];
static unsigned char SeguentTimer;
static volatile unsigned int Tics = 0;

void RSI_Timer0 () {
    // Pre: IMPORTANT! Funci� que ha der ser cridada des de la RSI, en en cas que TMR0IF==1.
    TMR0=RECARREGA_TMR0;
    TMR0IF=0;
    Tics++;    
}

void TI_Init () {
    SeguentTimer = 0;
	T0CON=T0CON_CONFIG;
    TMR0=RECARREGA_TMR0;
	INTCONbits.TMR0IF = 0;
	INTCONbits.TMR0IE = 1;
    // Caldr� que des del main o des d'on sigui s'activin les interrupcions globals!
}

unsigned char TI_NewTimer(unsigned char *TimerHandle) {
    if(SeguentTimer == TI_NUMTIMERS) return TI_FALS;
	*TimerHandle = SeguentTimer;
    SeguentTimer++;
    return (TI_CERT);
}

//TICS = 0 || copia a TicsInicials = Tics: copia el q val tics a TicsInicials
void TI_ResetTics (unsigned char TimerHandle) {
	di(); Timers[TimerHandle]=Tics; ei();
}

// RETURN TICS || RETURN (tics - TicsInicials)
unsigned int TI_GetTics (unsigned char TimerHandle) {
    unsigned int CopiaTicsActual;
    di(); CopiaTicsActual=Tics; ei();
	return (CopiaTicsActual-Timers[TimerHandle]);
}
