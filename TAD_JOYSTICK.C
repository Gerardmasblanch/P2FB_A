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
    TRISBbits.TRISB2 = 1;   // boto 
    INTCON2bits.RBPU = 0;   // pull-ups del PORTB activades
}

void JOY_Motor(void) {
    if(estat == 1) { // up
        if(SIO_PutString(C_UP)) estat = 0; // repos
        return;
    }
    if(estat == 2) { // down
        if(SIO_PutString(C_DOWN)) estat = 0; // repos
        return;
    }
    if(estat == 3) { // left
        if(SIO_PutString(C_LEFT)) estat = 0; // repos
        return;
    }
    if(estat == 4) { // right
        if(SIO_PutString(C_RIGHT)) estat = 0; // repos
        return;
    }
    if(estat == 5) { // select
        if(SIO_PutString(C_SELECT)) estat = 0; // repos
        return;
    }

    direccioNova = 0;
    botoActual = !PORTBbits.RB2;

    // GetMostra 0-->X 1-->Y
    if(AD_GetMostra(1) < 80) {
        direccioNova = 1; // up
    } else if(AD_GetMostra(1) > 175) {
        direccioNova = 2; // down
    } else if(AD_GetMostra(0) < 80) {
        direccioNova = 3; // left
    } else if(AD_GetMostra(0) > 175) {
        direccioNova = 4; // right
    }

    if(botoActual && !botoAnterior) {
        estat = 5; // select
    } else if(!direccioActual && direccioNova) {
        estat = direccioNova;
    }

    direccioActual = direccioNova;
    botoAnterior = botoActual; // guarda si estava premut per detectar nomes el flanc
}
