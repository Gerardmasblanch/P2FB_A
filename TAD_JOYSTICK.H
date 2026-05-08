#ifndef TAD_JOYSTICK_H
#define TAD_JOYSTICK_H

#define JOY_CENTRE 0
#define JOY_AMUNT  1
#define JOY_AVALL  2
#define JOY_ESQ    3
#define JOY_DRETA  4

void          JOY_Init(void);

// Retorna AMUNT/AVALL/ESQ/DRETA nomes una vegada per moviment.
// Mentre el joystick es mantingui desplacat retorna CENTRE.
// L'usuari ha de tornar al centre per poder generar un nou moviment.
unsigned char JOY_GetMoviment(void);

// Retorna 1 nomes UN cicle quan es detecta el flanc de premuda del boto.
unsigned char JOY_BotoNouPremut(void);

// Motor per actualitzar estat intern (deteccio de flancs).
// Cal cridar-lo periodicament des del bucle principal.
void          JOY_Motor(void);

#endif