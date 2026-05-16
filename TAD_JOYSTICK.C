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

#define JOY_REPOS 0
#define JOY_UP 1
#define JOY_DOWN 2
#define JOY_LEFT 3
#define JOY_RIGHT 4
#define JOY_SELECT 5

static const char *textJoy[6] = {0, C_UP, C_DOWN, C_LEFT, C_RIGHT, C_SELECT};

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
    if(estat != JOY_REPOS) {
        if(SIO_PutString(textJoy[estat])) estat = JOY_REPOS;
        return;
    }

    direccioNova = 0;
    botoActual = !PORTBbits.RB2;

    // GetMostra 0-->X 1-->Y
    if(AD_GetMostra(1) < 80) {
        direccioNova = JOY_UP;
    } else if(AD_GetMostra(1) > 175) {
        direccioNova = JOY_DOWN;
    } else if(AD_GetMostra(0) < 80) {
        direccioNova = JOY_LEFT;
    } else if(AD_GetMostra(0) > 175) {
        direccioNova = JOY_RIGHT;
    }

    if(botoActual && !botoAnterior) {
        estat = JOY_SELECT;
    } else if(!direccioActual && direccioNova) {
        estat = direccioNova;
    }

    direccioActual = direccioNova;
    botoAnterior = botoActual; // guarda si estava premut per detectar nomes el flanc
}
