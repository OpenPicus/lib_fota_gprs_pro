/** \file FTPlib.c
 *  \brief library to manage FTP file exchange. It uses Hilo internal flash memory as file storage for Flyport module
 */

/**
\addtogroup GSM
@{
*/

/* **************************************************************************																					
 *                                OpenPicus                 www.openpicus.com
 *                                                            italian concept
 * 
 *            openSource wireless Platform for sensors and Internet of Things	
 * **************************************************************************
 *  FileName:        FTPlib.c
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
#include "FTPlib.h"

//extern void callbackDbg(BYTE smInt);
extern void gsmDebugPrint(char*);

//extern int Cmd;
extern int mainGSMStateMachine;
extern OpStat	mainOpStatus;
extern GSMModule mainGSM;

extern int CheckCmd(int countData, int chars2read, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);
extern int CheckEcho(int countData, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);
extern void CheckErr(int result, BYTE* smInt, DWORD* tickUpdate);
extern int EventType;
extern char msg2send[200];

static BYTE smInternal = 0; // State machine for internal use of callback functions
static BYTE maxtimeout = 2;

static WORD xFTPPort;
extern FTP_SOCKET* xFTPSocket;
static char* xFTPServName;
static char* xFTPLogin;
static char* xFTPPassw;
static char* xFTPServFilename;
static char* xFTPServPath;
static char* xFTPFlashFilename;
static BOOL xFTPAppeMode = FALSE;
/// @endcond

/**
\defgroup FTP
@{
Provides FTP Client functions
*/

static void _ftpSetErrorCallback()
{
    // DEFAULT CASE IS USED TO launch cFTPGetError
    mainOpStatus.ExecStat = OP_EXECUTION;
    mainOpStatus.Function = 38; // FP_GSM[38] is cFTPGetError
    mainOpStatus.ErrorCode = 0;
    mainGSMStateMachine = SM_GSM_CMD_PENDING;
    smInternal = 0;
}

/**
 * Configures FTP Server parameters
 * \param ftpSocket FTP_SOCKET to be used for connection
 * \param serverName char* to serverName string
 * \param login char* to username login string
 * \param password char* to password string
 * \param portNumber WORD with port number
 * \return None
 */
