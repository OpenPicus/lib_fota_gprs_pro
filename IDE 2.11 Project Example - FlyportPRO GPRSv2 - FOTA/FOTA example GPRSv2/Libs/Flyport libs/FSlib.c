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
 *  FileName:        FSlib.c
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
#include "FSlib.h"

extern int Cmd;
extern int mainGSMStateMachine;
extern OpStat	mainOpStatus;
extern GSMModule mainGSM;

extern int CheckCmd(int countData, int chars2read, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);
extern int CheckEcho(int countData, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);
extern void CheckErr(int result, BYTE* smInt, DWORD* tickUpdate);
extern void callbackDbg(BYTE smInt);
extern void gsmDebugPrint(char*);

extern int EventType; // use on FSWrite to ignore errors on delete command (forcing a correct response from modem)

static BYTE smInternal = 0; // State machine for internal use of callback functions
static BYTE maxtimeout = 2;

static char* xFSFilename;
static BYTE* xFSBuffer;
static unsigned int xFSDatasize;
static int* xFSSize;
static __eds__ BYTE* xFSBufferEDS;

/// @endcond

/**
\defgroup SARA Flash Files
@{
FSlib contains the functions to manage Hilo internal flash files

*/

/**
 * Writes data on a file. If files exists, this function overrides all data.
 * \param filename - char[] with name of file to write
 * \param dataBuffer - BYTE[] with data to write
 * \param dataSize - amount of data to write
 * \return None
 */
