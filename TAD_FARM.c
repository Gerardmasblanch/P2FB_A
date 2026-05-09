#include <xc.h>
#include "TAD_TIMER.H"
#include "TAD_SIOFARM.H"
#include "TAD_SIO.H"
#include "TAD_FARM.H"
#include "TAD_LCD.H"

#define ESTAT_DEMANAR_DATA    0
#define ESTAT_LLEGIR_DATA     1
#define ESTAT_ESPERAR_JAVA    2
#define ESTAT_FUNCIONAMENT    3

#define LONG_DATA 14
#define BUFFER_PLE 1
#define CMD_INITIALIZE "INITIALIZE:"
#define LONG_CMD_INITIALIZE 11
#define LONG_JAVA 64
#define NOM_GRANJA 20

static char    bufferData[LONG_DATA + 1];
static char    bufferJava[LONG_JAVA + 1];
static unsigned char idxBuffer;
static unsigned char overflowBuffer;

/* Cua de sortida pendent */
static const char *cadenaPendent;
static unsigned char idxCadena;

static unsigned char estat;
static unsigned char idxJava;
static unsigned char overflowJava;


static unsigned char dia, mes, hora, minut, segon;
static unsigned char NomGranja[NOM_GRANJA + 1];
static unsigned char Tvaca, Tcavall, Tporc, Tgallina;



/* -------------------------------------------------- */
/* Cua de sortida                                     */
/* -------------------------------------------------- */

static void GuardaInit(const char *bufferjava){

    unsigned char i = 11;
    unsigned char j = 0;

    while (bufferjava[i] != '$'){
        bufferjava[i] = NomGranja[j]; 
        j++;
        i++;
    }

    i++;
    NomGranja[j] = '\0';
    
    Tvaca = bufferjava[i];
    Tcavall = bufferjava[i+1];
    Tporc = bufferjava[i+2];
    Tgallina = bufferjava[i+3];
    
    return;
}

static void EncuaCadena(const char *s)
{
    cadenaPendent = s;
    idxCadena = 0;
}

static unsigned char EstaEnviantCadena(void)
{
    return (cadenaPendent != 0) ? 1 : 0;
}

static void MotorSortida(void)
{
    if (cadenaPendent == 0) return;

    if (cadenaPendent[idxCadena] == 0) {
        cadenaPendent = 0;
        idxCadena = 0;
        return;
    }

    if (SIOFARM_EnviaCaracter(cadenaPendent[idxCadena])) {
        idxCadena++;
    }
}

static void EnviaEco(char c)
{
    SIOFARM_EnviaCaracter(c);
}

static void EnviaBackspace(void)
{
    SIOFARM_EnviaCaracter('\b');
    SIOFARM_EnviaCaracter(' ');
    SIOFARM_EnviaCaracter('\b');
}

/* -------------------------------------------------- */
/* Validacio                                          */
/* -------------------------------------------------- */

static unsigned char CharToNum(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    return 0xFF;
}

static unsigned char ValidaData(void)
{
    unsigned char d1, d2, m1, m2, h1, h2, mi1, mi2, s1, s2;

    if (idxBuffer != LONG_DATA) return 0;

    if (bufferData[2]  != '/') return 0;
    if (bufferData[5]  != ' ') return 0;
    if (bufferData[8]  != ':') return 0;
    if (bufferData[11] != ':') return 0;

    d1  = CharToNum(bufferData[0]);
    d2  = CharToNum(bufferData[1]);
    m1  = CharToNum(bufferData[3]);
    m2  = CharToNum(bufferData[4]);
    h1  = CharToNum(bufferData[6]);
    h2  = CharToNum(bufferData[7]);
    mi1 = CharToNum(bufferData[9]);
    mi2 = CharToNum(bufferData[10]);
    s1  = CharToNum(bufferData[12]);
    s2  = CharToNum(bufferData[13]);

    if (d1  == 0xFF || d2  == 0xFF) return 0;
    if (m1  == 0xFF || m2  == 0xFF) return 0;
    if (h1  == 0xFF || h2  == 0xFF) return 0;
    if (mi1 == 0xFF || mi2 == 0xFF) return 0;
    if (s1  == 0xFF || s2  == 0xFF) return 0;

    dia   = d1*10 + d2;
    mes   = m1*10 + m2;
    hora  = h1*10 + h2;
    minut = mi1*10 + mi2;
    segon = s1*10 + s2;

    if (dia   < 1 || dia   > 31) return 0;
    if (mes   < 1 || mes   > 12) return 0;
    if (hora  > 23) return 0;
    if (minut > 59) return 0;
    if (segon > 59) return 0;

    return 1;
}