void FTPConfig(FTP_SOCKET* ftpSocket, char* serverName, char* login, char* password, WORD portNumber)
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
			mainOpStatus.Function = 12;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xFTPSocket = ftpSocket;
			xFTPSocket->ftpError = 0;
			xFTPServName = serverName;
			xFTPLogin = login;
			xFTPPassw = password;
			xFTPPort = portNumber;
			
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
int cFTPConfig()
{
	char cmdReply[200];
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
    IP_ADDR ftpIp;
    int op_code;
	
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
            // First of all, close FTP Connection...
            // Close connection
			op_code = 0;
            sprintf(msg2send, "AT+UFTPC=%d\r", op_code);
            GSMWrite(msg2send);
            
            // Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
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
			}
			/* no break */
            
        case 4: // Get reply +UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]
            vTaskDelay(1);
            op_code = 0;
            sprintf(msg2send, "\r\n+UUFTPCR:");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>+UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]<CR><LF>

            tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 60*3+2; // TODO: how many seconds of timeout should I insert here? // INDBG 
            // Wait to have enough data on buffer...
			waitForGsmMsg(msg2send, maxtimeout);
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
            else
            {
                char temp[25];
                // Get op_code:
                int res = getfield(' ', ',', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    
                    int _opcode = atoi(temp);
                    if(_opcode != op_code)
                    {
                        smInternal = 100;
                        break;
                    }
                }	
                
                // Get ftp_result:
                res = getfield(',', '\r', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    int _ftp_result = atoi(temp);
                    if((_ftp_result == 1)||(_ftp_result == 0))
                    {
                        // No error, and no failure, probably the socket was closed yet!
                    }
                    else
                    {
                        // Unkown value received! We have to act as a error code result!
                        _dbgline("Unknown value on FTP Reply!");
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }
            }
			/* no break */
                     
				
		case 5:	
            {
                // Send first AT command
                // ----------	FTP Connection Configuration	----------
                // Sara modules do not have a single-shot AT command to setup FTP Configuration,
                // instead we have to setup each parameter one by one using 
                // AT+UFTP=<op_code>[,<param1>[,<param2>]]
                // 
                // The params <op_code> we have to set are:
                // 0. Server IP address (xFTPServName if it is a valid IP)
                // 1. Server Name (xFTPServName used if it is not a valid IP)
                // 2. User Name (xFTPLogin)
                // 3. Password (xFTPPassw)
                // 7. Server Port (xFTPPort)
                op_code = 1; // set by default as server name...
                if(StringToIPAddress((BYTE*)xFTPServName, &ftpIp) == TRUE)
                {
                    // it is a valid IP!
                    op_code = 0;
                }

                sprintf(msg2send, "AT+UFTP=%d,\"", op_code);
                GSMWrite(msg2send);
                GSMWrite(xFTPServName);
                GSMWrite("\"\r");

                // Start timeout count
                tick = TickGetDiv64K(); // 1 tick every seconds
                maxtimeout = 2;
                smInternal++;
            }
			/* no break */
            
		case 6:
			vTaskDelay(20);
			// Check ECHO 
			countData = 0;
			
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
						
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				int dummyLen = strlen(xFTPServName) + 2;// + strlen(xFTPLogin) + strlen(xFTPPassw) + strlen(portnum) + 10;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			/* no break */
			
		case 7:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
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
		
        case 8:
            // Set username (op_code 2)
            sprintf(msg2send, "AT+UFTP=2,\"");
            GSMWrite(msg2send);
            GSMWrite(xFTPLogin);
            GSMWrite("\"\r");
            
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;
			/* no break */
            
		case 9:
			vTaskDelay(20);
			// Check ECHO 
			countData = 0;
			
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
						
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				int dummyLen = strlen(xFTPLogin) + 2;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			/* no break */
			
		case 10:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
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
            
        case 11:
            // Set password (op_code 3)
            sprintf(msg2send, "AT+UFTP=3,\"");
            GSMWrite(msg2send);
            GSMWrite(xFTPPassw);
            GSMWrite("\"\r");
            
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;
			/* no break */
            
		case 12:
			vTaskDelay(20);
			// Check ECHO 
			countData = 0;
			
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
						
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				int dummyLen = strlen(xFTPPassw) + 2;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			/* no break */
			
		case 13:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
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
            
        case 14:
            // Set port (op_code 7)
            sprintf(msg2send, "AT+UFTP=7,%d\r", xFTPPort);
            GSMWrite(msg2send);
            
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;
			/* no break */
            
		case 15:
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
			
		case 16:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
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
            else
                break; // quit state machine avoiding entering default case!
            
		default:
			break;
	}
    
    if(smInternal == 100)
    {
        _ftpSetErrorCallback();
        return -1;
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

// In FTP Receive path, we will use always the "ftp" directory
/**
 * Download a file from FTP Server
 * \param ftpSocket FTP_SOCKET to be used for connection
 * \param flashFilename char* to flash file name string to create/overwrite
 * \param serverPath char* to server path string
 * \param serverFilename char* to server file name string
 * \return None
 */
void FTPReceive(FTP_SOCKET* ftpSocket, char* flashFilename, char* serverPath, char* serverFilename)
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
			mainOpStatus.Function = 13;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xFTPSocket = ftpSocket;
			xFTPFlashFilename = flashFilename;
			xFTPServPath = serverPath;
			xFTPServFilename = serverFilename;

			
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
int cFTPReceive()
{
	char cmdReply[200];
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
    static int op_code = 0;
	
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
			
			// ----------	FTP Receive Command ----------
            // 1. activate FTP connection!!
            op_code = 1;
            sprintf(msg2send, "AT+UFTPC=1\r");
            GSMWrite(msg2send);
            
            // Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
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
            
        case 4: // Get reply +UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]
            vTaskDelay(1);
            op_code = 1;
            sprintf(msg2send, "\r\n+UUFTPCR:");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>+UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]<CR><LF>

            tick = TickGetDiv64K(); // 1 tick every seconds
			int ftpTimeout = 60*10+2; // 10 MINUTES?
            // Wait to have enough data on buffer...
			waitForGsmMsg(msg2send, ftpTimeout);
            
            tick = TickGetDiv64K(); // we have to update tick at this point so CheckCmd will not exit as soon as it starts its internal loop....
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
            else
            {
                char temp[25];
                // Get op_code:
                int res = getfield(' ', ',', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    gsmDebugPrint( "Error in getfield\r\n");
                    break;
                }
                else
                {
                    
                    int _opcode = atoi(temp);
                    if(_opcode != op_code)
                    {
                        res = -1;
                        break;
                    }
                }	
                
                // Get ftp_result:
                res = getfield(',', '\r', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    //gsmDebugPrint( "Error in getfield\r\n");
                    smInternal = 100; // go to default case now (and execute cFTPGetError)
                    break;
                }
                else
                {
                    int _ftp_result = atoi(temp);
                    if(_ftp_result == 1) // SUCCESS!
                    {
                        
                    }
                    else if(_ftp_result == 0) // Failure
                    {
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                    else
                    {
                        // Unkown value received! We have to act as a error code result!
                        _dbgline("Unknown value on FTP Reply!");
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
            }	
			/* no break */
            
        case 5:
            // 2. change directory AT+UFTPC=8,"xFTPServPath"
            
            // Change directory on server:
            op_code = 8;
            sprintf(msg2send, "AT+UFTPC=8,\"");
            GSMWrite(msg2send);
            GSMWrite(xFTPServPath);
            GSMWrite("\"\r");
            
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
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
				return mainOpStatus.ErrorCode;
			}
			else
			{
				int dummyLen = strlen(xFTPServPath) + 2;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			/* no break */
			
		case 7:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
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
			
        case 8:
            vTaskDelay(1);
            op_code = 8;
            sprintf(msg2send, "\r\n+UUFTPCR:");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>+UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]<CR><LF>

            tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 30+1;
            // Wait to have enough data on buffer...
			waitForGsmMsg(msg2send, maxtimeout);
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
            else
            {
                char temp[25];
                // Get op_code:
                int res = getfield(' ', ',', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    //gsmDebugPrint( "Error in getfield\r\n");
                    smInternal = 100; // go to default case now (and execute cFTPGetError)
                    break;
                }
                else
                {
                    
                    int _opcode = atoi(temp);
                    if(_opcode != op_code)
                    {
                        // Execute Error Handler
                        //gsmDebugPrint( "Error in getfield\r\n");
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
                
                // Get ftp_result:
                res = getfield(',', '\r', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    gsmDebugPrint( "Error in getfield\r\n");
                    break;
                }
                else
                {
                    int _ftp_result = atoi(temp);
                    if(_ftp_result == 1) // SUCCESS!
                    {
                        
                    }
                    else if(_ftp_result == 0) // Failure
                    {
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                    else
                    {
                        // Unkown value received! We have to act as a error code result!
                        _dbgline("Unknown value on FTP Reply!");
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
            }	
			/* no break */
            
        case 9:	
			// ----------	FSDelete File Delete Command ----------
			// AT+UDELFILE="filename"\r
			sprintf(msg2send, "AT+UDELFILE=\"");
			GSMWrite(msg2send);
			GSMWrite(xFTPFlashFilename);
			GSMWrite("\"\r");
			
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;	
			/* no break */
				
		case 10:
			vTaskDelay(1);
			// Check ECHO 
			countData = 0;
			
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
						
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				int dummyLen = strlen(xFTPFlashFilename) + 2;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			/* no break */
			
		case 11:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
			// Get OK
			sprintf(msg2send, "\r\nOK\r\n");//sprintf(msg2send, "OK\r\n");
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
            // Don't bother of "ERROR" result:
//            if((mainOpStatus.ExecStat == OP_SYNTAX_ERR)&&(mainGSMStateMachine == SM_GSM_IDLE)&&(resCheck == 1))
//            {
//                // Avoid to handle such error, we assume it is not a problem in this condition!
//                resCheck = 0;
//                mainOpStatus.ExecStat = OP_EXECUTION;
//                mainOpStatus.ErrorCode = 0;
//                // Clear event type:
//                EventType = NO_EVENT;	
//                mainGSMStateMachine = SM_GSM_CMD_PENDING;
//            }
//            else 
            if((resCheck == 3) && (mainOpStatus.ExecStat == OP_CME_ERR) && (mainOpStatus.ErrorCode == 1612)) // Error 1612 is File not found error for CME error list...
            {
                // Avoid to handle such error, we assume it is not a problem in this condition!
                resCheck = 0;
                mainOpStatus.ExecStat = OP_EXECUTION;
                mainOpStatus.ErrorCode = 0;
                // Clear event type:
                EventType = NO_EVENT;	
                mainGSMStateMachine = SM_GSM_CMD_PENDING;
            }
            else
            {
                CheckErr(resCheck, &smInternal, &tick);
            }
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			/* no break */
				
		case 12:	
			// 2. Retrieve the file from the FTP server AT+UFTPC=4,<remote_file_name>,<local_file_name>[,<retrieving_mode>]

            // Change directory on server:
            op_code = 4;
            sprintf(msg2send, "AT+UFTPC=4,\"");
            GSMWrite(msg2send);
            GSMWrite(xFTPServFilename);
            GSMWrite("\",\"");
            GSMWrite(xFTPFlashFilename);
            GSMWrite("\"\r");
            
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;
			/* no break */
            
		case 13:
			vTaskDelay(20);
			// Check ECHO 
			countData = 0;
			
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
						
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				int dummyLen = strlen(xFTPServFilename) + strlen(xFTPFlashFilename) + 5;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			/* no break */
			
		case 14:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
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
            
        case 15:
            vTaskDelay(1);
            op_code = 4;
            sprintf(msg2send, "\r\n+UUFTPCR:");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>+UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]<CR><LF>

            tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 180+1;
            // Wait to have enough data on buffer...
			waitForGsmMsg(msg2send, maxtimeout);
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
            else
            {
                char temp[25];
                // Get op_code:
                int res = getfield(' ', ',', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    //gsmDebugPrint( "Error in getfield\r\n");
                    smInternal = 100; // go to default case now (and execute cFTPGetError)
                    break;
                }
                else
                {
                    
                    int _opcode = atoi(temp);
                    if(_opcode != op_code)
                    {
                        // Execute Error Handler
                        //gsmDebugPrint( "Error in getfield\r\n");
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
                
                // Get ftp_result:
                res = getfield(',', '\r', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    //gsmDebugPrint( "Error in getfield\r\n");
                    smInternal = 100; // go to default case now (and execute cFTPGetError)
                    break;
                }
                else
                {
                    int _ftp_result = atoi(temp);
                    if(_ftp_result == 1) // SUCCESS!
                    {
                        
                    }
                    else if(_ftp_result == 0) // Failure
                    {
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                    else
                    {
                        // Unkown value received! We have to act as a error code result!
                        _dbgline("Unknown value on FTP Reply!");
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
            }
			/* no break */
            
        case 16:
            // Close connection
			op_code = 0;
            sprintf(msg2send, "AT+UFTPC=0\r");
            GSMWrite(msg2send);
            
            // Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;
			/* no break */
            
		case 17:
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
            
        case 18:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
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
            
        case 19: // Get reply +UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]
            vTaskDelay(1);
            op_code = 0;
            sprintf(msg2send, "\r\n+UUFTPCR:");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>+UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]<CR><LF>

            tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 30+1;
            // Wait to have enough data on buffer...
			waitForGsmMsg(msg2send, maxtimeout);
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
            else
            {
                char temp[25];
                // Get op_code:
                int res = getfield(' ', ',', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    //gsmDebugPrint( "Error in getfield\r\n");
                    smInternal = 100; // go to default case now (and execute cFTPGetError)
                    break;
                }
                else
                {
                    
                    int _opcode = atoi(temp);
                    if(_opcode != op_code)
                    {
                        // Execute Error Handler
                        //gsmDebugPrint( "Error in getfield\r\n");
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
                
                // Get ftp_result:
                res = getfield(',', '\r', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    //gsmDebugPrint( "Error in getfield\r\n");
                    smInternal = 100; // go to default case now (and execute cFTPGetError)
                    break;
                }
                else
                {
                    int _ftp_result = atoi(temp);
                    if(_ftp_result == 1) // SUCCESS!
                    {
                        
                    }
                    else if(_ftp_result == 0) // Failure
                    {
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                    else
                    {
                        // Unkown value received! We have to act as a error code result!
                        _dbgline("Unknown value on FTP Reply!");
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
            }	
			/* no break */
			
		default:
			break;
	}
	if(smInternal == 100)
    {
        _ftpSetErrorCallback();
        return -1;
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
 * Uploads a file to FTP Server
 * \param ftpSocket FTP_SOCKET to be used for connection
 * \param flashFilename char* to flash file name string to upload
 * \param serverPath char* to server path string
 * \param serverFilename char* to server file name string
 * \param appendMode: TRUE enables append, FALSE disables append
 * \return None
 */
void FTPSend(FTP_SOCKET* ftpSocket, char* flashFilename, char* serverPath, char* serverFilename, BOOL appendMode)
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
			mainOpStatus.Function = 14;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xFTPSocket = ftpSocket;
			xFTPFlashFilename = flashFilename;
			xFTPServPath = serverPath;
			xFTPServFilename = serverFilename;
			xFTPAppeMode = appendMode;
			
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
int cFTPSend()
{
	char cmdReply[200];
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
    static int op_code = 0;
	
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
			
			// ----------	FTP Receive Command ----------
            // 1. activate FTP connection!!
            op_code = 1;
            sprintf(msg2send, "AT+UFTPC=1\r");
            GSMWrite(msg2send);
            
            // Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
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
            
        case 4: // Get reply +UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]
            vTaskDelay(1);
            op_code = 1;
            sprintf(msg2send, "\r\n+UUFTPCR:");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>+UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]<CR><LF>

            tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 30+1;
            // Wait to have enough data on buffer...
			waitForGsmMsg(msg2send, maxtimeout);
                    
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
            else
            {
                char temp[25];
                // Get op_code:
                int res = getfield(' ', ',', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    
                    int _opcode = atoi(temp);
                    if(_opcode != op_code)
                    {
                        smInternal = 100;
                        break;
                    }
                }	
                
                // Get ftp_result:
                res = getfield(',', '\r', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    int _ftp_result = atoi(temp);
                    if(_ftp_result == 1) // SUCCESS!
                    {
                        
                    }
                    else if(_ftp_result == 0) // Failure
                    {
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                    else
                    {
                        // Unkown value received! We have to act as a error code result!
                        _dbgline("Unknown value on FTP Reply!");
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
            }	
			/* no break */
            
        case 5:
            // 2. change directory AT+UFTPC=8,"xFTPServPath"
            
            // Change directory on server:
            op_code = 8;
            sprintf(msg2send, "AT+UFTPC=8,\"");
            GSMWrite(msg2send);
            GSMWrite(xFTPServPath);
            GSMWrite("\"\r");
            
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
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
				return mainOpStatus.ErrorCode;
			}
			else
			{
				int dummyLen = strlen(xFTPServPath) + 2;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			/* no break */
			
		case 7:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
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
			
        case 8:
            vTaskDelay(1);
            op_code = 8;
            sprintf(msg2send, "\r\n+UUFTPCR:");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>+UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]<CR><LF>

            tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 30+1;
            // Wait to have enough data on buffer...
			waitForGsmMsg(msg2send, maxtimeout);
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
            else
            {
                char temp[25];
                // Get op_code:
                int res = getfield(' ', ',', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    
                    int _opcode = atoi(temp);
                    if(_opcode != op_code)
                    {
                        smInternal = 100;
                        break;
                    }
                }	
                
                // Get ftp_result:
                res = getfield(',', '\r', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    int _ftp_result = atoi(temp);
                    if(_ftp_result == 1) // SUCCESS!
                    {
                        
                    }
                    else if(_ftp_result == 0) // Failure
                    {
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                    else
                    {
                        // Unkown value received! We have to act as a error code result!
                        _dbgline("Unknown value on FTP Reply!");
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
            }	
			/* no break */
            
        case 9:
            // Send file from local file to remote server
            // 2. SEnd the file from local flash to FTP server AT+UFTPC=5,<local_file_name>,<remote_file_name>[,<retrieving_mode>]

            // Change directory on server:
            op_code = 5;
            sprintf(msg2send, "AT+UFTPC=5,\"");
            GSMWrite(msg2send);
            GSMWrite(xFTPFlashFilename);
            GSMWrite("\",\"");
            GSMWrite(xFTPServFilename);
            GSMWrite("\"\r");
            
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;
			/* no break */
            
		case 13:
			vTaskDelay(20);
			// Check ECHO 
			countData = 0;
			
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
						
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				int dummyLen = strlen(xFTPServFilename) + strlen(xFTPFlashFilename) + 5;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			/* no break */
			
		case 14:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
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
            
        case 15:
            vTaskDelay(1);
            op_code = 5;
            sprintf(msg2send, "\r\n+UUFTPCR:");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>+UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]<CR><LF>

            tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 180+1;
            // Wait to have enough data on buffer...
			waitForGsmMsg(msg2send, maxtimeout);
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
            else
            {
                char temp[25];
                // Get op_code:
                int res = getfield(' ', ',', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    
                    int _opcode = atoi(temp);
                    if(_opcode != op_code)
                    {
                        smInternal = 100;
                        break;
                    }
                }	
                
                // Get ftp_result:
                res = getfield(',', '\r', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    int _ftp_result = atoi(temp);
                    if(_ftp_result == 1) // SUCCESS!
                    {
                        
                    }
                    else if(_ftp_result == 0) // Failure
                    {
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                    else
                    {
                        // Unkown value received! We have to act as a error code result!
                        _dbgline("Unknown value on FTP Reply!");
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
            }
			/* no break */
            
        case 16:
            // Close connection
			op_code = 0;
            sprintf(msg2send, "AT+UFTPC=0\r");
            GSMWrite(msg2send);
            
            // Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;
			/* no break */
            
		case 17:
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
            
        case 18:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
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
            
        case 19: // Get reply +UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]
            vTaskDelay(1);
            op_code = 0;
            sprintf(msg2send, "\r\n+UUFTPCR:");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>+UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]<CR><LF>

            tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 30+1;
            // Wait to have enough data on buffer...
			waitForGsmMsg(msg2send, maxtimeout);
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
            else
            {
                char temp[25];
                // Get op_code:
                int res = getfield(' ', ',', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    
                    int _opcode = atoi(temp);
                    if(_opcode != op_code)
                    {
                        smInternal = 100;
                        break;
                    }
                }	
                
                // Get ftp_result:
                res = getfield(',', '\r', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    int _ftp_result = atoi(temp);
                    if(_ftp_result == 1) // SUCCESS!
                    {
                        
                    }
                    else if(_ftp_result == 0) // Failure
                    {
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                    else
                    {
                        // Unkown value received! We have to act as a error code result!
                        _dbgline("Unknown value on FTP Reply!");
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
            }	
			/* no break */
            
		default:
			break;
	}
    if(smInternal == 100)
    {
        _ftpSetErrorCallback();
        return -1;
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
 * Deletes a file from FTP Server
 * \param ftpSocket FTP_SOCKET to be used for connection
 * \param serverPath char* to server path string
 * \param serverFilename char* to server file name string
 * \return None
 */
void FTPDelete(FTP_SOCKET* ftpSocket, char* serverPath, char* serverFilename)
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
			mainOpStatus.Function = 15;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xFTPSocket = ftpSocket;
			xFTPServPath = serverPath;
			xFTPServFilename = serverFilename;
			
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
int cFTPDelete()
{
	char cmdReply[200];
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
    static int op_code = 0;
	
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
			
			// ----------	FTP Receive Command ----------
            // 1. activate FTP connection!!
            op_code = 1;
            sprintf(msg2send, "AT+UFTPC=1\r");
            GSMWrite(msg2send);
            
            // Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
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
            
        case 4: // Get reply +UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]
            vTaskDelay(1);
            op_code = 1;
            sprintf(msg2send, "\r\n+UUFTPCR:");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>+UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]<CR><LF>

            tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 30+1;
            // Wait to have enough data on buffer...
			waitForGsmMsg(msg2send, maxtimeout);
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
            else
            {
                char temp[25];
                // Get op_code:
                int res = getfield(' ', ',', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    
                    int _opcode = atoi(temp);
                    if(_opcode != op_code)
                    {
                        smInternal = 100;
                        break;
                    }
                }	
                
                // Get ftp_result:
                res = getfield(',', '\r', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    int _ftp_result = atoi(temp);
                    if(_ftp_result == 1) // SUCCESS!
                    {
                        
                    }
                    else if(_ftp_result == 0) // Failure
                    {
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                    else
                    {
                        // Unkown value received! We have to act as a error code result!
                        _dbgline("Unknown value on FTP Reply!");
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
            }	
			/* no break */
            
        case 5:
            // 2. change directory AT+UFTPC=8,"xFTPServPath"
            
            // Change directory on server:
            op_code = 8;
            sprintf(msg2send, "AT+UFTPC=8,\"");
            GSMWrite(msg2send);
            GSMWrite(xFTPServPath);
            GSMWrite("\"\r");
            
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
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
				return mainOpStatus.ErrorCode;
			}
			else
			{
				int dummyLen = strlen(xFTPServPath) + 2;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			/* no break */
			
		case 7:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
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
			
        case 8:
            vTaskDelay(1);
            op_code = 8;
            sprintf(msg2send, "\r\n+UUFTPCR:");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>+UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]<CR><LF>

            tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 30+1;
            // Wait to have enough data on buffer...
			waitForGsmMsg(msg2send, maxtimeout);
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
            else
            {
                char temp[25];
                // Get op_code:
                int res = getfield(' ', ',', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    
                    int _opcode = atoi(temp);
                    if(_opcode != op_code)
                    {
                        smInternal = 100;
                        break;
                    }
                }	
                
                // Get ftp_result:
                res = getfield(',', '\r', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    int _ftp_result = atoi(temp);
                    if(_ftp_result == 1) // SUCCESS!
                    {
                        
                    }
                    else if(_ftp_result == 0) // Failure
                    {
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                    else
                    {
                        // Unkown value received! We have to act as a error code result!
                        _dbgline("Unknown value on FTP Reply!");
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
            }	
			/* no break */
            
        case 9:
            // Delete file from remote server
            // 2. Delete file from FTP server AT+UFTPC=2,<remote_file_name>

            // Change directory on server:
            op_code = 2;
            sprintf(msg2send, "AT+UFTPC=2,\"");
            GSMWrite(msg2send);
            GSMWrite(xFTPServFilename);
            GSMWrite("\"\r");
            
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;
			/* no break */
            
		case 13:
			vTaskDelay(20);
			// Check ECHO 
			countData = 0;
			
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
						
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				int dummyLen = strlen(xFTPServFilename) + 2;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			/* no break */
			
		case 14:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
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
            
        case 15:
            vTaskDelay(1);
            op_code = 2;
            sprintf(msg2send, "\r\n+UUFTPCR:");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>+UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]<CR><LF>

            tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 30+1;
            // Wait to have enough data on buffer...
			waitForGsmMsg(msg2send, maxtimeout);
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
            else
            {
                char temp[25];
                // Get op_code:
                int res = getfield(' ', ',', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    
                    int _opcode = atoi(temp);
                    if(_opcode != op_code)
                    {
                        smInternal = 100;
                        break;
                    }
                }	
                
                // Get ftp_result:
                res = getfield(',', '\r', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    int _ftp_result = atoi(temp);
                    if(_ftp_result == 1) // SUCCESS!
                    {
                        
                    }
                    else if(_ftp_result == 0) // Failure
                    {
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                    else
                    {
                        // Unkown value received! We have to act as a error code result!
                        _dbgline("Unknown value on FTP Reply!");
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
            }
			/* no break */
            
        case 16:
            // Close connection
			op_code = 0;
            sprintf(msg2send, "AT+UFTPC=0\r");
            GSMWrite(msg2send);
            
            // Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;
			/* no break */
            
		case 17:
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
            
        case 18:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
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
            
        case 19: // Get reply +UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]
            vTaskDelay(1);
            op_code = 0;
            sprintf(msg2send, "\r\n+UUFTPCR:");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>+UUFTPCR: <op_code>,<ftp_result>[,<md5_sum>]<CR><LF>

            tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 30+1;
            // Wait to have enough data on buffer...
			waitForGsmMsg(msg2send, maxtimeout);
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
            else
            {
                char temp[25];
                // Get op_code:
                int res = getfield(' ', ',', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    
                    int _opcode = atoi(temp);
                    if(_opcode != op_code)
                    {
                        smInternal = 100;
                        break;
                    }
                }	
                
                // Get ftp_result:
                res = getfield(',', '\r', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    smInternal = 100;
                    break;
                }
                else
                {
                    int _ftp_result = atoi(temp);
                    if(_ftp_result == 1) // SUCCESS!
                    {
                        
                    }
                    else if(_ftp_result == 0) // Failure
                    {
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                    else
                    {
                        // Unkown value received! We have to act as a error code result!
                        _dbgline("Unknown value on FTP Reply!");
                        // Execute Error Handler
                        smInternal = 100; // go to default case now (and execute cFTPGetError)
                        break;
                    }
                }	
            }	
			/* no break */
				
		default:
			break;
	}
    if(smInternal == 100)
    {
        _ftpSetErrorCallback();
        return -1;
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


/// @cond debug
int cFTPGetError()
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
			
			// ----------	FTP Receive Command ----------
            sprintf(msg2send, "AT+UFTPER\r");
            GSMWrite(msg2send);
            
            // Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
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
                    
        case 3: // Get reply +UFTPER: <error_class>,<error_code>
            vTaskDelay(1);
            sprintf(msg2send, "\r\n+UFTPER:");
            chars2read = 2;
            countData = 0; // GSM buffer should be: <CR><LF>+UFTPER: <error_class>,<error_code><CR><LF>

            tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 30+1;
            // Wait to have enough data on buffer...
			waitForGsmMsg(msg2send, maxtimeout);
            // resCheck calls the "getAnswer function and the parsing of +UFTPER reply is done on such function!
            resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

            CheckErr(resCheck, &smInternal, &tick);

            if(resCheck)
            {
                return mainOpStatus.ErrorCode;
            }
            else
            {
                int ftp_err_class = 0;
                int ftp_err_code = 0;
                char temp[25];
                // Get error_class // it should be always 1 for FTP, 0 if no erorrs found!
                int res = getfield(' ', ',', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    gsmDebugPrint( "Error in getfield\r\n");
                    break;
                }
                else
                {
                    ftp_err_class = atoi(temp);
                }	
                
                // Get ftp_result:
                res = getfield(',', '\r', 24, 1, cmdReply, temp, 500);
                if(res != 1)
                {
                    // Execute Error Handler
                    gsmDebugPrint( "Error in getfield\r\n");
                    break;
                }
                else
                {
                    ftp_err_code = atoi(temp);
                }	
                
                if(ftp_err_class != 0) // There is an error:
                {
                    switch(ftp_err_class)
                    {
                        case 2: // SMTP only!
                        case 9: // SMTP only!
                        {
                            mainOpStatus.ErrorCode = ftp_err_code;
                            // SET State Machine as IDLE
                            mainOpStatus.ExecStat = OP_SMTP_ERR;
                        }
                        break;
                        
                        
                        case 3: // HTTP only
                        case 10: // HTTP only
                        {
                            mainOpStatus.ErrorCode = ftp_err_code;
                            // SET State Machine as IDLE
                            mainOpStatus.ExecStat = OP_SYNTAX_ERR;
                        }
                        break;
                        
                        case 1:
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                        case 8:
                        case 11:
                        case 12:
                        default:
                        {
                            // a FTP Error occured (or errors related to FTP operations!)
                            xFTPSocket->ftpError = ftp_err_code;
                            mainOpStatus.ErrorCode = ftp_err_code;
                            // SET State Machine as IDLE
                            mainOpStatus.ExecStat = OP_FTP_ERR;
                        }
                    }
                    // Set GSM Event:
                    EventType = ON_ERROR;

                    smInternal = 0;
                    // Cmd = 0 only if the last command successfully executed
                    mainOpStatus.Function = 0;
                    mainGSMStateMachine = SM_GSM_IDLE;
                    
                    return mainOpStatus.ErrorCode;
                }
                
            }	
			/* no break */
            
                
        case 4:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
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

/*! @} */

/*! @} */

