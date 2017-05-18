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
 *  FileName:        TCPlib.c
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

extern void callbackDbg(BYTE smInt);

//extern int Cmd;
extern int mainGSMStateMachine;
extern OpStat	mainOpStatus;
extern GSMModule mainGSM;

extern int CheckCmd(int countData, int chars2read, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);
extern int CheckEcho(int countData, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);
extern void CheckErr(int result, BYTE* smInt, DWORD* tickUpdate);
extern void gsmDebugPrint(char*);
extern int EventType;
extern char msg2send[200];
static BYTE smInternal = 0; // State machine for internal use of callback functions
static BYTE maxtimeout = 2;
static int counterLen = 0;

static char* tcpWriteBuffer;
static int tcpWriteBufferCount;
static char* tcpReadBuffer;
static int tcpReadBufferCount;
static BOOL tcpTLSrequested = FALSE;

/***
 * Converts the ublox tcp socket status value to Flyport TCP Socket status values
 */
int sockStatConversion(int newVal)
{
	/*
	 *
#define		SOCK_NOT_DEF	0
#define		SOCK_NOT_USED 	1
#define		SOCK_OPENING  	2
#define		SOCK_CONNECT  	3
#define		SOCK_CLOSING  	4
#define		SOCK_CLOSED		5

#define		_SOCK_INACTIVE		0
#define		_SOCK_LISTEN		1
#define		_SOCK_SYN_SENT  	2
#define		_SOCK_SYN_RCV		3
#define		_SOCK_ENSTABILISHED	4
#define		_SOCK_FIN_WAIT_1	5
#define		_SOCK_FIN_WAIT_2	6
#define		_SOCK_CLOSE_WAIT	7
#define		_SOCK_CLOSING		8
#define		_SOCK_LAST_ACK		9
#define		_SOCK_TIME_WAIT		10
	 */
	int statVal = SOCK_CLOSED;
	switch(newVal)
	{
	case _SOCK_INACTIVE:
		statVal = SOCK_CLOSED;
		break;

	case _SOCK_SYN_RCV:
	case _SOCK_SYN_SENT:
		statVal = SOCK_OPENING;
		break;

	case _SOCK_ENSTABILISHED:
	case _SOCK_LISTEN:
		statVal = SOCK_CONNECT;
		break;

	case _SOCK_CLOSE_WAIT:
	case _SOCK_CLOSING:
	case _SOCK_FIN_WAIT_1:
	case _SOCK_FIN_WAIT_2:
	case _SOCK_LAST_ACK:
	case _SOCK_TIME_WAIT:
		statVal = SOCK_CLOSING;
		break;

	}

	return statVal;
}
/// @endcond

/**
\defgroup TCP
@{
Provides TCP Client functions
*/


/**
 * Creates a TCP client on specified IP address and port
 * \param sock TCP_SOCKET of reference.
 * \param tcpaddr IP address of the remote server. Example: "192.168.1.100" (the char array must be NULL terminated).
 * \param tcpport Port of the remote server to connect. Example: "1234" (the char array must be NULL terminated).
 * \return - INVALID_SOCKET: the operation was failed. Maybe there are not available sockets.
 * \return - A TCP_SOCKET handle to the created socket. It must be used to access the socket in the program (read/write operations and close socket).
 */
