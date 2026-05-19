#include <xc.h>
#include "pic18f4321.h"
#include "TAD_ADC.H"
#include "TAD_SIO.H"
#include "TAD_JOYSTICK.H"

#define C_UP "U\r\n"
#define C_DOWN "D\r\n"
#define C_LEFT "L\r\n"
#define C_RIGHT "R\r\n"
#define C_SELECT "S\r\n"

static unsigned char direccioActual;
static unsigned char direccioNova;
static unsigned char botoAnterior;
static unsigned char botoActual;
static unsigned char estat;

void JOY_Init(void) {
    TRISBbits.TRISB2 = 1;   // polsador joystick 
    INTCON2bits.RBPU = 0;   // pull-ups interns
}

void JOY_Motor(void) {
    if(estat == 1) { // U
        if(SIO_PutString(C_UP)) estat = 0; 
        return;
    }
    if(estat == 2) { // D
        if(SIO_PutString(C_DOWN)) estat = 0; 
        return;
    }
    if(estat == 3) { // L
        if(SIO_PutString(C_LEFT)) estat = 0; 
        return;
    }
    if(estat == 4) { // R
        if(SIO_PutString(C_RIGHT)) estat = 0; 
        return;
    }
    if(estat == 5) { 
        if(SIO_PutString(C_SELECT)) estat = 0; 
        return;
    }

    direccioNova = 0;
    botoActual = !PORTBbits.RB2;

    
    if(AD_GetMostra(1) < 80) {
        direccioNova = 1; // adalt
    } else if(AD_GetMostra(1) > 175) {
        direccioNova = 2; // abaix
    } else if(AD_GetMostra(0) < 80) {
        direccioNova = 3; // esquerra
    } else if(AD_GetMostra(0) > 175) {
        direccioNova = 4; // dreta
    }

    if(botoActual && !botoAnterior) {
        estat = 5; // seleccionat
    } else if(!direccioActual && direccioNova) {
        estat = direccioNova;
    }

    direccioActual = direccioNova;
    botoAnterior = botoActual; 
}