
#include "POLS.H"
#include "TIMER.H"
#include "CCODIS.H"
#include <xc.h>

#define POLSADOR_PREMUT 0

static unsigned char estat = 0;
static unsigned char timerRebots;

void Pols_Init(){
    
    
    TRISBbits.TRISB0 = 1;
    INTCON2bits.RBPU = 0;
    TI_NewTimer(&timerRebots);
}

void Pols_motor(){

    switch (estat){

        case 0:
            if(PORTBbits.RB0 == POLSADOR_PREMUT){
                TI_ResetTics(timerRebots);
                estat = 1;
            } else {
                estat = 0;
            }
            break;
        case 1 : 
            if(TI_GetTics(timerRebots) >= 5) {
                estat = 2;
            } else {
                estat = 1;
            }
            break;
        case 2:
            if(PORTBbits.RB0 != POLSADOR_PREMUT) {
                TI_ResetTics(timerRebots);
                estat = 3;
            } else {
                estat = 2;
            }
            break;
        case 3:
            if(TI_GetTics(timerRebots) >= 5) {
                RB0_Pols(1); 
                estat = 0;
            } else {
                estat = 3;
            }
            break;
        default:
            estat = 0;
            break;
    }
}   