void TCPClientOpen(TCP_SOCKET* sock, char* tcpaddr, char* tcpport)
{
	BOOL opok = FALSE;
	
	if(mainGSM.HWReady != TRUE)
		return;
	
	// Check if sock->number is INVALID SOCKET
	if(sock->number != INVALID_SOCKET)
		return;
			
	//	Function cycles until it is not executed
	while (!opok)
	{
		while (xSemaphoreTake(xSemFrontEnd,0) != pdTRUE);		//	xSemFrontEnd TAKE

		// Check mainOpStatus.ExecStat
		if (mainOpStatus.ExecStat != OP_EXECUTION)
		{		
			mainOpStatus.ExecStat = OP_EXECUTION;
			mainOpStatus.Function = 20;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xTCPPort = atoi(tcpport);
			int termChar = strlen(tcpaddr);
			if(termChar > 100) // xIPAddress array size is 100
				termChar = 100;
			strncpy((char*)xIPAddress, tcpaddr, termChar);
			xIPAddress[termChar] = '\0';
			xSocket = sock;
            tcpTLSrequested = FALSE;
			xSocket->number = INVALID_SOCKET;
			
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


/**
 * Creates a TLS enabled TCP client on specified IP address and port
 * \param sock TCP_SOCKET of reference.
 * \param tcpaddr IP address of the remote server. Example: "192.168.1.100" (the char array must be NULL terminated).
 * \param tcpport Port of the remote server to connect. Example: "1234" (the char array must be NULL terminated).
 * \return - INVALID_SOCKET: the operation was failed. Maybe there are not available sockets.
 * \return - A TCP_SOCKET handle to the created socket. It must be used to access the socket in the program (read/write operations and close socket).
 */
void TCPClientOpenTLS(TCP_SOCKET* sock, char* tcpaddr, char* tcpport)
{
	BOOL opok = FALSE;
	
	if(mainGSM.HWReady != TRUE)
		return;
	
	// Check if sock->number is INVALID SOCKET
	if(sock->number != INVALID_SOCKET)
		return;
			
	//	Function cycles until it is not executed
	while (!opok)
	{
		while (xSemaphoreTake(xSemFrontEnd,0) != pdTRUE);		//	xSemFrontEnd TAKE

		// Check mainOpStatus.ExecStat
		if (mainOpStatus.ExecStat != OP_EXECUTION)
		{		
			mainOpStatus.ExecStat = OP_EXECUTION;
			mainOpStatus.Function = 20;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xTCPPort = atoi(tcpport);
			int termChar = strlen(tcpaddr);
			if(termChar > 100) // xIPAddress array size is 100
				termChar = 100;
			strncpy((char*)xIPAddress, tcpaddr, termChar);
			xIPAddress[termChar] = '\0';
			xSocket = sock;
            tcpTLSrequested = TRUE;
			xSocket->number = INVALID_SOCKET;
			
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
//****************************************************************************
//	Only internal use:
//	cTCPClientOpen callback function
//****************************************************************************
int cTCPClientOpen()
{
	char cmdReply[200];
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
    IP_ADDR tempIp;
    char tempIpStr[25];
    BOOL dnsIsNeeded = TRUE;
	
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
        	// We have to add the DNS calculation if provided IP Address should be resolved, so we have to:
			// 1. check if xIPAddress is a IP address or a domain name (we can use the StringToIPAddress function for this...)
			// 2. if needed, require DNS resolution to the module AT+UDNSRN=<resolution_type>,<domain_string>
			// 3. Check for response +UDNSRN: "resolved_ip_address" + \r\nOK\r\n
							// Now we are sure we have the IP address!
			// 4. Create a socket with AT+USOCR=<protocol>[,<local_port>]
			// 5. Receive socket number + \r\nOK\r\n
			// 6. Connect tcp socket AT+USOCO=<socket>,<remote_addr>,<remote_port>
			// 7. Read results and finish!
            {
                if(StringToIPAddress((BYTE*)xIPAddress, &tempIp) == TRUE)
                {
                    dnsIsNeeded = FALSE;
                    // Provided string is a IP Address, we can jump to step 5!
                    smInternal = 5;
                }
                else
                {
                    dnsIsNeeded = TRUE;
                    // We have to use DNS to resolve IP Address
                    // AT+UDNSRN=0,"www.google.com"
                    // +UDNSRN: "216.239.59.147"
                    // OK
                    sprintf(msg2send, "AT+UDNSRN=0,\"%s\"\r", xIPAddress);
                    GSMWrite(msg2send);
                    // Start timeout count
                    tick = TickGetDiv64K(); // 1 tick every seconds
                    maxtimeout = 10;
                    smInternal++;
                }
            }
            /* no break */
			
		case 2:
            if(dnsIsNeeded)
            {
                vTaskDelay(1);
                // Check ECHO 
                countData = 0;

                resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);

                CheckErr(resCheck, &smInternal, &tick);

                if(resCheck)
                {
                    return mainOpStatus.ErrorCode;
                }
            }
            /* no break */
			
		case 3:
            if(dnsIsNeeded)
            {
                // Get reply +UDNSRN: "<ip_address_string>"
                vTaskDelay(1);
                sprintf(msg2send, "\r\n+UDNSRN: ");
                chars2read = 2;
                countData = 0; // countData = 2; // GSM buffer should be: <CR><LF>+UDNSRN: "<ip_address_string>"<CR><LF>
                maxtimeout = 60 + 2; // over 1 minute to resolve DNS
                resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

                CheckErr(resCheck, &smInternal, &tick);

                if(resCheck)
                {
                    return mainOpStatus.ErrorCode;
                }
                else
                {
                    // Get ip address string:
                    int res = getfield('\"', '\"', 24, 1, cmdReply, tempIpStr, 500);
                    if(res != 1)
                    {
                        // Execute Error Handler
                        gsmDebugPrint( "Error in getfield\r\n");
                        break;
                    }
                    else
                    {
                        if(StringToIPAddress((BYTE*)tempIpStr, &tempIp))
                        {
                        	/* INDBG
                            _dbgwrite("ip string converted: ");
                            _dbgline(tempIpStr);
                            // indbg */
                        }
                        else
                            tempIp.Val = 0;
                    }	
                }	
            }
            /* no break */
			
		case 4:
            if(dnsIsNeeded)
            {
                // Get reply (\r\nOK\r\n)
                vTaskDelay(1);
                // Get OK
                sprintf(msg2send, "\r\nOK\r\n");
                chars2read = 2;
                countData = 0; // GSM buffer should be: <CR><LF>OK<CR><LF>
                resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

                CheckErr(resCheck, &smInternal, &tick);

                if(resCheck)
                {
                    return mainOpStatus.ErrorCode;
                }
            }
            /* no break */

        case 5:
            // TCP Socket Creation:
            // AT+USOCR=<protocol>[,<local_port>] - <protocol>: param 6 means TCP, param 17 UDP
			sprintf(msg2send, "AT+USOCR=6\r");
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;
            /* no break */

		case 6:
			vTaskDelay(1);
			// Check ECHO
			countData = 0;

			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            /* no break */

        case 7: 
            // Get reply 
			vTaskDelay(1);
			sprintf(msg2send, "\r\n+USOCR: ");
			chars2read = 2;
			countData = 0; // countData = 2; // GSM buffer should be: <CR><LF>+USOCR: <socket_number><CR><LF>
			
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				// Get ip address string:
				int res = getfield(':', '\r', 10, 1, cmdReply, tempIpStr, 500);
                tempIpStr[10] = '\0';
				if(res != 1)
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield\r\n");
					break;
				}
				else
				{
                    xSocket->number = atoi(tempIpStr);
				}	
			}	
            /* no break */
            
		case 8:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
			// Get OK
			sprintf(msg2send, "\r\nOK\r\n");
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            /* no break */
            
        case 9:
            if(tcpTLSrequested)
            {
                // set TLS flag on current TCP socket:
                // AT+USOSEC=<socket>,<ssl_tls_status>[,<usecmng_profile_id>]
                sprintf(msg2send, "AT+USOSEC=%d,1,0\r", xSocket->number);
                GSMWrite(msg2send);
                // Start timeout count
                tick = TickGetDiv64K(); // 1 tick every seconds
                maxtimeout = 2;
                smInternal++;
            }
            /* no break */
            
        case 10:
            if(tcpTLSrequested)
            {
                vTaskDelay(1);
                // Check ECHO
                countData = 0;

                resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);

                CheckErr(resCheck, &smInternal, &tick);

                if(resCheck)
                {
                    return mainOpStatus.ErrorCode;
                }
            }
            /* no break */
            
        case 11:
            if(tcpTLSrequested)
            {
                // Get reply (\r\nOK\r\n)
                vTaskDelay(1);
                // Get OK
                sprintf(msg2send, "\r\nOK\r\n");
                chars2read = 2;
                countData = 0; // GSM buffer should be: <CR><LF>OK<CR><LF>
                resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

                CheckErr(resCheck, &smInternal, &tick);

                if(resCheck)
                {
                    return mainOpStatus.ErrorCode;
                }
            }
            /* no break */
                        
			
        case 12:
			// ----------	Initiate TCP Connection	----------
            // AT+USOCO=<socket>,<remote_addr>,<remote_port>
            IPAddressToString(tempIp, tempIpStr);
            /* INDBG
            _dbgwrite("restored IP string: ");
            _dbgline(tempIpStr);
            // indbg */
			sprintf(msg2send, "AT+USOCO=%d,\"%s\",%d\r",xSocket->number, tempIpStr, xTCPPort);
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 70;
			smInternal++;
            /* no break */
			
        case 13:
			vTaskDelay(1);
			// Check ECHO 
			countData = 0;
			
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
						
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				xSocket->number = INVALID_SOCKET;
				return mainOpStatus.ErrorCode;
			}
            /* no break */
			
        case 14:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
			// Get OK
			sprintf(msg2send, "\r\nOK\r\n");//sprintf(msg2send, "\r\nOK\r\n");
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				xSocket->number = INVALID_SOCKET;
				return mainOpStatus.ErrorCode;
			}
			else
			{
				xSocket->status = SOCK_CONNECT;
			}
            /* no break */
			
		default:
			break;
	
	}
	
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
 * Closes the client socket specified by the handle.
 * \param sock The handle of the socket to close (the handle returned by the command TCPClientOpen).
 * \return None.
 */
