static unsigned int minutes = 0;
static unsigned int wd_counter = 0;
static char btn_delay = 0; 

void timer_init(void)
{
    BCSCTL2 |= DIVS_3;                        // SMCLK/8
    WDTCTL = WDT_MDLY_32;                     // WDT Timer interval
    
    IE1 |= WDTIE;                             // Enable WDT interrupt
}; //timer_init

// Watchdog Timer interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(WDT_VECTOR))) watchdog_timer (void)
#else
#error Compiler not supported!
#endif
{
    wd_counter++;
    if (wd_counter > 250) // approx. 1 minute lag
    {
        wd_counter = 0;
        if (minutes < 65535) minutes++;
            else minutes = 0;
    };
    
    if (btn_delay > 0) btn_delay--; // антидребезг
    
    SD16CCTL0 |= SD16SC;                      // Start SD16 conversion
}

