//////////////////////////////////////////////////////////////////////////////
//
// SofTec Microsystems AK-S12NE64-A Demo Board Sample
//
// ---------------------------------------------------------------------------
// Copyright (c) 2004 SofTec Microsystems
// http://www.softecmicro.com/
//
//////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include <string.h>

#include "debug.h"
#include "tcp_ip.h"
#include "MC9S12NE64.h"
#include "main.h"


// ---------------------------------------------------------------------------
// UDP/TCP protocol definitions
// ---------------------------------------------------------------------------
char * ptr_cmdbuf;
INT8   tcp_soch;								/* Socket handle	*/
INT8   udp_soch;
UINT32 udp_ipaddr;
UINT16 udp_port;
char   udp_tcp_buf[20];
char   udp_data_to_send;

#define UDP_AK_PORT                 5000
#define TCP_AK_PORT                 5000
#define GET_DATA()                  RECEIVE_NETWORK_B()

INT32 tcp_control_panel_eventlistener(INT8 , UINT8 , UINT32 , UINT32 );
void  tcp_control_panel_init(void);
INT32 udp_control_panel_eventlistener(INT8 cbhandle, UINT8 event, UINT32 ipaddr, UINT16 port, UINT16 buffindex, UINT16 datalen);
void  udp_control_panel_init(void);


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#define CH_POTENTIOMETER            0
#define CH_CURRENT_SENSOR           1
#define CH_NTC_SENSOR               2       

#define PUSH_BUTTONS                (PTG & 0x0F)
#define DIP_SWITCHES                ((PTG & 0xF0) | (PTH & 0x0F))
#define LEDS                        (PORTA)

#define MOT_FAILURE_SENS()          (!(PORTK & 0x10))

#define ERR_INVALID_CMD             "ERR:CMD\r\n"  
#define ERR_SYNTAX                  "ERR:SYNTAX\r\n" 
#define ERR_INVALID_DATA            "ERR:DATA\r\n"

unsigned char MotorSpeed;           // range: 0-100 
unsigned char MotorDirection;       // 0=SX, 1= DX
unsigned char MotorRunning;         // 0=OFF 1=RUN
unsigned char SpeakerFreq;	        // 1 - 50 (from 100Hz to 5KHz)
unsigned char SpeakerOn;            // 0=OFF 1=ON
unsigned char SpeakerCount;
// ---------------------------------------------------------------------------
// Initialization routine
// ---------------------------------------------------------------------------
void AK_HCS12NE_Init(void)
{

// Leds PA[7..0]
    PORTA = 0x00;                       // Sets Port A data to 0x00
    DDRA = 0xFF;                        // Sets Port A as output

// Push-Buttons PG[3..0] 
// Dip-Switches PG[7..4] - PH[3..0]
 
    PERG = 0xFF;                        // Enables pull-ups on Port G[3..0]
    PPSG = 0xF0;  					    // Enables pull-downs on Port G[7..4]

    PERH = 0x0F;                        // Enables pull-downs on Port H[3..0]
    PPSH = 0x0F;  
    
// A/D Converters (Potentiometer and NTC input)

    ATDCTL3 = 0x08;                     // One conversion per sequence
    ATDCTL4 |= 0x83;                    // Sets A/D conversion to 8 bit resolution
    ATDCTL2 = 0x80;                     // Normal ATD functionality

  
// MC33887 driver

    PORTK = 0x02;                   
    DDRK  = 0x07;
    
    PTT = 0x10;
    DDRT = 0x70;
    
// Timer configurated as output compare
    TIOS = 0x30 | 0x80;
    OC7M = 0x30;
    
    TTOV = 0x30;
    TCTL1 = 0x05;		
    TSCR2 = 0x09;   

    TC7 = 1250;     // 10Kz
    TC4 = 0xFFFF;
    TC5 = 0xFFFF;
    TCNT = 0;
    TSCR1 = 0x80;	  
   

init_serial_memories(); 
}

// ---------------------------------------------------------------------------
// TIM0 CH7 ISR
// DESCRIPTION:
// ---------------------------------------------------------------------------
#pragma CODE_SEG NON_BANKED        
interrupt void tim0_ch7_isr(void)
{

TFLG1 = 0x80;

if(++SpeakerCount >= SpeakerFreq)
    {
    PTT ^= 0x40;
    SpeakerCount = 0;
    }
}
#pragma CODE_SEG DEFAULT

