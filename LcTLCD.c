//
// ADT for manipulating the alphanumeric display of the 
// HD44780 controller using only 4 data bits.
// This is the controller that almost all displays have integrated. 
// The maximum size is 4 rows and 40 columns.
//
// F. Escudero vCli v1.0 Piera, January of 2004
//
// I have tested this ADT with a 2x16 LCD. If you observe any error please 
// report it to sisco@salleurl.edu.
//
// Vcli V1.1, Sisco, at 26th of November of 2004. I have seen that with some LCDs 
// we must first wait 2ms and then activate a Clear, independently of what Busy says.
//
// VCli V1.3, jnavarro, a 2013. I have extended the initialization time (now it takes 
// 150 ms. but it is initialized at the first attempt. More info here: 
// http://web.alfredstate.edu/weimandn/lcd/lcd_initialization/lcd_initialization_index.html
// Still observing the same alteration with Busy, except with the timeout.

#include <xc.h>
#include "TiTTimer.h"
#include "LcTLCD.h"


//
//--------------------------------CONSTANTS---AREA-----------
//
#define FUNCTION_SET	0x20
#define BITS_8			0x10
#define DISPLAY_CONTROL	0x08
#define DISPLAY_ON		0x04
#define CURSOR_ON		0x02
#define DISPLAY_CLEAR	0x01
#define ENTRY_MODE		0x04
#define SET_DDRAM		0x80

//ESTATS MOTOR LCD

#define LCD_IDLE			0
#define LCD_BUSY_SETUP		1
#define LCD_BUSY_POLLING	2

//OPERACIONS DESPRES DE BUSY = 0

#define LCD_OP_CLEAR_ENVIA 	3
#define LCD_OP_CLEAR_ESPERA	4
#define LCD_OP_CURSOR_ON    5
#define LCD_OP_CURSOR_OFF   6
#define LCD_OP_GOTOXY		7
#define LCD_OP_PUTCHAR		8
#define LCD_OP_PUTCHAR_2	9
#define LCD_OP_PUTSTRING	10

//
//---------------------------End--CONSTANTS---AREA-----------
//


//
//--------------------------------VARIABLES---AREA-----------
//
static unsigned char Rows, Columns;
static unsigned char RowAct, ColumnAct;
static int Timer;
static unsigned char LcdEstat;
static unsigned char LcdOp;
static unsigned char LcdCharPendent;
static char *LcdStringPendent;

//
//---------------------------End--VARIABLES---AREA-----------
//

//
//--------------------------------PROTOTIPE--AREA-----------
//
void Espera(int Timer, int ms);
void CantaIR(char IR);
void CantaData(char Data);
void WaitForBusy(void);
void EscriuPrimeraOrdre(char);

//
//---------------------------End--PROTOTIYPE--AREA-----------
//


//
//--------------------------------PUBLIQUES---AREA-----------
//
void LcMotor(void){

	char Busy;

	switch (LcdEstat) {
	case LCD_IDLE:
		break;

	
	case LCD_BUSY_SETUP:
		SetD4_D7Entrada();
		RSDown();
		RWUp();
		TiResetTics(Timer);
		LcdEstat = LCD_BUSY_POLLING;
		break;

	case LCD_BUSY_POLLING:
		EnableUp();
		EnableUp(); //Making sure the 500ns of the pulse time
		Busy = GetBusyFlag();
		EnableDown();
		EnableDown();
		EnableUp();
		EnableUp();
		// The lower part of the address counter, it is not interesting for us. 
		EnableDown();
		EnableDown();
		if (TiGetTics(Timer)) {
			LcdEstat = LcdOp ;
			break; // More than one ms means that the LCD has gone mad.
		} 
		
		if (!Busy) {
			LcdEstat = LcdOp;
		} else{
			return;
		}
		break;
	
	case LCD_OP_CLEAR_ENVIA:
		CantaIR(DISPLAY_CLEAR);	   //Spaces
		TiResetTics(Timer);
		LcdEstat = LCD_OP_CLEAR_ESPERA;
		break;
	case LCD_OP_CLEAR_ESPERA:
		if (TiGetTics(Timer) < 3) break; // V1.1
		LcdEstat = LCD_IDLE;
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
		LcGotoXY(LcdCharPendent, LcdOp);
		LcdEstat = LCD_IDLE;
		break;
	case LCD_OP_PUTCHAR:
		CantaData(LcdCharPendent);
		LcdEstat = LCD_OP_PUTCHAR_2;
		break;
	case LCD_OP_PUTCHAR_2:
		// The cursor position is recalculated.
		++ColumnAct;
		if (Rows == 3) {
			if (ColumnAct >= 20) {
				ColumnAct = 0;
				if (++RowAct >= 4) RowAct = 0;
				LcGotoXY(ColumnAct, RowAct);
			}
		} else
		if (Rows == 2) {
			if (ColumnAct >= 40) {
				ColumnAct = 0;
				if (++RowAct >= 2) RowAct = 0;
				LcGotoXY(ColumnAct, RowAct);	
			}
		} else
		if (RowAct == 1) {
			if (ColumnAct >= 40) ColumnAct = 0;
			LcGotoXY(ColumnAct, RowAct);
		} else {
			LcdEstat = LCD_IDLE;
		}
		break;

	case LCD_OP_PUTSTRING:
		if (*LcdStringPendent) {
			CantaData(*LcdStringPendent++);
			LcdEstat = LCD_OP_PUTCHAR_2;
		} else {
			LcdEstat = LCD_IDLE;
		}
		break;
	
	}
}

