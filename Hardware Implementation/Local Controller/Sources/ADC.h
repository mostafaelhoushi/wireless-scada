/* ************************ ADC.h *****************************
 * Jonathan W. Valvano   12/12/02
 * Modified by EE345L students Charlie Gough && Matt Hawk
 * Modified by EE345M students Agustinus Darmawan + Mingjie Qiu
 * Modified by Mostafa Elhoushi
 * Simple routines to read from analog input. 
 * ************************************************************ */


unsigned char ADC_In(unsigned int chan) ; // returns average value of analog input
void ADC_Init(void); // initializes ADC
