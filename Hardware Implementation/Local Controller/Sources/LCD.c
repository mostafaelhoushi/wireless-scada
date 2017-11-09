#include "LCD.h"

//-----------------------------------------------------------------------
// send_code()
// Sends a control code or a character to the LCD display 
//-----------------------------------------------------------------------
void send_code(const unsigned char CodeToSend,const unsigned char CodCtrl)
{
   Port_LCD_Control = (unsigned char)(CodCtrl+Enable); //sets 'enable' at 1
   Port_LCD_Data = CodeToSend;
   Port_LCD_Control = CodCtrl;                         //sets 'enable' at 0
}

//------------------------------------------------------------------------
// LCD_Init()
// Initialization of the LCD display.
//------------------------------------------------------------------------
void LCD_Init(void)
{
   DDR_LCD_Data = 0xFF;
   DDR_LCD_Control = 0xFF;
   
   send_code(Data_Length,Code_Instruction);     //sends 3 times Data_Length=8 bits
   send_code(Data_Length,Code_Instruction);
   send_code(Data_Length,Code_Instruction);
   send_code(Display_Lines,Code_Instruction);   //use 2 lines
   send_code(Disp_Curs_ON_OFF,Code_Instruction);//display on , cursor not displayed
   send_code(Clear_Display,Code_Instruction);   //erases the display and sets AC (address counter)at 0
   send_code(Entry_Mode,Code_Instruction);      //increases AC after character reception
}

//-------------------------------------------------------------------------
// display_string()
// Sends a string to the LCD display.
//-------------------------------------------------------------------------
void display_string(const char* ch)
{
   int i;
   unsigned int longueur=strlen(ch);            //string length
   for (i=0;i<longueur;i++) {
       if (ch[i]=='\n') {
           send_code(Line_Jump,Code_Instruction);
       } else {
           send_code((unsigned char)ch[i],Char_Code);
       }
   }
}