void FSWrite(char* filename, BYTE* dataBuffer, unsigned int dataSize)
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
			mainOpStatus.Function = 30;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xFSFilename = filename;
			xFSBuffer = dataBuffer;
			xFSDatasize = dataSize;
			
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
int cFSWrite()
{
	char cmdReply[200];
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
	char sizeStr[10];
	
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
			// ----------	KFSFILE File Delete Command ----------
			// AT+UDELFILE="filename"\r
			sprintf(msg2send, "AT+UDELFILE=\"");
			GSMWrite(msg2send);
			GSMWrite(xFSFilename);
			GSMWrite("\"\r");
			
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;	
				
		case 2:
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
				int dummyLen = strlen(xFSFilename) + 2;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			
		case 3:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
			// Get OK
			sprintf(msg2send, "\r\nOK\r\n");
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
            // Special case:  ERROR responde means no file named as xFSFilename content was found, so delete it is impossible!
//            if(     (resCheck == 1) &&  // Received ERROR response
//                    (mainOpStatus.ExecStat == OP_SYNTAX_ERR) && 
//                    (mainGSMStateMachine == SM_GSM_IDLE) )
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
				
		case 4:	
			// ----------	KFSFILE File Write Command ----------
			// AT+UDWNFILE=<filename>,<size>[,<tag>]
			sprintf(sizeStr, "%d\r", xFSDatasize);
            sprintf(msg2send, "AT+UDWNFILE=\"");
			GSMWrite(msg2send);
			GSMWrite(xFSFilename);
			GSMWrite("\",");
			GSMWrite(sizeStr);
			
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;	
				
		case 5:
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
                int dummyLen = 0;
                if(strstr(mainGSM.ModelVersion, "SARA-U201") != NULL)
                    dummyLen = strlen(xFSFilename) + strlen(sizeStr) + 1;
                else
                    dummyLen = strlen(xFSFilename) + strlen(sizeStr) + 2;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
                    cmdReply[rxcnt] = '\0';
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			
		case 6:
			// Check Reply ">"
			vTaskDelay(2);
			sprintf(msg2send, "\r\n>");
			chars2read = 1;
			countData = 0; // GSM buffer should be: <CR><LF>> 
			
			// here it is not possible to use the "getAnswer" (so neither CheckCmd) function 
			// since there is no '\n' chars...
			// for this case only we have to use a custom loop:
			int lenAnsw = strlen(msg2send);
            maxtimeout = 30;
            waitForGsmMsg(msg2send, maxtimeout);

			GSMpRead(cmdReply, lenAnsw);
			cmdReply[lenAnsw] = '\0';
			
			if(strstr(cmdReply, msg2send) != NULL)
			{
				// Reply present! Read until the lineNumber occurrence of '\n' char are found...
				int cc = GSMRead(cmdReply, lenAnsw);//GSMReadN(200, '\n', chars2read, cmdReply);
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
            
        case 7:
            {
                // Send Data
                int cc;
                for(cc = 0; cc < xFSDatasize; cc++)
                {
                    GSMWriteCh(xFSBuffer[cc]);
                }
                // Start timeout count
                tick = TickGetDiv64K(); // 1 tick every seconds
                // Update max timeout for this command: 30 seconds
                maxtimeout = 30;
                smInternal++;
            }
            
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
 * Reads data from a file. 
 * \param filename - char[] with name of file to read
 * \param dataBuffer - BYTE[] to fill with data. <I>Please, remember to use the "&" operator</I>
 * \param dataSize - amount of data to read
 * \return None
 */
void FSRead(char* filename, BYTE* dataBuffer, unsigned int dataSize)
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
			mainOpStatus.Function = 31;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xFSFilename = filename;
			xFSBuffer = dataBuffer;
			xFSDatasize = dataSize;
			
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
int cFSRead()
{
	char cmdReply[200];
	
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
	char sizeStr[10];
	
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
			// ----------	KFSFILE File Read Command ----------
            // AT+URDBLOCK=<filename>,<offset>,<size>[,<tag>]
			sprintf(sizeStr, "%d\r", xFSDatasize);
			sprintf(msg2send, "AT+URDBLOCK=\"");
			GSMWrite(msg2send);
			GSMWrite(xFSFilename);
			GSMWrite("\",0,");
			GSMWrite(sizeStr);
			
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;	
				
		case 2:
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
				int dummyLen = 0;
                if(strstr(mainGSM.ModelVersion, "SARA-U201") != NULL)
                    dummyLen = strlen(xFSFilename) + strlen(sizeStr) + 4; //6;
                else
                    dummyLen = strlen(xFSFilename) + strlen(sizeStr) + 4; //6;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			
		case 3:
            vTaskDelay(20);
            // Get reply +URDBLOCK: filename,<size>,<data_content>
            if(strstr(mainGSM.ModelVersion, "SARA-U201") != NULL)
                sprintf(msg2send, "\r\n+URDBLOCK: \"%s\",", xFSFilename);
            else
                sprintf(msg2send, "\r\n+URDBLOCK: %s,", xFSFilename);
            
			chars2read = 0;
			countData = 2; // GSM buffer should be: <CR><LF>+URDBLOCK: filename,...params...<CR><LF>
			
            // using GSMReadN we can read data without clearing a the buffer, searching for a specific char (counting it a provided number of times)            
			// here it is not possible to use the "getAnswer" (so neither CheckCmd) function 
			// since there is no '\n' chars...
			// for this case only we have to use a custom loop:
			int lenAnsw = strlen(msg2send);
            maxtimeout = 60*3+2;
            waitForGsmLines(chars2read, maxtimeout);
			int readCount = GSMpRead(cmdReply, lenAnsw+2);
			cmdReply[lenAnsw+2] = '\0';
            if((readCount > 0)&&(readCount < lenAnsw+2))
                cmdReply[readCount] = '\0';
            
			if(strstr(cmdReply, msg2send) != NULL)
			{
				// Reply present! Read until the lineNumber occurrence of '\n' char are found...
				int cc = GSMReadN(200, ',', 2, cmdReply); // Reads all the Buffer until 2 ',' are found (in this way we have read also the <size> parameter!)
				cmdReply[cc] = '\0';
				resCheck = 0;
                
                // search for the first ',' char on cmdReply buffer to get <size> param
                int fn;
                char *sizePnt = cmdReply;
                sizePnt += 12;
                
                // Increase sizePnt until we detect the first ',' and then add a '\0' at the second ','...
                BOOL firstChar = FALSE;
                for(fn = 12; fn < 200; fn++)
                {
                    if((cmdReply[fn] == ',')||(cmdReply[fn] == '\r')||(cmdReply[fn] == '\n')||(cmdReply[fn] == '\0'))
                    {
                        if(firstChar == FALSE)
                            firstChar = TRUE;
                        else
                        {
                            cmdReply[fn] = '\0';
                            break;
                        }
                    }
                    else
                    {
                        if(firstChar == FALSE)
                            sizePnt++;
                    }
                }
                sizePnt++; // to point to the first digit and not the ',' char...
                int realFileSize = atoi(sizePnt);
                if(realFileSize <= xFSDatasize) // this is valid for few BYTEs than provided by user, or by errors on atoi (negative value)
                    xFSDatasize = realFileSize;
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
                // Read content data....
                if(xFSDatasize > 0)
                {
                    // Read first char to delete the '\"' character
                    GSMRead((char*)xFSBuffer, 1);
                    int rlen = GSMRead((char*)xFSBuffer, xFSDatasize);
                    if((rlen > 0) && (rlen < xFSDatasize))
                        xFSDatasize = rlen;
                    xFSBuffer[xFSDatasize] = '\0';
                    // Read another char to delete last '\"' character
                    char dummy[2];
                    GSMRead((char*)dummy, 1);
                }
            }
			
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
 * Reads data from a file and uses EDS BYTE array to store data. 
 * \param filename - char[] with name of file to read
 * \param dataBuffer - BYTE[] to fill with data. <I>Please, remember to use the "&" operator</I>
 * \param dataSize - amount of data to read
 * \return None
 */
void FSReadEDS(char* filename, __eds__ BYTE* dataBuffer, unsigned int dataSize)
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
			mainOpStatus.Function = 37;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xFSFilename = filename;
			xFSBufferEDS = dataBuffer;
			xFSDatasize = dataSize;
			
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
int cFSReadEDS()
{
	char cmdReply[200];
	
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
	char sizeStr[10];
	
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
            
            GSMFlush();
            
			// ----------	KFSFILE File Read Command ----------
			// AT+KFSFILE=1,<uri>,<NbData>\r
            // AT+URDBLOCK=<filename>,<offset>,<size>[,<tag>]
			sprintf(sizeStr, "%d\r", xFSDatasize);
			sprintf(msg2send, "AT+URDBLOCK=\"");
			GSMWrite(msg2send);
			GSMWrite(xFSFilename);
			GSMWrite("\",0,");
			GSMWrite(sizeStr);
			
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;	
				
		case 2:
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
//                _dbgwrite("Reading ECHO:");
				int dummyLen = strlen(xFSFilename) + strlen(sizeStr) + 4; //6;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
//                    _dbgwrite(cmdReply);
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
//                _dbgwrite("***\r\n");
			}
			
		case 3:
            //vTaskDelay(20);
            // Get reply +URDBLOCK: filename,<size>,<data_content>
            if(strstr(mainGSM.ModelVersion, "SARA-U201") != NULL)
                sprintf(msg2send, "\r\n+URDBLOCK: \"%s\",", xFSFilename);
            else
                sprintf(msg2send, "\r\n+URDBLOCK: %s,", xFSFilename);          
			chars2read = 0;
			countData = 2; // GSM buffer should be: <CR><LF>+URDBLOCK: filename,...params...<CR><LF>
			
            // using GSMReadN we can read data without clearing a the buffer, searching for a specific char (counting it a provided number of times)
			// here it is not possible to use the "getAnswer" (so neither CheckCmd) function 
			// since there is no '\n' chars...
			// for this case only we have to use a custom loop:
			int lenAnsw = strlen(msg2send);
			int lenAnsw2 = lenAnsw + strlen(sizeStr);   // if actual FTP file size is less than provided string, we could have a timeout condition here
                                                        // this is not a real problem since this timeout is not considered as error!
            maxtimeout = 60*3+2;
            tick = TickGetDiv64K();
            
            int l = GSMBufferSize();
            do
            {
                SARA_RTS_IO = 1; // Stop char reception from UART
                DelayMs(1);
                SARA_RTS_IO = 0; // Allow char reception form UART
                Delay10us(1);
                l = GSMBufferSize();
            } while(l < lenAnsw2);
            SARA_RTS_IO = 1; // Stop char reception from UART
            
			int readCount = GSMpRead(cmdReply, lenAnsw+2);
			cmdReply[lenAnsw+2] = '\0';
            if((readCount > 0)&&(readCount < lenAnsw+2))
                cmdReply[readCount] = '\0';
            
			if(strstr(cmdReply, msg2send) != NULL)
			{
				// Reply present! Read until the lineNumber occurrence of '\n' char are found...
				int cc = GSMReadN(200, ',', 2, cmdReply); // Reads all the Buffer until 2 ',' are found (in this way we have read also the <size> parameter!)
				cmdReply[cc] = '\0';
				resCheck = 0;
                
                // search for the first ',' char on cmdReply buffer to get <size> param
                int fn;
                char *sizePnt = cmdReply;
                sizePnt += 12;
                
                // Increase sizePnt until we detect the first ',' and then add a '\0' at the second ','...
                BOOL firstChar = FALSE;
                for(fn = 12; fn < 200; fn++)
                {
                    if((cmdReply[fn] == ',')||(cmdReply[fn] == '\r')||(cmdReply[fn] == '\n')||(cmdReply[fn] == '\0'))
                    {
                        if(firstChar == FALSE)
                            firstChar = TRUE;
                        else
                        {
                            cmdReply[fn] = '\0';
                            break;
                        }
                    }
                    else
                    {
                        if(firstChar == FALSE)
                            sizePnt++;
                    }
                }
                sizePnt++; // to point to the first digit and not the ',' char...
                int realFileSize = atoi(sizePnt);
                if(realFileSize <= xFSDatasize) // this is valid for few BYTEs than provided by user, or by errors on atoi (negative value)
                    xFSDatasize = realFileSize;
                
                SARA_RTS_IO = 0; // Allow char reception form UART
			}
			else
            {
                SARA_RTS_IO = 0; // Allow char reception form UART
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
                tick = TickGetDiv64K();
                // Read content data....
                if(xFSDatasize > 0)
                {
                    // Read first char to delete the '\"' character
                    GSMRead((char*)cmdReply, 1);
                    // Read data from file
                    unsigned int rxCount = 0;
                    cmdReply[0] = '\0';
                    while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(rxCount < xFSDatasize))
                    {
                        char tmp[2];
                        int rxcnt = GSMRead(tmp, 1);
                        if(rxcnt != 0)
                        {
                            tick = TickGetDiv64K(); // 1 tick every seconds
                            *(xFSBufferEDS+rxCount) = tmp[0];
                            rxCount += rxcnt;
                        }
                    }
                    *(xFSBufferEDS+xFSDatasize) = '\0';
                    
                    // Read another char to delete last '\"' character
                    GSMRead((char*)cmdReply, 1);
                    cmdReply[0] = '\0';
                    // Set tcpReadBufferCount as the effective number of BYTEs read
                    xFSDatasize = rxCount;
                }
            }
            SARA_RTS_IO = 0; // Allow char reception form UART
			
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
 * Deletes file from flash memory. 
 * \param filename - char[] with name of file to delete
 * \return None
 */
void FSDelete(char* filename)
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
			mainOpStatus.Function = 32;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xFSFilename = filename;
			
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
int cFSDelete()
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
			// ----------	KFSFILE File Delete Command ----------
			// AT+UDELFILE="filename"\r
			sprintf(msg2send, "AT+UDELFILE=\"");
			GSMWrite(msg2send);
			GSMWrite(xFSFilename);
			GSMWrite("\"\r");
			
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;	
				
		case 2:
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
				int dummyLen = strlen(xFSFilename) + 2;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			
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
 * Provide file size (in bytes). 
 * \param filename - char[] with name of file
 * \param dataSize - int number of data size of file.<I>Please, remember to use the "&" operator</I>
 * \return None
 */
void FSSize(char* filename, int* dataSize)
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
			mainOpStatus.Function = 33;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xFSFilename = filename;
			xFSSize = dataSize;
			
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
int cFSSize()
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
			// ----------	KFSFILE File Size Command ----------
			// AT+ULSTFILE=2,<filename>\r
			sprintf(msg2send, "AT+ULSTFILE=2,\"");
			GSMWrite(msg2send);
			GSMWrite(xFSFilename);
			GSMWrite("\"\r");
			
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 60;//2+3;
			smInternal++;

		case 2:
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
				int dummyLen = 0; //strlen(xFSFilename) + 4;
                if(strstr(mainGSM.ModelVersion, "SARA-U201") != NULL)
                    dummyLen = strlen(xFSFilename) + 2;
                else
                    dummyLen = strlen(xFSFilename) + 2;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			
		case 3:
			// Get reply (\r\n+ULSTFILE: <size>\r\n)
			sprintf(msg2send, "\r\n+ULSTFILE: ");
			chars2read = 2;
			countData = 0; 
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				// Get size
				char temp[50];
				int res = getfield(':', '\n', 25, 1, cmdReply, temp, 500);
				if(res != 1)
				{
					// Execute Error Handler
					gsmDebugPrint("Error in getfield for +ULSTFILE socket\r\n");
					*xFSSize = 0;
					break;
				}
				else
				{
					*xFSSize = atoi(temp);
				}
			}
			
			
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
 * Appends data on a file.
 * \param filename - char[] with name of file to write
 * \param dataBuffer - BYTE[] with data to write
 * \param dataSize - amount of data to write
 * \return None
 */
void FSAppend(char* filename, BYTE* dataBuffer, unsigned int dataSize)
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
			mainOpStatus.Function = 34;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xFSFilename = filename;
			xFSBuffer = dataBuffer;
			xFSDatasize = dataSize;
			
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
int cFSAppend()
{
	char cmdReply[200];
	
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
	char sizeStr[10];
	
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
			// ----------	KFSFILE File Write Command ----------
			// AT+UDWNFILE=<filename>,<size>[,<tag>]
			sprintf(sizeStr, "%d\r", xFSDatasize);
//			sprintf(msg2send, "AT+KFSFILE=0,\"/ftp/");
            sprintf(msg2send, "AT+UDWNFILE=\"");
			GSMWrite(msg2send);
			GSMWrite(xFSFilename);
			GSMWrite("\",");
			GSMWrite(sizeStr);
			
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2;
			smInternal++;	
				
		case 2:
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
                int dummyLen = 0;
                if(strstr(mainGSM.ModelVersion, "SARA-U201") != NULL)
                    dummyLen = strlen(xFSFilename) + strlen(sizeStr) + 1;
                else
                    dummyLen = strlen(xFSFilename) + strlen(sizeStr) + 2;
				while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
				{
					int rxcnt = GSMRead(cmdReply, 1);
					dummyLen -= rxcnt;
                    if(rxcnt != 0)
                        tick = TickGetDiv64K(); // 1 tick every seconds
				}
				cmdReply[0] = '\0';
			}
			
		case 3:
			// Check Reply ">"
			vTaskDelay(2);
			sprintf(msg2send, "\r\n>");
			chars2read = 1;
			countData = 0; // GSM buffer should be: <CR><LF>> 
			
			// here it is not possible to use the "getAnswer" (so neither CheckCmd) function 
			// since there is no '\n' chars...
			// for this case only we have to use a custom loop:
			int lenAnsw = strlen(msg2send);
            maxtimeout = 30;
            waitForGsmMsg(msg2send, maxtimeout);

            
			GSMpRead(cmdReply, lenAnsw);
			cmdReply[lenAnsw] = '\0';
			
			if(strstr(cmdReply, msg2send) != NULL)
			{
				// Reply present! Read until the lineNumber occurrence of '\n' char are found...
				int cc = GSMRead(cmdReply, lenAnsw);//GSMReadN(200, '\n', chars2read, cmdReply);
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
            
        case 4:
            {
                // Send Data
                int cc;
                for(cc = 0; cc < xFSDatasize; cc++)
                {
                    GSMWriteCh(xFSBuffer[cc]);
                }
                // Start timeout count
                tick = TickGetDiv64K(); // 1 tick every seconds
                // Update max timeout for this command: 30 seconds
                maxtimeout = 30;
                smInternal++;
            }
            
		case 5:
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
// @endcond


/**
 * Formats FTP flash memory Folder.
 */
void FSFormat()
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
			mainOpStatus.Function = 35;
			mainOpStatus.ErrorCode = 0;
						
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
int cFSFormat()
{
	char cmdReply[201];
	char msg2send[201];
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
    char formatFileName[150];
	
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
			// ----------	ULSTFILE List Directory and File Information Command ----------
			// AT+ULSTFILE=0\r
            vTaskDelay(20);
            GSMFlush();
			sprintf(msg2send, "AT+ULSTFILE=0\r");
			GSMWrite(msg2send);

#if defined __DEBUG
#warning debug (__DEBUG) flag defined!!!
            vTaskDelay(50);
#endif
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 20;
			smInternal++;	
				
		case 2:
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
//                // Search for the '+' char on buffer of first List Response:
//                int p, plusPos = -1;
//                
//                while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(plusPos == -1))
//                {
//                    int buffLen = GSMBufferSize();
//                    if(buffLen > 100)
//                    {
//                        buffLen = 100;
//                    }
//                    int rxcnt = GSMpRead(cmdReply, buffLen);
//                    
//                    // Where is the '+' char?
//                    for(p = 0; p < rxcnt; p++)
//                    {
//                        if(cmdReply[p] == '+')
//                        {
//                            plusPos = p;
//                            
//                            // Read all previous data from buffer:
//                            GSMRead(cmdReply, plusPos);
//                            cmdReply[plusPos] = '\0';
//                            
//                            p = rxcnt + 1; // quit loop 
//                        }
//                    }
//                }
//				cmdReply[0] = '\0'; // put '\0' to "clear" strlen array size...
			}
			
		case 3:
            // Now get the list of all files:
            
			// Get reply \r\n+ULSTFILE:+ULSTFILE: [<filename1>[,<filename2>[,...[,<filenameN>]]]]\r\n
			vTaskDelay(1);
			// Get +KFSFILE: 
			sprintf(msg2send, "\r\n+ULSTFILE: ");
			chars2read = 2;
			countData = 0;
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            else
            {
                char  replyMessConst[] = "\r\n+ULSTFILE: ";
                char* fileNamePnt = strstr(cmdReply, replyMessConst);
                
                // First file name is located starting from ": " and ',' or "\r\n" (if there is only one file stored!)
                if(fileNamePnt != NULL)
                {
                    char* firstFileNamePnt = fileNamePnt;
                    firstFileNamePnt += strlen(replyMessConst);
                    if((*firstFileNamePnt != '\r')&&(*firstFileNamePnt != '\n')&&(*firstFileNamePnt != '\0'))
                    {
                        fileNamePnt += strlen(replyMessConst);
                        if(*fileNamePnt == '\"')
                            fileNamePnt++;
#ifdef __DEBUG
                        _dbgwrite("fileNamePnt:"); _dbgline(fileNamePnt);
#endif
                        // Get file name
                        int fn;
                        for(fn = 0; fn < 150; fn++)
                        {
                            formatFileName[fn] = *fileNamePnt;
//                            *(xFSFilename+fn) = formatFileName[fn];
                            if((formatFileName[fn] == ' ')||(formatFileName[fn] == '\"')||(formatFileName[fn] == ',')||(formatFileName[fn] == '\r')||(formatFileName[fn] == '\n'))
                            {
                                formatFileName[fn] = '\0';
//                                *(xFSFilename+fn) = formatFileName[fn];
#ifdef __DEBUG
                                _dbgwrite("formatFileName:"); _dbgline(formatFileName);
#endif
                                break;
                            }
                            fileNamePnt++;
                        }
                        // Remove all the buffer until "OK\r" message:
                        char okStr[5] = {0, 0, 0, 0, 0};
                        int c;
                        do
                        {
                            int blen = abs(GSMBufferSize());
                            if(blen > 200)
                                blen = 200;
                            
                            blen = abs(GSMRead(cmdReply, blen));
                            int lll = 0;
                            while(lll < blen)
                            {
                                lll++;
                                tick = TickGetDiv64K(); // 1 tick every seconds
                                // add data to okStr fifo:
                                for(c = 4; c > 0; c--)
                                {
                                    okStr[c] = okStr[c-1];
                                }
                                okStr[0] = cmdReply[lll];

                                if( (okStr[3] == 'O') &&
                                    (okStr[2] == 'K') &&
                                    (okStr[1] == '\r') )
                                {
                                    break;
                                }
                            }
                            
                            if(abs(GSMBufferSize() == 0))
                            {
                                vTaskDelay(10);
                            }
                        } while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(abs(GSMBufferSize()) > 0));
                        
                        GSMFlush();
                        
                        // set smInternal to case 5
                        smInternal = 5;
                        return -1;
                    }
                    else 
                    {
                        formatFileName[0] = '\0';
                        // It's the end of file listing...
                        smInternal++;
                    }
                }
            }
            
        case 4: // End file deleting...
            {
                smInternal = 100;
                formatFileName[0] = '\0';
            }
        
        case 5:
            {
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
                }
            }
            
        case 6: // Delete File and Try again with file listing...
            {
                // ----------	KFSFILE File Delete Command ----------
#ifdef __DEBUG
                _dbgwrite("Deleting file:"); _dbgline(formatFileName);
#endif
                // AT+UDELFILE=<filename>\r
                sprintf(msg2send, "AT+UDELFILE=\"");
                GSMWrite(msg2send);
                GSMWrite(formatFileName);
                GSMWrite("\"\r");
                
#ifdef __DEBUG
                vTaskDelay(50);
#endif
                        // Get file name
                // Start timeout count
                tick = TickGetDiv64K(); // 1 tick every seconds
                maxtimeout = 20;
                smInternal++;	
            }
				
		case 7:
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
                else
                {
                    int dummyLen = 0;
                    if(strstr(mainGSM.ModelVersion, "SARA-U201") != NULL)
                        dummyLen = strlen(formatFileName) + 2;
                    else
                        dummyLen = strlen(formatFileName) + 2;
                    while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(dummyLen > 0))
                    {
                        int rxcnt = GSMRead(cmdReply, 1);
                        dummyLen -= rxcnt;
                        if(rxcnt != 0)
                            tick = TickGetDiv64K(); // 1 tick every seconds
                    }
                    cmdReply[0] = '\0';
                }
            }
			
		case 8:
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
                else
                {
                    // Repeat the command 
                    GSMFlush();
                    smInternal = 1;
                    return -1;
                }
            }
				
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
 * FSGetFirstFileName Gets first file name listed by Hilo internal flash memory. 
 * \param filename - char[] with name of file to get
 * \return None - LastExecStat() will be OP_SYNTAX_ERR (-2) with error code -1 if no files are found... 
 */
