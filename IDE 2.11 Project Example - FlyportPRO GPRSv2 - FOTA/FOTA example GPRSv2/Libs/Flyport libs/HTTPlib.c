/**
\addtogroup GSM GSM/GPRS
@{
*/
/* **************************************************************************																					
 *                                OpenPicus                 www.openpicus.com
 *                                                            italian concept
 * 
 *            openSource wireless Platform for sensors and Internet of Things	
 * **************************************************************************
 *  FileName:        HTTPlib.c
 *  Dependencies:    Microchip configs files
 *  Module:          FlyPort GPRS/3G
 *  Compiler:        Microchip C30 v3.12 or higher
 *
 *  Author               Rev.    Date              Comment
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Gabriele Allegria    1.0     02/08/2013		   First release  (core team)
 *  Simone Marra
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  Software License Agreement
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  This is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License (version 2) as published by 
 *  the Free Software Foundation AND MODIFIED BY OpenPicus team.
 *  
 *  ***NOTE*** The exception to the GPL is included to allow you to distribute
 *  a combined work that includes OpenPicus code without being obliged to 
 *  provide the source code for proprietary components outside of the OpenPicus
 *  code. 
 *  OpenPicus software is distributed in the hope that it will be useful, but 
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details. 
 * 
 * 
 * Warranty
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * WE ARE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 **************************************************************************/
/// @cond debug
#include "HWlib.h"
#include "Sara.h"
#include "GSMData.h"
#include "TCPlib.h"
#include "HTTPlib.h"

//extern void callbackDbg(BYTE smInt);
extern void gsmDebugPrint(char*);

//extern int Cmd;
extern int mainGSMStateMachine;
extern OpStat	mainOpStatus;
extern GSMModule mainGSM;

extern int CheckCmd(int countData, int chars2read, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);
extern int CheckEcho(int countData, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);
extern void CheckErr(int result, BYTE* smInt, DWORD* tickUpdate);
extern char msg2send[200];

static BYTE smInternal = 0; // State machine for internal use of callback functions
static BYTE maxtimeout = 2;

static int  xHTTPCode = 0;
static char* httpReqUrl;
static BYTE httpReqType = 0;
static char* httpParams;
static char* httpWriteBuffer;
static char* httpReadBuffer;
static int httpReadBufferCount;
/// @endcond

/**
\defgroup HTTP
@{
Provides functions to send HTTP Get and POST request, poll socket status and read response data
*/

/**
 * Opens a connection to HTTP host
 * \param sock TCP_SOCKET to use. <I>Please, remember to use the "&" operator</I>
 * \param httphost name of HTTP host, or its IP address
 * \param httpport name of HTTP port
 * \return None
 */
void HTTPOpen(TCP_SOCKET* sock, char* httphost, char* httpport)
{
	return TCPClientOpen(sock, httphost, httpport);
}

/**
 * Opens a TLS connection to HTTP host
 * \param sock TCP_SOCKET to use. <I>Please, remember to use the "&" operator</I>
 * \param httphost name of HTTP host, or its IP address
 * \param httpport name of HTTP port
 * \return None
 */
void HTTPOpenTLS(TCP_SOCKET* sock, char* httphost, char* httpport)
{
	return TCPClientOpenTLS(sock, httphost, httpport);
}

/**
 * Closes connection to HTTP host
 * \param sock TCP_SOCKET to use
 * \return None
 */
void HTTPClose(TCP_SOCKET* sock)
{
	return TCPClientClose(sock);
}

/**
 * Fills TCP_SOCKET struct with updated values.
 * \param sock TCP_SOCKET to update. <I>Please, remember to use the "&" operator</I>
 * \return None
 */
void HTTPStatus(TCP_SOCKET* sock)
{
	TCPStatus(sock);
}

/**
 * Sends a HTTP Request to host
 * \param sock TCP_SOCKET to use. <I>Please, remember to use the "&" operator</I>
 * \param type method to use
 <UL>
	<LI>HTTP_GET</LI>
	<LI>HTTP_POST</LI>
 </UL>
 * \param reqUrl URL of the request
 * \param data2snd data to send with request
 * \param param addictional parameters to add to request. It can be used HTTP_NO_PARAM to ignore this feature
 * \return None
 */
