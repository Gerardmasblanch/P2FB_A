#include <xc.h>
#include "pic18f4321.h"
#include "TAD_ADC.H"
#include "TAD_SIO.H"
#include "TAD_JOYSTICK.H"

// El joystick en repos = ~128. Posem llindars amb zona morta.
#define LLINDAR_BAIX  80
#define LLINDAR_ALT   175

#define CANAL_JOYX_AN 0   // RA5 / AN4
#define CANAL_JOYY_AN 1   // RE0 / AN5

#define PIN_BOTO PORTBbits.RB2

#define CMD_JOY_UP     "MOVE_UP\r\n"
#define CMD_JOY_DOWN   "MOVE_DOWN\r\n"
#define CMD_JOY_LEFT   "MOVE_LEFT\r\n"
#define CMD_JOY_RIGHT  "MOVE_RIGHT\r\n"
#define CMD_JOY_SELECT "SELECT\r\n"

static unsigned char indexX;
static unsigned char indexY;

// estat intern per a deteccio de flancs
static unsigned char direccioActual;       // direccio fisica que esta llegint el ADC ara mateix
static unsigned char direccioPendent;      // moviment pendent de ser consumit (0 = res)
static unsigned char botoEstatAnterior;    // 1 si l'ultim cicle el boto estava premut
static unsigned char botoNouFlanc;         // 1 si s'acaba de detectar premuda

static unsigned char LongitudCadena(const char *s)
{
    unsigned char i;

    i = 0;
    while(s[i] != 0) {
        i++;
    }

    return i;
}

static unsigned char EnviaJavaSiHiHaEspai(const char *s)
{
    unsigned char i;
    unsigned char longitud;

    longitud = LongitudCadena(s);
    if(SIO_TXAvail() < longitud) {
        return 0;
    }

    for(i = 0; i < longitud; i++) {
        SIO_PutChar((unsigned char)s[i]);
    }

    return 1;
}

static unsigned char DireccioFisica(void)
{
    unsigned char valX;
    unsigned char valY;

    valX = AD_GetMostra(indexX);
    valY = AD_GetMostra(indexY);

    if(valY < LLINDAR_BAIX)  return JOY_AMUNT;
    if(valY > LLINDAR_ALT)   return JOY_AVALL;
    if(valX < LLINDAR_BAIX)  return JOY_ESQ;
    if(valX > LLINDAR_ALT)   return JOY_DRETA;

    return JOY_CENTRE;
}

void JOY_Init(void)
{
    // boto a RB2 com a entrada digital
    TRISBbits.TRISB2 = 1;

    // activa pull-up intern del PORTB
    INTCON2bits.RBPU = 0;

    indexX = AD_RegistraCanal(CANAL_JOYX_AN);
    indexY = AD_RegistraCanal(CANAL_JOYY_AN);

    direccioActual = JOY_CENTRE;
    direccioPendent = JOY_CENTRE;
    botoEstatAnterior = 0;
    botoNouFlanc = 0;
}

void JOY_Motor(void)
{
    unsigned char direccioNova;
    unsigned char botoEstatActual;

    // ---- direccio: deteccio de moviment ----
    direccioNova = DireccioFisica();

    // si abans estavem al centre i ara hi ha moviment, registrem-lo
    if(direccioActual == JOY_CENTRE && direccioNova != JOY_CENTRE) {
        direccioPendent = direccioNova;
    }

    direccioActual = direccioNova;

    // ---- boto: deteccio de flanc de premuda ----
    botoEstatActual = (PIN_BOTO == 0) ? 1 : 0;   // active LOW

    if(botoEstatActual && !botoEstatAnterior) {
        botoNouFlanc = 1;       // acaba de baixar (premuda)
    }

    botoEstatAnterior = botoEstatActual;
}

unsigned char JOY_GetMoviment(void)
{
    unsigned char mov;

    mov = direccioPendent;
    direccioPendent = JOY_CENTRE;   // consumit
    return mov;
}

unsigned char JOY_BotoNouPremut(void)
{
    unsigned char res;

    res = botoNouFlanc;
    botoNouFlanc = 0;               // consumit
    return res;
}

void JOY_MotorInterficie(void)
{
    unsigned char moviment;

    moviment = JOY_GetMoviment();

    switch(moviment)
    {
        case JOY_AMUNT:
            EnviaJavaSiHiHaEspai(CMD_JOY_UP);
            break;

        case JOY_AVALL:
            EnviaJavaSiHiHaEspai(CMD_JOY_DOWN);
            break;

        case JOY_ESQ:
            EnviaJavaSiHiHaEspai(CMD_JOY_LEFT);
            break;

        case JOY_DRETA:
            EnviaJavaSiHiHaEspai(CMD_JOY_RIGHT);
            break;

        default:
            break;
    }

    if(JOY_BotoNouPremut()) {
        EnviaJavaSiHiHaEspai(CMD_JOY_SELECT);
    }
}
