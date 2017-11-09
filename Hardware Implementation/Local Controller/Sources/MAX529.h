/* ************************ MAX529.H *****************************
 * Simple routines for MAX529 DAC.
 * ************************************************************ */
 
 
//------------Initialization-------------------------------------
void MAX529_Init(void);  
 
//--------------Output/Transmit to DAC-------------------
void MAX529_Out(unsigned char channelNo, unsigned char value);   // Output analog value to required channelNo, gadfly 


void MAX529_NOP(void);  // NOP: to be called after outputting all values