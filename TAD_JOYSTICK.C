#include <xc.h>
#include "pic18f4321.h"
#include "TAD_ADC.H"
#include "TAD_SIO.H"
#include "TAD_JOYSTICK.H"

#define C_UP "MOVE_UP\r\n"
#define C_DOWN "MOVE_DOWN\r\n"
#define C_LEFT "MOVE_LEFT\r\n"
#define C_RIGHT "MOVE_RIGHT\r\n"
#define C_SELECT "SELECT\r\n"

static unsigned char direccioActual;
static unsigned char direccioNova;
static unsigned char botoAnterior;
static unsigned char botoActual;
static unsigned char estat;

void JOY_Init(void) {
    TRISBbits.TRISB2 = 1;   // boto 
    INTCON2bits.RBPU = 0;   // pull-ups del PORTB activades

    direccioActual = 0;
    direccioNova = 0;
    botoAnterior = 0;
    botoActual = 0;
    estat = 0;
}

void JOY_Motor(void) {
    switch(estat) {
        case 0:
            direccioNova = 0;
            botoActual = (PORTBbits.RB2 == 0) ? 1 : 0;

            // GetMostra 0-->X 1-->Y
            if(AD_GetMostra(1) < 80) {
                direccioNova = 1; // UP
            } else if(AD_GetMostra(1) > 175) {
                direccioNova = 2; // DOWN
            } else if(AD_GetMostra(0) < 80) {
                direccioNova = 3; // LEFT
            } else if(AD_GetMostra(0) > 175) {
                direccioNova = 4; // RIGHT
            }

            if(botoActual && !botoAnterior) {
                estat = 5; // SELECT
            } else if(direccioActual == 0 && direccioNova != 0) {
                estat = direccioNova;
            }

            direccioActual = direccioNova;
            botoAnterior = botoActual; // guarda si estava premut per detectar nomes el flanc
            break;

        case 1:  // comanda mes llarga --> MOVE_RIGHT\r\n 12 caracters
            if(SIO_TXAvail() >= 12) {
                SIO_PutString(C_UP);
                estat = 0;
            }
            break;

        case 2: 
            if(SIO_TXAvail() >= 12) { 
                SIO_PutString(C_DOWN);
                estat = 0;
            }
            break;

        case 3: 
            if(SIO_TXAvail() >= 12) { 
                SIO_PutString(C_LEFT);
                estat = 0;
            }
            break;

        case 4: 
            if(SIO_TXAvail() >= 12) { 
                SIO_PutString(C_RIGHT);
                estat = 0;
            }
            break;

        case 5:
            if(SIO_TXAvail() >= 12) { 
                SIO_PutString(C_SELECT);
                estat = 0;
            }
            break;
    }
}
