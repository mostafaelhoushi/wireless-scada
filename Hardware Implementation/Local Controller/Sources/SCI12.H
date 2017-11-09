/* ************************ SCI12.H *****************************
 * Jonathan W. Valvano     12/12/02
 * Modified from EE345L students Charlie Gough && Matt Hawk
 * Modified by EE345M students Agustinus Darmawan + Mingjie Qiu
 * Simple I/O routines to SCI0 serial port 
 * ************************************************************ */
 
// Bits of Registers----------------------------------------------
// Bits of SCI 0 Status Register 1 (SC0SR1)
#define RDRF 0x20   // Receive Data Register Full Bit
#define TDRE 0x80   // Transmit Data Register Empty Bit

// Bits of SCI 0 Control Register 2 (SC0CR2)
#define TIE 0x80    // Transmitter Interrupt Enable Bit



// ECHO Flag
#define ECHO 0

/* ASCII symbols 
     CR = carriage return 
     LF = line feed 
     BS = backspace 
     SP = space 
 */

#define CR   0x0D
#define LF   0x0A
#define BS   0x08
#define ESC  0x1B
#define SP   0x20       
#define DEL  0x7F

// ANSI macro   
#define dESC()         OutChar(ESC),OutChar('[')
#define dClearScreen() dESC(),OutChar('2'),OutChar('J')
 
//------------Initialization-------------------------------------
// BaudRate=500000/br
// br = 52 means 9200 bits/sec
// br = 13 means 38400 bits/sec
// br = 1  means 500000 bits/sec
void SCI_Init(unsigned short br);  
 
//--------------Input/Receive from serial port-------------------
char SCI_InChar(void);              // Reads in a character, gadfly 
void SCI_InString(char *, unsigned int); // Reads in a String of max length
void SCI_InString1(char *, char first_char, unsigned int); // Reads in a String of max length with first character already read
unsigned short SCI_InUDec(void);    // Reads in an Unsigned 16 bit Decimal number
short SCI_InSDec(void);             // Reads in a Signed 16 bit Decimal number
unsigned short SCI_InUHex(void);    // Reads in an Unsigned 16 bit Hexadecimal number
unsigned char SCI_InStatus(void);   // Returns true if a call to InChar will return right away

//--------------Output/Transmit to serial port-------------------
unsigned char SCI_OutStatus(void);  // Returns true if a call to OutChar will return right away
void SCI_OutChar(char);             // Output an 8-bit character, gadfly 
void SCI_OutUDec(unsigned short);   // Output as 16-bit unsigned decimal 
void SCI_OutSDec(short);            // Output as 16-bit signed decimal 
void SCI_OutString(char *);         // Output a string, null terminated 
void SCI_OutUHex(unsigned short);   // Outputs an Unsigned 16 bit Hexadecimal number
void SCI_Echo(char);                // Echoes the character received

//----------------Utilities---------------------------------------
char SCI_UpCase(char);                  // Converts lower case character to upper case
void SCI_upCaseString(char *inString);  // Converts string to upper case

//----------------Interrupt Service Routines-----------------------
void SCI_Handler ();
void SCI_InterruptRx(void);