void TCPClientClose (TCP_SOCKET* sock)
{
	BOOL opok = FALSE;
	
	if(mainGSM.HWReady != TRUE)
		return;
	
	if(sock->number == INVALID_SOCKET)
		return;
	
	//	Function cycles until it is not executed
	while (!opok)
	{
		while (xSemaphoreTake(xSemFrontEnd,0) != pdTRUE);		//	xSemFrontEnd TAKE

		// Check mainOpStatus.ExecStat
		if (mainOpStatus.ExecStat != OP_EXECUTION)
		{		
			mainOpStatus.ExecStat = OP_EXECUTION;
			mainOpStatus.Function = 21;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xSocket = sock;
			
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
//****************************************************************************
//	Only internal use:
//	cTCPClientClose callback function
//****************************************************************************
int cTCPClientClose()
{
	char cmdReply[200];
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
	
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
		case 1:	
			// Send first AT command
			// ----------	TCP Connection Close	----------
            // AT+USOCL=<socket>
			sprintf(msg2send, "AT+USOCL=%d\r",xSocket->number);
			
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 60 + 2;
			smInternal++;
            /* no break */
			
		case 2:
			vTaskDelay(20);
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
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
			// Get OK
			sprintf(msg2send, "\r\nOK\r\n");
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			} // else continues above after the comments
//			
//		case 4:	
//			// Send AT command
//			// ----------	Delete TCP Socket ----------
//			sprintf(msg2send, "AT+KTCPDEL=%d\r",xSocket->number);
//			
//			GSMWrite(msg2send);
//			// Start timeout count
//			tick = TickGetDiv64K(); // 1 tick every seconds
//			maxtimeout = 2+2;
//			smInternal++;
//			
//		case 5:
//			vTaskDelay(20);
//			// Check ECHO 
//			countData = 0;
//			
//			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
//						
//			CheckErr(resCheck, &smInternal, &tick);
//			
//			if(resCheck)
//			{
//				return mainOpStatus.ErrorCode;
//			}
//			
//		case 6:
//			// Get reply (\r\nOK\r\n)
//			vTaskDelay(1);
//			// Get OK
//			sprintf(msg2send, "\r\nOK\r\n");
//			chars2read = 2;
//			countData = 0; // GSM buffer should be: <CR><LF>OK<CR><LF>
//			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
//			
//			CheckErr(resCheck, &smInternal, &tick);
//			
//			if(resCheck)
//			{
//				return mainOpStatus.ErrorCode;
//			}
			else
			{
				xSocket->number = INVALID_SOCKET;
                xSocket->status = SOCK_CLOSED;
			}
            /* no break */
				
		default:
			break;
	
	}
	
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
 * Reads the specified number of characters from a TCP socket and puts them into the specified char array. <B>NOTE:</B> This function flushes the buffer after reading!
 * \param sock The handle of the socket to read (the handle returned by the command TCPClientOpen or TCPServerOpen).
 * \param readch The char array to fill with the read characters.
 * \param rlen The number of characters to read. 
 * \warning The length of the array must be AT LEAST = rlen+1, because at the end of the operation the array it's automatically NULL terminated (is added the '\0' character).
 * \return None.
 */
void TCPRead(TCP_SOCKET* sock , char* readch , int rlen)
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
			mainOpStatus.Function = 24;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xSocket = sock;
			tcpReadBuffer = readch;
			
			// Set max of socket buffer size
			if(rlen > 1024)//1460)
				rlen = 1024;//1460;
			tcpReadBufferCount = rlen;
			
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
//****************************************************************************
//	Only internal use:
//	cTCPRead callback function
//****************************************************************************
int cTCPRead()
{
	char cmdReply[200];
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
	
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
			// Read TCP DATA:
            // AT+USRD=<socket>,<length>\r
			sprintf(msg2send, "AT+USORD=%d,%d\r",xSocket->number, tcpReadBufferCount);
			
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 60;
			smInternal++;
            /* no break */
			
		case 2:
			vTaskDelay(20);
			// Check ECHO 
			countData = 0;
			
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				#if defined (STACK_USE_UART)
				char debugStr[100];
				sprintf(debugStr, "error on function %d, smInternal %d\r\n", mainOpStatus.Function, smInternal);
				gsmDebugPrint( debugStr);
				vTaskDelay(20);
				#endif
				return mainOpStatus.ErrorCode;
			}
            /* no break */
			
		case 3:
			// Get reply "\r\n+USORD: <socket>,<length>,\"<data in the ASCII [0x00,0xFF] range>\"\r\n"
			vTaskDelay(20);
			sprintf(msg2send, "\r\n+USORD: ");
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>+USORD: <socket>,<length>,\"<data in the ASCII [0x00,0xFF] range>\"<CR><LF>

			//while((GSMBufferSize() < strlen(msg2send) && ((TickGetDiv64K()-tick) < maxtimeout)))
			//{
			//	vTaskDelay(100);
			//}

            //			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
            // using GSMReadN we can read data without clearing a the buffer, searching for a specific char (counting it a provided number of times)
            
			// here it is not possible to use the "getAnswer" (so neither CheckCmd) function 
			// since there is no '\n' chars...
			// for this case only we have to use a custom loop:
			int lenAnsw = strlen(msg2send);

			maxtimeout = 20;
			waitForGsmLines(chars2read, maxtimeout);
			int readCount = GSMpRead(cmdReply, lenAnsw+2);
			cmdReply[lenAnsw+2] = '\0';
            if((readCount > 0)&&(readCount < lenAnsw+2))
                cmdReply[readCount] = '\0';
            
			if(strstr(cmdReply, msg2send) != NULL)
			{
                // Reply present! Read until the first ':' char is found...
				int cc = GSMReadN(200, ':', 1, cmdReply);
				cmdReply[cc] = '\0';
//                _dbgwrite("cmdReply #2: "); _dbgline(cmdReply);
				resCheck = 0;        

				// Here there are 2 different cases:
				// "\r\n+USORD: 0,length,"data to be read"\r"
				//	or
				// "\r\n+USORD: 0,0\r" and no data available...

				// I should read data length BEFORE to search for '\"' character!!
				// So, I have to use GSMpRead to read some chars from buffer (without removing it) and
				// execute a atoi to read actual data length!

				cc = GSMpRead(cmdReply, 10); // a data length greater than 1024 is not possible...
				cmdReply[cc] = '\0';

                // Get socket
				char temp[25];
				int res = getfield(' ', ',', 5, 1, cmdReply, temp, 500);
				if((res != 1)||(xSocket->number != atoi(temp)))
				{              
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield #1 for +USORD socket\r\n");
					break;
				}
				else
				{
//					xSocket->status = atoi(temp);
				}

                // Get length
				char* lenPtr = strstr(cmdReply, ","); // point the lenPtr to the first occurrence of ',' char
				lenPtr++;
				int lenVal = atoi(lenPtr);
//				_dbgwrite("lenPrt content: "); _dbgline(lenPtr);
				if(lenVal >= 0)
				{
					tcpReadBufferCount = lenVal;
					char breakChar = '\"';
					if(lenVal == 0) // read until '\r' is found
					{
						breakChar = '\r';
					}

					cc = GSMReadN(200, breakChar, 1, cmdReply); // Now we have removed all chars from buffer that are not tcp data!
					cmdReply[cc] = '\0';

				}
				else
				{
					gsmDebugPrint("Error reading tcp read data length!\r\n");
					res = 0;
					break;
				}

// get data!
                if(tcpReadBufferCount > 0)
                {
                    tick = TickGetDiv64K(); // 1 tick every seconds
                    maxtimeout = 10;
                    int rxlen = GSMBufferSize();
                    while((rxlen < tcpReadBufferCount)&&(TickGetDiv64K()<(tick+maxtimeout)))
                    {
                        // update timeout tick
                        vTaskDelay(1);
                        rxlen = GSMBufferSize();
                    }
                    //GSMReadN(tcpReadBufferCount, '\r', 1, (char*)tcpReadBufferCount);
                    rxlen = GSMBufferSize();
                    if(rxlen >= tcpReadBufferCount)
                    {
                        int bytesRead = GSMRead(tcpReadBuffer, tcpReadBufferCount);
                        if(bytesRead > 0)
                            tcpReadBuffer[bytesRead] = '\0';
//                        _dbgwrite("Data read: "); _dbgline(tcpReadBuffer);
                        
                        // now flush 3 bytes from GSM buffer to remove last string: "\"\r\n"
                        char dummy[4];
                        int currRead = 0;
                        while((currRead < 3)&&(TickGetDiv64K()<(tick+maxtimeout)))
						{
                        	int rl = GSMRead(dummy, 3);
                        	if(rl > 0)
                        		currRead += rl;
                        	else if(rl < 0) // overflow, we don't bother about it now! // FIXME: check what to do with overflow!
                        		currRead -= rl;
						}
                        if(currRead < 3)
                        {
                        	_dbgline("Problems flushing dummy chars after cTCPRead data reading...");
                        }
                        dummy[3] = '\0';
//                        _dbgwrite("Removed from buffer: "); _dbgline(dummy);
//                        _dbghex("Removed from buffer:", dummy, 3);
                        smInternal++;
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
                }
                else
                {
                	// now flush 1 more byte from GSM buffer to remove last string: "\n"
					char dummy[4];
					int currRead = 0;
					while((currRead < 1)&&(TickGetDiv64K()<(tick+maxtimeout)))
					{
						int rl = GSMRead(dummy, 1);
						if(rl > 0)
							currRead += rl;
						else if(rl < 0) // overflow, we don't bother about it now! // FIXME: check what to do with overflow!
							currRead -= rl;
					}
					if(currRead < 1)
					{
						_dbgline("Problems flushing dummy chars after cTCPRead data reading...");
					}
					dummy[1] = '\0';
//                        _dbgwrite("Removed from buffer: "); _dbgline(dummy);
//					_dbghex("Removed from buffer:", dummy, 3);
					smInternal++;
                }
			}
			else
            {
				resCheck = 1; // it means ERROR
				tick = TickGetDiv64K();
                CheckErr(resCheck, &smInternal, &tick);

                if(resCheck)
				{
					return mainOpStatus.ErrorCode;
				}
            }
            /* no break */
            
			
		case 4:
			smInternal = 4;
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
			// Get OK
			sprintf(msg2send, "\r\nOK\r\n");
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			
		case 5:
			// Send first AT command
			// Read TCP DATA:
            // AT+USRD=<socket>,<length>\r
			sprintf(msg2send, "AT+USORD=%d,0\r",xSocket->number);
			
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 6; // 60
			smInternal++;
            /* no break */
			
		case 6:
			vTaskDelay(20);
			// Check ECHO 
			countData = 0;
			maxtimeout = 6; // 60
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
			if(resCheck)
			{
				#if defined (STACK_USE_UART)
				char debugStr[100];
				sprintf(debugStr, "error on function %d, smInternal %d\r\n", mainOpStatus.Function, smInternal);
				gsmDebugPrint( debugStr);
				vTaskDelay(20);
                sprintf(debugStr, "Problems on cTCPRead parsing at smInternal %d with cmdReply content: %s", smInternal, cmdReply);
                gsmDebugPrint( debugStr);
                vTaskDelay(20);
				#endif
			}
			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            /* no break */
			
		case 7:
			// Get reply "\r\n+USORD: <socket>,<length>,\"<data in the ASCII [0x00,0xFF] range>\"\r\n"
			vTaskDelay(20);
			sprintf(msg2send, "\r\n+USORD");
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>+USORD: <socket>,<length>,\"<data in the ASCII [0x00,0xFF] range>\"<CR><LF>
			
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);

			if( 	(resCheck == 3) &&
					(mainOpStatus.ExecStat == OP_CME_ERR) &&
					(xSocket->status != SOCK_CONNECT))/*&& (mainOpStatus.ErrorCode == 3)) // Error 3 is raised when Socket Closed?! */
			{
				// Avoid to handle such error, we assume it is not a problem in this condition!
				resCheck = 0;
				mainOpStatus.ExecStat = OP_EXECUTION;
				mainOpStatus.ErrorCode = 0;
				// Clear event type:
				EventType = NO_EVENT;
				mainGSMStateMachine = SM_GSM_CMD_PENDING;
				smInternal=9; // Avoid the search of OK string in this case!
				break;
			}
			else if(resCheck != 0)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				// Get socket
				char temp[25];
				int res = getfield(':', ',', 5, 1, cmdReply, temp, 500);
				if((res != 1)||(xSocket->number != atoi(temp)))
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield for +USORD socket\r\n");
					break;
				}
				else
				{
//					xSocket->status = atoi(temp);
				}
				
				// Get length
				res = getfield(',', '\r', 5, 1, cmdReply, temp, 500);
				if(res != 1)
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield for +USORD socket\r\n");
					break;
				}
				else
				{
                    xSocket->rxLen = atoi(temp);
                    if(xSocket->rxLen > 1024)
                        xSocket->rxLen= 1024;
				}
			}	
            /* no break */
			
		case 8:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
			// Get OK
			sprintf(msg2send, "\r\nOK\r\n");
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
			break;
	}
	
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
 * Writes an array of characters on the specified socket.
 * \param sock The socket to which data is to be written (it's the handle returned by the command TCPClientOpen or TCPServerOpen).
 * \param writech Pointer to the array of characters to be written.
 * \param wlen The number of characters to write.
 * \return The number of bytes written to the socket. If less than len, the buffer became full or the socket is not conected.
 */
void TCPWrite(TCP_SOCKET* sock , char* writech , int wlen)
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
			mainOpStatus.Function = 23;
			mainOpStatus.ErrorCode = 0;

			// Set Params
			xSocket = sock;
			tcpWriteBuffer = writech;
			tcpWriteBufferCount = wlen;

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
//****************************************************************************
//	Only internal use:
//	cTCPWrite callback function
//****************************************************************************
int cTCPWrite()
{
	char cmdReply[200];
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
    counterLen = 0;
	
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
			// ----------	TCP Write Command ----------
			sprintf(msg2send, "AT+USOWR=%d,%d\r",xSocket->number, tcpWriteBufferCount);
			
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 10+3;
			smInternal++;
            /* no break */
			
		case 2:
			vTaskDelay(20);
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
			countData = 0;
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
            /* no break */
            
        case 4:
//			sprintf(msg2send, "\r\n@");
//			chars2read = 2;
//			countData = 4; // GSM buffer should be: <CR><LF>@<CR><LF>
//			
//			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
//			
//			CheckErr(resCheck, &smInternal, &tick);
//			
//			if(resCheck)
//			{
//				return mainOpStatus.ErrorCode;
//			}
//			else
			{
            	// On documentation it is stated we should wait at least 50ms before to write out data...
            	vTaskDelay(5);
				while((counterLen < tcpWriteBufferCount) && (GSMBufferSize() == 0))
				{
					GSMWriteCh(tcpWriteBuffer[counterLen]);
					counterLen++;
				}		
                if(GSMBufferSize() > 0)
                {
                    // Check if NO CARRIER message arrived
                    smInternal++;
                }
				// and write --EOF--Pattern-- (without \r)
				// No need of "end of file pattern" on SARA Modules GSMWrite("--EOF--Pattern--");
			}	
            /* no break */
	
		case 5:
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
					//xSocket->status = atoi(temp);
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
					//xSocket->notif = atoi(temp);
                    if(tcpWriteBufferCount != atoi(temp))
                    {
                        gsmDebugPrint("Warning on getfield #2 for +USOWR: send a different number of bytes on TCP socket!\r\n");
                    }
				}
			}	
            /* no break */

        case 6:
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
			break;
	
	}
	
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
 * Updates the status of a TCP socket.
 * \param sock The handle of the socket to control.
 * \return none
 */
