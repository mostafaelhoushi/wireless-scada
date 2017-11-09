//////////////////////////////////////////////////////////////////////////////
//
// SofTec Microsystems AK-S12NE64-A Demo Board Sample
//
// ---------------------------------------------------------------------------
// Copyright (c) 2004 SofTec Microsystems
// http://www.softecmicro.com/
//
//////////////////////////////////////////////////////////////////////////////
/* Including used modules for compilling procedure */
#include "debug.h"
#include "datatypes.h"
#include "timers.h"
#include "system.h"
#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "tcp_ip.h"

#include "http_server.h"
#include "dns.h"
#include "pop3_client.h"
#include "smtp_client.h"
#include "tftps.h"

#include "ne64driver.h"
#include "ne64api.h"
#include "mBuf.h"
#include "ne64config.h"

#include "address.h"

#include "MC9S12NE64.h"

#include "main.h"

/* Network Interface definition. Must be somewhere so why not here? :-)*/
struct netif localmachine;

extern void RTI_Enable (void);
extern void RTI_Init (void);

extern tU16 gotxflowc;	    /* Global Variable For Determination of Flow Control Packets are sent in Full Duplex */
extern tU08 gotlink;		/* Global Variable For Determination if link is active                               */

#if ZERO_COPY
#else
  tU08 ourbuffer [1518];    /**< Space for packet temporary storage if zero copy not used*/
#endif

#if USE_SWLED
tU16 LEDcounter=0;
#endif

UINT32 IP_POP3_Server = 0;
UINT8  POP3_Connected = 0;
//==============================================================
//==============================================================
void dns_pop3_listener(UINT8 event, UINT32 data)
{
//>>>>>
if(event == DNS_EVENT_SUCCESS)
    {
    IP_POP3_Server = data;
    }
}

