#include <xc.h>
#include "TAD_TIMER.H"
#include "TAD_LCD.H"


//
//--------------------------------CONSTANTS---AREA-----------
//
#define FUNCTION_SET    0x20
#define BITS_8          0x10
#define DISPLAY_CONTROL 0x08
#define DISPLAY_ON      0x04
#define CURSOR_ON       0x02
#define DISPLAY_CLEAR   0x01
#define ENTRY_MODE      0x04
#define SET_DDRAM       0x80

// Estats del motor
#define LCD_IDLE            0
#define LCD_BUSY_SETUP      1   // Prepara lectura del flag Busy (configura pins, reset timer)
#define LCD_BUSY_POLLING    2   // Comprova flag Busy cada tic; quan Busy=0 salta a LcdOp
#define LCD_OP_CLEAR_ENVIA  3   // Envia DISPLAY_CLEAR i arma el timer d'espera
#define LCD_OP_CLEAR_ESPERA 4   // Espera >= 4 tics (1.648ms > 1.52ms max del LCD)
#define LCD_OP_GOTOXY       7   // Envia SET_DDRAM amb l'adreca calculada
#define LCD_OP_PUTSTRING    10  // Envia el seguent caracter de la cadena

// Tipus d'ordre encolada (cua de 3 ordres pendents)
#define LCD_ORDRE_CLEAR       0
#define LCD_ORDRE_GOTOXY      3
#define LCD_ORDRE_PUTSTRING   5

#define MAX_ORDRES 3
//
//---------------------------End--CONSTANTS---AREA-----------
//


//
//--------------------------------VARIABLES---AREA-----------
//
typedef struct {
    unsigned char op;
    unsigned char arg1;     // caracter (PutChar) o columna (GotoXY)
    unsigned char arg2;     // fila (GotoXY)
    char         *str;      // cadena (PutString)
} OrdreLCD;

static OrdreLCD      CuaOrdres[MAX_ORDRES];
static unsigned char IniciCua;       // index on s'escriu la propera ordre
static unsigned char FiCua;          // index de la propera ordre a executar
static unsigned char QuantsOrdres;

static unsigned char Timer;
static unsigned char LcdEstat;
static unsigned char LcdOp;          // Estat al qual saltar despres del busy-check
static unsigned char LcdCharPendent; // Parametre per PutChar i columna per GotoXY
static unsigned char LcdRowPendent;  // Parametre fila per GotoXY
static char         *LcdStringPendent;
//
//---------------------------End--VARIABLES---AREA-----------
//


//
//--------------------------------PROTOTIPS--AREA-----------
//
static unsigned char EncolaOrdre(unsigned char op, unsigned char a1, unsigned char a2, char *s);
static void Espera(unsigned int ms);
static void CantaPartAlta(char c);
static void CantaPartBaixa(char c);
static void CantaIR(char IR);
static void CantaData(char Data);
static void WaitForBusy(void);
static void EscriuPrimeraOrdre(char ordre);
static void AplicaGotoXY(unsigned char col, unsigned char row);
//
//---------------------------End--PROTOTIPS--AREA-----------
//


//
//--------------------------------PUBLIQUES---AREA-----------
//

