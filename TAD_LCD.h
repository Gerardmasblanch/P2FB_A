#ifndef TAD_LCD_H
#define TAD_LCD_H
//
// ADT for manipulating the alphanumeric display of the
// HD44780 controller using only 4 data bits.
//
// ------------------------------------HARDWARE---AREA--------------------
//  RS    --> RB7
//  R/!W  --> RB6
//  E     --> RB5
//  D4    --> RB4
//  D5    --> RB3
//  D6    --> RB1
//  D7    --> RB0
// -------------------------------END--HARDWARE---AREA--------------------

#include <xc.h>

#define SetD4_D7Sortida()   (TRISBbits.TRISB4 = TRISBbits.TRISB3 = TRISBbits.TRISB1 = TRISBbits.TRISB0 = 0)
#define SetD4_D7Entrada()   (TRISBbits.TRISB4 = TRISBbits.TRISB3 = TRISBbits.TRISB1 = TRISBbits.TRISB0 = 1)
#define SetControlsSortida()(TRISBbits.TRISB7 = TRISBbits.TRISB6 = TRISBbits.TRISB5 = 0)
#define SetD4(On)           (LATBbits.LATB4 = (On))
#define SetD5(On)           (LATBbits.LATB3 = (On))
#define SetD6(On)           (LATBbits.LATB1 = (On))
#define SetD7(On)           (LATBbits.LATB0 = (On))
#define GetBusyFlag()       (PORTBbits.RB0)
#define RSUp()              (LATBbits.LATB7 = 1)
#define RSDown()            (LATBbits.LATB7 = 0)
#define RWUp()              (LATBbits.LATB6 = 1)
#define RWDown()            (LATBbits.LATB6 = 0)
#define EnableUp()          (LATBbits.LATB5 = 1)
#define EnableDown()        (LATBbits.LATB5 = 0)

void LcInit(char rows, char columns);
void LcClear(void);
void LcGotoXY(char Column, char Row);
void LcPutString(char *s);

void LcMotor(void);
unsigned char LcIsBusy(void);

#endif
