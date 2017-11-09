//**********************************************************************
//  Local Controller    Author: Wireless Scada Team
//  
//  The main function of this module is to interpret commands received
//  via the wireless network which include reading output values and
//  configuring control and data acquisition.
//**********************************************************************

#include <hidef.h>      /* common defines and macros */
#include <mc68hc812a4.h>     /* derivative information */
#include <string.h>
#include "ADC.h"
#include "EEPROM.h"
#include "Events.h"
#include "LCD.h"
#include "MAX529.h"
#include "SCI12.h"
#include "SPI12.h"
#pragma LINK_INFO DERIVATIVE "mc68hc812a4"

// Type DefInitions------------------------------------------------------
typedef unsigned char bit;


// DefInitions-----------------------------------------------------------
#define TRUE 1
#define FALSE 0

#define ON 1
#define OFF 0

#define IDLE 0
#define OPEN_LOOP 1
#define CLOSE_LOOP 2

#define INPUT 0
#define OUTPUT 1

#define ZERO_TO_FIVE_VOLT 0
#define ZERO_TO_TEN_VOLT 1
#define FOUR_TO_TWENTY_MA 2

#define DIGITAL_OUTPUTS        PORTC // Digital output port
#define DIGITAL_OUTPUTS_DDR    DDRC  // Digital output port DDR
#define DIGITAL_INPUTS         PORTD // Digital input port
#define DIGITAL_INPUTS_DDR     DDRD  // Digital input port DDR

#define ANALOG_NUMBER_MASK     0x07
#define ANALOG_DIRECTION_MASK  0x80
#define ANALOG_MODE_MASK       0x30
#define ANALOG_OVERWRITE_FLAG  0x40
#define ANALOG_RANGE_MASK      0x18

#define DIGITAL_NUMBER_MASK     0x07
#define DIGITAL_VALUE_MASK      0x08
#define DIGITAL_DIRECTION_MASK  0x10
#define DIGITAL_MODE_MASK       0x60
#define DIGITAL_OVERWRITE_FLAG  0x80



// Global Structures------------------------------------------------------
struct analog_channel {
  unsigned char value;
  unsigned char status;
  unsigned char AOverwriteValue;
};

struct digital_channel {
  unsigned char status;
  unsigned char DOverwriteValue;
};

struct PID_controller {
  float Kp, Ki ,Kd;
  unsigned char N;
  float SP1;
  float e_prev, e_sum, c_prev, c1_prev, m_diff;
  struct analog_channel *input_channel, *output_channel ;
  bit mode;
};

struct OnOff_controller {
  float SP2;
  float NZ;
  struct analog_channel *input_channel;
  struct digital_channel *output_channel;
  bit mode;
};


// Global Variables------------------------------------------------------
struct analog_channel analog_input_channels[8], analog_output_channels[8];
struct digital_channel digital_input_channels[8], digital_output_channels[8];
struct PID_controller PID_controllers[8];
struct OnOff_controller OnOff_controllers[8];

unsigned int Ts; // sampling time in ms
unsigned int counter; // counts number of 4us

//EEPROM base addresses
// Digital outputs
unsigned char DOverwrite_add;
unsigned char DOutput_add;

// Analog outputs
unsigned char AOverwrite_add;
unsigned char AOutput_base;

// Analog PID
unsigned char PID_SP_base;
unsigned char PID_Kp_base;
unsigned char PID_Ki_base;
unsigned char PID_Kd_base;
unsigned char PIDAct_base;

// Analog On/Off 
unsigned char OnOff_SP_base;	
unsigned char NZ_base;	
unsigned char OnOffAct_base; 


// Function Prototypes---------------------------------------------------
//Initialization functions
void Init();
void TOF_Init();
void ports_Init();
void misc_Init();
void channels_Init();
void PID_Init();
void OnOff_Init();

// Analog I/O functions
void a_channel_read(struct analog_channel *input_channel);
void a_channel_write(struct analog_channel *output_channel, unsigned char value);
void a_overwrite(struct analog_channel *output_channel, unsigned char value);

