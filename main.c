/*
 * LSFarm - PIC18F4321
 */

#include <xc.h>
#include "TAD_TIMER.H"
#include "TAD_SIOFARM.H"
#include "TAD_SIO.H"
#include "TAD_ADC.H"
#include "TAD_LCD.H"
#include "TAD_EEPROM.H"
#include "TAD_LED.H"
#include "TAD_JOYSTICK.H"
#include "TAD_FARM.H"

#pragma config OSC    = HS
#pragma config PBADEN = DIG
#pragma config MCLRE  = ON
#pragma config DEBUG  = OFF
#pragma config PWRT   = OFF
#pragma config BOR    = OFF
#pragma config WDT    = OFF
#pragma config LVP    = OFF

void __interrupt() LaRSI(void)
{
    if (INTCONbits.TMR0IE && INTCONbits.TMR0IF) {
        RSI_Timer0();
    }

    if (PIE1bits.RCIE && PIR1bits.RCIF) {
        SIO_InterrupcioRX();
    }

    if (PIE1bits.TXIE && PIR1bits.TXIF) {
        SIO_InterrupcioTX();
    }
}

void main(void)
{
    TI_Init();
    SIOFARM_Init();
    SIO_Init();
    AD_Init();
    EEPROM_Init();
    LED_Init();
    JOY_Init();
    FARM_Init();

    ei();
    LcInit(2, 16);

    while (1) {
        SIOFARM_Motor();
        SIO_Motor();
        LcMotor();
        AD_Motor();
        JOY_Motor();
        FARM_Motor();
        LED_Motor();
    }
}
