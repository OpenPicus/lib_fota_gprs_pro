/**
\mainpage Main page
This guide is a quick reference for all the Flyport GPRS APIs. For each command and variable, is provided a brief description and the syntax, as a quick reference for the first-time and experienced users needing
to remind a specified command usage. This guide can directly be accessed by Flyport IDE (v.2.3 or greater) using the F1 key. If a keyword is highlighted, the guide will open to the specific keyword.<BR><BR>
This guide is NOT a programmer's guide or a complete reference to Flyport GPRS usage. To get examples and more detailed tutorials about Flyport, you can get it in the OpenPicus Wiki page: <a href="http://wiki.openpicus.com">wiki.openpicus.com</a>
*/
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
 *  FileName:        HILOlib.c
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
#include "Hilo.h"
#include "HILOlib.h"
#include "GSMData.h"
#include "DATAlib.h"

extern int Cmd;
extern int mainGSMStateMachine;
extern OpStat	mainOpStatus;
extern GSMModule mainGSM;
extern int signal_dBm_Gsm;

extern int CheckCmd(int countData, int chars2read, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);
extern int CheckEcho(int countData, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);
extern void CheckErr(int result, BYTE* smInt, DWORD* tickUpdate);
extern void gsmDebugPrint(char*);

static BYTE smInternal = 0; // State machine for internal use of callback functions
static BYTE maxtimeout = 2;

//static BOOL connVal = FALSE;
/// @endcond

/**
\defgroup HILO GSM management
@{
Provides IMEI and power management functions for GSM module.
*/

/**
 * Provides modem IMEI
 * \return char* with Hilo IMEI
 */
char* GSMGetIMEI()
{
	return (char*)mainGSM.IMEI;
}

/**
 * Provides Operator Name (16 char long)
 * \return char* with operator name
 */
char* GSMGetOperatorName()
{
    return (char*)mainGSM.OperatorName;
}

/**
 * Provides modem GPRS Packet Service network attach status
 * \return BOOL TRUE for Gprs Packet Service attached, FALSE otherwise
 */
BOOL  GSMGetGprsPacketServiceStatus()
{
    return mainGSM.IsGPRSPacketServiceReady;
}

/**
 * Turns GSM module OFF.
 * <B>Note:</B> Sleep and Hibernation are not suggested when your application needs to wake up more often than daily.
 */
