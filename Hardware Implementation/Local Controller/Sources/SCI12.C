/* ************************ SCI12.C *****************************
 * Jonathan W. Valvano   12/12/02
 * Modified by EE345L students Charlie Gough && Matt Hawk
 * Modified by EE345M students Agustinus Darmawan + Mingjie Qiu
 * Simple I/O routines to SCI0 serial port 
 * ************************************************************ */
#include <hidef.h>      /* common defines and macros */
#include <mc68hc812a4.h>     /* derivative information */
#include "SCI12.H"
#include "Events.h"

//-------------------------Start of SCI_Init------------------------
// Initialize Serial port SC0
// BaudRate=500000/br
// br = 52 means 9200 bits/sec
// br = 13 means 38400 bits/sec
// br = 1  means 500000 bits/sec
void SCI_Init(unsigned short br){
  SC0BDH = 0;   // br=MCLK/(16*BaudRate) 
  SC0BDL = br;  // assumes MCLK is 8 Mhz
  SC0CR1 = 0;
/* bit value meaning
    7   0    LOOPS, no looping, normal
    6   0    WOMS, normal high/low outputs
    5   0    RSRC, not appliable with LOOPS=0
    4   0    M, 1 start, 8 data, 1 stop
    3   0    WAKE, wake by idle (not applicable)
    2   0    ILT, short idle time (not applicable)
    1   0    PE, no parity
    0   0    PT, parity type (not applicable with PE=0) */ 
  SC0CR2 = 0x2C; 
/* bit value meaning
    7   0    TIE, no transmit interrupts on TDRE
    6   0    TCIE, no transmit interrupts on TC
    5   1    RIE, enable receive interrupts on RDRF
    4   0    ILIE, no interrupts on idle
    3   1    TE, enable transmitter
    2   1    RE, enable receiver
    1   0    RWU, no receiver wakeup
    0   0    SBK, no send break */ 
}
    
//-------------------------SCI_InChar------------------------
// Wait for new serial port input, return ASCII code for key typed
char SCI_InChar(void){
  while((SC0SR1 & RDRF) == 0){};
  return(SC0DRL);
}
        
//-------------------------SCI_OutChar------------------------
// Wait for buffer to be empty, output ASCII to serial port
void SCI_OutChar(char data){
  while((SC0SR1 & TDRE) == 0){};
  SC0DRL = data;
}

//-------------------------SCI_Echo------------------------
// Echoes a received character
void SCI_Echo(char data){
  if (ECHO) SCI_OutChar(data);
}

   
//-------------------------SCI_InStatus--------------------------
// Checks if new input is ready, TRUE if new input is ready
unsigned char SCI_InStatus(void){
  return(SC0SR1 & RDRF);}

//-----------------------SCI_OutStatus----------------------------
// Checks if output data buffer is empty, TRUE if empty
unsigned char SCI_OutStatus(void){
  return(SC0SR1 & TDRE);}

//-------------------------SCI_OutString------------------------
// Output String (NULL termination)
void SCI_OutString(char *pt){ char letter;
  while(letter=*pt++){
    SCI_OutChar(letter);
  }
}

//--------------------SCI_UpCase-------------------------------
// converts lowercase to uppercase
// char by subtracting	$20 from lowercase ASCII to	make uppercase ASCII
char SCI_UpCase(char character){	
  return ((character>='a') && (character<='z'))?character-0x20:character;}

//----------------------SCI_InUDec-------------------------------
// InUDec accepts ASCII input in unsigned decimal format
//     and converts to a 16 bit unsigned number
//     with a maximum value of 65535
// If you enter a number above 65535, it will truncate without reporting the error
// Backspace will remove last digit typed
unsigned short SCI_InUDec(void){	
unsigned short number=0, length=0;
unsigned char character;	
  while((character=SCI_InChar())!=CR){ // accepts until carriage return input
// The next line checks that the input is a digit, 0-9.
// If the character is not 0-9, it is ignored and not echoed
    if((character>='0') && (character<='9')) {
      number = 10*number+(character-'0');   // this line overflows if above 65535
      length++;
      SCI_Echo(character);
    } 
// If the input is a backspace, then the return number is
// changed and a backspace is outputted to the screen
    else if((character==BS) && length){
      number /= 10;
      length--;
      SCI_Echo(character);
    }
  }
  return number;
}

//----------------------------SCI_InSDec-----------------------------
// InSDec accepts ASCII input in signed decimal format
//    and converts to a signed 16 bit number 
//    with an absolute value up to 32767
// If you enter a number above 32767 or below -32767, 
//    it will truncate without reporting the error
// Backspace will remove last digit typed
short SCI_InSDec(void){	
short number=0, sign=1;	// sign flag 1=positive 0=negative
unsigned int length=0;
unsigned char character;
  while ((character=SCI_InChar())!=CR){ // Check for carriage return
    if(!length) {			        // + or - only valid as first char			
      if(character=='-'){
        sign = -1;
        length++;
        SCI_Echo('-');	// if - inputted, sign is negative
      }
      else if(character=='+'){
        length++;
        SCI_Echo('+');	//if + inputted, sign is positive
      }
    }
// The next line checks that the input is a digit, 0-9
// If the character is not 0-9, it is ignored and not echoed
    if((character>='0') && (character<='9')){
      number = number*10+character-'0';   // this line overflows if above 32767
      length++;
      SCI_Echo(character);
    }
// If the input is a backspace, then the return number is changed and a backspace
// is outputted to the screen.  If the backspace erases a minus, then sign is 
// reset to positive
    else if((character==BS) && length){
      number /=10;
      length--;
      if(!length){
        sign = 1;
      }
      SCI_Echo(BS);
    }
  }
  return sign*number;
}