// Digital I/O functions
void d_channel_read(struct digital_channel *input_channel);
void d_channel_write(struct digital_channel *output_channel, bit value);
void d_overwrite(struct digital_channel *output_channel, bit value);

// Auxillary functions
void PID(struct PID_controller *PID_module);
void OnOff(struct OnOff_controller *OnOff_module);
float get_c(unsigned char channelNo);
float sat(float number);


// Function defInitions--------------------------------------------------

//-----------------------------------------------------------------------
// main()
// main function.
//-----------------------------------------------------------------------
void main(void) {

  EnableInterrupts;
  
  Init();
  
  while(1); // wait forever 
  
}

//-----------------------------------------------------------------------
// TOFhandler()
// Invoked every fixed time period to perform control action and send 
// values to the network.
//-----------------------------------------------------------------------
interrupt 0x10 void TOFhandler(void){ 
  unsigned char i;
    
  TFLG2 = 0x80;           // TOF interrupt acknowledge
  COPRST = 0x55;          // make COP happy 
  COPRST = 0xAA;
  
  counter++;
  if (counter == (Ts * 250)) {
    counter = 0;
    // Write code here
    for (i=0; i<8; i++) {
      a_channel_read(&analog_input_channels[i]);
      d_channel_read(&digital_input_channels[i]);
    }
    for (i=0; i<8; i++) {
    
      PID(&PID_controllers[i]);
      OnOff(&OnOff_controllers[i]);
    }
  }
  
}

//-----------------------------------------------------------------------
// Init()
// Initializes the hardware modules in the microcontroller and any 
// software values and structures.
//-----------------------------------------------------------------------
void Init(void) {
  LCD_Init();
  ports_Init();
  channels_Init();
  misc_Init();
  PID_Init();
  OnOff_Init();
  TOF_Init();
  SPI_Init();
  SCI_Init(13);
  ADC_Init();
}

//-----------------------------------------------------------------------
// ports_Init()
// Initializes any I/O pins or ports for the microcontroller.
//-----------------------------------------------------------------------
void ports_Init(void) {
  // Write code here
  PORTT |= 0x40;    // Turn LED On
  DIGITAL_OUTPUTS_DDR = 0xFF;
  DIGITAL_OUTPUTS = 0x00;
  DIGITAL_INPUTS_DDR = 0x00;
}


//-----------------------------------------------------------------------
// channels_Init()
// Initializes all the analog and digital channels.
//-----------------------------------------------------------------------
void channels_Init(void) {
  int i = 0;
  
  for (i=0; i<8; i++) {
    analog_input_channels[i].status &= ~ANALOG_OVERWRITE_FLAG;   // set Analog Overwrite Flag to FALSE
    analog_input_channels[i].AOverwriteValue = 0;
    analog_input_channels[i].status &= ~ANALOG_DIRECTION_MASK;   // set Analog Direction to INPUT
    analog_input_channels[i].status &= ~ANALOG_MODE_MASK;        // set Analog Mode to IDLE
    analog_input_channels[i].status |= i;
    analog_input_channels[i].status &= ~ANALOG_RANGE_MASK;       // set Analog Range to ZERO_TO_FIVE_VOLT
    analog_input_channels[i].value = 0;
    
    analog_output_channels[i].status &= ~ANALOG_OVERWRITE_FLAG;   // set Analog Overwrite Flag to FALSE
    analog_output_channels[i].AOverwriteValue = 0;
    analog_output_channels[i].status |= ANALOG_DIRECTION_MASK;   // set Analog Direction to OUTPUT
    analog_output_channels[i].status &= ~ANALOG_MODE_MASK;        // set Analog Mode to IDLE
    analog_output_channels[i].status |= i;
    analog_output_channels[i].status &= ~ANALOG_RANGE_MASK;       // set Analog Range to ZERO_TO_FIVE_VOLT
    analog_output_channels[i].value = 0;
    
    digital_input_channels[i].status &= ~DIGITAL_OVERWRITE_FLAG ;  // set Digital Overwrite Flag to FALSE
    digital_input_channels[i].DOverwriteValue = 0;
    digital_input_channels[i].status &= ~DIGITAL_DIRECTION_MASK;   // set Digital Direction to INPUT
    digital_input_channels[i].status &=~DIGITAL_MODE_MASK;         // set Digital Mode to OFF
    digital_input_channels[i].status |= i;                         // set Digital Channel Number to i
    digital_input_channels[i].status &= ~DIGITAL_VALUE_MASK;       // set Digital Value to 0
    
    digital_output_channels[i].status &= ~DIGITAL_OVERWRITE_FLAG ;  // set Digital Overwrite Flag to FALSE
    digital_output_channels[i].DOverwriteValue = 0;
    digital_output_channels[i].status |= DIGITAL_DIRECTION_MASK;   // set Digital Direction to OUTPUT
    digital_output_channels[i].status &=~DIGITAL_MODE_MASK;         // set Digital Mode to OFF
    digital_output_channels[i].status |= i;                         // set Digital Channel Number to i
    digital_output_channels[i].status &= ~DIGITAL_VALUE_MASK;       // set Digital Value to 0
  }
}

