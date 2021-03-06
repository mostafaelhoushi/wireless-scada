#include "datatypes.h"
#include "system.h"
#include "debug.h"
#include "ne64debug.h"
#include "MC9S12NE64.h"


UINT32 base_timer;		/**< System 1.024 msec timer	*/

UINT8 sleep_mode = 0;	/**< Used to store information about power-saving state we're in (if any) */

/** \brief Transmit buffer used by all OpenTCP applications
 *
 *	This buffer is the transmit buffer used by all OpenTCP applications
 *	for sending of data. Please note the warnings below for correct usage
 *	of this buffer that ensures proper operation of the applications.
 *
 *	\warning
 *		\li <b>Transmit buffer start</b> - to avoid data copying, the TCP/IP
 *		stack will use first part of the net_buf buffer to add it's data. This
 *		means that applications using TCP and/or UDP <b>must not</b> write
 *		application-level data from the beginning of the buffer but from certain
 *		offset. This offset depends on the transport-layer protocol (it's
 *		header size that is). For TCP this value is defined by the
 *		TCP_APP_OFFSET and for the UDP it is UDP_APP_OFFSET.
 *		\li <b>Buffer sharing</b> - since all applications share this buffer among each other, 
 *		and with the TCP/IP stack as well, care must be taken not to
 *		overwrite other applications' data before it is sent. This is best
 *		achieved if all applications work in the main loop and when they
 *		wish to send data they fill in the buffer and send it immediately.
 *
 */
UINT8 net_buf[NETWORK_TX_BUFFER_SIZE];	/* Network transmit buffer	*/

/********************************************************************************
Function:		strlen

Parameters:		UINT8* str - start address of string buffer
				UINT16 len - buffer length
				
Return val:		INT16 - (-1) Not a string
						(>=0) Length of string
				
Date:			12.8.2002

Desc:			Calculates the length of given string
*********************************************************************************/


INT16 __strlen (UINT8* buf, UINT16 len)
{
	UINT16 i;
	
	for(i=0; i<len; i++) {
		if(*buf == '\0')
			return( i );
		
		buf++;
	}
	
	/* Not found	*/
	
	return(-1);


}


/********************************************************************************
Function:		bufsearch

Parameters:		UINT8* startadr - start address of given buffer
				UINT16 len - buffer length
				UINT8* str - given searchstring
				
Return val:		INT16 - (-1) Not found
						(>=0) Start of matched string from startadr
				
Date:			12.7.2002

Desc:			Seeks given string from given buffer
*********************************************************************************/

INT16 bufsearch (UINT8* startadr, UINT16 len, UINT8* str)
{
	UINT16 i;
	INT16 position;
	UINT8 matchesneeded;
	UINT8 matchesnow;
	UINT8* target;
	UINT8* key;
	
	target = startadr;
	position = -1;
	key = str;
	matchesnow = 0;
	matchesneeded = 0;
	
	/* How many matches we need?	*/
	
	while( *key++ != '\0' ) {
		/* Break possible deadlock	*/
		
		matchesneeded++;
		if(matchesneeded > 30)
			return(-1);
	}
	
	/* Search for first mark and continue searching if found	*/
	
	key = str;
	
	for(i=0; i<len; i++) {
		if( *target == *key) {
			/* We found matching character		*/
			
			matchesnow++;
			
			/* Move to next character of key	*/
			
			key++;
			target++;
			
			if(matchesnow == 1) {
				/* First character match	*/
				
				position = i;
			}
			
			if(matchesneeded == matchesnow) {
				/* Whole string matched	*/
				
				return(position);
			}
			
		} else {
		
			if( matchesnow != 0) {
				/* It wasn't a complete match...				*/
				/* Initialize counters and start again			*/
			
				matchesnow = 0;
				key = str;
			
				/* Move to next character of target after 		*/
				/* previous matching character					*/
			
				target = startadr;
				target += position;
				target += 1;
			
				i = position;
			} else {
				/* Just continue searching the first match		*/
				target++;
			}
		}
	
	}
	
	/* No matches found...	*/
	
	return(-1);
	
}


/********************************************************************************
Function:		tolower

Parameters:		UINT8 ch - ASCII character to be converted lowercase
				
Return val:		UINT8 - converted character
				
Date:			21.8.2002

Desc:			If ch is UPPERCASE letter it is converted to lowercase and 
				returned. Otherwise original character is returned
*********************************************************************************/

UINT8 __tolower (UINT8 ch)
{
	if( (ch < 91) && (ch > 64) )
		return(ch + 32);
	
	return(ch); 

}


/********************************************************************************
Function:		toupper

Parameters:		UINT8 ch - ASCII character to be converted UPPERCASE
				
Return val:		UINT8 - converted character
				
Date:			21.8.2002

Desc:			If ch is lowercase letter it is converted to UPPERCASE and 
				returned. Otherwise original character is returned
*********************************************************************************/

UINT8 __toupper (UINT8 ch)
{
	if( (ch < 123) && (ch > 96) )
		return(ch - 32);
	
	return(ch); 

}

/* Is the given ASCII code numerical	*/
/* e.g. '0','1','2' ... '9'				*/

