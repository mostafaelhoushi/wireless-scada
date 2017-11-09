/******************************************************************************
 *
 *                      (c) Freescale  Inc. 2004 All rights reserved
 *
 * File Name     : Rti.c
 * Description   : 
 *
 * Version : 1.0
 * Date    : Jun/21/2004
 *
 *
 ******************************************************************************/ 
#include "timers.h"
#include "MC9S12NE64.h"


/******************************************************************************
*
*
******************************************************************************/
void RTI_Init (void)
{
    CRGINT_RTIE = 0;                     /* Disable interrupt */
    RTICTL_RTR = 0x73;                   /* Store given value to the prescaler */

}

/******************************************************************************
*
*
******************************************************************************/
void RTI_Enable (void)
{
    CRGFLG = CRGINT_RTIE_MASK;         /* Reset interrupt request flag */
    CRGINT_RTIE = 1;                   /* Enable interrupt */
}


/******************************************************************************
*
*
******************************************************************************/
void RTI_Disable (void)
{
    CRGINT_RTIE = 0;                   /* Disable interrupt */
}


/******************************************************************************
*
*
******************************************************************************/
#pragma CODE_SEG NON_BANKED
interrupt void RealTimeInterrupt (void)
{			   
volatile static unsigned char state = 0;

    CRGFLG = CRGINT_RTIE_MASK;            /* Reset interrupt request flag */
    
    decrement_timers ();
    
    if (state = ~state)
    {
        //PTL_PTL0  = 0;			 //Turn on ACTLED
    }
    else
    {
        //PTL_PTL0  = 1;			 //Turn off ACTLED
    }
}
#pragma CODE_SEG DEFAULT