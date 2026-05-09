#ifndef TAD_LCD_H
#define TAD_LCD_H
//
// ADT for manipulating the alphanumeric display of the
// HD44780 controller using only 4 data bits.
//
// Versio cooperativa amb cua FIFO de 3 ordres pendents.
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
#define SetControlsSortida()(TRISBbits.TRISB3 = TRISBbits.TRISB1 = TRISBbits.TRISB0 = 0)
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
// Pre:  rows = {1, 2, 4}, columns = {8, 16, 20, 24, 32, 40}
// Pre:  40ms de marge entre VCC i aquesta crida
// Pre:  hi ha un timer lliure
// Post: BLOQUEJANT (~100ms). Pantalla neta, cursor apagat, posicio (0,0).
//       Cua d'ordres buida.

void LcEnd(void);

unsigned char LcClear(void);
// Post: Encola una ordre Clear. Retorna 1 si encolada, 0 si la cua esta plena.

unsigned char LcCursorOn(void);
// Post: Encola activacio del cursor. Retorna 1/0 segons disponibilitat.

unsigned char LcCursorOff(void);
// Post: Encola desactivacio del cursor. Retorna 1/0 segons disponibilitat.

unsigned char LcGotoXY(char Column, char Row);
// Pre:  Column 0..39, Row 0..3
// Post: Encola moviment de cursor. Retorna 1/0 segons disponibilitat.

unsigned char LcPutChar(char c);
// Post: Encola escriure un caracter. Retorna 1/0 segons disponibilitat.

unsigned char LcPutString(char *s);
// Pre:  La cadena 's' ha de romandre valida fins que la pinti el motor.
// Post: Encola escriure la cadena. Retorna 1/0 segons disponibilitat.

void LcMotor(void);
// Pre:  Cridar un cop per tic des del bucle principal.
// Post: Avanca un pas de l'operacio en curs (o en treu una de la cua) i retorna.

unsigned char LcIsBusy(void);
// Post: Retorna 1 si el motor esta processant O la cua te ordres pendents.
//       Retorna 0 nomes si tot esta acabat.


#endif /* TAD_LCD_H */
