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
#define LCD_OP_CURSOR_ON    5   // Envia instruccio cursor on
#define LCD_OP_CURSOR_OFF   6   // Envia instruccio cursor off
#define LCD_OP_GOTOXY       7   // Envia SET_DDRAM amb l'adreca calculada
#define LCD_OP_PUTCHAR      8   // Envia el byte de dades
#define LCD_OP_PUTCHAR_2    9   // Actualitza ColumnAct/RowAct; fa wrap si cal
#define LCD_OP_PUTSTRING    10  // Envia el seguent caracter de la cadena
#define LCD_OP_PUTSTRING_2  11  // Actualitza cursor despres de cada caracter de cadena

// Tipus d'ordre encolada (cua de 3 ordres pendents)
#define LCD_ORDRE_CLEAR       0
#define LCD_ORDRE_CURSOR_ON   1
#define LCD_ORDRE_CURSOR_OFF  2
#define LCD_ORDRE_GOTOXY      3
#define LCD_ORDRE_PUTCHAR     4
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

static unsigned char Rows, Columns;
static unsigned char RowAct, ColumnAct;
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
static void Espera(int ms);
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
                case LCD_ORDRE_CURSOR_ON:
                    LcdOp = LCD_OP_CURSOR_ON;
                    break;
                case LCD_ORDRE_CURSOR_OFF:
                    LcdOp = LCD_OP_CURSOR_OFF;
                    break;
                case LCD_ORDRE_GOTOXY:
                    LcdCharPendent = prox->arg1;
                    LcdRowPendent  = prox->arg2;
                    LcdOp = LCD_OP_GOTOXY;
                    break;
                case LCD_ORDRE_PUTCHAR:
                    LcdCharPendent = prox->arg1;
                    LcdOp = LCD_OP_PUTCHAR;
                    break;
                case LCD_ORDRE_PUTSTRING:
                    LcdStringPendent = prox->str;
                    LcdOp = LCD_OP_PUTSTRING;
                    break;
            }
            FiCua = (FiCua + 1) % MAX_ORDRES;
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

        case LCD_OP_CURSOR_ON:
            CantaIR(DISPLAY_CONTROL | DISPLAY_ON | CURSOR_ON);
            LcdEstat = LCD_IDLE;
            break;

        case LCD_OP_CURSOR_OFF:
            CantaIR(DISPLAY_CONTROL | DISPLAY_ON);
            LcdEstat = LCD_IDLE;
            break;

        case LCD_OP_GOTOXY:
            AplicaGotoXY(LcdCharPendent, LcdRowPendent);
            LcdEstat = LCD_IDLE;
            break;

        case LCD_OP_PUTCHAR:
            CantaData(LcdCharPendent);
            TI_ResetTics(Timer);
            LcdEstat = LCD_OP_PUTCHAR_2;
            break;

        case LCD_OP_PUTCHAR_2:
            if (TI_GetTics(Timer) == 0) break;
            // Ha passat >= 1 tic (412us) des de CantaData. Temps max escriptura LCD = 53us.
            // Podem enviar SET_DDRAM directament sense busy-check.
            ++ColumnAct;
            if (Rows == 3) {
                if (ColumnAct >= 20) {
                    ColumnAct = 0;
                    if (++RowAct >= 4) RowAct = 0;
                    AplicaGotoXY(ColumnAct, RowAct);
                }
            } else if (Rows == 2) {
                if (ColumnAct >= 40) {
                    ColumnAct = 0;
                    if (++RowAct >= 2) RowAct = 0;
                    AplicaGotoXY(ColumnAct, RowAct);
                }
            } else if (RowAct == 1) {
                if (ColumnAct >= 40) {
                    ColumnAct = 0;
                    AplicaGotoXY(ColumnAct, RowAct);
                }
            }
            LcdEstat = LCD_IDLE;
            break;

        case LCD_OP_PUTSTRING:
            if (TI_GetTics(Timer) == 0) break;
            if (*LcdStringPendent) {
                LcdCharPendent = *LcdStringPendent++;
                CantaData(LcdCharPendent);
                TI_ResetTics(Timer);
                LcdEstat = LCD_OP_PUTSTRING_2;
            } else {
                LcdEstat = LCD_IDLE;
            }
            break;

        case LCD_OP_PUTSTRING_2:
            if (TI_GetTics(Timer) == 0) break;
            // Ha passat >= 1 tic (412us) des de CantaData -> podem actuar sense busy-check.
            // Si hi ha wrap, AplicaGotoXY envia SET_DDRAM; el seguent tic torna a PUTSTRING
            // que fa CantaData, amb >= 412us de marge (temps max SET_DDRAM = 37us). Segur.
            ++ColumnAct;
            if (Rows == 3) {

                if (ColumnAct >= 20) {
                    ColumnAct = 0;
                    if (++RowAct >= 4) RowAct = 0;
                    AplicaGotoXY(ColumnAct, RowAct);
                }

            } else if (Rows == 2) {
                if (ColumnAct >= 40) {
                    ColumnAct = 0;
                    if (++RowAct >= 2) RowAct = 0;
                    AplicaGotoXY(ColumnAct, RowAct);
                }

            } else if (RowAct == 1) {
                if (ColumnAct >= 40) {
                    ColumnAct = 0;
                    AplicaGotoXY(ColumnAct, RowAct);
                }
            }

            LcdEstat = LCD_OP_PUTSTRING;
            break;
    }
}