//NOT OK!
void LcInit(char rows, char columns) {
// Pre: Rows = {1, 2, 4}  Columns = {8, 16, 20, 24, 32, 40 }
// Pre: It needs 40ms of tranquility between VCC raising until this constructor is called.
// Pre: There is a free timer
// Post: This routine can last until 100ms
// Post: The display remains cleared, the cursor is turned OFF and at the position (0, 0).
	int i;
	Timer = TiGetTimer(); 
	Rows = rows; Columns = columns;
	RowAct = ColumnAct = 0;
	SetControlsSortida();
	for (i = 0; i < 2; i++) {
		Espera(Timer, 100);
		// This sequence is set by the manual.

		EscriuPrimeraOrdre(CURSOR_ON | DISPLAY_CLEAR);
		Espera(Timer, 5);
		EscriuPrimeraOrdre(CURSOR_ON | DISPLAY_CLEAR);
		Espera(Timer, 1);
		EscriuPrimeraOrdre(CURSOR_ON | DISPLAY_CLEAR);
		Espera(Timer, 1);
		// .. three times. 
		// Now one at 4 bits
		EscriuPrimeraOrdre(CURSOR_ON);
		Espera(Timer, 1);
		CantaIR(FUNCTION_SET | DISPLAY_CONTROL); 	// 4bits, 1 row, font 5x7
		// The first line is erased here 
		// Now we can wait for busy
		WaitForBusy(); 	CantaIR(DISPLAY_CONTROL);  	// Display Off
		WaitForBusy(); 	CantaIR(DISPLAY_CLEAR);	   	// All spaces
		Espera(Timer,3); // 1.64ms V1.1
		WaitForBusy(); 	CantaIR(DISPLAY_ON | CURSOR_ON); // Auto Increment and shift
		WaitForBusy(); 	CantaIR(DISPLAY_CONTROL | DISPLAY_ON | CURSOR_ON | DISPLAY_CLEAR); 		// Display On
	}
	//The manual says that it should work but it doesn't initialize 
    //correctly after 40ms. Therefore, there is a loop with two initializations 
    //from here the initialization works correctly if a reset is made or if
    //the supply is turned ON and OFF. 
}

void LcEnd(void) {
// The destructor
	TiCloseTimer (Timer); // It is not needed anymore
}

//NOT OK!
void LcClear(void) {
// Post: Erases the display and sets the cursor to its previous state. 
// Post: The next order can last up to 1.6ms. 
	WaitForBusy(); 	CantaIR(DISPLAY_CLEAR);	   //Spaces
	Espera(Timer, 3); // V1.1
}

//NOT OK!
void LcCursorOn(void) {
// Post: Turn on the cursor
// Post: The next order can last up to 40us. 
	WaitForBusy();
	CantaIR(DISPLAY_CONTROL | DISPLAY_ON | CURSOR_ON);
}

//NOT OK!
void LcCursorOff(void) {
// Post: Turns off the cursor
// Post: The next order can last up to 40us. 
	WaitForBusy();
	CantaIR(DISPLAY_CONTROL | DISPLAY_ON);
}

//NOT OK!
void LcGotoXY(char Column, char Row) {
// Pre : Column between 0 and 39, row between 0 and 3. 
// Post: Sets the cursor to those coordinates. 
// Post: The next order can last until 40us.
	int Fisics;
	// calculating the effective address of the LCD ram. 
	switch (Rows) {
		case 2:
			Fisics = Column + (!Row ? 0 : 0x40); break;
		case 4:
			Fisics = Column;
			if (Row == 1) Fisics += 0x40; else
			if (Row == 2) Fisics += Columns;      /* 0x14; */ else
			if (Row == 3) Fisics += 0x40+Columns; /* 0x54; */
			break;
		case 1:
		default:
			Fisics = Column; break;
	}
	// applying the command
	WaitForBusy();
	CantaIR(SET_DDRAM | Fisics);
	// Finally, I refresh the local images.
	RowAct    = Row;
	ColumnAct = Column;
}

