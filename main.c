#include <msp430f2013.h>
#define relay   BIT0
#include "LCD.c"
#include "SD16.c"


void main(void) {
    WDTCTL = WDTPW + WDTHOLD;
    
    P1OUT = 0;
    P1DIR = 0;

    LCM_init();
    clear();
    
    P1DIR |= relay;

    SD16_init();

	P2OUT = 0;
    P2SEL = 0;
    P2DIR = 0;
    P2REN |= (BIT6 + BIT7); //разрешаем подтяжку
    P2OUT |= (BIT6 + BIT7); //подтяжка вывода вверх

	P2IES |= (BIT6 + BIT7); // прерывание по переходу 1->0 выбирается битом IESx = 1.
	P2IFG &= ~(BIT6 + BIT7); // Для предотвращения немедленного вызова прерывания,
                        // обнуляем его флаг до разрешения прерываний
	P2IE |= (BIT6 + BIT7);   // Разрешаем прерывания для P2.6, P2.7 

    _BIS_SR(LPM0_bits + GIE);      // Enter LPM0 with interrupt
    
} // main

// Buttons interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT2_VECTOR
__interrupt void P2_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT2_VECTOR))) P2_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch(P2IFG & (BIT6 | BIT7))
    {
        case BIT7:
          P2IFG &= ~BIT7;
          btn_sel_pressed = TRUE;
        
        case BIT6:
          P2IFG &= ~BIT6;
          btn_next_pressed = TRUE;
        
        default:
          P2IFG = 0;
          return;
    }
} //P2_ISR