void LcInit(char rows, char columns) {
// BLOQUEJANT: es crida una sola vegada abans d'arrencar el bucle principal.
// El flag Busy no es fiable durant la sequencia d'inicialitzacio del HD44780,
// per tant s'utilitzen Espera() i EscriuPrimeraOrdre() com marca el datasheet.
    int i;
    TI_NewTimer(&Timer);
    Rows = rows; Columns = columns;
    RowAct = ColumnAct = 0;
    LcdEstat = LCD_IDLE;
    IniciCua = FiCua = QuantsOrdres = 0;
    SetControlsSortida();
    RSDown();
    RWDown();
    EnableDown();
    for (i = 0; i < 2; i++) {
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
}

void LcEnd(void) {
    // TAD_TIMER no exposa cap funcio per alliberar timers; ho deixem buit.
}

unsigned char LcClear(void) {
    return EncolaOrdre(LCD_ORDRE_CLEAR, 0, 0, 0);
}

unsigned char LcCursorOn(void) {
    return EncolaOrdre(LCD_ORDRE_CURSOR_ON, 0, 0, 0);
}

unsigned char LcCursorOff(void) {
    return EncolaOrdre(LCD_ORDRE_CURSOR_OFF, 0, 0, 0);
}

unsigned char LcGotoXY(char Column, char Row) {
    return EncolaOrdre(LCD_ORDRE_GOTOXY, Column, Row, 0);
}

unsigned char LcPutChar(char c) {
    return EncolaOrdre(LCD_ORDRE_PUTCHAR, c, 0, 0);
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
    IniciCua = (IniciCua + 1) % MAX_ORDRES;
    QuantsOrdres++;
    return 1;
}

static void Espera(int ms) {
    TI_ResetTics(Timer);
    while (TI_GetTics(Timer) < (unsigned long) ms);
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
    do {
        EnableUp(); EnableUp();
        Busy = GetBusyFlag();
        EnableDown(); EnableDown();
        EnableUp(); EnableUp();
        EnableDown(); EnableDown();
        if (TI_GetTics(Timer)) break; // Timeout: LCD bloquejat
    } while (Busy);
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
    // Calcula l'adreca DDRAM fisica i envia SET_DDRAM. Actualitza RowAct/ColumnAct.
    int Fisics;
    switch (Rows) {
        case 2:
            Fisics = col + (!row ? 0 : 0x40);
            break;
        case 4:
            Fisics = col;
            if      (row == 1) Fisics += 0x40;
            else if (row == 2) Fisics += Columns;
            else if (row == 3) Fisics += 0x40 + Columns;
            break;
        default:
            Fisics = col;
            break;
    }
    CantaIR(SET_DDRAM | Fisics);
    TI_ResetTics(Timer);
    RowAct    = row;
    ColumnAct = col;
}
//
//---------------------------End--PRIVADES----AREA-----------
//