//-----------------------SCI_OutUDec-----------------------
// Output a 16 bit number in unsigned decimal format
// Variable format 1-5 digits with no space before or after
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string 
void SCI_OutUDec(unsigned short n){
  if(n >= 10){
    SCI_OutUDec(n/10);
    n=n%10;
  }
  SCI_OutChar(n+'0'); /* n is between 0 and 9 */
}

//----------------------SCI_OutSDec---------------------------------------
// Output a 16 bit number in signed decimal format
// Variable format (optional sign)1 to 5 digits with no space before or after
// This function checks if the input parameter is negative,  
// If the number is negative, then 
//    1) it outputs a "-", 
//    2) negates the number and 
//    3) outputs it with OutUDec.
// Otherwise, it just calls OutUDec (i.e., no "+" sign)
void SCI_OutSDec(short number){
  if(number<0){	
    number = -number;
    SCI_OutChar('-');
  }
  SCI_OutUDec(number);
}

//---------------------SCI_InUHex----------------------------------------
// InUHex accepts ASCII input in unsigned hexadecimal (base 16) format
// No '$' or '0x' need be entered, just the 1 to 4 hex digits
// It will convert lower case a-f to uppercase A-F
//     and converts to a 16 bit unsigned number
//     with a maximum value of FFFF
// If you enter a number above FFFF, it will truncate without reporting the error
// Backspace will remove last digit typed
unsigned short SCI_InUHex(void){	
unsigned short number=0, digit, length=0;
unsigned char character;
  while((character=SCI_UpCase(SCI_InChar()))!=CR){	
    digit = 0x10; // assume bad
    if((character>='0') && (character<='9')){
      digit = character-'0';
    }
    else if((character>='A') && (character<='F')){ 
      digit = (character-'A')+0xA;
    }
// If the character is not 0-9 or A-F, it is ignored and not echoed
    if(digit<=0xF ){	
      number = number*0x10+digit;
      length++;
      SCI_Echo(character);
    }
// Backspace outputted and return value changed if a backspace is inputted
    else if(character==BS && length){
      number /=0x10;
      length--;
      SCI_Echo(character);
    }
  }
  return number;
}

//--------------------------SCI_OutUHex----------------------------
// Output a 16 bit number in unsigned hexadecimal format
// Variable format 1 to 4 digits with no space before or after
// This function uses recursion to convert the number of 
//   unspecified length as an ASCII string
void SCI_OutUHex(unsigned short number){
  if(number>=0x10)	{
    SCI_OutUHex(number/0x10);
    SCI_OutUHex(number%0x10);
  }
  else if(number<0xA){
    SCI_OutChar(number+'0');
  }
  else{
    SCI_OutChar((number-0x0A)+'A');
  }
}

//------------------------SCI_InString------------------------
// This function accepts ASCII characters from the serial port
//    and adds them to a string until a carriage return is inputted 
//    or until max length of the string is reached.  
// It echoes each character as it is inputted.  
// InString terminates the string with a null character
// -- Modified by Agustinus Darmawan + Mingjie Qiu --
void SCI_InString(char *string, unsigned int max) {	
unsigned int length=0;
unsigned char character;
  while((character=SCI_InChar())!=CR){
    if(length<max){
      *string++=character;
      length++; 
      SCI_Echo(character);
    }
  }
  *string = 0;
}

//------------------------SCI_InString1------------------------
// It is the same as SCI_InString1 but assumes that the first
// character of the string has already received been received.
void SCI_InString1(char *string, unsigned char first_char, unsigned int max) {	
unsigned int length=0;
unsigned char character;
*string++=first_char;
  while((character=SCI_InChar())!=CR){
    if(length<max){
      *string++=character;
      length++; 
      SCI_Echo(character);
    }
  }
  *string = 0;
}

//------------------------SCI_upCaseString------------------------
// converts a NULL terminated string to uppercase
void SCI_upCaseString(char *inString){
  char *pt = inString;
// 'a' = 0x61 and 'A' = 0x41, so their difference is 0x20 
  while(*pt){  //  NULL => done 
    if((*pt >= 'a') && (*pt <= 'z'))
      *pt -= 0x20;
    pt++;
  }
}


//-----------------------SCI_InterruptRx----------------------------
// services the receive interrupt of SCI peripheral.
void SCI_InterruptRx(void){
  unsigned char Data;                           /* Temporary variable for data */
  
  Data = SC0DRL;                       /* Read data from the receiver */
  
   
  SCI_OnRxChar(Data);                  /* Invoke user event */
    
}


//------------------------SCI_Handler------------------------------
// handles the SCI interrupt.
interrupt 0x14 void SCI_Handler () {
  char StatReg = SC0SR1;

  if (StatReg & RDRF) {                 /* Is the receiver interrupt flag set? */
    SCI_InterruptRx();                 /* If yes, then invoke the internal service routine. This routine is inlined. */
  }
  
}