void HTTPRequest(TCP_SOCKET* sock, BYTE type, char* reqUrl, char* data2snd, char* param)
{
	BOOL opok = FALSE;
	
	if(mainGSM.HWReady != TRUE)
		return;
	
	//	Function cycles until it is not executed
	while (!opok)
	{
		while (xSemaphoreTake(xSemFrontEnd,0) != pdTRUE);		//	xSemFrontEnd TAKE

		// Check mainOpStatus.ExecStat
		if (mainOpStatus.ExecStat != OP_EXECUTION)
		{		
			mainOpStatus.ExecStat = OP_EXECUTION;
			mainOpStatus.Function = 27;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xHTTPCode = 0;
			xSocket = sock;
			
			httpReqUrl = reqUrl;
			httpReqType = type;
			httpWriteBuffer = data2snd;
			
			if(param != HTTP_NO_PARAM)
			{
				httpParams = param;
			}
			else
			{
				httpParams[0] = '\0';
			}
			
			xQueueSendToBack(xQueue,&mainOpStatus.Function,0);	//	Send COMMAND request to the stack
			
			xSemaphoreGive(xSemFrontEnd);						//	xSemFrontEnd GIVE, the stack can answer to the command
			opok = TRUE;
		}
		else
		{
			xSemaphoreGive(xSemFrontEnd);
			taskYIELD();
		}
	}
}

