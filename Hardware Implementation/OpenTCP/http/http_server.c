#include"datatypes.h"
#include"globalvariables.h"
#include"debug.h"
#include"system.h"
#include"tcp_ip.h"
#include"http_server.h"


UINT8 https_enabled = 0; /**< Defines whether https_init has already been invoked or not */

/** \brief Used for storing state information about different HTTP sessions
 *
 *	This is an array of http_server_state structures holding various state 
 *	information about the HTTP sessions. HTTP server uses this information
 *	to determine actions that need to be taken on sockets.
 */
struct http_server_state https [NO_OF_HTTP_SESSIONS];

/** \brief Initialize HTTP server variables
 * 	\author 
 *		\li Jari Lahti (jari.lahti@violasysems.com)
 *	\date 13.10.2002
 *
 *	This function should be called before the HTTP Server application
 *	is used to set the operating parameters of it	
 */
INT8 https_init(void)
{
	UINT8 i;
	INT8 soch;


	for( i=0; i<NO_OF_HTTP_SESSIONS; i++)
	{
		https[i].state       = HTTPS_STATE_FREE;
		https[i].ownersocket = 0;
		https[i].fstart      = 0;
		https[i].fpoint      = 0;
		https[i].flen        = 0;
		https[i].funacked    = 0;
		
		soch = 	tcp_getsocket(TCP_TYPE_SERVER, TCP_TOS_NORMAL, TCP_DEF_TOUT, https_eventlistener);
		
		if(soch < 0)
		{
			DEBUGOUT("HTTP Server uncapable of getting socket\r\n");
			RESET_SYSTEM();
			/*return(-1);*/
		}
		
		https[i].ownersocket = soch;
		
		kick_WD();
		
		soch = tcp_listen(https[i].ownersocket, HTTPS_SERVERPORT);
		
		if(soch < 0)
		{
			DEBUGOUT("HTTP Server uncapable of setting socket to listening mode\r\n");
			RESET_SYSTEM();
			/*return(-1);*/
		}		
		
	
	} 
	
	https_enabled  = 1;
	
	return(i);	
	
}


/********************************************************************************
Function:		https_run

Parameters:		void	
				
Return val:		void
				
Date:			13.10.2002

Desc:			This function is main 'thread' of HTTP server program
				and should be called periodically from main loop.
*********************************************************************************/

void https_run (void)
{
	UINT8 i;
	INT16 len;
	static UINT8 ses = 0;
	
	if( https_enabled == 0)
		return;
		
	/* Walk thru all sessions untill we found something to send or so	*/
	
	for(i=0; i<NO_OF_HTTP_SESSIONS; i++)
	{
		kick_WD();
		
		if(ses >= NO_OF_HTTP_SESSIONS)
			ses = 0;

		/* Keep sockets listening	*/
		
		if(tcp_getstate(https[ses].ownersocket) < TCP_STATE_LISTENING)
		{
			(void)tcp_listen(https[ses].ownersocket, HTTPS_SERVERPORT);
			ses++;
			continue;	
		}

		if(https[ses].state != HTTPS_STATE_ACTIVE)
		{
			ses++;
			continue;
		}
		
		if(https[ses].funacked != 0)
		{
			ses++;
			continue;		
		}
		
		if(https[ses].fstart == 0)
		{
			ses++;
			continue;		
		}		
		
		/* End of data?	*/
		
		if( https[ses].fpoint >= https[ses].flen)
		{
			(void)tcp_close(https[ses].ownersocket);
			(void)tcp_abort(https[ses].ownersocket);
			https_deletesession(ses);
			
			ses++;
			
			return;			
		
		}
		
		/* More data to send	*/
		
		len = https_loadbuffer(ses, &net_buf[TCP_APP_OFFSET], NETWORK_TX_BUFFER_SIZE - TCP_APP_OFFSET);
			
		if(len<0)
			return;
			
		len = tcp_send(https[ses].ownersocket, &net_buf[TCP_APP_OFFSET], NETWORK_TX_BUFFER_SIZE - TCP_APP_OFFSET, len);	
		
		if(len<0)
		{
			(void)tcp_close(https[ses].ownersocket);
			https_deletesession(ses);
			
			ses++;

			return;			
		
		}
		
		https[ses].funacked = len;
		
		
		/* Serve another session on next run	*/
		
		ses++;
		
		return;		
		
	
	}	
	
	


}

/********************************************************************************
Function:		https_eventlistener

Parameters:		INT8 cbhandle - handle to TCP socket where event is coming from	
				UINT8 event - type of event
				UINT32 par1 - parameter the meaning of depends on event
				UINT32 par2 - parameter the meaning of depends on event
				
Return val:		INT32 - depends on event but usually (-1) is error of some
						kind and positive reply means OK
				
Date:			13.10.2002

Desc:			This function is given to TCP socket as function pointer to be
				used by TCP engine to make callbacks to inform about events
				on TCP e.g. arriving data. 
*********************************************************************************/


