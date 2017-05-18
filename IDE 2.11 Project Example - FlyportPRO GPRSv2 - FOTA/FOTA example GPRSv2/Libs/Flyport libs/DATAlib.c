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
 *  FileName:        DATAlib.c
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
#include "DATAlib.h"

//extern int Cmd;
extern int mainGSMStateMachine;
extern OpStat	mainOpStatus;
extern GSMModule mainGSM;

extern int CheckCmd(int countData, int chars2read, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);
extern int CheckEcho(int countData, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);
extern void CheckErr(int result, BYTE* smInt, DWORD* tickUpdate);
extern int EventType;
extern char msg2send[200];

//extern void callbackDbg(BYTE smInt);

static BYTE smInternal = 0; // State machine for internal use of callback functions
static BYTE maxtimeout = 2;

static char* xApn;
static char* xLogin;
static char* xPassw;
static char* xIp;
static char* xDns1;
static char* xDns2;

static int xAuth;

static IP_ADDR _lastIpKnown;
/// @endcond

/**
\defgroup APN Data Connection
@{
DATAlib contains the functions to setup APN parameters

*/

/**
 * Prints the MODEM IP String (if any) to provided char*
 * @param ipStr char array to store IP STRING
 */
void LastIp(char* ipStr)
{
    return IPAddressToString(_lastIpKnown, ipStr);
}

/**
 * Configures APN params for data connections. Please, contact your SIM mobile operator to retrive more info.
 * \param apn - APN host name
 * \param login - login name - if not used provide NULL parameter
 * \param passw - password name - if not used provide NULL parameter
 * \param ip - IP address of modem. Example: "DYNAMIC" or "192.168.1.100" (the char array must be NULL terminated).
 * \param dns1 - IP address of primary dns. Example: "DYNAMIC" or "192.168.1.100" (the char array must be NULL terminated). 
 * \param dns2 - IP address of secondary dns. Example: "DYNAMIC" or "192.168.1.100" (the char array must be NULL terminated). 
 * \return - none.
 */
void APNConfig(char* apn, char* login, char* passw, char* ip, char* dns1, char* dns2)
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
			mainOpStatus.Function = 26;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xApn = apn;
			xLogin = login;
			xPassw = passw;
			xIp = ip;
			xDns1 = dns1;
			xDns2 = dns2;
            xAuth = -1;
			
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
 * Configures APN params for data connections. Please, contact your SIM mobile operator to retrive more info.
 * \param apn - APN host name
 * \param login - login name - if not used provide NULL parameter
 * \param passw - password name - if not used provide NULL parameter
 * \param ip - IP address of modem. Example: "DYNAMIC" or "192.168.1.100" (the char array must be NULL terminated).
 * \param dns1 - IP address of primary dns. Example: "DYNAMIC" or "192.168.1.100" (the char array must be NULL terminated). 
 * \param dns2 - IP address of secondary dns. Example: "DYNAMIC" or "192.168.1.100" (the char array must be NULL terminated). 
 * \params authMethod - Useful to specify the AUTH method used ( APN_AUTH_NONE, APN_AUTH_PAP, APN_AUTH_CHAP )
 * \return - none.
 */