void LcMotor(void) {
    char Busy;
    OrdreLCD *prox;

    switch (LcdEstat) {

        case LCD_IDLE:
            if (QuantsOrdres == 0) break;

            // Treu la propera ordre i copia els args a les variables de treball.
            prox = &CuaOrdres[FiCua];
            switch (prox->op) {
                case LCD_ORDRE_CLEAR:
                    LcdOp = LCD_OP_CLEAR_ENVIA;
                    break;
                case LCD_ORDRE_GOTOXY:
                    LcdCharPendent = prox->arg1;
                    LcdRowPendent  = prox->arg2;
                    LcdOp = LCD_OP_GOTOXY;
                    break;
                case LCD_ORDRE_PUTSTRING:
                    LcdStringPendent = prox->str;
                    LcdOp = LCD_OP_PUTSTRING;
                    break;
            }
            FiCua++;
            if(FiCua >= MAX_ORDRES) FiCua = 0;
            QuantsOrdres--;
            LcdEstat = LCD_BUSY_SETUP;
            break;

        // --- Gestio del flag Busy ---

        case LCD_BUSY_SETUP:
            SetD4_D7Entrada();
            RSDown();
            RWUp();
            TI_ResetTics(Timer);
            LcdEstat = LCD_BUSY_POLLING;
            break;

        case LCD_BUSY_POLLING:
            EnableUp();
            EnableUp();          // Pols Enable part alta (nibble address counter)
            Busy = GetBusyFlag();
            EnableDown();
            EnableDown();
            EnableUp();
            EnableUp();          // Pols Enable part baixa (descartem)
            EnableDown();
            EnableDown();
            if (TI_GetTics(Timer)) {          // Timeout > 1ms: LCD bloquejat, continuem igualment
                LcdEstat = LcdOp;
                break;
            }
            if (!Busy) {
                LcdEstat = LcdOp;
            }
            break;

        // --- Operacions ---

        case LCD_OP_CLEAR_ENVIA:
            CantaIR(DISPLAY_CLEAR);
            TI_ResetTics(Timer);
            LcdEstat = LCD_OP_CLEAR_ESPERA;
            break;

        case LCD_OP_CLEAR_ESPERA:
            // El busy flag no es fiable despres de Clear (V1.1); esperem per timer.
            // 4 tics x 412us = 1.648ms > 1.52ms temps maxim de Clear del HD44780.
            if (TI_GetTics(Timer) >= 4) {
                LcdEstat = LCD_IDLE;
            }
            break;

        case LCD_OP_GOTOXY:
            AplicaGotoXY(LcdCharPendent, LcdRowPendent);
            LcdEstat = LCD_IDLE;
            break;

        case LCD_OP_PUTSTRING:
            if (TI_GetTics(Timer) == 0) break;
            if (*LcdStringPendent) {
                LcdCharPendent = *LcdStringPendent++;
                CantaData(LcdCharPendent);
                TI_ResetTics(Timer);
            } else {
                LcdEstat = LCD_IDLE;
            }
            break;
    }
}

void LcInit(char rows, char columns) {
// BLOQUEJANT: es crida una sola vegada abans d'arrencar el bucle principal.
// El flag Busy no es fiable durant la sequencia d'inicialitzacio del HD44780,
// per tant s'utilitzen Espera() i EscriuPrimeraOrdre() com marca el datasheet.
    TI_NewTimer(&Timer);
    LcdEstat = LCD_IDLE;
    IniciCua = FiCua = QuantsOrdres = 0;
    SetControlsSortida();
    RSDown();
    RWDown();
    EnableDown();

    Espera(100);                                                        // >= 41.2ms
    EscriuPrimeraOrdre(CURSOR_ON | DISPLAY_CLEAR);
    Espera(5);                                                          // >= 2.06ms
    EscriuPrimeraOrdre(CURSOR_ON | DISPLAY_CLEAR);
    Espera(1);                                                          // >= 412us
    EscriuPrimeraOrdre(CURSOR_ON | DISPLAY_CLEAR);
    Espera(1);                                                          // >= 412us
    EscriuPrimeraOrdre(CURSOR_ON);                                      // Commuta a 4 bits
    Espera(1);
    CantaIR(FUNCTION_SET | DISPLAY_CONTROL);                            // 4 bits, 1 fila
    WaitForBusy(); CantaIR(DISPLAY_CONTROL);                            // Display Off
    WaitForBusy(); CantaIR(DISPLAY_CLEAR);                              // Esborrar
    Espera(4);                                                          // >= 1.648ms (V1.1)
    WaitForBusy(); CantaIR(DISPLAY_ON | CURSOR_ON);                     // Entry mode
    WaitForBusy(); CantaIR(DISPLAY_CONTROL | DISPLAY_ON);               // Display On sense cursor
}

unsigned char LcClear(void) {
    return EncolaOrdre(LCD_ORDRE_CLEAR, 0, 0, 0);
}