void TCPStatus(TCP_SOCKET* sock)
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
			mainOpStatus.Function = 22;
			mainOpStatus.ErrorCode = 0;

			// Set Params
			xSocket = sock;

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
//****************************************************************************
//	Only internal use:
//	cTCPStatus callback function
//****************************************************************************
int cTCPStatus()
{
	char cmdReply[200];
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
	
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
			// ----------	TCP Status Update ----------
            // AT+USOCTL=<socket_number>,<update_param> (10 for socket status)
			sprintf(msg2send, "AT+USOCTL=%d,10\r",xSocket->number);
			
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 90;//INDBG //70;
			smInternal++;
            /* no break */
			
		case 2:
			vTaskDelay(20);
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
			// Get reply "\r\n+USOCTL: <socket>,<param_id>,<param_val>[,<param_val2>]\r\n"
			vTaskDelay(20);
			sprintf(msg2send, "\r\n+USOCTL");
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>+USOCTL: <socket>,<param_id>,<param_val>[,<param_val2>]<CR><LF>
			
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				// Get socket
				char temp[25];
				int res = getfield(':', ',', 5, 1, cmdReply, temp, 500);
				if((res != 1)||(xSocket->number != atoi(temp)))
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield #1 for +USOCTL\r\n");
					break;
				}
				else
				{
					// xSocket->status = atoi(temp);
				}
				
				// Get param_id
				res = getfield(',', ',', 5, 1, cmdReply, temp, 500);
				if((res != 1)||(atoi(temp) != 10))
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield #2 for +USOCTL\r\n");
					break;
				}
				else
				{
					//xSocket->notif = atoi(temp);
				}				
				// Get param_val
				res = getfield(',', '\r', 24, 2, cmdReply, temp, 500);
				if(res != 1)
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield #3 for +USOCTL\r\n");
					break;
				}
				else
				{
					int readStat = atoi(temp);
					xSocket->status = sockStatConversion(readStat);
				}
			}	
            /* no break */
			
		case 4:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
			// Get OK
			maxtimeout = 3;
			sprintf(msg2send, "\r\nOK\r\n");//sprintf(msg2send, "OK\r\n");
            // Wait to have enough data on buffer...
			//waitForGsmMsg(msg2send, maxtimeout);
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            /* no break */
			
        case 5:
			// Read 0 bytes from TCP DATA to get rxBufferSize
            // AT+USRD=<socket>,<length>\r
			sprintf(msg2send, "AT+USORD=%d,0\r",xSocket->number);
			
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 60;
			smInternal++;
            /* no break */
			
		case 6:
			vTaskDelay(20);
			// Check ECHO 
			countData = 0;
			
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
						
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				/* INDBG
				#if defined (STACK_USE_UART)
				char debugStr[100];
				sprintf(debugStr, "error on function %d, smInternal %d\r\n", mainOpStatus.Function, smInternal);
				gsmDebugPrint( debugStr);
				vTaskDelay(20);
				#endif
				// indbg */
				return mainOpStatus.ErrorCode;
			}
            /* no break */
			
		case 7:
			// Get reply "\r\n+USORD: <socket>,<length>\r\n"
			vTaskDelay(20);
			sprintf(msg2send, "\r\n+USORD");
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>+USORD: <socket>,<length><CR><LF>
			
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				// Get socket
				char temp[25];
				int res = getfield(':', ',', 5, 1, cmdReply, temp, 500);
				if((res != 1)||(xSocket->number != atoi(temp)))
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield #1 for +USORD socket\r\n");
					break;
				}
				else
				{
//					xSocket->status = atoi(temp);
				}
				
				// Get length
				res = getfield(',', '\r', 20, 1, cmdReply, temp, 500);
				if(res != 1)
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield #2 for +USORD socket\r\n");
					break;
				}
				else
				{
                    xSocket->rxLen = atoi(temp);
                    if(xSocket->rxLen > 1024)
                        xSocket->rxLen= 1024;
				}
			}	
            /* no break */
			
		case 8:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
			// Get OK
			sprintf(msg2send, "\r\nOK\r\n");
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
			break;
	
	}
	
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
 * Empty specified TCP socket RX Buffer.
 * \param sock The handle of the socket to empty (the handle returned by the command TCPClientOpen or TCPServerOpen).
 * \return none.
 */