void APNConfigAuth(char* apn, char* login, char* passw, char* ip, char* dns1, char* dns2, int authMethod)
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
			mainOpStatus.Function = 26;
			mainOpStatus.ErrorCode = 0;
			
			// Set Params	
			xApn = apn;
			xLogin = login;
			xPassw = passw;
			xIp = ip;
			xDns1 = dns1;
			xDns2 = dns2;
            xAuth = authMethod;
			
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
//	cAPNConfig callback function
//****************************************************************************
int cAPNConfig()
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
            // If there is a opened PSD connection, close it!
            sprintf(msg2send, "AT+UPSDA=0,4\r");
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2 + 3;
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
			
            // Error 148 is Unspecified GPRS error (no connection yet...)
            // Error 3 (SARA-U201 3G)
            if( ((resCheck == 3) && (mainOpStatus.ExecStat == OP_CME_ERR) && (mainOpStatus.ErrorCode == 148)) || 
                ((resCheck == 3) && (mainOpStatus.ExecStat == OP_CME_ERR) && (mainOpStatus.ErrorCode == 3)) )
            {
                // Avoid to handle such error, we assume it is not a problem in this condition!
                resCheck = 0;
                mainOpStatus.ExecStat = OP_EXECUTION;
                mainOpStatus.ErrorCode = 0;
                // Clear event type:
                EventType = NO_EVENT;	
                mainGSMStateMachine = SM_GSM_CMD_PENDING;
                smInternal=4;
            }
            
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			/* no break */
            
        case 4:
            // Restore Factory params!
            sprintf(msg2send, "AT+UPSDA=0,0\r");
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2 + 3;
			smInternal++;
			/* no break */
			
		case 5:
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
			
		case 6:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
			// Get OK
			sprintf(msg2send, "\r\nOK\r\n");
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
            
            if((resCheck == 3) && (mainOpStatus.ExecStat == OP_CME_ERR) && (mainOpStatus.ErrorCode == 148)) // Error 148 is Unspecified GPRS error (no connection yet...)
            {
                // Avoid to handle such error, we assume it is not a problem in this condition!
                resCheck = 0;
                mainOpStatus.ExecStat = OP_EXECUTION;
                mainOpStatus.ErrorCode = 0;
                // Clear event type:
                EventType = NO_EVENT;	
                mainGSMStateMachine = SM_GSM_CMD_PENDING;
                smInternal = 7;
            }
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			/* no break */
				
		case 7:	
			// Send first AT command
			// ----------	APN Configuration	----------
            // Sara modules do not have a single-shot AT command to setup PDP context for internal Stack usages,
            // instead we have to setup each parameter one by one using 
            // AT+UPSD=<profile_id>,<param_tag>,<param_val>
            // 
            // The params <param_tag> we have to set are:
            // 1. Apn Name (xApn)
            // 2. User Name (xLogin)
            // 3. Password (xPassw)
            // 7. Ip Address (xIp)
            // 4. DNS1 (xDns1)
            // 5. DNS2 (xDns2)
            
            // Start with xApn:
            if((xApn == NULL)||(xApn[0] == '\0')) //||(strcmp("", xApn) != 0))
            {
                smInternal += 3;
				mainGSMStateMachine = SM_GSM_CMD_PENDING;
				return -1;
            }
            sprintf(msg2send, "AT+UPSD=0,1,\"%s\"\r", xApn);
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2 + 3;
			smInternal++;
			/* no break */
			
		case 8:
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
			
		case 9:
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
            
        case 10:
            if((xLogin == NULL)||(xLogin[0] == '\0'))//||(strcmp("", xLogin) != 0))
            {
                smInternal += 3;
				mainGSMStateMachine = SM_GSM_CMD_PENDING;
				return -1;
            }
            // xLogin:
            sprintf(msg2send, "AT+UPSD=0,2,\"%s\"\r", xLogin);
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2 + 3;
			smInternal++;
			/* no break */
			
		case 11:
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
			
		case 12:
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
            
        case 13:
            // xPassw:
            if((xPassw == NULL)||(xPassw[0] == '\0'))//||(strcmp("", xPassw) != 0))
            {
                smInternal += 3;
				mainGSMStateMachine = SM_GSM_CMD_PENDING;
				return -1;
            }
            sprintf(msg2send, "AT+UPSD=0,3,\"%s\"\r", xPassw);
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2 + 3;
			smInternal++;
			/* no break */
			
		case 14:
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
			
		case 15:
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
            
        case 16:
            // xIp:
            if(strstr(xIp,DYNAMIC_IP) != NULL)
                sprintf(msg2send, "AT+UPSD=0,7,\"%s\"\r", "0.0.0.0");
            else
                sprintf(msg2send, "AT+UPSD=0,7,\"%s\"\r", xIp);
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2 + 3;
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
			
        case 19:
            // xDns1:
            if(strstr(xDns1,DYNAMIC_IP) != NULL)
            	sprintf(msg2send, "AT+UPSD=0,4,\"%s\"\r", "0.0.0.0");
            else
                sprintf(msg2send, "AT+UPSD=0,4,\"%s\"\r", xDns1);
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2 + 3;
			smInternal++;
			/* no break */
			
		case 20:
            
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
			
		case 21:
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
			
        case 22:
            // xDns2:
        	if(strstr(xDns2,DYNAMIC_IP) != NULL)
        		sprintf(msg2send, "AT+UPSD=0,5,\"%s\"\r", "0.0.0.0");
            else
                sprintf(msg2send, "AT+UPSD=0,5,\"%s\"\r", xDns2);
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2 + 3;
			smInternal++;
			/* no break */
			
		case 23:
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
			
		case 24:
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
            
        case 25:
            // apn auth:
            if(xAuth >= 0)
            {
                sprintf(msg2send, "AT+UPSD=0,6,%d\r", xAuth);
                GSMWrite(msg2send);
                // Start timeout count
                tick = TickGetDiv64K(); // 1 tick every seconds
                maxtimeout = 2 + 3;
            }
            smInternal++;
            /* no break */
			
		case 26:
            if(xAuth >= 0)
            {
                vTaskDelay(20);
                // Check ECHO 
                countData = 0;

                resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);

                CheckErr(resCheck, &smInternal, &tick);

                if(resCheck)
                {
                    return mainOpStatus.ErrorCode;
                }
            }
            smInternal++;
            /* no break */
            
		case 27:
            if(xAuth >= 0)
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
            smInternal++;
            /* no break */
            
        case 28:
            // Activate PDP context profile
            // AT+UPSDA=<profile_id>,3
            sprintf(msg2send, "AT+UPSDA=0,3\r");
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2 + 3;
			smInternal++;
			/* no break */
			
		case 29:
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
			
		case 30:
			// Get reply (\r\nOK\r\n)
			vTaskDelay(1);
			// Get OK
			maxtimeout = 3*60 + 2; // up to 3 minutes of timeout!
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
            
        case 31:
            // Now read assigned IP
            // AT+UPSND=<profile_id>,0
            sprintf(msg2send, "AT+UPSND=0,0\r");
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			maxtimeout = 2 + 3;
			smInternal++;
            /* no break */
            
        case 32:
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
            
        case 33:
            // Read response:
            // Get reply "\r\n+UPSND: <profile_id>,<param_tag>,<dynamic_param_val>\r\n"  - (for example "\r\n+UPSND: 0,0,"93.68.225.175"\r\n")
			vTaskDelay(20);
			sprintf(msg2send, "\r\n+UPSND");
			chars2read = 2;
			countData = 0; // GSM buffer should be: <CR><LF>+UPSND: <profile_id>,<param_tag>,<dynamic_param_val><CR><LF>
			
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				// Get <profile_id>
				char temp[50];
				int res = getfield(':', ',', 5, 1, cmdReply, temp, 500);
				if(res != 1)
				{
					// Execute Error Handler
					_dbgwrite( "Error in getfield #1 for +UPSND command\r\n");
					break;
				}
				else
				{
					//xSocket->status = atoi(temp);
				}
				
				// Get <param_tag>
				res = getfield(',', ',', 5, 1, cmdReply, temp, 500);
				if(res != 1)
				{
					// Execute Error Handler
					_dbgwrite( "Error in getfield #2 for +UPSND command\r\n");
					break;
				}
				else
				{
					//xSocket->notif = atoi(temp);
				}
				
				// Get <dynamic_param_val>
				res = getfield('\"', '\"', 45, 1, cmdReply, temp, 500);
				if(res != 1)
				{
					// Execute Error Handler
					_dbgwrite( "Error in getfield #3 for +UPSND command\r\n");
//                    _dbgwrite("temp: "); _dbgline(temp);
					break;
				}
				else
				{
                    // Get IP ADDRESS:
                    IP_ADDR tempIP;
                    if(StringToIPAddress((BYTE*)temp, &tempIP)!=TRUE)
                    {
                        tempIP.Val = 0;
                    }
                    _lastIpKnown = tempIP;
				}
			}
			/* no break */
			
		case 34:
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
                smInternal++;
                break;
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

/*! @} */

/*! @} */