//-----------------------------------------------------------------------
// misc_Init()
// Modifies the initalized values if they were modified in EEPROM.
//-----------------------------------------------------------------------
void misc_Init(void) {
  unsigned char i, mask, DOverwriteFlags, DOverwriteValues, AOverwriteFlags;
	DOverwriteFlags		= ee_read(DOverwrite_add);
	DOverwriteValues	= ee_read(DOutput_add);
	
	for (i=0, mask=1;i<8;i++,mask<<=1) {
	  if (DOverwriteFlags & mask == 1) digital_output_channels[i].status |= DIGITAL_OVERWRITE_FLAG;
	  else digital_output_channels[i].status &= ~DIGITAL_OVERWRITE_FLAG;
	  
	  if (DOverwriteValues & mask == 1) digital_output_channels[i].DOverwriteValue = 1;
	  else digital_output_channels[i].DOverwriteValue = 0;
	}
	  
	
	AOverwriteFlags 	= ee_read(AOverwrite_add);
	
	for(i=0;i<8;i++)
	{
		analog_output_channels[i].value = ee_read(AOutput_base + i);;
	}	
}

//-----------------------------------------------------------------------
// PID_Init()
// Initializes PID structures.
//-----------------------------------------------------------------------
void PID_Init(void) {
  unsigned char i;
  
  for (i=0; i<8; i++) {
    PID_controllers[i].c1_prev = 0;
    PID_controllers[i].c_prev = 0;
    PID_controllers[i].e_prev = 0;
    PID_controllers[i].e_sum = 0;
    
    PID_controllers[i].mode = OFF;
  }
}

//-----------------------------------------------------------------------
// OnOff_Init()
// Initializes On/Off structures.
//-----------------------------------------------------------------------
void OnOff_Init(void) {
  unsigned char i;
  
  for (i=0; i<8; i++) {
    OnOff_controllers[i].mode = OFF;
  }
}


//-----------------------------------------------------------------------
// TOF_Init()
// Initialize timer interrupts.
//-----------------------------------------------------------------------
void TOF_Init(void){
  asm(" sei");   // Make ritual atomic 
  TSCR = 0x80;   // TEN(enable)
  TMSK2 = 0xA5;  // TOI arm, TPU(pullup) timer/32 (4us)
  CLKCTL = 0x00;
  
  asm(" cli");
}


//------------------------------------------------------------------------
// a_channel_read()
// Assigns the value member of the analog_channel structure the value
// read from the ADC module.
//------------------------------------------------------------------------
void a_channel_read(struct analog_channel *input_channel) {
  input_channel->value = ADC_In(input_channel->status & ANALOG_NUMBER_MASK);
}

//------------------------------------------------------------------------
// a_channel_write()
// Assigns the value given to the DAC output as well as the channel's
// structure,
//------------------------------------------------------------------------
void a_channel_write(struct analog_channel *output_channel, unsigned char value) {
  MAX529_Out(output_channel->status & ANALOG_NUMBER_MASK, value);
  MAX529_NOP();
  
  output_channel->value = value;
}

