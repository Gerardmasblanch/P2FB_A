// josepmaria.ribes@salle.url.edu (si hi trobeu alguna errada, si us plau envieu-me un correu :-)
// Arbeca, bressol de l'oliva arbequina
// Mar�, any del Senyor de 2023
// Abril 2023, afegits els #ifndef

// TAD TIMER. Honor i gl�ria


#ifndef T_TIMER_H
#define T_TIMER_H


// CONSTANTS
#define TI_FALS 0
#define TI_CERT 1


//La RSI
void RSI_Timer0(void);
    // IMPORTANT! Funci� que ha der ser cridadda des de la RSI, en en cas que TMR0IF==1.
    // La RSI ha de contenir: if (TMR0IF==1) RSI_Timer0();

void TI_Init (void);
	// Post: Constructor. �s precondici� global haver cridat aquesta funci� quan es cridi qualsevol altra funci� del TAD

unsigned char TI_NewTimer(unsigned char *TimerHandle) ;
	// Post: Retorna TI_CERT en cas que s'hagi creat adequadament un nou timer, i TI_FALS en cas contrati.
    // Posa a *TimerHandle l'identificador de timer assignat, necessari per usar les funcions TI_GetTics i TI_ResetTics.

void TI_ResetTics (unsigned char TimerHandle);
	// Pre: Handle ha estat retornat per Ti_NewTimer.
	// Post: Engega la temporitzaci� associada a 'TimerHandle', guardant la referencia temporal en el moment de la crida.

unsigned long TI_GetTics (unsigned char TimerHandle);
	// Pre: Handle ha estat retornat per TI_NewGetTimer.
	// Post: Retorna els tics transcorreguts des de la crida a TI_ResetTics per al mateix TimerHandle.

void TI_End (void);
	// Post: Destructor del TAD

#endif
