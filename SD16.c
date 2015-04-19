static unsigned int warm_delay, cold_delay;    
static int ChA1results = 0x00;
static int ChA6results = 0x00;
static unsigned int ChA7results = 0x00;
static unsigned int ch_counter = 0;         

static unsigned int minutes = 0;
static unsigned char m = 0;
static int data[35];

static unsigned char k = 0;
_Bool show_info = FALSE;
_Bool btn_sel_pressed = FALSE;
_Bool btn_next_pressed = FALSE;

void SD16_init(void)
{
  warm_delay = 0;
  cold_delay = 0;


  BCSCTL2 |= DIVS_3;                        // SMCLK/8
  WDTCTL = WDT_MDLY_32;                     // WDT Timer interval

  SD16CTL = SD16REFON +SD16SSEL_1;          // 1.2V ref, SMCLK
  SD16INCTL0 = SD16INCH_7;                  // A7+/- (calibrate)
  SD16CCTL0 = SD16SNGL + SD16IE + SD16UNI;           // Single conv, interrupt, unipolar
  
  SD16INCTL0 |= SD16INTDLY_0;               // Interrupt on 4th sample 
  
  SendByte(0x04, FALSE);  // Сдвигаем курсор назад.
  
  IE1 |= WDTIE;                             // Enable WDT interrupt
}

void PrintFloat(char i, int temp)
{
	_Bool minus = FALSE;
	
    if(temp < 0)
    {
	    temp *= -1;
	    minus = TRUE;
	}

	MoveCursor(i,7);

    SendByte(0x30 + (temp%10), TRUE);    
    temp /= 10;
    SendByte(0x2e, TRUE);   // печатаем '.'
    SendByte(0x30 + (temp%10), TRUE);    
    temp /= 10;
    SendByte(0x30 + (temp%10), TRUE);  
    
    if(minus) SendByte(0x2d, TRUE);  // print '-'
        else SendByte(0x2b, TRUE);  // print '+'
}
    

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=SD16_VECTOR
__interrupt void SD16ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(SD16_VECTOR))) SD16ISR (void)
#else
#error Compiler not supported!
#endif
{
    
    unsigned int temp = SD16MEM0;
    
    switch(ch_counter)
    {  
    case 0:
        ChA7results = temp;             // Save CH7 results (clears IFG)

        SD16INCTL0 &= ~SD16INCH_7;          // Disable channel A7+/-
        ch_counter++;
        
        SD16INCTL0 |= SD16INCH_4;           // Enable channel A4+/-                          
        SD16AE |= SD16AE1;                  // Enable external input on A4+ 
        break; 
        
    case 1:
        temp -= 13653;
        temp /= 55;

        ChA1results = temp;             // Save CH1 results (clears IFG)
        ChA1results -= ChA7results;			// Subtracting delta
        ChA1results -= 270; //250;			// calibrate

        SD16AE &= ~SD16AE1;                 // Disable external input A4+, A4
        SD16INCTL0 &= ~SD16INCH_4;          // Disable channel A4+/-
        ch_counter++;
        
        SD16INCTL0 |= SD16INCH_6;           // Enable channel A6+/-                          
        break; 
        
    case 2:
        temp -= 39361;
        ChA6results = temp;				// Save CH6 results (clears IFG)        
        ChA6results *= 2; 
        ChA6results /= 29;

        SD16INCTL0 &= ~SD16INCH_6;          // Disable channel A1+/-
        ch_counter = 0;
        
        SD16INCTL0 |= SD16INCH_7;           // Enable channel A7+/-                          
        break; 
    }
    
 
//    SendByte(0x06, FALSE);  // Сдвигаем курсор вперед.

    if (btn_sel_pressed)
    {
        btn_sel_pressed = FALSE;
        k = 0;
        show_info = !show_info;
        MoveCursor(0,7);
        PrintStr("        ");
        MoveCursor(1,7);
        PrintStr("        ");
    };
    
    if(btn_next_pressed)
    {
        btn_next_pressed = FALSE;
        if (k < 34) k++;
        else k = 0;
    };
    
    if(show_info)
    {
        PrintFloat(0, k);
        PrintFloat(1, data[k]);
    } else
    {
        PrintFloat(1, ChA1results);
        PrintFloat(0, ChA6results);
    };
    
// logging 

    if(minutes % 1250 == 0 && m < 35)
    {
        data[m] = ChA1results;
        m++;
        MoveCursor(0, 1);
        SendByte(0x30 + (m%10), TRUE);
        SendByte(0x30 + (m/10), TRUE); 
    };
    
    if(minutes < 65000)
    {
        minutes++;
    };

// end logging
        
    if (ChA1results > 60)		// порог +6
    {                          // on
        warm_delay++;
        if (warm_delay > 450)  // после повышения температуры ждем 2 мин
        {
            cold_delay = 0;
            warm_delay = 0;
			P1OUT |= relay;
//            MoveCursor(0, 1);
//            PrintStr("no");
        }

        MoveCursor(1, 0);
        SendByte(0xda, TRUE); // arrow down
	};
    
	if(ChA1results < 30)		// порог +3
    {                          // off
        cold_delay++;
        if (cold_delay > 1251)  // после опускания температуры ждем 5 мин
        {
            cold_delay = 0;
            warm_delay = 0;
    		P1OUT &= ~relay;
    		MoveCursor(0, 1);
//            PrintStr("  ");
        }

        MoveCursor(1, 0);
        SendByte(0xd9, TRUE); // arrow up
	};
        
    
}

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
    SD16CCTL0 |= SD16SC;                      // Start SD16 conversion
}