//------------------------------------------------------------------------
// a_overwrite()
// Forces a value to a certain analog output.
//------------------------------------------------------------------------
void a_overwrite(struct analog_channel *output_channel, unsigned char value) {
  a_channel_write(output_channel, value);
}

//------------------------------------------------------------------------
// d_channel_read()
// Assigns the value member of the digital_channel structure the value
// read from the port register.
//------------------------------------------------------------------------
void d_channel_read(struct digital_channel *input_channel){
  char mask, number = (input_channel->status) & DIGITAL_NUMBER_MASK;
  
  switch (number) {
    case 0: mask = 1; break;
    case 1: mask = 2; break;
    case 2: mask = 4; break;
    case 3: mask = 8; break;
    case 4: mask = 16; break;
    case 5: mask = 32; break;
    case 6: mask = 64; break;
    case 7: mask = 128; break;
  }
  
  input_channel->status |= ((DIGITAL_INPUTS & mask)/mask)<<3;
}

//------------------------------------------------------------------------
// d_channel_write()
// Assigns the value given to the digital port output as well as the channel's
// structure,
//------------------------------------------------------------------------
void d_channel_write(struct digital_channel *output_channel, bit value) {
  char mask, number = (output_channel->status) & DIGITAL_NUMBER_MASK;
  
  switch (number) {
    case 0: mask = 1; break;
    case 1: mask = 2; break;
    case 2: mask = 4; break;
    case 3: mask = 8; break;
    case 4: mask = 16; break;
    case 5: mask = 32; break;
    case 6: mask = 64; break;
    case 7: mask = 128; break;
  }
  
  output_channel->status |= (((DIGITAL_OUTPUTS & mask)/mask)<<3 & value);
}


//------------------------------------------------------------------------
// d_overwrite()
// Forces a value to a certain digital output.
//------------------------------------------------------------------------
void d_overwrite(struct digital_channel *output_channel, bit value) {
  d_channel_write(output_channel, value); 
}


//------------------------------------------------------------------------
// get_c()
// Converts the value obtained from the ADC from integer range 0-255
// to flaot range 0-5.
//------------------------------------------------------------------------
float get_c(unsigned char channelNo) {
  return (float)analog_output_channels[channelNo].value * (float)5/255;
}

//------------------------------------------------------------------------
// sat()
// Clips the value to be from 0-5.
//------------------------------------------------------------------------
float sat(float number) {
  if (number<0)  return 0;
  else if (number>5) return 5;
  else return number;
}

//------------------------------------------------------------------------
// PID()
// Performs the calculations for a PID module.
//------------------------------------------------------------------------
void PID(struct PID_controller *PID_module) {
  float e_mod, m_sat, c, c1, e, m;  
  if (PID_module->mode == OFF || PID_module->mode == OPEN_LOOP) return;
  
  
  c = get_c(PID_module->input_channel->value);
  e = PID_module->SP1 - c;
  e_mod = e + PID_module->m_diff;
  PID_module->e_sum = PID_module->e_sum + e_mod;
  c1 = PID_module->Kd * (e - PID_module->e_prev) - PID_module->Kd / PID_module->N * PID_module->c1_prev; 
  m = PID_module->Kp * (e + PID_module->Ki * PID_module->e_sum - c1);
  m_sat = sat(m);
  PID_module->m_diff = m_sat - m;
  
  PID_module->c_prev = c;
  PID_module->c1_prev = c1;
  
  a_channel_write(PID_module->output_channel, m_sat/5*255);
  
  }
  
//------------------------------------------------------------------------
// OnOff()
// Performs the calculations for an OnOff module.
//------------------------------------------------------------------------
void OnOff(struct OnOff_controller *OnOff_module) {
  float c;  
  if (OnOff_module->mode == OFF || OnOff_module->mode == OPEN_LOOP) return;
  
  c = get_c(OnOff_module->input_channel->value);
  
  if (c <= (OnOff_module->SP2 - OnOff_module->NZ)) d_channel_write(OnOff_module->output_channel, ON);
  if (c >= (OnOff_module->SP2 + OnOff_module->NZ)) d_channel_write(OnOff_module->output_channel, OFF);
  
  }
    