//==============================================================
//==============================================================
/* main stuff */
void main(void)
{
  INT16 len;
  UINT16 t1, t2;

  /* System clock initialization */
  CLKSEL=0;
  CLKSEL_PLLSEL = 0;                   /* Select clock source from XTAL */
  PLLCTL_PLLON = 0;                    /* Disable the PLL */
  SYNR = 0;                            /* Set the multiplier register */
  REFDV = 0;                           /* Set the divider register */
  PLLCTL = 192;
  PLLCTL_PLLON = 1;                    /* Enable the PLL */
  while(!CRGFLG_LOCK);                 /* Wait */
  CLKSEL_PLLSEL = 1;                   /* Select clock source from PLL */

  INTCR_IRQEN = 0;                     /* Disable the IRQ interrupt. IRQ interrupt is enabled after CPU reset by default. */
	
	  /* initialize processor-dependant stuff (I/O ports, timers...).
	  * Mostimportant things to do in this function as far as the TCP/IP stack
	  * is concerned is to:
	  *  - initialize some timer so it executes decrement_timers
	  * 	on every 10ms (TODO: Throw out this dependency from several files
	  *	so that frequency can be adjusted more freely!!!)
	  *  - not mess too much with ports allocated for Ethernet controller
	  */
    init();
    RTI_Init();
	AK_HCS12NE_Init();
	if((PTG & 0x0C) == 0)	   // Reset of all board parameters (PG3+PG2)
	    {   
	    eeprom_write_buffer(EE_ADDR_MAC, (unsigned char *)hard_addr, 6);
	    eeprom_write_buffer(EE_ADDR_IP_ADDR, (unsigned char *)prot_addr, 4);
	    eeprom_write_buffer(EE_ADDR_IP_SUBNET, (unsigned char *)netw_mask, 4);
	    eeprom_write_buffer(EE_ADDR_IP_GATEWAY, (unsigned char *)dfgw_addr, 4);
	    // feedback on speaker output
        for(t1=0; t1<300; t1++)
            {
            for(t2=0; t2<1500; t2++)
                ;   
            PTT ^= 0x40;
            }
	    }

    /* Set our network information. This is for static configuration.
    * if using BOOTP or DHCP this will be a bit different.
    */
   	/* IP address */
   	eeprom_read_buffer(EE_ADDR_IP_ADDR, (unsigned char *)&localmachine.localip, 4); 
   	// localmachine.localip = *((UINT32 *)ip_address);
   	
   	/* Default gateway */
   	eeprom_read_buffer(EE_ADDR_IP_GATEWAY, (unsigned char *)&localmachine.defgw, 4); 
   	// localmachine.defgw   = *((UINT32 *)ip_gateway);
   	
   	/* Subnet mask */
   	eeprom_read_buffer(EE_ADDR_IP_SUBNET, (unsigned char *)&localmachine.netmask, 4); 
   	// localmachine.netmask = *((UINT32 *)ip_netmask);

   	/* Ethernet (MAC) address */
   	eeprom_read_buffer(EE_ADDR_MAC, localmachine.localHW, 6);   	
    // localmachine.localHW[0] = hard_addr[0];
    // localmachine.localHW[1] = hard_addr[1];
    // localmachine.localHW[2] = hard_addr[2];
    // localmachine.localHW[3] = hard_addr[3];
    // localmachine.localHW[4] = hard_addr[4];
    // localmachine.localHW[5] = hard_addr[5];


	
	/* Init system services		*/    
	timer_pool_init();
		
    /* Initialize all buffer descriptors */
	mBufInit ();

	/*interrupts can be enabled AFTER timer pool has been initialized */
    	
   	/* Initialize all network layers	*/
   	EtherInit();
    __asm CLI;     /* Enable Interrupts */

#if USE_EXTBUS
    ExternalBusCfg();
#endif   	
   	arp_init();
   	(void)udp_init();
  	(void)tcp_init();

	/* Initialize applications	*/    
	(void)https_init ();
    (void)tftps_init(); 

    /* Note: If enabled, this call initializes the SMTPC protocol */
    /*  smtpc_init ();  */
    /* Note: If enabled, this call initializes the DNS */
    /*    dns_init();   */
    /* Note: If enabled, this call initializes the POP3 protocol */
    /*    pop3c_init(); */

    tcp_control_panel_init();     
    udp_control_panel_init(); 
	
	RTI_Enable ();


    /* Note: If enabled, this call starts an E-mail sending procedure           */
    /* Remember to set the Outgoing mail IP Server and the Server Port number   */
    /* Also, please verify the SMTPC_TOUT parameter (Server Timeout)            */ 
    /* (void)smtpc_connect (0xC0A80101, 25);    */


//>>>>> DNS	
//    (void)get_host_by_name((unsigned char *)"mail.softecmicro.com", dns_pop3_listener);

	DEBUGOUT(">>>>>>>>>Entering to MAIN LOOP>>>>>>>>>\n\r");

	/* main loop */
	for (;;)
	{
		/* take care of watchdog stuff */
#if USE_SWLED
      UseSWLedRun();
#endif
    if (gotlink) 
        {	  
   		/* Try to receive Ethernet Frame	*/    	
   		if( NETWORK_CHECK_IF_RECEIVED() == TRUE )	
		    {
            switch( received_frame.protocol)
                {
   				case PROTOCOL_ARP:
				    process_arp (&received_frame);
	    			break;
   				case PROTOCOL_IP:
   					len = process_ip_in(&received_frame);
  					if(len < 0)
     				    break;
     	  		    switch (received_ip_packet.protocol)
  				        {
     					case IP_ICMP:
      					    process_icmp_in (&received_ip_packet, len);
  						    break;
     					case IP_UDP:
                            process_udp_in (&received_ip_packet,len);   							
                            break;
                        case IP_TCP:
      						process_tcp_in (&received_ip_packet, len);				
     						break;
     					default:
    						break;
    				    }
       			    break;
                default:
   					break;
            }
            /* discard received frame */    		
   			NETWORK_RECEIVE_END();
        }
  		/* Application main loops */
    	/* manage arp cache tables */
		arp_manage();   	
    	/* TCP/IP stack Periodic tasks here... */
  		/* manage opened TCP connections (retransmissions, timeouts,...)*/
		tcp_poll();
		https_run ();
		tftps_run();
        udp_control_panel_run();
		
    /* Note: Enable this function if you are using the SMTP protocol */
    /*    smtpc_run();  */

/*      dns_run();
        if(IP_POP3_Server && !POP3_Connected)
            {
            (void)pop3c_connect(IP_POP3_Server, 110);
            POP3_Connected = 1;
            }
        if(IP_POP3_Server && POP3_Connected)
            pop3c_run();  	*/  


		
	  } else {
	 	   //NO LINK
	  }	 // if (gotlink)
	}		 //for (;;)
}
