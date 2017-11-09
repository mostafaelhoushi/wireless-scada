// **********SPI.c************************************
// SPI implementation.

#include <mc68hc812a4.h>     /* derivative information */
#include "SPI12.h"
#pragma LINK_INFO DERIVATIVE "mc68hc812a4"

//******** SPI_Init *************** 
// Initialize SPI interface
// make  PS7,PS6,PS5 outputs, 
// SPI master data in, PS4, is not used
void SPI_Init(void){     // PS7=_CS=1
  DDRS = 0xE0;           // PS6=CLK=SPI clock out
  PORTS = 0x80;          // PS5=SRI=SPI master data out
/* bit SP0CR1
 7 SPIE = 0   no interrupts
 6 SPE  = 1   enable SPI
 5 SWOM = 0   regular outputs
 4 MSTR = 1   master
 3 CPOL = 1   output changes on +ve edge
 2 CPHA = 0   and input clocked in on rise
 1 SSOE = 0   PS7 regular output 
 0 LSBF = 0   most significant bit first */
  SP0CR1 = 0x58;
/* bit SP0CR2
 3 PUPS = 0   no internal pullups
 2 RDS  = 0   regular drive
 0 SPC0 = 0   normal mode */
  SP0CR2 = 0x00;
  SP0BR = 0x01; // 2 MHz
}
#define SPIF 0x80

//******** SPI_Out8 *************** 
// Output 8-bit data using SPI port
void SPI_Out8(unsigned char code){ 
  unsigned char dummy;
  PORTS &= ~0x80;            // PS7=_CS=0
  SP0DR = code;              // data
  while((SP0SR&SPIF)==0);    // gadfly wait for SPIF
  dummy = SP0DR;             // clear SPIF
  
  PORTS |= 0x80;             // PS7=_CS=1
  PORTS &= ~0x80;            // PS7=_CS=0
}

//******** SPI_Out16 *************** 
// Output 2 8-bit data using SPI port
void SPI_Out16(unsigned char code1, unsigned char code2){ 
  unsigned char dummy;
  PORTS &= ~0x80;            // PS7=_CS=0
  SP0DR = code1;             // 1st data
  while((SP0SR&SPIF)==0);    // gadfly wait for SPIF
  dummy = SP0DR;             // clear SPIF
  
  SP0DR = code2;             // 2nd data
  while((SP0SR&SPIF)==0);    // gadfly wait for SPIF
  dummy = SP0DR;             // clear SPIF
  
  PORTS |= 0x80;             // PS7=_CS=1
  PORTS &= ~0x80;            // PS7=_CS=0
}

