#define LCM_DIR P1DIR
#define LCM_OUT P1OUT

#define RS  BIT2  // it's better not to change these
#define EN  BIT3
#define D4  BIT4
#define D5  BIT5
#define D6  BIT6
#define D7  BIT7
#define MASK    (RS + EN + D4 + D5 + D6 + D7)

#define FALSE 0
#define TRUE 1


/*  Global Variables  */
unsigned char overflows;

/*  Function Definitions  */
inline void LCM_init(void);
void pulse(void);
inline void clear(void);
void SendByte(char, char);
void MoveCursor(char, char);
void PrintStr(char *);


void pulse(void) {    
    LCM_OUT |= EN;
    __delay_cycles(200);
    
    LCM_OUT &= ~EN;
    __delay_cycles(200);
} // pulse


void SendByte(char ByteToSend, char IsData) {
    LCM_OUT &= ~MASK;
    LCM_OUT |= (ByteToSend & 0xF0);
    
    if (IsData == TRUE) LCM_OUT |= RS;
    else LCM_OUT &= ~RS;
    pulse();
    
    LCM_OUT &= ~MASK;
    LCM_OUT |= ((ByteToSend & 0x0F) << 4);
    
    if (IsData == TRUE) LCM_OUT |= RS;
    else LCM_OUT &= ~RS;
    pulse();
} // SendByte


inline void LCM_init(void) {
    LCM_DIR |= MASK;
    LCM_OUT &= ~MASK;
    
   __delay_cycles(100000);
   
   LCM_OUT |= D5;  // enter 4-bit mode
   pulse();
   
   SendByte(0x28, FALSE);
   SendByte(0x0C, FALSE);
   SendByte(0x06, FALSE);
} // LCM_init


inline void clear(void) {
    SendByte(0x01, FALSE);
    SendByte(0x02, FALSE);
    __delay_cycles(100000);
} // clear


void MoveCursor(char Row, char Col) {
    char address;
    if (Row == 0) address = 0;
    else address = 0x40;
    address |= Col;
    SendByte(0x80|address, FALSE);
    __delay_cycles(10000);
} // MoveCursor

void PrintStr(char *Text) {
    char *c;
    c = Text;
    while ((c != 0) && (*c != 0)) {
        SendByte(*c, TRUE);
        c++;
    }
} // PrintStr