void TCPRxFlush(TCP_SOCKET* sock)
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
			mainOpStatus.Function = 25;
			mainOpStatus.ErrorCode = 0;

			// Set Params
			xSocket = sock;

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
//****************************************************************************
//	Only internal use:
//	cTCPisConn callback function
//****************************************************************************
int cTCPRxFlush()
{
	char cmdReply[200];
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
	BOOL repeatLoop = FALSE;
    
	do
	{
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
                {
					smInternal++;
                    xSocket->rxLen = 1024;
                    if(xSocket->rxLen > GSM_BUFFER_SIZE)
                        xSocket->rxLen = GSM_BUFFER_SIZE;
                }
	            /* no break */
					
			case 1:	
                // Send first AT command
                // Read TCP DATA:
                // AT+USRD=<socket>,<length>\r
                sprintf(msg2send, "AT+USORD=%d,%d\r",xSocket->number, xSocket->rxLen);

                GSMWrite(msg2send);
                // Start timeout count
                tick = TickGetDiv64K(); // 1 tick every seconds
                maxtimeout = 60;
                smInternal++;
                /* no break */

            case 2:
                vTaskDelay(20);
                // Check ECHO 
                countData = 0;

                resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
                CheckErr(resCheck, &smInternal, &tick);

                if(resCheck)
                {
                    #if defined (STACK_USE_UART)
                    char debugStr[100];
                    sprintf(debugStr, "error on function %d, smInternal %d\r\n", mainOpStatus.Function, smInternal);
                    gsmDebugPrint( debugStr);
                    vTaskDelay(20);
                    #endif
                    return mainOpStatus.ErrorCode;
                }
                /* no break */

            case 3:
                // Get reply "\r\n+USORD: <socket>,<length>,\"<data in the ASCII [0x00,0xFF] range>\"\r\n"
                sprintf(msg2send, "\r\n+USORD: ");
                chars2read = 2;
                countData = 0; // GSM buffer should be: <CR><LF>+USORD: <socket>,<length>,\"<data in the ASCII [0x00,0xFF] range>\"<CR><LF>
                //while((GSMBufferSize() < strlen(msg2send) && ((TickGetDiv64K()-tick) < maxtimeout)))
				//{
				//	vTaskDelay(10);
				//}

                //			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

                // using GSMReadN we can read data without clearing a the buffer, searching for a specific char (counting it a provided number of times)

                // here it is not possible to use the "getAnswer" (so neither CheckCmd) function 
                // since there is no '\n' chars...
                // for this case only we have to use a custom loop:
                int lenAnsw = strlen(msg2send);

                int readCount = GSMpRead(cmdReply, lenAnsw+2);
                cmdReply[lenAnsw+2] = '\0';
                if((readCount > 0)&&(readCount < lenAnsw+2))
                    cmdReply[readCount] = '\0';

                if(strstr(cmdReply, msg2send) != NULL)
                {
                    // Reply present! Read until the first ':' char is found...
                    int cc = GSMReadN(200, ':', 1, cmdReply);
                    cmdReply[cc] = '\0';
    //                _dbgwrite("cmdReply #2: "); _dbgline(cmdReply);
                    resCheck = 0;        
                    // Now read until the first '\"' is found!
                    cc = GSMReadN(200, '\"', 1, cmdReply);
                    cmdReply[cc] = '\0';         
                    // Get socket
                    char temp[25];
                    int res = getfield(' ', ',', 5, 1, cmdReply, temp, 500);
                    if((res != 1)||(xSocket->number != atoi(temp)))
                    {              
                        // Execute Error Handler
                        gsmDebugPrint( "Error in getfield #1 for +USORD socket\r\n");
                        break;
                    }
                    else
                    {
    //					xSocket->status = atoi(temp);
                    }

                    // Get length
                    res = getfield(',', ',', 5, 1, cmdReply, temp, 500);
                    if(res != 1)
                    {                    
                    	// It could be the case of "AT+USORD=0,0\r" reply...
						res = getfield(',', '\r', 5, 1, cmdReply, temp, 500);
						if(res == 1)
						{
							xSocket->rxLen = atoi(temp);
	                        if(xSocket->rxLen > 1024)
	                            xSocket->rxLen= 1024;
						}
						else
						{
							// Execute Error Handler
							gsmDebugPrint( "Error in getfield #2 for +USORD socket\r\n");
							break;
						}
                    }
                    else
                    {
                        xSocket->rxLen = atoi(temp);
                        if(xSocket->rxLen > 1024)
                            xSocket->rxLen= 1024;
                    }

    // get data!
                    if(xSocket->rxLen >= 0)
                    {
                        tick = TickGetDiv64K(); // 1 tick every seconds
                        maxtimeout = 30;
                        int rxcount = 0;
                        while((rxcount < xSocket->rxLen)&&(TickGetDiv64K()<(tick+maxtimeout)))
                        {
                            char tmp[2] = {0,0};
                            GSMRead(tmp, 1);
                            rxcount++;
                        }
                        
                        if(rxcount < xSocket->rxLen) // operation timed out!
                        {
                            resCheck = 1; // it means ERROR
                            tick = TickGetDiv64K();
                            CheckErr(resCheck, &smInternal, &tick);
                        }
                        else
                        {
                            // Read 3 more chars from buffer (\"\r\n):
                            char dummy[4];
                            GSMRead(dummy, 3);
                        }

                        if(resCheck)
                        {
                            return mainOpStatus.ErrorCode;
                        }
                    }
                }
                else
                {
                    resCheck = 1; // it means ERROR
                    tick = TickGetDiv64K();
                    CheckErr(resCheck, &smInternal, &tick);
                }
                /* no break */

            case 4:
                // Get reply (\r\nOK\r\n)
                vTaskDelay(1);
                // Get OK
                sprintf(msg2send, "\r\nOK\r\n");
                chars2read = 2;
                countData = 0; // GSM buffer should be: <CR><LF>OK<CR><LF>
                resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

                CheckErr(resCheck, &smInternal, &tick);

                if(resCheck)
                {
                    return mainOpStatus.ErrorCode;
                }
                /* no break */

            case 5:
                // Send first AT command
                // Read TCP DATA:
                // AT+USRD=<socket>,<length>\r
                sprintf(msg2send, "AT+USORD=%d,0\r",xSocket->number);

                GSMWrite(msg2send);
                // Start timeout count
                tick = TickGetDiv64K(); // 1 tick every seconds
                maxtimeout = 60;
                smInternal++;
                /* no break */

            case 6:
                vTaskDelay(20);
                // Check ECHO 
                countData = 0;

                resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);

                CheckErr(resCheck, &smInternal, &tick);

                if(resCheck)
                {
                    #if defined (STACK_USE_UART)
                    char debugStr[100];
                    sprintf(debugStr, "error on function %d, smInternal %d\r\n", mainOpStatus.Function, smInternal);
                    gsmDebugPrint( debugStr);
                    vTaskDelay(20);
                    #endif
                    return mainOpStatus.ErrorCode;
                }
                /* no break */

            case 7:
                // Get reply "\r\n+USORD: <socket>,<length>\r\n"
                vTaskDelay(20);
                sprintf(msg2send, "\r\n+USORD");
                chars2read = 2;
                countData = 0; // GSM buffer should be: <CR><LF>+USORD: <socket>,<length><CR><LF>

                resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

                CheckErr(resCheck, &smInternal, &tick);

                if(resCheck)
                {
                    return mainOpStatus.ErrorCode;
                }
                else
                {
                    // Get socket
                    char temp[25];
                    int res = getfield(':', ',', 5, 1, cmdReply, temp, 500);
                    if((res != 1)||(xSocket->number != atoi(temp)))
                    {
                        // Execute Error Handler
                        gsmDebugPrint( "Error in getfield for +USORD socket\r\n");
                        break;
                    }
                    else
                    {
    //					xSocket->status = atoi(temp);
                    }

                    // Get length
                    res = getfield(',', '\r', 5, 1, cmdReply, temp, 500);
                    if(res != 1)
                    {
                        // Execute Error Handler
                        gsmDebugPrint( "Error in getfield for +USORD socket\r\n");
                        break;
                    }
                    else
                    {
                        xSocket->rxLen = atoi(temp);
                        if(xSocket->rxLen > 1024)
                            xSocket->rxLen= 1024;
                    }
                }
                /* no break */

            case 8:
                // Get reply (\r\nOK\r\n)
                vTaskDelay(1);
                // Get OK
                sprintf(msg2send, "\r\nOK\r\n");
                // Wait to have enough data on buffer...
    			waitForGsmMsg(msg2send, maxtimeout);
                chars2read = 2;
                countData = 0; // GSM buffer should be: <CR><LF>OK<CR><LF>
                resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

                CheckErr(resCheck, &smInternal, &tick);

                if(resCheck)
                {
                    return mainOpStatus.ErrorCode;
                }
				else
				{
                    if(xSocket->rxLen > 0)
                    {
                        repeatLoop = TRUE;
                        smInternal = 1;
                    }
                    else
                    {
                        repeatLoop = FALSE;
                    }
					break;
				}
				
			default:
				break;
		
		}
	} while(repeatLoop == TRUE);
		
	smInternal = 0;
	// Cmd = 0 only if the last command successfully executed
	mainOpStatus.ExecStat = OP_SUCCESS;
	mainOpStatus.Function = 0;
	mainOpStatus.ErrorCode = 0;
	mainGSMStateMachine = SM_GSM_IDLE;
	return -1;
}
/// @endcond

/*! @} */

/*! @} */
