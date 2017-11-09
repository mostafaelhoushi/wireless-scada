/* ************************ SPI12.H *****************************
 * Simple routines to SPI0 serial port 
 * ************************************************************ */
 
 
//------------Initialization-------------------------------------
void SPI_Init(void);  
 
//--------------Output/Transmit to serial port-------------------
void SPI_Out8(unsigned char code);   // Output an 8-bit character, gadfly 
void SPI_Out16(unsigned char code1, unsigned char code2);  // Output 2 8-bit characters, gadfly 