//NOT OK!
void LcPutChar(char c) {
// Post: Paints the char in the actual cursor position and increments 
// its position. If the column gets to 39 it returns to 0.
// The row of the LCD is increased when this happens until the second
// row and then it is reset back to row 0 if it has 2 rows total. 
// If the LCD has 4 rows it will reset back to row 0 when it
// reaches row 4 and the columns will go till 39 before reseting to 0.
// The one row LCDs returns to 0 when a column gets to 39. 
// The row is never increased. 
	// The char is written
	WaitForBusy(); CantaData(c);
	// The cursor position is recalculated.
	++ColumnAct;
	if (Rows == 3) {
		if (ColumnAct >= 20) {
			ColumnAct = 0;
			if (++RowAct >= 4) RowAct = 0;
			LcGotoXY(ColumnAct, RowAct);
		}
	} else
	if (Rows == 2) {
		if (ColumnAct >= 40) {
			ColumnAct = 0;
			if (++RowAct >= 2) RowAct = 0;
			LcGotoXY(ColumnAct, RowAct);
		}
	} else
	if (RowAct == 1) {
		if (ColumnAct >= 40) ColumnAct = 0;
		LcGotoXY(ColumnAct, RowAct);
	}
}

//NOT OK!
void LcPutString(char *s) {
// Post: Paints the string from the actual cursor position. 
// The coordinate criteria is the same as the LcPutChar. 
// Post: Can last up to 40us for each char of a routine output.
	while(*s) LcPutChar(*s++);
}

//
//---------------------------End--PUBLIC---AREA-----------
//

//
//--------------------------------PRIVATE----AREA-----------
//

// NOT OK!
void Espera(int Timer, int ms) {
	TiResetTics(Timer);
	while(TiGetTics(Timer) < ms);
}

// OK
void CantaPartAlta(char c) {
	 SetD7(c & 0x80 ? 1 : 0);
	 SetD6(c & 0x40 ? 1 : 0);
	 SetD5(c & 0x20 ? 1 : 0);
	 SetD4(c & 0x10 ? 1 : 0);
}

//OK
void CantaPartBaixa(char c) {
	 SetD7(c & 0x08 ? 1 : 0);
	 SetD6(c & 0x04 ? 1 : 0);
	 SetD5(c & 0x02 ? 1 : 0);
	 SetD4(c & 0x01 ? 1 : 0);
}

//OK
void CantaIR(char IR) {
	SetD4_D7Sortida();
	RSDown();
	RWDown();
	EnableUp();
	CantaPartAlta(IR); 		// Data Setup = 80ns
	EnableUp();				// Making sure the pulse lasts 500ns
	EnableDown();   		// The pulse width "enable" is higher than 230ns
	EnableDown();
	EnableUp();
	CantaPartBaixa(IR); 	// Data Setup = 80ns
	EnableUp();				// Making sure the pulse lasts 500ns
	EnableDown();   		// The pulse width "enable" is higher than 230ns
	SetD4_D7Entrada();
}

//OK
void CantaData(char Data) {
	SetD4_D7Sortida();
	RSUp();
	RWDown();
	EnableUp();
	CantaPartAlta(Data); 	// Data Setup = 80ns
	EnableUp();				// Making sure the pulse lasts 500ns
	EnableDown();   		// The pulse width "enable" is higher than 230ns
	EnableDown();
	EnableUp();
	CantaPartBaixa(Data); 	// Data Setup = 80ns
	EnableUp();				// Making sure the pulse lasts 500ns
	EnableDown();   		// The pulse width "enable" is higher than 230ns
	SetD4_D7Entrada();
}

//NOT OK!
void WaitForBusy(void) { char Busy;
	SetD4_D7Entrada();
	RSDown();
	RWUp();
	TiResetTics(Timer);
	do {
		EnableUp();EnableUp(); //Making sure the 500ns of the pulse time
		Busy = GetBusyFlag();
		EnableDown();
		EnableDown();
		EnableUp();EnableUp();
		// The lower part of the address counter, it is not interesting for us. 
		EnableDown();
		EnableDown();
		if (TiGetTics(Timer)) break; // More than one ms means that the LCD has gone mad.
	} while(Busy);
}

//OK
void EscriuPrimeraOrdre(char ordre) {
	// Write the first as if there are 8 bits.
	SetD4_D7Sortida();  RSDown(); RWDown();
	EnableUp(); EnableUp();
	 SetD7(ordre & 0x08 ? 1 : 0);
	 SetD6(ordre & 0x04 ? 1 : 0);
	 SetD5(ordre & 0x02 ? 1 : 0);
	 SetD4(ordre & 0x01 ? 1 : 0);
	EnableDown();
}

//
//---------------------------End--PRIVATE----AREA-----------
//