// ---------------------------------------------------------------------------
// Adc Read routine (8-bit)
// ---------------------------------------------------------------------------
unsigned char AdcRead (unsigned char channel)
{
char n;
unsigned int average = 0;

for(n=0; n<8; n++)
    {
    ATDCTL5 = (channel & 0x07);              // selecting ADC the channel

    while (!(ATDSTAT1 & 0x01))              // wait until the end of conversion
        ;
    average +=  ATDDR0H;       
    }
return((unsigned char)(average/8));
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
const unsigned char lookup_NTC[] = {
0,
6,
11,
16,
21,
26,
32,
37,
43,
48,
54,
60,
66,
71,
77,
83,
89,
95,
100,
106,
112,
118,
124,
130,
136,
141,
147,
153,
158,
164,
169,
175,
180,
185,
191,
196,
201,
206,
211,
216,
221,
226,
230,
235,
240,
244,
249,
253,
255};

unsigned char ReadTemperature(void)
{
unsigned char i, adc;

adc = AdcRead (CH_NTC_SENSOR);
// NTC linearization
for(i=sizeof(lookup_NTC)/sizeof(unsigned char)-1; i>0; i--)
    {
    if(adc >= lookup_NTC[i])
        return(i);
    }
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void SetSpeakerParameters(void)
{
if(SpeakerOn)
    {
    SpeakerCount = 0;        
    TIE = 0x80;
    } 
else  
    {
    TIE = 0x00;
    } 
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void SetMotorParameters(void)
{
#define PWM_FULL_RANGE      1250
unsigned long tmr;
int i;

if(MotorRunning)
    {
    PORTK = 0x01;                   // MC33887: D1=0, nD2=1
    tmr = PWM_FULL_RANGE;
    tmr *= MotorSpeed;
    tmr /= 100;
    tmr = PWM_FULL_RANGE - tmr;
    if(MotorDirection)
        {
        if(TC4 == 0xFFFF)
            {
            while(TC5 < PWM_FULL_RANGE)
                {
               for(i=0; i<100; i++)
                    kick_WD();
                TC5++;
                }
            TC5 = 0xFFFF;
            TC4 = PWM_FULL_RANGE;
            while(TC4 > (unsigned int)tmr)
                {
                for(i=0; i<100; i++)
                    kick_WD();
                TC4--;
                }
            }
        TC4 = (unsigned int)tmr;
        TC5 = 0xFFFF;
        }
    else
        {
        if(TC5 == 0xFFFF)
            {
            while(TC4 < PWM_FULL_RANGE)
                {
                for(i=0; i<100; i++)
                    kick_WD();
                TC4++;
                }
            TC4 = 0xFFFF;
            TC5 = PWM_FULL_RANGE;
            while(TC5 > (unsigned int)tmr)
                {
                for(i=0; i<100; i++)
                    kick_WD();
                TC5--;
                }
            }
        TC5 = (unsigned int)tmr;
        TC4 = 0xFFFF;
        }
    } 
else
    PORTK = 0x02;                    // MC33887: D1=1, nD2=0
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void send_ack_data(void)
{
udp_tcp_buf[0] = '\r';      
udp_tcp_buf[1] = '\n';     
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void send_nack(char *buf)
{
(void)strcpy(udp_tcp_buf, buf);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void send_bool_data(unsigned char data)
{
if(data)
    udp_tcp_buf[0] = '1';
else
    udp_tcp_buf[0] = '0';
udp_tcp_buf[1] = '\r';       
udp_tcp_buf[2] = '\n';      
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
const unsigned int tab_dec[] = {     	  
    1,
	10,
	100,
	1000,
	10000,
  };
void send_dec_data(unsigned int val, char ndigit)
{
char tmp[14];
char c, i,len;

for(i=0,len=sizeof(tab_dec)/sizeof(unsigned int)-1; len>=0; len--,i++)
	{
	tmp[i] = '0';
	if(val >= tab_dec[len])
		{
		tmp[i] = (char)(val / tab_dec[len]);
		val   -= tmp[i] * tab_dec[len];
		tmp[i] += '0';
		}
	}
tmp[i] = '\0';
for(c=0, i-=ndigit; tmp[i]; i++,c++)
	udp_tcp_buf[c] = tmp[i];
udp_tcp_buf[c++] = '\r';       
udp_tcp_buf[c] = '\n';      
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
Bool get_8bit_data(unsigned char *data)
{
char c;

*data = 0;
for(;;)
    {
    if(*ptr_cmdbuf != ' ' && *ptr_cmdbuf != ',')
        break;
    ptr_cmdbuf++;
    }
for(;;)
	{
	c = *ptr_cmdbuf++;
	if (c >= '0' && c <= '9')
		c -= '0';
	else if(c == '\n' || c == '\r' || c == '\0' || c == ' ' || c == ',')
	    break;
	else
        {
    	send_nack(ERR_INVALID_DATA);
        return(FALSE);
        }
	*data *= 10;
	*data += c;
	}
return(TRUE);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
Bool get_bool_data(unsigned char *data)
{
if(!get_8bit_data(data))
    return(FALSE);
if(*data > 1)
    {
	send_nack(ERR_INVALID_DATA);
    return(FALSE);
    }
return(TRUE);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_sld(void)
{
unsigned char nbit, nval;

if(!get_8bit_data(&nbit))
    return;
if(!get_bool_data(&nval))
    return;
if(nval)
    LEDS |= (0x01 << nbit);
else
    LEDS &= ~(0x01 << nbit);
send_ack_data();
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_gld(void)
{
unsigned char nbit;

if(!get_8bit_data(&nbit))
    return;
send_bool_data(LEDS & (0x01 << nbit));
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_gsw(void)
{
unsigned char nbit;

if(!get_8bit_data(&nbit))
    return;
send_bool_data(DIP_SWITCHES & (0x01 << nbit));
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_gpt(void)
{
send_dec_data(AdcRead(CH_POTENTIOMETER), 3);
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_gpb(void)
{
unsigned char nbit;

if(!get_8bit_data(&nbit))
    return;
send_bool_data(PUSH_BUTTONS & (0x01 << nbit));
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_gtp(void)
{
send_dec_data(ReadTemperature(), 2);
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_gms(void)
{
send_dec_data(MotorSpeed, 3);
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_sms(void)
{
if(!get_8bit_data(&MotorSpeed))
    return;
SetMotorParameters();
send_ack_data();
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_gmd(void)
{
send_bool_data(MotorDirection & 0x01);
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_smd(void)
{
if(!get_bool_data(&MotorDirection))
    return;
SetMotorParameters();
send_ack_data();
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_gme(void)
{
send_bool_data(MotorRunning & 0x01);
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_sme(void)
{
if(!get_bool_data(&MotorRunning))
    return;
SetMotorParameters();
send_ack_data();
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_gmc(void)
{
unsigned long value;

value = (unsigned long)AdcRead(CH_CURRENT_SENSOR);   
value *= 3300;      // 3300 mV 
value /= 255;       
value *= 375;       // K
value /= 1000;      // Rx
send_dec_data((unsigned int)value, 4);
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_gmf(void)
{
send_bool_data(MOT_FAILURE_SENS() && MotorRunning);
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_gso(void)
{
send_bool_data(SpeakerOn & 0x01);
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_sso(void)
{
if(!get_bool_data(&SpeakerOn))
    return;
SetSpeakerParameters();
send_ack_data();
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_gsf(void)
{
send_dec_data(50-SpeakerFreq+1, 3);
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_ssf(void)
{
unsigned char val;

if(!get_8bit_data(&val))
    return;
if(val > 50)
    val = 50;
if(val < 1)
    val = 1;
SpeakerFreq = 50-val+1;
SetSpeakerParameters();
send_ack_data();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
unsigned char flash_data;
void flash_callback_read(unsigned long addr, unsigned char data)
{
addr++;	 // to avoid the compiler warning
flash_data = data;
}
// ---------------------------------------------------------------------------
void cmd_rsf(void)
{
unsigned char addr;

if(!get_8bit_data(&addr))
    return;
flash_read(addr, 1, flash_callback_read);
send_dec_data(flash_data, 3);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
unsigned char flash_callback_write(unsigned long addr)
{
addr++;	 // to avoid the compiler warning
return(flash_data);
}
// ---------------------------------------------------------------------------
void cmd_wsf(void)
{
unsigned char addr;

if(!get_8bit_data(&addr))
    return;
if(!get_8bit_data(&flash_data))
    return;
flash_program(addr, 1, flash_callback_write);
send_ack_data();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void cmd_esf(void)
{
flash_bulk_erase();
send_ack_data();
}

// ---------------------------------------------------------------------------
void cmd_rse(void)
{
unsigned char addr, data;

if(!get_8bit_data(&addr))
    return;
eeprom_read_buffer(addr,&data, 1);
send_dec_data(data, 3);
}

// ---------------------------------------------------------------------------
void cmd_wse(void)
{
unsigned char addr, data;

if(!get_8bit_data(&addr))
    return;
if(!get_8bit_data(&data))
    return;
eeprom_write_buffer(addr, &data, 1);
send_ack_data();
}


// ---------------------------------------------------------------------------
// Control Panel Commands
// ---------------------------------------------------------------------------
#define MAX_BUF_LEN     10
#define CMD_LEN         3
const struct tab_command
	{
	const char *cmd;
	void (*func)(void);
	}table_cmd[] = {
	// AK-HCS12NE board commands
	{"sld"		, cmd_sld	} ,/* Set Led value 	        */	
	{"gld"		, cmd_gld	} ,/* Get Led value	            */
	{"gsw"		, cmd_gsw	} ,/* Get dip-SWitches value	*/
	{"gpt"		, cmd_gpt	} ,/* Get PoTentiometer value	*/
	{"gpb"		, cmd_gpb	} ,/* Get Push-Button value     */	
	{"gtp"		, cmd_gtp	} ,/* Get TemPerature value     */
	{"gms"		, cmd_gms	} ,/* Get Motor Speed       	*/
	{"sms"		, cmd_sms	} ,/* Set Motor Speed	        */
	{"gmd"		, cmd_gmd	} ,/* Get Motor Direction		*/
	{"smd"		, cmd_smd	} ,/* Set Motor Direction		*/
	{"gme"		, cmd_gme	} ,/* Get Motor Enable      	*/
	{"sme"		, cmd_sme	} ,/* Set Motor Enable          */
	{"gmc"		, cmd_gmc	} ,/* Get Motor Current		    */
	{"gmf"		, cmd_gmf	} ,/* Get Motor Fault State		*/
	{"sso"		, cmd_sso	} ,/* Set Speaker On		    */
	{"gso"		, cmd_gso	} ,/* Get Speaker On		    */
	{"ssf"		, cmd_ssf	} ,/* Set Speaker Frequency		*/
	{"gsf"		, cmd_gsf	} ,/* Get Speaker Frequency		*/

	// CM-HCS12NE64 module commands
	{"rsf"		, cmd_rsf	} ,/* Read Serial Flash			*/
	{"wsf"		, cmd_wsf	} ,/* Write Serial Flash		*/
	{"esf"		, cmd_esf	} ,/* Erase Serial Flash		*/
	{"rse"		, cmd_rse	} ,/* Read Serial Eeprom		*/
	{"wse"		, cmd_wse	} ,/* Write Serial Eeeprom		*/
	{NULL		, NULL		} };

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void decod_commands(UINT32 len)
{
char i;
char buf_in[MAX_BUF_LEN+1];

if(len < CMD_LEN+1)
    {
    send_nack(ERR_SYNTAX);
    return;
    }
for(i=0; i<len; i++)
    {
    if(i > MAX_BUF_LEN)
        break;
    buf_in[i] = GET_DATA();
    if(buf_in[i] == '\n')
        break;
    }
for(i=0; i<CMD_LEN; i++)
   buf_in[i] |= 0x20;         // + Lower case conversion 
    
 for(i = 0;; i++)
	{
	if(table_cmd[i].cmd == NULL)
		{
		send_nack(ERR_INVALID_CMD);
		return;
		}
	if(strncmp(table_cmd[i].cmd, buf_in, CMD_LEN) == 0)
		{
		ptr_cmdbuf = buf_in + CMD_LEN;
		table_cmd[i].func();
		break;
		}
	}
	
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void tcp_send_buffer(void)
{
char len;
	
/* first check if data sending is possible (it may be that
 * previously sent data is not yet acknowledged)
 */
while(tcp_checksend(tcp_soch) < 0)
    ;       /* Not yet */
/* put message in buffer. Message needs to start from TCP_APP_OFFSET
 * because TCP/IP stack will put headers in front of the message to
 * avoid data copying
 */

for(len=0;;)
    {
	net_buf[TCP_APP_OFFSET+len] = udp_tcp_buf[len];
	if(udp_tcp_buf[len++] == '\n')
	    break;
	}
if(tcp_send(tcp_soch, &net_buf[TCP_APP_OFFSET], NETWORK_TX_BUFFER_SIZE - TCP_APP_OFFSET, len) == len)
    {
	DEBUGOUT("TCP: bytes reply sent!\r\n");
	}
else
	{
    DEBUGOUT("TCP: error occured while trying to send data!\r\n");
	}
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void tcp_control_panel_init(void)
{
	
	DEBUGOUT("Initializing TCP application. \r\n");
	
	/* Get socket:
	 * 	TCP_TYPE_SERVER - type of TCP socket is server
	 * 	TCP_TOS_NORMAL  - no other type of service implemented so far
	 *	TCP_DEF_TOUT	- timeout value in seconds. If for this many seconds
	 *		no data is exchanged over the TCP connection the socket will be
	 *		closed.
	 *	tcps_control_panel_eventlistener - pointer to event listener function for
	 * 		this socket.	
	 */
	
	tcp_soch = tcp_getsocket(TCP_TYPE_SERVER, TCP_TOS_NORMAL, TCP_DEF_TOUT, tcp_control_panel_eventlistener);
	
	if( tcp_soch < 0 )
	{
		DEBUGOUT("TCP: Unable to get socket. Resetting!!!\r\n");
		RESET_SYSTEM();
	}
	
	/* Put it to listen on some port */
	
	(void)tcp_listen(tcp_soch, TCP_AK_PORT);
}


// ---------------------------------------------------------------------------
 /*
 * Event listener invoked when TCP/IP stack receives TCP data for
 * a given socket. Parameters:
 * - cbhandle - handle of the socket this packet is intended for. Check it
 *	just to be sure, but in general case not needed
 * - event - event that is notified. For TCP there are quite a few possible
 *	events, check switch structure below for more information
 * - par1, par2 - parameters who's use depends on the event that is notified
 */
// ---------------------------------------------------------------------------
INT32 tcp_control_panel_eventlistener(INT8 cbhandle, UINT8 event, UINT32 par1, UINT32 par2)
{
par2++;	// to avoid the compiler warning

/* This function is called by TCP stack to inform about events	*/

	if( cbhandle != tcp_soch)		/* Not our handle	*/
		return(-1);
	
	switch( event )
	{
	
		/* Connection request event. Used by TCP/IP stack to inform
		 * the application someone is trying to establish a connection.
		 * Server can decide, based on provided IP address and port number,
		 * whether to allow or not connection establishment.
		 * Parameters:
		 *  - par1 - remote hosts IP address
		 *  - par2 - remote hosts port number
		 * 
		 * Return values from event listener:
		 * -1 - do not allow connection to be established (send reset)
		 * -2 - do not send any response for now to the SYN packet (let's
		 *		think a little before answering)
		 * 1  - allow connection to be established
		 */
		case TCP_EVENT_CONREQ:
			DEBUGOUT("TCP: Connection request arrived!\r\n");
			
			/* Enable all connections	*/
			return(1);
		
			break;
	
		/* Connection abort event. Connection on a given socket is beeing 
		 * aborted for somereason (usually retransmissions are used up or 
		 * some abnormal situation in communication happened).
 		 * Parameters:
		 *  - par1 - remote hosts IP address
		 *  - par2 - remote hosts port number
		 */
		case TCP_EVENT_ABORT:
			DEBUGOUT("Connection aborting!\r\n");	
			break;
		
		/* Connection established event - three-way handshaking performed
		 * OK and connection is established.
		 * Parameters:
		 *  - par1 - remote hosts IP address
		 *  - par2 - remote hosts port number
		 */
		case TCP_EVENT_CONNECTED:
			DEBUGOUT("TCP: TCP connection established!\r\n");
		
			break;
			
		/* Connection closing event. Happens when TCP connection is
		 * intentionally close by some side calling close function and
		 * initializing proper TCP connection close procedure.
		 * Parameters:
		 *  - par1 - remote hosts IP address
		 *  - par2 - remote hosts port number
		 */
		case TCP_EVENT_CLOSE:
			DEBUGOUT("TCP Connection closing...!\r\n");
			break;
			
		/* Data acknowledgment event. Happens when data that was
		 * previously sent gets acknowledged. This means we can now
		 * send some more data! :-)
		 * Parameters:
		 *  - par1 - remote hosts IP address
		 *  - par2 - remote hosts port number
		 */
		case TCP_EVENT_ACK:
			DEBUGOUT("TCP: Data acknowledged!\r\n");
			/* if more data should be sent, adjust variables and
				set tcps_demo_senddata variable */
				
			break;
			
		/* Data received event. Happens when we receive some data over the
		 * TCP connection.
		 * Parameters:
		 *  - par1 - number of data bytes received
		 *  - par2 = 0
		 */
		case TCP_EVENT_DATA:
			DEBUGOUT("TCP: Data arrived!\r\n");
			/* read data that was received (and 
			 * probably do something with it :-)
			 */
		    if(par1 <= 0)
		        break;
		    decod_commands(par1);
		    tcp_send_buffer();
			break;
			
		/* Regenerate data event. Happens when data needs to be
		 * retransmitted because of possible loss on the network.
		 * Note that THE SAME DATA must be sent over and over again
		 * until TCP_EVENT_ACK is generated (for that data)! 
		 * Parameters:
		 *  - par1 - amount of data to regenerate (usually all)
		 *	- par2 = 0
		 */
		case TCP_EVENT_REGENERATE:
		    tcp_send_buffer();
			break;
	
	
		default:
			return(-1);
	}
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
INT16 udp_send_buffer(void)
{
char len;
	
for(len=0;;)
    {
	net_buf[UDP_APP_OFFSET+len] = udp_tcp_buf[len];
	if(udp_tcp_buf[len++] == '\n')
	    break;
	}
	
return(udp_send(udp_soch, udp_ipaddr, udp_port, &net_buf[UDP_APP_OFFSET], NETWORK_TX_BUFFER_SIZE -UDP_APP_OFFSET, len));
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void udp_control_panel_init(void)
{
	DEBUGOUT("Initializing UDP Echo client\r\n");

	udp_soch = udp_getsocket(0 , udp_control_panel_eventlistener , UDP_OPT_SEND_CS | UDP_OPT_CHECK_CS);
	
	if(udp_soch == -1)
	    {
		DEBUGOUT("No free UDP sockets!! \r\n");
		RESET_SYSTEM();
	    }
	
	/* open socket */
	(void)udp_open(udp_soch, UDP_AK_PORT);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
INT32 udp_control_panel_eventlistener(INT8 cbhandle, UINT8 event, UINT32 ipaddr, UINT16 port, UINT16 buffindex, UINT16 datalen)
{
buffindex++;	// to avoid the compiler warning

if(cbhandle != udp_soch)
    {
	DEBUGOUT("UDP: not my handle!!!!");
	return (-1);
	}
	
switch(event)
    {
	case UDP_EVENT_DATA:

	    if(datalen <= 0)
            break;
	    udp_ipaddr = ipaddr;        /**< Remote IP address this application will send data to*/ 
	    udp_port = port;            /**< Port number on remote server we'll send data to */
	    decod_commands(datalen);
	    udp_data_to_send = TRUE;
	    break;
		
	default:
		DEBUGOUT("UDP: unknown UDP event :-(");
		break;
    }
return 0;
}

// ---------------------------------------------------------------------------
// UDP Demo app main loop that is periodically invoked from the main loop
// ---------------------------------------------------------------------------
void udp_control_panel_run(void)
{
if(udp_data_to_send) 
    {
	switch(udp_send_buffer())
	    {
		case -1:
			DEBUGOUT("Error (General error, e.g. parameters)\r\n");
			break; 
		case -2:
			DEBUGOUT("ARP or lower layer not ready, try again\r\n");
			break;
		case -3:
			DEBUGOUT("Socket closed or socket handle not valid!\r\n");
			break;
		default:
			// data sent (could check how many bytes too) if no more data to send, reset flag
        	udp_data_to_send = FALSE;
			break;
	    }	  
    }
}
