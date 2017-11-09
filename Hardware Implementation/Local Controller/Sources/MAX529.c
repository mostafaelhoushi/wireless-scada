// **********MAX529.c************************************
// Sends data to DAC MAX529 using SPI.

#include <mc68hc812a4.h>     /* derivative information */
#include "SPI12.h"
#include "MAX529.h"
#pragma LINK_INFO DERIVATIVE "mc68hc812a4"

//******** MAX529_Init *************** 
// Initialize MAX529 interface
void MAX529_Init(void){  
  SPI_Out16(0x00,0xFF);  // Set all channels to full buffer mode
  
}



//******** MAX529_Out *************** 
// Set required analog voltage to required channel.
void MAX529_Out(unsigned char channelNo , unsigned char value){ 
  SPI_Out16(channelNo,value);
}


//********* MAX529_NOP ***************
// No Operation
void MAX529_NOP() {
  SPI_Out16(0x00,0x00);  // NOP
}