/// @cond debug
int cHTTPRequest()
{
	char cmdReply[200];
//    const char contentParams[] = "\r\nContent-Type: application/x-www-form-urlencoded\r\nAccept: */*";
    char contentLengthStr[50];
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
	int msgLength = 0;
	static BOOL additionalParamsNeedsCRLF = FALSE;
        
	switch(smInternal)
    {
        case 0:
            // Check if Buffer is free
            if(GSMBufferSize() > 0)
            {
                // Parse Unsol Message
                mainGSMStateMachine = SM_GSM_CMD_PENDING;
                return -1;
            }
            else
                smInternal++;
			/* no break */
        case 1:
            // Send first AT command

            // Calculate tcp write length...
            // Content-Length string (if used...)
            if(strlen(httpWriteBuffer) > 0)
            {
                sprintf(contentLengthStr, "Content-Length: %d\r\n", strlen(httpWriteBuffer));
            }
            else
            {
                contentLengthStr[0] = '\0';
            }
            // Set Content-Length string length to msgLength
            msgLength = strlen(contentLengthStr);

            // now add Header length:
            // GET: strlen("GET ") + strlen(" HTTP/1.1") + strlen("\r\nHost: ") + strlen("\r\n\r\n") + strlen(httpReqUrl)
            // POST: strlen("POST ") + strlen(" HTTP/1.1") + strlen("\r\nHost: ") + strlen("\r\n\r\n") + strlen(httpReqUrl)
            if(httpReqType == HTTP_GET)
                msgLength += (25 + strlen(httpReqUrl));
            else //if(httpReqType == HTTP_POST)
                msgLength += (26 + strlen(httpReqUrl));
            
            // Now add length of message content body
            msgLength += strlen(httpWriteBuffer);
            
            // Now add the length of additional parameters...
            if(httpParams != HTTP_NO_PARAM)
            {
                msgLength += strlen(httpParams);
                // Check if additional params ends with "\r\n"...
                int endParams = strlen(httpParams) -2;
				if(endParams > 0)
				{
					if((httpParams[endParams] != '\r')&&(httpParams[endParams+1] != '\n'))
					{
						additionalParamsNeedsCRLF = TRUE;
						msgLength += 2;
					}
					else
					{
						additionalParamsNeedsCRLF = FALSE;
					}
				}
            }
            
            // ----------	TCP Write Command ----------
            sprintf(msg2send, "AT+USOWR=%d,%d\r",xSocket->number, msgLength);

            GSMWrite(msg2send);
            // Start timeout count
            tick = TickGetDiv64K(); // 1 tick every seconds
            maxtimeout = 10+3;
            smInternal++;
			/* no break */

        case 2:
            vTaskDelay(10);
            // Check ECHO
            countData = 0;

            resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
			/* no break */

        case 3:
            // Check Reply "\r\n@"
			sprintf(msg2send, "\r\n@");
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>@
			
			// here it is not possible to use the "getAnswer" (so neither CheckCmd) function 
			// since there is no '\n' chars...
			// for this case only we have to use a custom loop:
			int lenAnsw = strlen(msg2send);
			waitForGsmMsg(msg2send, maxtimeout);

			GSMpRead(cmdReply, lenAnsw+2);
			cmdReply[lenAnsw+2] = '\0';
			
			if(strstr(cmdReply, msg2send) != NULL)
			{
				// Reply present! Read until the lineNumber occurrence of '\n' char are found...
				int cc = GSMReadN(200, '@', 1, cmdReply);
				cmdReply[cc] = '\0';
		
				resCheck = 0;
			}
			else
            {
				resCheck = 1; // it means ERROR
				tick = TickGetDiv64K();
                CheckErr(resCheck, &smInternal, &tick);
			}
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            else
            {
            	// On documentation it is stated we should wait at least 50ms before to write out data...
            	vTaskDelay(5);

                // Write data to TCP:
                if(httpReqType == HTTP_GET)
                {
                    GSMWrite("GET ");
                }
                else
                {
                    GSMWrite("POST ");
                }

                char* req = strstr(httpReqUrl, "/");
                GSMWrite(req);
                GSMWrite(" HTTP/1.1\r\nHost: ");
                
				int tokenPos;
				for(tokenPos = 0; tokenPos < strlen(httpReqUrl); tokenPos++)
				{
					if(httpReqUrl[tokenPos] == '/')
					{
						break;
					}
					GSMWriteCh(httpReqUrl[tokenPos]);
				}
				GSMWrite("\r\n");
				
                // Write Content-Length string (if any)
                if(strlen(httpWriteBuffer) > 0)
                {
                    GSMWrite(contentLengthStr);
                }
                if(httpParams != HTTP_NO_PARAM)
                {
                    GSMWrite(httpParams);
                    if(additionalParamsNeedsCRLF)
                    {
                    	GSMWrite("\r\n");
                    	additionalParamsNeedsCRLF = FALSE;
                    }
                }
                GSMWrite("\r\n");
                GSMWrite(httpWriteBuffer);
            }
			/* no break */
            
        case 4:
        	smInternal = 4;
            // Get socket write event:
            // +USOWR: <socket>,<length>
			vTaskDelay(20);
			sprintf(msg2send, "\r\n+USOWR");
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>+USOWR: <socket>,<length><CR><LF>
			
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				// Get status
				char temp[25];
				int res = getfield(':', ',', 10, 1, cmdReply, temp, 500);
				if(res != 1)
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield #1 for +USOWR\r\n");
					break;
				}
				else
				{
                    if(xSocket->number != atoi(temp))
                    {
                        gsmDebugPrint("Errors on getfield #1 for +USOWR: unknown socket number!\r\n");
                    }
				}
				
				// Get tcp_notif
				res = getfield(',', '\r', 20, 1, cmdReply, temp, 500);
				if(res != 1)
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield #2 for +USOWR\r\n");
					break;
				}
				else
				{
                    if(msgLength != atoi(temp))
                    {
                        gsmDebugPrint("Warning on getfield #2 for +USOWR: send a different number of bytes on TCP socket!\r\n");
                    }
				}
			}	
			/* no break */

        case 5:
            // Get reply (\r\nOK\r\n)
            vTaskDelay(20);
            // Get OK
            sprintf(msg2send, "\r\nOK\r\n");//sprintf(msg2send, "OK\r\n");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>OK<CR><LF>
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
			/* no break */

        default:
            // Check if Buffer is free (I have to parse +UUSORD)...
            if(GSMBufferSize() > 0)
            {
                // Parse Unsol Message
                mainGSMStateMachine = SM_GSM_CMD_PENDING;
                return -1;
            }
            break;

    }// switch(smInternal)
    
	smInternal = 0;
	// Cmd = 0 only if the last command successfully executed
	mainOpStatus.ExecStat = OP_SUCCESS;
	mainOpStatus.Function = 0;
	mainOpStatus.ErrorCode = 0;
	mainGSMStateMachine = SM_GSM_IDLE;
	return -1;
}	
/// @endcond

/**
 * Reads data form http socket
 * \param sock TCP_SOCKET to use. <I>Please, remember to use the "&" operator</I>
 * \param dataBuffer data buffer to fill
 * \param datalen amount of BYTEs to read
 * \return None
 */
void HTTPReadData(TCP_SOCKET* sock, char* dataBuffer, int datalen)
{
	httpReadBuffer = dataBuffer;
	httpReadBufferCount = datalen;
	TCPRead(sock, httpReadBuffer, httpReadBufferCount);
}	

/*! @} */

/*! @} */
