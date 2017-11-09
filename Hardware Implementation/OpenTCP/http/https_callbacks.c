#include"datatypes.h"
#include"debug.h"
#include"globalvariables.h"
#include"system.h"
#include"http_server.h"

#include <string.h>
#include "main.h"

/** \brief File not found message
 *
 *	Message that will be displayed if a file with appropriate name (hash
 *	value) was not found.
 */
//>>>>> 
const char https_not_found_page[] = "HTTP/1.0 200 OK\r\nLast-modified: Mon, 30 Jul 2004 15:02:45 GMT\r\nServer: ESERV-10/1.0\nContent-type: text/html\r\nContent-length: 400\r\n\r\n<HEAD><TITLE>Viola Systems Embedded WEB Server</TITLE></HEAD><BODY><H2>HTTP 1.0 404 Error. File Not Found</H2>The requested URL was not found on this server.<HR><BR><I>Viola Systems Embedded WEB Server 2.01, 2004<BR><A HREF=http://192.168.1.100/index.html>Did you wish 192.168.1.100/index.html?</I></A><BR><A HREF=http://www.violasystems.com>www.violasystems.com - Embedding The Internet</A></BODY>";

/** \brief Brief function description here
 * 	\author 
 *		\li Jari Lahti (jari.lahti@violasystems.com)
 *	\date 09.10.2002
 *	\param hash Calculated file-name hash value. Used so that the whole
 *		file name doesn't need to be stored in RAM
 *	\param ses HTTP session identifier
 *	\return
 *		\li -1 - This function should return -1 if no file has been found
 *		\li 1 - This function should return 1 if a file with appropriate
 *			hash value has been found.
 *	\warning
 *		\li This function <b>MUST</b> be implemented by user application
 *		to work with local configuration
 *
 *	This function is invoked by the HTTP server once a hash value of a 
 *	requested file name has been calculated. User application uses this
 *	hash value to check if appropriate file is available to web server.
 *	Appropriate https session entry is then filled accordingly.	
 *
 */
const char *FAT_HEADER_ID = "WSD: SofTec Microsystems";
INT16 https_findfile (UINT8 len, UINT8 ses)
{
UINT32 i;
char buf[24];
unsigned long addr;

struct {
  char filename[24];
  unsigned long file_lenght;
  unsigned long file_addr;
}FAT;

  
flash_read_buffer(0x0000, (unsigned char *)buf, strlen(FAT_HEADER_ID));
if(strncmp((const char *)buf, FAT_HEADER_ID, 10) == 0)
    {     
    for(i=0; i<len; i++)
        {
        if(i == 24)
            break;
        buf[i] = RECEIVE_NETWORK_B();
        if(buf[i] == ' ')
          break;
        }
    buf[i] = '\0';
    if(buf[0] == '\0')
      strcpy(buf, "index.html"); 
           
    addr = 64;
    for(;;)
        {
        flash_read_buffer(addr, (unsigned char *)&FAT, sizeof(FAT));
        if(FAT.filename[0] == 0)
          break;
        if(strcmp(buf, FAT.filename) == 0)
            {
        		https[ses].fstart = FAT.file_addr;
        		https[ses].funacked  = 0;
        		https[ses].flen = FAT.file_lenght;
        		https[ses].fpoint = 0;
            return(1);
            }
        addr += sizeof(FAT);        
        }
    }
https[ses].fstart = 0xFFFFFFFF;
https[ses].funacked  = 0;
https[ses].flen = strlen(&https_not_found_page[0]);
https[ses].fpoint = 0;
	
return(-1);
}

/** \brief Fill network transmit buffer with HTTP headers&data
 * 	\author
 *		\li Jari Lahti (jari.lahti@violasystems.com)
 *	\date 09.10.2002
 *	\param ses HTTP session identifier
 *	\param buf Pointer to buffer where data is to be stored
 *	\param buflen Length of the buffer in bytes
 *	\return
 *		\li >=0 - Number of bytes written to buffer
 *	\warning 
 *		\li This function <b>MUST</b> be implemented by user application
 *		to work with local configuration
 *
 *	This handlers' job is to fill the buffer with the data that web server
 *	should return back through the TCP connection. This is accomplished
 *	based session identifer and values of variables in appropriate
 *	https entry.
 */
INT16 https_loadbuffer (UINT8 ses, UINT8* buf, UINT16 buflen) 
{
	UINT32 i, len;


	if( https[ses].fstart == 0xFFFFFFFF )
	{
		/* Error site asked	*/
		
		kick_WD();
		
		for(i=0; i < (https[ses].flen - https[ses].fpoint); i++)
		{
			if(i >= buflen)
				break;
		
			*buf++ = https_not_found_page[https[ses].fpoint + i];
		
		}
		
		return((INT16)i);
	
	}

	/* Access some storage media (internal/external flash...)*/
	len = https[ses].flen - https[ses].fpoint;



	if(len >= buflen)
	    len = buflen;
		
	flash_read_buffer(https[ses].fstart+https[ses].fpoint, buf, (int)len);
	return((INT16)len);

}