UINT8 isnumeric (UINT8 ch)
{
	if( (ch < 58) && (ch > 47) )
		return(1);
	return(0);
}


/* HexToAscii - Take one byte and return its two ASCII  */
/* values for both nibbles								*/

UINT16 hextoascii (UINT8 c)
{
	UINT8 ch = c;
	UINT8 as1;
	UINT8 as2;

	/* take the char and turn it to ASCII */
	
	as1 = ch;
	as1 >>= 4;
	as1 &= 0x0F;
	if ( as1<10 )
		as1 += 48;
	else
		as1 += 55;
		
	as2 = ch;
	as2 &= 0x0F;
	if ( as2<10 )
		as2 += 48;
	else
		as2 += 55;
		
	return( ((UINT16)(as1)<<8) + as2 );
	
	
}


/* Convert ASCII character to numerical	*/
/* e.g. '1' -> 0x01, 'A' ->0x0A			*/

UINT8 asciitohex (UINT8 ch)
{
	if( (ch < 58) && (ch > 47) )
		return(ch - 48);
	
	if( (ch < 71 ) && (ch > 64) )
		return(ch - 55); 
}


void __ltoa (UINT32 nmbr, UINT8 *ch )
{
	/* Transforms value of long word to ASCII string */
	/* Makes it iterative						 	 */
	
	UINT16 multiple;
	UINT32 decade,comp;
	UINT8 i,found;
	
	/* Init String */	
	
	for( i=0; i<10;i++ )
		*ch++ = '0';
	
	ch -= 10;
	
	/* See if Zero */
	
	if(nmbr == 0) {
		*ch++ = '0';
		*ch = '\0';
	}
	
	
	decade = 1000000000;
	
	found = FALSE;
	
	for( i=0; i<10; i++) {
		
		if(i != 0)
			decade /= 10;
		
		for( multiple=9; multiple>0; multiple--) {	
			if( (i==0) && (multiple > 2) )
				continue;
		
			comp = decade * multiple;
			
			if(nmbr >= comp) {
				*ch = (UINT8)(hextoascii((UINT8)(multiple)));
				nmbr -= comp; 
				found = TRUE;
					
				break;				/* Still processing */
			}
		}
		
		if( found == TRUE)
			ch++;
	
	}	

	*ch = '\0';			/* EOL */
	
}




void __itoa (UINT16 nmbr, UINT8* ch )
{
	/* Transforms value of word to ASCII string */
	/* Makes it iterative						*/

	UINT16 decade, multiple;
	UINT32 comp;
	UINT8 i,found;

	/* Init String */
	
	
	for( i=0; i<5;i++)
		*ch++ = '0';
	
	ch -= 5;
	
	/* See if Zero */
	
	if(nmbr == 0) {
		*ch++ = '0';
		*ch = '\0';
	}
	
	decade = 10000;
	
	found = FALSE;
	
	for( i=0; i<5; i++) {
		
		
		if(i != 0)
			decade /= 10;
		
		for( multiple=9; multiple>0; multiple--) {	
			if( (i==0) && (multiple > 6) )
				continue;
		
			comp = decade * multiple;
			
			if(nmbr >= comp) {
				*ch = (UINT8)(hextoascii((UINT8)(multiple)));
 				nmbr -= (UINT16)(comp); 
				found = TRUE;
					
				break;				/* Still processing */
			}
		}
		
		if( found == TRUE)
			ch++;
	
	}	

	*ch = '\0';			/* EOL */
	
}


/* Convert given buffer containing ASCII numbers	*/
/* to numerical positive INT16 value (max. 32767)	*/

INT16 __atoi (UINT8 *buf, UINT8 buflen)
{
	INT16 oval = 0;
	UINT8 nval = 0;
	
	while(buflen--) {
	
		if(*buf == '\0')
			break;
		
		if( isnumeric(*buf) == 0 )
			return(-1);
	
		nval = asciitohex(*buf++);
		
		oval = oval * 10;
		oval += nval;	
		
		/* Overflow?	*/
		
		if(oval < nval)
			return(-1);
	
	}
	
	return(oval);

}


/* Debug/String output	*/

void mputs (INT8* msg)
{
  _DEBUGT(msg);
msg++;
}

/* Debug/Hex output a number*/
void mputhex(UINT8 nbr)
{ 
    _DEBUGC(nbr);
nbr++;
}

/*	Watchdog refresh	*/

void kick_WD (void) 
{
}

/* Wait for unaccurate use	*/

void wait (INT16 i)
{
	for(;i;i--) kick_WD();
}


/* Return "Random" Number	*/

UINT32 random (void)
{
	/* TODO: Return REAL random number	*/
	return(0x345A2890);
}

/* Do nothing	*/

void dummy (void)
{
	/* That's it	*/
}

/* 	Power saving mode	*/

void enter_power_save (void)
{
	/* Are we on sleep mode already?	*/

	if (sleep_mode)
		return;
		
	sleep_mode = 1;
	
	return;
}



void exit_power_save (void)
{
	UINT8 i;

	if (sleep_mode) {
		/* Release RS transmitter chip	*/
		/* Wait for a while	*/
		
		for( i=0; i<128; i++)
			sleep_mode = 0;
		
	}
	
}


