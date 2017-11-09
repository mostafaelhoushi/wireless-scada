/* ************************ Events.c *****************************
 * Mostafa Elhoushi   10/06/07
 * Handles the events.
 * ************************************************************ */
#include <hidef.h>      /* common defines and macros */
#include <mc68hc812a4.h>     /* derivative information */
#include "SCI12.h"
#include "ADC.h"
#include "SPI12.h"
#include "Events.h"
#include "MAX529.h"

//********** SCI_OnRxChar ***********
void SCI_OnRxChar(unsigned char character) {
  unsigned char channel, value;
  channel = character;
  
  if (SCI_InChar()==CR) {
    value = SCI_InUDec();
    MAX529_Out(channel, value);
    MAX529_NOP();
  }
}


