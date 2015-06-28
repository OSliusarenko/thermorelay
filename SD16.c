static int ChA1results = 0x00;
static int ChA6results = 0x00;
static unsigned int ChA7results = 0x00;
static unsigned int ch_counter = 0;    
static unsigned int onofftime[3] = {0,0,0}; 

_Bool show_info = FALSE;
_Bool summer = FALSE;
_Bool btn_sel_pressed = FALSE;
_Bool btn_next_pressed = FALSE;

#include "timer.c"

void add_time(void)
{
        onofftime[0] = onofftime[1];
        onofftime[1] = onofftime[2];
        onofftime[2] = minutes;
};

void SD16_init(void)
{
  timer_init();

  SD16CTL = SD16REFON + SD16SSEL_1 + SD16VMIDON;          // 1.2V ref, SMCLK
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
        
    case 1:    // inside fridge temperature
        temp -= 13653;
        temp /= 55;

        ChA1results = temp;             // Save CH1 results (clears IFG)
        ChA1results -= ChA7results;			// Subtracting delta
        ChA1results -= 240; //250;			// calibrate

        SD16AE &= ~SD16AE1;                 // Disable external input A4+, A4
        SD16INCTL0 &= ~SD16INCH_4;          // Disable channel A4+/-
        ch_counter++;
        
        SD16INCTL0 |= SD16INCH_6;           // Enable channel A6+/-                          
        break; 
        
    case 2:    // outside fridge temperature
        temp -= 39361;
        ChA6results = temp;				// Save CH6 results (clears IFG)        
        ChA6results *= 2; 
        ChA6results /= 29;
        ChA6results -= 5;              // some calibrating

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
    

// thermo on\off       

char on_temp, off_temp; 
// if outside temp > 25 switch to  summer mode
// if outside temp < 24 switch off summer mode. It is not good to
// turn on compressor immediately after it stops. That's why we avoid
// turning off summer mode (i.e. lowering limiting temperatures)
// when the comressor is off
    if((ChA6results > 250)) summer = TRUE;
    if((ChA6results < 240) && (P1OUT & relay)) summer = FALSE;

    if (summer)
    {
        on_temp  = 80;
        off_temp = 50;
    } else
    {
        on_temp  = 60;
        off_temp = 30;
    };

    if ((ChA1results > on_temp) &&  // верхний порог и еще не включен
        (!(P1OUT & relay)) &&       // и с первого пуска прошло 2 минуты
        (minutes > 1))		 
    {                          // on
        add_time();
        P1OUT |= relay;
	};
    
	if((ChA1results < off_temp) &&
       (P1OUT & relay) &&
       (minutes - onofftime[2] > 5))		// нижний порог и включен
    {                          // off
        add_time();
        P1OUT &= ~relay;
	};
    
// show on screen

    if(show_info) // display previous on\off times
    {
        if(P1OUT & relay) // if on
        {
            temp = onofftime[1]-onofftime[0];
            PrintInt(0, temp);
            PrintStr("   no");
            temp = onofftime[2]-onofftime[1];
            PrintInt(1, temp);
            PrintStr("  ffo");
        } else           // if off
        {
            temp = onofftime[2]-onofftime[1];
            PrintInt(0, temp);
            PrintStr("   no");
            temp = onofftime[1]-onofftime[0];
            PrintInt(1, temp);
            PrintStr("  ffo");
        };       
    } else  // normal display mode of temperatures
    {
        PrintFloat(1, ChA1results);
        PrintFloat(0, ChA6results);
        
        MoveCursor(0, 0);
        if (summer) SendByte(0xee, TRUE); // draw 'sun'
            else SendByte(0x2a, TRUE);  // draw *
            
        MoveCursor(1, 0);
        if(ChA1results > on_temp)
            SendByte(0xda, TRUE); // arrow down
        if(ChA1results < off_temp)                   
            SendByte(0xd9, TRUE); // arrow up
            
    };
    
}