/* -------------------------------------------------- */
/* Java                                               */
/* -------------------------------------------------- */

static void ReiniciaRecepcioJava(void)
{
    idxJava = 0;
    overflowJava = 0;
}

static unsigned char ProcessaCaracterJava(unsigned char c)
{
    static const char cmdInitialize[] = CMD_INITIALIZE;
    unsigned char i;

    if (c == '\r') {
        return 0;
    }

    if (c == '\n') {
        bufferJava[idxJava] = 0;

        if (!overflowJava) {
            for (i = 0; i < LONG_CMD_INITIALIZE; i++) {
                if (bufferJava[i] != cmdInitialize[i]) {
                    ReiniciaRecepcioJava();
                    return 0;
                }
            }

            ReiniciaRecepcioJava();
            return 1;
        }

        ReiniciaRecepcioJava();
        return 0;
    }

    if (idxJava < LONG_JAVA) {
        bufferJava[idxJava] = (char)c;
        idxJava++;
    }
    else {
        overflowJava = BUFFER_PLE;
    }

    return 0;
}

/* -------------------------------------------------- */
/* Init                                               */
/* -------------------------------------------------- */

void FARM_Init(void)
{
    estat = ESTAT_DEMANAR_DATA;
    idxBuffer = 0;
    overflowBuffer = 0;
    cadenaPendent = 0;
    idxCadena = 0;
    idxJava = 0;
    overflowJava = 0;
    dia = mes = hora = minut = segon = 0;
}

/* -------------------------------------------------- */
/* Motor                                              */
/* -------------------------------------------------- */

void FARM_Motor(void)
{
    char c;
    unsigned char valor;

    // 1. sempre prova de buidar la cadena pendent
    MotorSortida();

    // 2. mentre estem enviant alguna cosa, deixem el RX en cua i el processem
    //    quan la sortida ja estigui lliure.
    if (EstaEnviantCadena()) return;

    // 3. ara nomes processem RX si la cua de TX esta lliure
    switch(estat) {
        case ESTAT_DEMANAR_DATA:
            EncuaCadena("\r\n=== LSFarm ===\r\nData i hora (DD/MM HH:MM:SS): ");
            idxBuffer = 0;
            overflowBuffer = 0;
            estat = ESTAT_LLEGIR_DATA;
            break;

        case ESTAT_LLEGIR_DATA:

            if (!SIOFARM_HiHaCaracter()) return;

            c = SIOFARM_LlegeixCaracter();
            valor = (unsigned char)c;

            if (c == '\r') {
                EnviaEco('\r');
                EnviaEco('\n');

                bufferData[idxBuffer] = '\0';

                if (!overflowBuffer && ValidaData()) {
                    EncuaCadena("Data correcta. Esperant INITIALIZE de Java...\r\n");
                    ReiniciaRecepcioJava();
                    //Lc_PutString(bufferData);
                   
                    estat = ESTAT_ESPERAR_JAVA;

                }
                else {
                    EncuaCadena("Data incorrecta. Format: DD/MM HH:MM:SS\r\n");
                    estat = ESTAT_DEMANAR_DATA;
                }
            }
            else if (c == '\b' || valor == 127) {
                if (idxBuffer > 0) {
                    idxBuffer--;
                    overflowBuffer = 0;
                    EnviaBackspace();
                }
            }
            else if (valor >= 32 && valor <= 126) {
                if (idxBuffer < LONG_DATA) {
                    bufferData[idxBuffer] = c;
                    idxBuffer++;
                    EnviaEco(c);
                }
                else {
                    overflowBuffer = BUFFER_PLE;
                }
            }
            break;

        case ESTAT_ESPERAR_JAVA:
            if (!SIO_RXAvail()) return;

            c = SIO_GetChar();

            if (ProcessaCaracterJava((unsigned char)c)) {
                EncuaCadena("INITIALIZE rebut.\r\n");
                estat = ESTAT_FUNCIONAMENT;
                GuardaInit(bufferJava);
                Lc_PutString(bufferData);
            }

            break;

        case ESTAT_FUNCIONAMENT:
            
            

            
            
            

        
            break;
    }
}