unsigned char LcGotoXY(char Column, char Row) {
    return EncolaOrdre(LCD_ORDRE_GOTOXY, Column, Row, 0);
}

unsigned char LcPutString(char *s) {
    return EncolaOrdre(LCD_ORDRE_PUTSTRING, 0, 0, s);
}

unsigned char LcIsBusy(void) {
    return (LcdEstat != LCD_IDLE) || (QuantsOrdres > 0);
}
//
//---------------------------End--PUBLIQUES---AREA-----------
//


//
//--------------------------------PRIVADES----AREA-----------
//

static unsigned char EncolaOrdre(unsigned char op, unsigned char a1, unsigned char a2, char *s) {
    if (QuantsOrdres >= MAX_ORDRES) return 0;
    CuaOrdres[IniciCua].op   = op;
    CuaOrdres[IniciCua].arg1 = a1;
    CuaOrdres[IniciCua].arg2 = a2;
    CuaOrdres[IniciCua].str  = s;
    IniciCua++;
    if(IniciCua >= MAX_ORDRES) IniciCua = 0;
    QuantsOrdres++;
    return 1;
}

static void Espera(unsigned int ms) {
    TI_ResetTics(Timer);
    while(TI_GetTics(Timer) < ms);
}

static void CantaPartAlta(char c) {
    SetD7(c & 0x80 ? 1 : 0);
    SetD6(c & 0x40 ? 1 : 0);
    SetD5(c & 0x20 ? 1 : 0);
    SetD4(c & 0x10 ? 1 : 0);
}

static void CantaPartBaixa(char c) {
    SetD7(c & 0x08 ? 1 : 0);
    SetD6(c & 0x04 ? 1 : 0);
    SetD5(c & 0x02 ? 1 : 0);
    SetD4(c & 0x01 ? 1 : 0);
}

static void CantaIR(char IR) {
    SetD4_D7Sortida();
    RSDown();
    RWDown();
    EnableUp();
    CantaPartAlta(IR);
    EnableUp();
    EnableDown();
    EnableDown();
    EnableUp();
    CantaPartBaixa(IR);
    EnableUp();
    EnableDown();
    EnableDown();
    SetD4_D7Entrada();
}

static void CantaData(char Data) {
    SetD4_D7Sortida();
    RSUp();
    RWDown();
    EnableUp();
    CantaPartAlta(Data);
    EnableUp();
    EnableDown();
    EnableDown();
    EnableUp();
    CantaPartBaixa(Data);
    EnableUp();
    EnableDown();
    EnableDown();
    SetD4_D7Entrada();
}

static void WaitForBusy(void) {
    char Busy;
    SetD4_D7Entrada();
    RSDown();
    RWUp();
    TI_ResetTics(Timer);
    EnableUp(); EnableUp();
    Busy = GetBusyFlag();
    EnableDown(); EnableDown();
    EnableUp(); EnableUp();
    EnableDown(); EnableDown();
    while(Busy && !TI_GetTics(Timer)) {
        EnableUp(); EnableUp();
        Busy = GetBusyFlag();
        EnableDown(); EnableDown();
        EnableUp(); EnableUp();
        EnableDown(); EnableDown();
    }
}

static void EscriuPrimeraOrdre(char ordre) {
    // Envia la primera ordre com si fossin 8 bits (fase d'inicialitzacio).
    SetD4_D7Sortida(); RSDown(); RWDown();
    EnableUp(); EnableUp();
    SetD7(ordre & 0x08 ? 1 : 0);
    SetD6(ordre & 0x04 ? 1 : 0);
    SetD5(ordre & 0x02 ? 1 : 0);
    SetD4(ordre & 0x01 ? 1 : 0);
    EnableDown();
}

static void AplicaGotoXY(unsigned char col, unsigned char row) {
    unsigned char Fisics;

    Fisics = col;
    if(row != 0) Fisics += 0x40;

    CantaIR(SET_DDRAM | Fisics);
    TI_ResetTics(Timer);
}
//
//---------------------------End--PRIVADES----AREA-----------
//