void GSMHibernate()
{
	BOOL opok = FALSE;
	
	if(mainGSM.HWReady != TRUE)
		return;
	if(mainGSMStateMachine == SM_GSM_HIBERNATE)
		return;
	
	//	Function cycles until it is not executed
	while (!opok)
	{
		while (xSemaphoreTake(xSemFrontEnd,0) != pdTRUE);		//	xSemFrontEnd TAKE

		// Check mainOpStatus.ExecStat
		if (mainOpStatus.ExecStat != OP_EXECUTION)
		{		
			mainOpStatus.ExecStat = OP_EXECUTION;
			mainOpStatus.Function = 28;
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
int cGSMHibernate()
{
	// AT*PSCPOF -> response: OK
	char cmdReply[200];
	char msg2send[200];
	int resCheck = 0;
	DWORD tick;
	int countData = 0;
	int chars2read = 1;

	switch (smInternal)
	{
		case 0:
            // Pause to reduce the bugs on this command
            vTaskDelay(350);
			if(GSMBufferSize() > 0)
			{
				// Parse Unsol Message
				mainGSMStateMachine = SM_GSM_CMD_PENDING;
                vTaskDelay(250); // wait a while to complete incoming message...
				return -1;
			}
			else
				smInternal++;
		
		case 1:
            //
            HILO_POK_IO = 0;
            vTaskDelay(250);
            HILO_POK_IO = 1;
			sprintf(msg2send, "AT*PSCPOF\r");// sprintf(msg2send, "AT+CPOF\r");// sprintf(msg2send, "AT*PSCPOF\r");
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			// Real max timeout for this command: 5 seconds
			maxtimeout = 15;
			smInternal++;
			
		case 2:
			vTaskDelay(2);
			// Check ECHO 
			countData = 0;
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}


            maxtimeout = 15;
            smInternal++;
			
		case 3:
			// Get reply (OK, BUSY, ERROR, etc...)
			vTaskDelay(20);

			// Get OK
			sprintf(msg2send, "OK");
			chars2read = 2;
			countData = 2; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				
				// Patch for registration:
				mainGSM.ConnStatus = NO_REG;

//				HILO_POK_IO = 0;
//			   	vTaskDelay(250);
//			    HILO_POK_IO = 1;

//				int ctsTris = 0;
//				if(HILO_CTS_TRIS != 0)
//					ctsTris = 0;
				HILO_CTS_TRIS = 1;
				int cnt = 30;
				
				while ((cnt > 0)&&(HILO_CTS_IO != 0))
				{
					cnt--;
					vTaskDelay(20);
				}
				//HILO_CTS_TRIS = ctsTris;
			}

        case 4:
            if(HILO_CTS_IO != 0)
            {
                mainOpStatus.ExecStat = OP_HIB_ERR;
                //mainOpStatus.Function = 0;
                //mainOpStatus.ErrorCode = 0;
                //mainGSMStateMachine = SM_GSM_HIBERNATE;
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
	mainGSMStateMachine = SM_GSM_HIBERNATE;
	return -1;
	
}

int cGSMHibernate_GSM_DISABLED()
{
	// AT*PSCPOF -> response: OK
	char cmdReply[200];
	char msg2send[200];
	int resCheck = 0;
	DWORD tick;
	int countData = 0;
	int chars2read = 1;

	switch (smInternal)
	{
		case 0:
			if(GSMBufferSize() > 0)
			{
				// Parse Unsol Message
				mainGSMStateMachine = SM_GSM_CMD_PENDING;
				return -1;
			}
			else
				smInternal++;

		case 1:
			//
            HILO_POK_IO = 0;
            vTaskDelay(250);
            HILO_POK_IO = 1;
			sprintf(msg2send, "AT*PSCPOF\r");// sprintf(msg2send, "AT+CPOF\r");// sprintf(msg2send, "AT*PSCPOF\r");
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			// Real max timeout for this command: 5 seconds
			maxtimeout = 15;
			smInternal++;

		case 2:
			/*
            vTaskDelay(2);
            
			// Check ECHO
			countData = 0;
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            */
		case 3:
			// Get reply (OK, BUSY, ERROR, etc...)
			vTaskDelay(20);

			// Get OK
			sprintf(msg2send, "OK");
			chars2read = 2;
			countData = 2; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				
				// Patch for registration:
				mainGSM.ConnStatus = NO_REG;

//				HILO_POK_IO = 0;
//			   	vTaskDelay(250);
//			    HILO_POK_IO = 1;

//				int ctsTris = 0;
//				if(HILO_CTS_TRIS != 0)
//					ctsTris = 0;
				HILO_CTS_TRIS = 1;
				int cnt = 30;
				
				while ((cnt > 0)&&(HILO_CTS_IO != 0))
				{
					cnt--;
					vTaskDelay(20);
				}
				//HILO_CTS_TRIS = ctsTris;
			}

        case 4:
            if(HILO_CTS_IO != 0)
            {
                mainOpStatus.ExecStat = OP_HIB_ERR;
                //mainOpStatus.Function = 0;
                //mainOpStatus.ErrorCode = 0;
                //mainGSMStateMachine = SM_GSM_HIBERNATE;
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
	mainGSMStateMachine = SM_GSM_HIBERNATE;
	return -1;

}
/// @endcond

/**
 * Turns GSM module in "Low Power Mode". GSM Module can quit from this mode using GSMOn() function, but also receiving incoming SMSs and incoming voice calls.
 * <B>NOTE: </B>each time Flyport quits from LowPowerMode with a external event (SMS, Voice Call), framework will raise also "OnLowPowerDisabled()" event on GSM_Events.c
 */
void GSMLowPowerEnable()
{
    BOOL opok = FALSE;

	if(mainGSM.HWReady != TRUE)
		return;
	if(mainGSMStateMachine == SM_GSM_LOW_POWER)
		return;

	//	Function cycles until it is not executed
	while (!opok)
	{
		while (xSemaphoreTake(xSemFrontEnd,0) != pdTRUE);		//	xSemFrontEnd TAKE

		// Check mainOpStatus.ExecStat
		if (mainOpStatus.ExecStat != OP_EXECUTION)
		{
			mainOpStatus.ExecStat = OP_EXECUTION;
			mainOpStatus.Function = 9;
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
int cGSMLowPowerEnable()
{
    // >> AT+KSLEEP=0\r
    // ECHO
    // +KSLEEP=0
    // OK

    // DTR = "high level" => Module goes low power mode!
	char cmdReply[200];
	char msg2send[200];
	int resCheck = 0;
	DWORD tick;
	int countData = 0;
	int chars2read = 1;

	switch (smInternal)
	{
		case 0:
            // Pause to reduce the bugs on this command
            //vTaskDelay(350);
			if(GSMBufferSize() > 0)
			{
				// Parse Unsol Message
				mainGSMStateMachine = SM_GSM_CMD_PENDING;
                //vTaskDelay(250); // wait a while to complete incoming message...
				return -1;
			}
			else
				smInternal++;

		case 1:
			sprintf(msg2send, "AT+KSLEEP=0\r");
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			// Real max timeout for this command: 5 seconds
			maxtimeout = 15;
			smInternal++;

		case 2:
			vTaskDelay(2);
			// Check ECHO
			countData = 0;
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}


            maxtimeout = 15;
            smInternal++;

		case 3:
			// Get reply (OK, BUSY, ERROR, etc...)
			vTaskDelay(20);

			// Get OK
			sprintf(msg2send, "OK");
			chars2read = 2;
			countData = 2; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
                // Turn OFF DTR
                HILO_DTR_IO = TURN_HILO_DTR_OFF;
			}

		default:
			break;
	}

	smInternal = 0;
	// Cmd = 0 only if the last command successfully executed
	mainOpStatus.ExecStat = OP_SUCCESS;
	mainOpStatus.Function = 0;
	mainOpStatus.ErrorCode = 0;
	mainGSMStateMachine = SM_GSM_LOW_POWER;
	return -1;
}

/// @endcond

/**
 * Turns GSM module OFF and PIC microcontroller in Sleep mode 
 * <B>Note:</B> Sleep and Hibernation are not suggested when your application needs to wake up more often than daily.
 */
void GSMSleep()
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
			mainOpStatus.Function = 28; //29;
			mainOpStatus.ErrorCode = 0;
			
			xQueueSendToBack(xQueue,&mainOpStatus.Function,0);	//	Send COMMAND request to the stack
			
			xSemaphoreGive(xSemFrontEnd);						//	xSemFrontEnd GIVE, the stack can answer to the command
			opok = TRUE;
			taskYIELD();
		}
		else
		{
			xSemaphoreGive(xSemFrontEnd);
			taskYIELD();
		}
	}
	while (mainOpStatus.ExecStat == OP_EXECUTION);		// wait for callback Execution...	

	if(mainGSMStateMachine == SM_GSM_HIBERNATE)
	{
		// Set Sleep for PIC 
		// PIC sleep mode
		RCONbits.VREGS = 0;
		vTaskDelay(50);
		asm("PWRSAV #0");
	}
}

/**
 * Turns GSM module ON after GSMHibernate, GSMSleep or GSMLowPowerEnable. 
 <B>NOTE:</B> using Low Power Mode, GSM Module can be awaken using SMSs or Incoming Voice Calls, so the GSMOn can be avoided in this cases (the event "OnLowPowerDisabled()" - file GSM_Events.c is raised in case).
 */
void GSMOn()
{
	BOOL opok = FALSE;
	
	if(mainGSM.HWReady != TRUE)
		return;
    if(( mainGSMStateMachine != SM_GSM_HIBERNATE)&&(mainGSMStateMachine != SM_GSM_LOW_POWER))
		return;
	
	//	Function cycles until it is not executed
	while (!opok)
	{
		while (xSemaphoreTake(xSemFrontEnd,0) != pdTRUE);		//	xSemFrontEnd TAKE

		// Check mainOpStatus.ExecStat
		if (mainOpStatus.ExecStat != OP_EXECUTION)
		{		
			mainOpStatus.ExecStat = OP_EXECUTION;
			mainOpStatus.Function = 29;
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
int cGSMOn()
{
    if(mainGSMStateMachine == SM_GSM_LOW_POWER) // Hilo module is in its sleep mode (not power off)
    {
        // Turn on module by setting DTR:
        HILO_DTR_IO = TURN_HILO_DTR_ON;

        DWORD tick = TickGetDiv64K();
        while((HILO_CTS_IO == 1)&&((TickGetDiv64K() - tick) < 16)) // wait for CTS or about 15 seconds
        {
            vTaskDelay(20);
        }
        if(HILO_CTS_IO == 0)
        {
            mainGSMStateMachine = SM_GSM_IDLE;
            mainOpStatus.ExecStat = OP_SUCCESS;
            mainOpStatus.Function = 0;
            mainOpStatus.ErrorCode = 0;
        }
        else
        {
            // CTS signal is yet at high level, so we have to try again!
            mainGSMStateMachine = SM_GSM_IDLE;
            mainOpStatus.ExecStat = OP_LOW_POW_ERR;
            mainOpStatus.Function = 0;
            mainOpStatus.ErrorCode = 0;
        }
    }
    else
    {
        while(cSTDModeEnable());
    }
	
	mainGSMStateMachine = SM_GSM_IDLE;
	return -1;
}
/// @endcond


/**
 * Forces the update of GSM network connection status.
 * To read the connection status value, please use LastConnStatus() function
 */
void UpdateConnStatus()
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
			mainOpStatus.Function = 4;
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
int cUpdateConnStatus()
{
    // AT+CGREG? --> response: ECHO\r\n+CGREG: <n>,<stat>[,<lac>,<ci>]\r\nOK\r\n
    int resCheck = 0;
	char cmdReply[200];
	char msg2send[200];
	DWORD tick;
	int countData = 0;
	int chars2read = 1;

	switch (smInternal)
	{
		case 0:
			if(GSMBufferSize() > 0)
			{
				// Parse Unsol Message
				mainGSMStateMachine = SM_GSM_CMD_PENDING;
				return -1;
			}
			else
				smInternal++;

		case 1:
            // Send AT command
			sprintf(msg2send, "AT+CGREG?\r");
			GSMWrite(msg2send);
            // Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			// Update max timeout for this command: 2 seconds (3 seconds to decrease error frequency...)
			maxtimeout = 3 + 2;
			smInternal++;

        case 2:
			vTaskDelay(2);
			// Check ECHO
			countData = 0;
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            maxtimeout = 3 + 2;
            smInternal++;
            
        case 3:
			vTaskDelay(2);
            maxtimeout = 3 + 2;
			// Get +CREG
			sprintf(msg2send, "+CGREG");
			chars2read = 2;
			countData = 2; // GSM buffer should be: <CR><LF>+CGREG:...<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            else
            {
                // Response
                // +CGREG: <n>,<stat>[,<lac>,<ci>]\r\n
                // OK\r\n
                // Get first number <n>
				char temp[25];
				int res = getfield(':', ',', 5, 1, cmdReply, temp, 500);
				if(res != 1)
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield for AT+CGREG command\r\n");
					break;
				}
                // Get connection status <stat>
                res = getfield(',', '\n', 30,1, cmdReply, temp, 500);
				if(res != 1)
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield for AT+CGREG command\r\n");
					break;
				}
				else
                {
                    int EventType=0;
                    int regStatus = atoi(temp);
                    switch(regStatus)
                    {
                        case 0:
                            // Set GSM Event:
                            EventType = ON_REG;
                            // Set mainGSM Connection Status
                            mainGSM.ConnStatus = NO_REG;
                            break;
                        case 1:
                            // Set GSM Event:
                            EventType = ON_REG;
                            // Set mainGSM Connection Status
                            mainGSM.ConnStatus = REG_SUCCESS;
                            break;
                        case 2:
                            // Set GSM Event:
                            EventType = ON_REG;
                            // Set mainGSM Connection Status
                            mainGSM.ConnStatus = SEARCHING;
                            break;
                        case 3:
                            // Set GSM Event:
                            EventType = ON_REG;
                            // Set mainGSM Connection Status
                            mainGSM.ConnStatus = REG_DENIED;
                            break;
                        case 4:
                            // Set GSM Event:
                            EventType = ON_REG;
                            // Set mainGSM Connection Status
                            mainGSM.ConnStatus = UNKOWN;
                            break;
                        case 5:
                            // Set GSM Event:
                            EventType = ON_REG;
                            // Set mainGSM Connection Status
                            mainGSM.ConnStatus = ROAMING;
                            break;
                        default:
                            // Set GSM Event:
                            EventType = ON_REG;
                            // Set mainGSM Connection Status
                            mainGSM.ConnStatus = UNKOWN;
                            break;
                    }
                    // In this case the event Handler will not print any message
                    // since the state machine is not IDLE, but CMD_PEN
                    EventHandler();
                }
            }

		case 4:
			// Get reply (OK, BUSY, ERROR, etc...)
			vTaskDelay(2);
            maxtimeout = 3 + 2;
			// Get OK
			sprintf(msg2send, "OK");
			chars2read = 2;
			countData = 2; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            
            // if connection status is registered on network or roaming
            // check also GPRS connection...
            if((mainGSM.ConnStatus == REG_SUCCESS)||(mainGSM.ConnStatus == ROAMING))
              smInternal++;
            else
            {
                // can't proceeded with AT+CGATT and AT+COPS params...
                // just to be sure to break switch now
                smInternal = 0;
                // Cmd = 0 only if the last command successfully executed
                mainOpStatus.ExecStat = OP_SUCCESS;
                mainOpStatus.Function = 0;
                mainOpStatus.ErrorCode = 0;
                mainGSMStateMachine = SM_GSM_IDLE;
                return -1;
            }

        case 5:
            // Write here AT+CGATT=1
            // Send AT command
            mainGSM.IsGPRSPacketServiceReady = FALSE;
			sprintf(msg2send, "AT+CGATT=1\r");
			GSMWrite(msg2send);
            // Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			// Update max timeout for this command: 2 seconds (3 seconds to decrease error frequency...)
			maxtimeout = 60 + 2;
			smInternal++;

        case 6:
			vTaskDelay(2);
			// Check ECHO
			countData = 0;
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            maxtimeout = 60 + 2;
            smInternal++;

		case 7:
			// Get reply (OK, BUSY, ERROR, etc...)
			vTaskDelay(2);
            maxtimeout = 3 + 2;
			// Get OK
			sprintf(msg2send, "OK");
			chars2read = 2;
			countData = 2; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            else
            {
                mainGSM.IsGPRSPacketServiceReady = TRUE;
            }

        // AT+COPS
        case 8:
            // Send AT command
			sprintf(msg2send, "AT+COPS?\r");
			GSMWrite(msg2send);

            // Clear Operator Name
            for(maxtimeout = 0; maxtimeout < 16; maxtimeout++)
                mainGSM.OperatorName[maxtimeout] = '\0';
            
            // Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			// Update max timeout for this command: 2 seconds (3 seconds to decrease error frequency...)
			maxtimeout = 120 + 2;
			smInternal++;

        case 9:
			vTaskDelay(2);
			// Check ECHO
			countData = 0;
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            maxtimeout = 120 + 2;
            smInternal++;

        case 10:
			vTaskDelay(2);
            maxtimeout = 120 + 2;
			// Get +CREG
			sprintf(msg2send, "+COPS");
			chars2read = 2;
			countData = 2; // GSM buffer should be: <CR><LF>+COPS:...<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            else
            {
                // Response
                // +COPS: <n>[,<name>[,<format>]]\r\n
                // OK\r\n
                // Get first number <n>
				char temp[25];
				int res = getfield(':', ',', 5, 1, cmdReply, temp, 500);
				if(res != 1)
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield for AT+COPS? command\r\n");
					break;
				}
                // Get operator name <name>
                char name[16];
                res = getfield(',', '\n', 16, 2, cmdReply, name, 500);
				if(res != 1)
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield for AT+COPS? command\r\n");
					break;
				}
                else
                {
                    for(maxtimeout = 0; maxtimeout < 16; maxtimeout++)
                        mainGSM.OperatorName[maxtimeout] = name[maxtimeout];
                }
            }

		case 11:
			// Get reply (OK, BUSY, ERROR, etc...)
			vTaskDelay(2);
            maxtimeout = 120 + 2;
			// Get OK
			sprintf(msg2send, "OK");
			chars2read = 2;
			countData = 2; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
                for(maxtimeout = 0; maxtimeout < 16; maxtimeout++)
                        mainGSM.OperatorName[maxtimeout] = '\0';
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
 * Forces the update of GSM network signal power value.
 * To read the signal power value, please use LastSignalRssi() function
 */
void GSMSignalQualityUpdate()
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
			mainOpStatus.Function = 16;
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
int cGSMSignalQualityUpdate(void)
{
    // AT+CSQ --> response: ECHO\r\n+CSQ:x,y\r\nOK\r\n
    int resCheck = 0;
	char cmdReply[200];
	char msg2send[200];
	DWORD tick;
	int countData = 0;
	int chars2read = 1;

	switch (smInternal)
	{
		case 0:
			if(GSMBufferSize() > 0)
			{
				// Parse Unsol Message
				mainGSMStateMachine = SM_GSM_CMD_PENDING;
				return -1;
			}
			else
				smInternal++;

		case 1:
            // Send AT command
			sprintf(msg2send, "AT+CSQ\r");
			GSMWrite(msg2send);
            // Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			// Update max timeout for this command: 2 seconds (3 seconds to decrease error frequency...)
			maxtimeout = 3;
			smInternal++;

        case 2:
			vTaskDelay(2);
			// Check ECHO
			countData = 0;
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}

        case 3:
			vTaskDelay(2);

			// Get +CSQ
			sprintf(msg2send, "+CSQ");
			chars2read = 2;
			countData = 2; // GSM buffer should be: <CR><LF>+CSQ:...<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);

			CheckErr(resCheck, &smInternal, &tick);

			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
            else
            {
                // Response
                // +CSQ: <rssi>,<ber>\r\n
                // OK\r\n
                // Get first number <rssi>
				char temp[25];
				int res = getfield(':', ',', 5, 1, cmdReply, temp, 500);
				if(res != 1)
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield for AT+CSQ command\r\n");
					break;
				}
                else
                {
                    int readVal = atoi(temp);
                    
                    if(readVal > 31)
                        signal_dBm_Gsm = 0;
                    else
                        signal_dBm_Gsm = (-113)+(2*readVal);
                }
                // Get <ber> (not used yet in FlyportGPRS framework...
                res = getfield(',', '\n', 30,1, cmdReply, temp, 500);
				if(res != 1)
				{
					// Execute Error Handler
					gsmDebugPrint( "Error in getfield for AT+CGREG command\r\n");
					break;
				}
				else
                {
                    // Warning!! Not Used Yet!!
                    int ber = atoi(temp);
                    if(ber > 7)
                        ber = -1; // not valid
                }
            }

		case 4:
			// Get reply (OK, BUSY, ERROR, etc...)
			vTaskDelay(2);
			// Get OK
			sprintf(msg2send, "OK");
			chars2read = 2;
			countData = 2; // GSM buffer should be: <CR><LF>OK<CR><LF>
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

/*! @} */

/*! @} */

