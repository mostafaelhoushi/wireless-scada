/* ************************ ADC.C *****************************
 * Jonathan W. Valvano   12/12/02
 * Modified by EE345L students Charlie Gough && Matt Hawk
 * Modified by EE345M students Agustinus Darmawan + Mingjie Qiu
 * Modified by Mostafa Elhoushi
 * Simple routines to read from analog input. 
 * ************************************************************ */


#include <hidef.h>      /* common defines and macros */
#include <mc68hc812a4.h>     /* derivative information */
#include "ADC.h"
#pragma LINK_INFO DERIVATIVE "mc68hc812a4"


//-----------------------------------------------------------------------
// ADC_In()
// perform 4 ADC samples on given channel number and return average 4 
// results. 
// analog input    return value
//  0.00            0
//  1.25            64
//  2.50            128
//  5.00            255
//-----------------------------------------------------------------------
unsigned char ADC_In(unsigned int chan){ 
  ATDCTL5 = chan;                 // start sequence
  while((ATDSTAT&0x8000)==0){};   // wait for SCF 
  return (ADR0H+ADR1H+ADR2H+ADR3H)/4; 
}

//-----------------------------------------------------------------------
// ADC_Init()
// Initialize ADC
//-----------------------------------------------------------------------
void ADC_Init(void){
  ATDCTL2 = 0x80; // enable ADC
}
