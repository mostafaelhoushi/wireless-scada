#include <mc68hc812a4.h>     /* derivative information */

// LCD Commands defInition
#define Code_Instruction   0x00  //RW=0,RS=0,E=0 (to send an instruction)
#define Char_Code     0x01  //RW=0,RS=1,E=0 (to send a character)
#define Enable             0x04  //bit E=1
#define Data_Length        0x30  //data length=8 bits
#define Display_Lines      0x38  //display on 2 lines
#define Disp_Curs_ON_OFF   0x0c  //lights the display on and set the cursor invisible
#define Clear_Display      0x01  //erases the display and set AC at 0
#define Entry_Mode         0x06  //AC increased when sending a character
#define Display_Left       0x18  //moves a character display to the left
#define Display_Right      0x1C  //moves a character display to the right
#define Line_Jump         0xc0  //goes to the beginning of line 2

// Address DefInitions-----------------------------------------------

// Lcd ports addresses
#define Port_LCD_Data          PORTA // LCD display data port
#define Port_LCD_Control       PORTB // LCD display control port
#define DDR_LCD_Data           DDRA  // LCD display data DDR
#define DDR_LCD_Control        DDRB  // LCD display control DDR



// LCD functions
void LCD_Init(void);
void send_code(const unsigned char CodeToSend,const unsigned char CodCtrl);
void display_string(const char* ch);