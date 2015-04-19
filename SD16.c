#include "timer.c"

static int ChA1results = 0x00;
static int ChA6results = 0x00;
static unsigned int ChA7results = 0x00;
static unsigned int ch_counter = 0;    
static unsigned int on_time = 0; 
static unsigned int off_time = 0;     

_Bool show_info = FALSE;
_Bool summer = FALSE;
_Bool btn_sel_pressed = FALSE;
_Bool btn_next_pressed = FALSE;

void SD16_init(void)
{
  timer_init();

  SD16CTL = SD16REFON +SD16SSEL_1;          // 1.2V ref, SMCLK
  SD16INCTL0 = SD16INCH_7;                  // A7+/- (calibrate)
  SD16CCTL0 = SD16SNGL + SD16IE + SD16UNI;           // Single conv, interrupt, unipolar
  
  SD16INCTL0 |= SD16INTDLY_0;               // Interrupt on 4th sample 
  
  SendByte(0x04, FALSE);  // Сдвигаем курсор назад. 

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
        
    PrintStr("  ");
}

void PrintInt(char i, int temp)
{
    if(temp < 0)
    {
	    temp *= -1;
	}

	MoveCursor(i,7);
    
    SendByte(0x30 + (temp%10), TRUE);
    temp /= 10;
    SendByte(0x30 + (temp%10), TRUE);
    temp /= 10;
    SendByte(0x30 + (temp%10), TRUE);
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
    
    // buttons
 
    if (btn_sel_pressed)
    {        
        btn_delay = 4;
        btn_sel_pressed = FALSE;

        show_info = !show_info;
    };
    
    if(btn_next_pressed)
    {
        btn_delay = 4;
        btn_next_pressed = FALSE;

        summer = !summer;
    };
    
    if(show_info)
    {
        if(P1OUT & relay) // if on
        {
            temp = minutes - on_time;
            PrintInt(0, temp);
            PrintStr("   no");
            temp = on_time - off_time;
            PrintInt(1, temp);
            PrintStr("  ffo");
        } else           // if off
        {
            temp = minutes - off_time;
            PrintInt(1, temp);
            PrintStr("  ffo");
            temp = off_time - on_time;
            PrintInt(0, temp);
            PrintStr("   no");
        };       
    } else
    {
        PrintFloat(1, ChA1results);
        PrintFloat(0, ChA6results);
        
        MoveCursor(0, 0);
        if (summer) SendByte(0xee, TRUE);
            else SendByte(0x2a, TRUE);
            
    };
    
// thermo on\off       

char on_temp, off_temp; 

    if (summer)
    {
        on_temp  = 80;
        off_temp = 40;
    } else
    {
        on_temp  = 60;
        off_temp = 30;
    };

    if ((ChA1results > on_temp) &&  // верхний порог и еще не включен
        (!(P1OUT & relay)) &&       // и с первого пуска прошло 2 минуты
        (minutes > 1))		 
    {                          // on
        on_time = minutes;
        P1OUT |= relay;
	};
    
	if((ChA1results < off_temp) &&
       (P1OUT & relay))		// нижний порог и включен
    {                          // off
        off_time = minutes;
        P1OUT &= ~relay;
	};
    
    if(!show_info) 
    {
        MoveCursor(1, 0);
        
        if(ChA1results > on_temp)
            SendByte(0xda, TRUE); // arrow down
        if(ChA1results < off_temp)                   
            SendByte(0xd9, TRUE); // arrow up
    };

    
            
    
}