void FSGetFirstFileName(char *filename)
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
			mainOpStatus.Function = 36;
			mainOpStatus.ErrorCode = 0;
						
            xFSFilename = filename;
            
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
int cFSGetFirstFileName()
{
	char cmdReply[201];
	char msg2send[201];
	int resCheck = 0;
	static DWORD tick;
	int countData;
	int chars2read;
    char formatFileName[150];
	
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
			// ----------	ULSTFILE List Directory and File Information Command ----------
			// AT+ULSTFILE=0\r
            vTaskDelay(20);
            GSMFlush();
			sprintf(msg2send, "AT+ULSTFILE=0\r");
			GSMWrite(msg2send);

#if defined __DEBUG
#warning debug (__DEBUG) flag defined!!!
            vTaskDelay(50);
#endif
			
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 20;
			smInternal++;

				
		case 2:
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
//                // Search for the '+' char on buffer of first List Response:
//                int p, plusPos = -1;
//                
//                while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(plusPos == -1))
//                {
//                    int buffLen = GSMBufferSize();
//                    if(buffLen > 100)
//                    {
//                        buffLen = 100;
//                    }
//                    int rxcnt = GSMpRead(cmdReply, buffLen);
//                    
//                    // Where is the '+' char?
//                    for(p = 0; p < rxcnt; p++)
//                    {
//                        if(cmdReply[p] == '+')
//                        {
//                            plusPos = p;
//                            
//                            // Read all previous data from buffer:
//                            GSMRead(cmdReply, plusPos);
//                            cmdReply[plusPos] = '\0';
//                            
//                            p = rxcnt + 1; // quit loop 
//                        }
//                    }
//                }
//				cmdReply[0] = '\0'; // put '\0' to "clear" strlen array size...
			}
			
		case 3:
            // Now get the list of all files:
            
			// Get reply \r\n+ULSTFILE:+ULSTFILE: [<filename1>[,<filename2>[,...[,<filenameN>]]]]\r\n
			vTaskDelay(1);
			// Get +KFSFILE: 
			sprintf(msg2send, "\r\n+ULSTFILE: ");
			chars2read = 2;
			countData = 0;
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            else
            {
                char  replyMessConst[] = "\r\n+ULSTFILE: ";
                char* fileNamePnt = strstr(cmdReply, replyMessConst);
                
                // First file name is located starting from ": " and ',' or "\r\n" (if there is only one file stored!)
                if(fileNamePnt != NULL)
                {
                    char* firstFileNamePnt = fileNamePnt;
                    firstFileNamePnt += strlen(replyMessConst);
                    if((*firstFileNamePnt != '\r')&&(*firstFileNamePnt != '\n')&&(*firstFileNamePnt != '\0'))
                    {
                        fileNamePnt += strlen(replyMessConst);
                        if(*fileNamePnt == '\"')
                            fileNamePnt++;
#ifdef __DEBUG
                        _dbgwrite("fileNamePnt: "); _dbgline(fileNamePnt);
#endif
                        // Get file name
                        int fn;
                        for(fn = 0; fn < 150; fn++)
                        {
                            formatFileName[fn] = *fileNamePnt;
                            *(xFSFilename+fn) = formatFileName[fn];
                            if((formatFileName[fn] == ' ')||(formatFileName[fn] == '\"')||(formatFileName[fn] == ',')||(formatFileName[fn] == '\r')||(formatFileName[fn] == '\n'))
                            {
                                formatFileName[fn] = '\0';
                                *(xFSFilename+fn) = formatFileName[fn];
                                break;
                            }
                            fileNamePnt++;
                        }
                        // Remove all the buffer until "OK\r" message:
                        char okStr[5] = {0, 0, 0, 0, 0};
                        int c;
                        do
                        {
                            int blen = abs(GSMBufferSize());
                            if(blen > 200)
                                blen = 200;
                            
                            blen = abs(GSMRead(cmdReply, blen));
                            int lll = 0;
                            while(lll < blen)
                            {
                                lll++;
                                tick = TickGetDiv64K(); // 1 tick every seconds
                                // add data to okStr fifo:
                                for(c = 4; c > 0; c--)
                                {
                                    okStr[c] = okStr[c-1];
                                }
                                okStr[0] = cmdReply[lll];

                                if( (okStr[3] == 'O') &&
                                    (okStr[2] == 'K') &&
                                    (okStr[1] == '\r') )
                                {
                                    break;
                                }
                            }
                            
                            if(abs(GSMBufferSize() == 0))
                            {
                                vTaskDelay(10);
                            }
                        } while(((TickGetDiv64K() - tick) < (maxtimeout + 2))&&(abs(GSMBufferSize()) > 0));
                        
                        GSMFlush();
                        
                        // set smInternal to case 5
                        smInternal = 5;
                        return -1;
                    }
                    else 
                    {
                        xFSFilename[0] = '\0';
                        // It's the end of file listing...
                        smInternal++;
                    }
                }
            }
            
        case 4: // End file deleting...
            {
                xFSFilename[0] = '\0';
                smInternal = 0;
                // Cmd = 0 only if the last command successfully executed
                mainOpStatus.ExecStat = OP_SYNTAX_ERR;
                mainOpStatus.Function = 0;
                mainOpStatus.ErrorCode = -1;
                mainGSMStateMachine = SM_GSM_IDLE;
                return -1;
            }
            break;
        
        case 5:
            {
                
            }
            break;
            
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

/*! @} */
/*! @} */
/*! @} */