INT32 https_eventlistener (INT8 cbhandle, UINT8 event, UINT32 par1, UINT32 par2)
{
	/* This function is called by TCP stack to inform about events	*/
	
	INT16 	i;
	INT16 	session;
	par2 = 0;
		
	if( https_enabled == 0)
		return(-1);
	
	if(cbhandle < 0)
		return(-1);	
		
	/* Search for rigth session	*/
	
	session = https_searchsession(cbhandle);	
	
	switch( event )
	{
	
		case TCP_EVENT_CONREQ:
		
			/* Do we have a session for requesting socket?	*/
			
			if(session < 0)
				return(-1);

			
			/* Try to get new session	*/
			
			session = https_bindsession(cbhandle);
			
			if(session < 0)			/* No resources	*/
				return(-1);

			
			return(1);
	
		case TCP_EVENT_ABORT:
		
			if(session < 0)
				return(1);
			
			https_deletesession((UINT8)session);		
			
			return(1);
		
		case TCP_EVENT_CONNECTED:
		
			if(session < 0)
				return(-1);
		
			https_activatesession((UINT8)session);
			
			return(1);
			
		case TCP_EVENT_CLOSE:
		
			if(session < 0)
				return(-1);
			
			https_deletesession((UINT8)session);		
			
			return(1);		
		
			
		case TCP_EVENT_ACK:
		
			if(session < 0)
				return(-1);
		
			https[session].fpoint += https[session].funacked;
			https[session].funacked = 0;
		
			return(1);
			
		case TCP_EVENT_DATA:
		
			/* Check for GET request	*/
			
			if(session < 0)
				return(-1);
			
			if(https[session].fstart == 0)
			{
				if(par1 <= 3)
					return(1);
				
				/* Check for GET	*/
				
				if(RECEIVE_NETWORK_B() != 'G') return(1);
				if(RECEIVE_NETWORK_B() != 'E') return(1);	
				if(RECEIVE_NETWORK_B() != 'T') return(1);			
					
				par1 -= 3;
				
				/* Search for '/'	*/
				
				for(i=0; i<(INT16)par1; i++)
				{
					if(RECEIVE_NETWORK_B() == '/')
					{
						i++;
						break;
					}
				}		
				
				par1 -= i;
				
				/* Calculate Hash	*/
				
//				i = https_calculatehash(par1);	
				
//				if(i < 0)
//				{
					/* Invalid GET	*/
//					return(1);
//				}	
				
				/* Get FileRef	*/
                //>>>>> 			
				i = https_findfile((UINT8)par1, (UINT8)session);
						
				
				return(1);		
				
			
			}
		
		
			return(1);
		
			
		case TCP_EVENT_REGENERATE:
		
			if(session < 0)
				return(-1);
		
			if(https[session].state != HTTPS_STATE_ACTIVE)
				return(-1);
		
			i = https_loadbuffer((char)session, &net_buf[TCP_APP_OFFSET], (UINT16)par1);
			
			if(i<0)
				return(-1);
			
			(void)tcp_send(https[session].ownersocket, &net_buf[TCP_APP_OFFSET], NETWORK_TX_BUFFER_SIZE - TCP_APP_OFFSET, i);	
		
			return(i);
	
	
		default:
			return(-1);
	}	

}


void https_deletesession (UINT8 ses)
{
	https[ses].state    = HTTPS_STATE_FREE;
	https[ses].fstart   = 0;
	https[ses].fpoint   = 0;
	https[ses].flen     = 0;
	https[ses].funacked = 0;

}

INT16 https_searchsession (UINT8 soch)
{
	UINT8 i;
	
	for(i=0; i<NO_OF_HTTP_SESSIONS; i++)
	{
		if(https[i].ownersocket == soch)
			return(i);
		
	}
	
	return(-1);

}

INT16 https_bindsession (UINT8 soch)
{
	UINT8 i;
	
	for(i=0; i<NO_OF_HTTP_SESSIONS; i++)
	{
		if(https[i].ownersocket == soch)
		{
			if(https[i].state == HTTPS_STATE_FREE)
			{
				https[i].state = HTTPS_STATE_RESERVED;
				return(i);
			}			
		}	
		
	}
	
	
	return(-1);

}

void https_activatesession (UINT8 ses)
{
	https[ses].state = HTTPS_STATE_ACTIVE;

}

/* read two encoded bytes from HTTP URI and return
 * decoded byte
 */
UINT8 https_read_encoded(void)
{
	UINT8 temp,ch;
	
	temp = RECEIVE_NETWORK_B();
	if ((temp>='0') && (temp<='9'))
	{
		ch = (temp-'0')<<4;
	} 
	else
	{
		if ((temp>='a') && (temp<='f'))
		{
			ch = (temp-'a'+10)<<4;
		}
		else
		{
			ch = (temp-'A'+10)<<4;
		}
	}
		
	temp = RECEIVE_NETWORK_B();
	if ((temp>='0') && (temp<='9'))
	{
		ch |= (temp-'0');
	}
	else
	{
		if ((temp>='a') && (temp<='f'))
		{
			ch |= (temp-'a'+10);
		}
		else
		{
			ch |= (temp-'A'+10);
		}
	}
	return ch;
}

INT16 https_calculatehash (UINT32 len)
{
	UINT8 hash=0;
	UINT8 ch;
	UINT8 i;
	
	
	/* Read Max 60 characters */
	
	if(len > 60)
		len = 60;
	
	for( i=0; i<len; i++)
	{
		ch = RECEIVE_NETWORK_B();
		
		if ( ch ==' ')	/* End reached? 		*/
			break;
		
		/* encoded HTTP URI ? */
		if ( ch == '%')
		{
			ch = https_read_encoded();

			/* is this UNICODE char encoded? (for now allow only
			 * one byte encoded UNICODE chars)
			 */
			if ( ( ch & 0xe0 ) == 0xc0 )
			{
				/* yes */
				ch = ( ch & 0x1F ) << 6; 
				(void)RECEIVE_NETWORK_B();	/* skip first % */
				ch |= (https_read_encoded() & 0x3F);
			}
		}

		hash *= 37;
		hash += ch;
	
	}
	
	if(i==len)
		return(-1);
	
	/* construct address for Hash table */
	
	if(hash == 0)		/* User asked defaul file	*/
	{
		/* Set hash to index.html value */
		
		hash = 0x0B;
	
	}
	
	/* Now we have hash value calculated		*/
	
	return( hash );

}


