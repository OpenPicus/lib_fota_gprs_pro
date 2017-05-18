/** \file Sara.c
 *  \brief Hardware library to manage the analog and digital IOs and UART
 */

/**
\addtogroup Hardware
@{
*/

/* **************************************************************************																					
 *                                OpenPicus                 www.openpicus.com
 *                                                            italian concept
 * 
 *            openSource wireless Platform for sensors and Internet of Things	
 * **************************************************************************
 *  FileName:        Hilo.c
 *  Dependencies:    Microchip configs files
 *  Module:          FlyPort WI-FI
 *  Compiler:        Microchip C30 v3.12 or higher
 *
 *  Author               Rev.    Date              Comment
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Gabriele Allegria    1.0     1/20/2011		   First release  (core team)
 *  Stefano Cappello 							   Added I2C + support in PWM
 *  Simone Marra         2.3    08/02/2013              Added GPRS/3G support
 *                       3.0     08/22/2016        Support for uBlox SARA-G350 renamed file from Hilo.c
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

#include "HWmap.h"

#include "Sara.h"
#include "p24FJ256GB206.h"
#include "SARAlib.h"
#include <ctype.h>
#if defined (FLYPORTGPRS)

extern int *UMODEs[];
extern int *USTAs[];
extern int *UBRGs[];
extern int *UIFSs[];
extern int *UIECs[];
extern int *UTXREGs[];
extern int *URXREGs[];
extern int UTXIPos[];
extern int URXIPos[];

/*static */int bufind_w_GSM;
/*static */int bufind_r_GSM;
static int last_op;
/*static */char GSMBuffer[GSM_BUFFER_SIZE];
static char GSMbuffover;
static char UnsolBuffer[150];
char msg2send[200];

// FrontEnd variables
/*
extern int xFrontEndStat;
extern int xFrontEndStatRet;
extern int xErr;
*/

extern const long int baudComp[];

//	RTOS variables
extern xTaskHandle hGSMTask;
extern xTaskHandle hFlyTask;
extern xQueueHandle xQueue;
extern xSemaphoreHandle xSemFrontEnd;
extern xSemaphoreHandle xSemHW;
extern portBASE_TYPE xStatus;

// GSM variables
GSMModule 		mainGSM;
CALLData		mainCall;
static int		errorCode;
int             signal_dBm_Gsm = 0;

SMSData			mainSMS;
static BYTE		IncomingSMS_MemType;
static int		IncomingSMS_Index;
OpStat			mainOpStatus;
extern int 	mainGSMStateMachine;
extern FTP_SOCKET* xFTPSocket;

int EventType=0;
int SaraComTest();
int CheckEcho(int countData, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);

/**
 * Returns last GSM network connection status
 * \return - Connection status
 * <UL>
 *	<LI><B> NO_REG </B>(0) - not registered</LI>
 *	<LI><B> REG_SUCCESS </B>(1) - registered correctly on network</LI>		
 *	<LI><B> SEARCHING </B>(2) - searching for operator</LI>
 *	<LI><B> REG_DENIED </B>(3) - registration on network denied</LI>
 *	<LI><B> UNKOWN </B>(4) - unknown status</LI>
 *	<LI><B> ROAMING </B>(5) - registered in roaming mode</LI>
 * </UL>
 */
BYTE LastConnStatus()
{
	return mainGSM.ConnStatus;
}

/**
 * Returns last GSM Task operation status
 * \return - Operation status.
 * <UL>
 *	<LI><B> OP_SUCCESS </B>(0) - Last function executed correctly</LI>
 *	<LI><B> OP_EXECUTION </B>(-1) - Function still executing</LI>		
 *	<LI><B> OP_LL </B>(-2) - Low Level mode is activate</LI>
 *	<LI><B> OP_TIMEOUR </B>(1) - Timeout error: GPRS module has not answered within the required timeout for the operation </LI>	
 *	<LI><B> OP_SYNTAX_ERR </B>(2) - GPRS module reported a "syntax error" message</LI>	
 *	<LI><B> OP_CMS_ERR </B>(3) - GPRS module reported a CMS error</LI>
 *	<LI><B> OP_CME_ERR </B>(4) - GPRS module reported a CME error</LI>
 *	<LI><B> OP_NO_CARR_ERR </B>(5) - GPRS module reported a NO CARRIER error</LI>
 *	<LI><B> OP_SMTP_ERR </B>(6) - Error in sending the email</LI>
 *	<LI><B> OP_FTP_ERR </B>(7) - Error message received in FTP operation</LI>
 *	<LI><B> OP_HIB_ERR </B>(8) - GPRS module is turned off and cannot reply to commands</LI>
 *	<LI><B> OP_LOW_POW_ERR </B>(9) - GPRS module is in low power mode and cannot reply to commands</LI>
 * </UL>
 */
int LastExecStat()
{
//	if(mainOpStatus.ExecStat == OP_EXECUTION)
//		vTaskDelay(1);
	return mainOpStatus.ExecStat;
}	

/**
 * Returns last GSM Task error code
 * \return - error code value.
 */
int LastErrorCode()
{
	return mainOpStatus.ErrorCode;
}

int LastSignalRssi()
{
    return signal_dBm_Gsm;
}

void callbackDbg(BYTE smInt)
{
	#if defined (STACK_USE_UART)
	char dbgcall[100];
	sprintf(dbgcall, "CALLBACK DEBUG:\r\n\tFunction: %d\r\n\tsmInternal: %d\r\n",
					mainOpStatus.Function, smInt);
	_dbgwrite(dbgcall);
	#endif
}

void gsmDebugPrint(char* dbgmessage)
{
	_dbgwrite(dbgmessage);
}

void SaraPowerOnMosfet(BOOL turnOn)
{
    if(turnOn == FALSE)
        SARA_ON_IO = TURN_SARA_DTR_OFF;
    else
        SARA_ON_IO = TURN_SARA_DTR_ON;
}

// Executes hardware reset to modem, with time delay
void SaraReset()
{
	// Reset the module:
    SARA_RESET_IO = 0;
    DelayMs(50); //20);
    SARA_RESET_IO = 1;
    DelayMs(50);
}

// Applies low level pulse on POK to startup the module, and waits for CTS signal
BOOL Sara_PwrOn(int timeout)
{
//#warning non ho capito perchè non funziona al contrario (dato che è opendrain dovrebbe dare valore basso quando viene settato a 1!!)
	SARA_PWR_ON_IO = 0;//1;
    DelayMs(timeout);
    SARA_PWR_ON_IO = 1;//0;
    DelayMs(timeout*2);
    return TRUE;
}

// Initializes Flyport UART4 to be used with Hilo Modem with sperified "long int baud" baudrate. It enables also HW flow signals CTS/RTS
void SaraUARTInit(long int baud)
{
    // Initialize HILO UART...
    int port = SARA_UART-1;
	long int brg , baudcalc , clk , err;
	clk = GetInstructionClock();
	brg = (clk/(baud*16ul))-1;
	baudcalc = (clk/16ul/(brg+1));
	err = (abs(baudcalc-baud)*100ul)/baud;

	int UMODEval = 0;
	UMODEval = (*UMODEs[port] & 0x3CFF);
	
	if (err<2)
	{
		*UMODEs[port] = (0xFFF7 & UMODEval);
		*UBRGs[port] = brg;
	}
	else
	{
		brg = (clk/(baud*4ul))-1;
		*UMODEs[port] = (0x8 | UMODEval);
		*UBRGs[port] = brg;
	}
	
	// UART ON:
	*UMODEs[port] = *UMODEs[port] | 0x8000;
	*USTAs[port] = *USTAs[port] | 0x400;

	*UIFSs[port] = *UIFSs[port] & (~URXIPos[port]);
	*UIFSs[port] = *UIFSs[port] & (~UTXIPos[port]);
	*UIECs[port] = *UIECs[port] | URXIPos[port];
}

// Initializes Hilo using Reset, UART setup, Pok and waits until the procedure is finished
void SaraInit(long int baud)
{
    _dbgline("Initializing SARA module...");
    SaraPowerOnMosfet(TRUE);
    vTaskDelay(1);
    Sara_PwrOn(15); // Apply a PWR_ON low pulse even if it is unnecessary
    
    int a = 1;
	while(a != 0)
    {
        a = SaraComTest();
        if(a != 0)
        {
            _dbgline("Failed UART communication... resetting modem!");
            SaraReset();
            vTaskDelay(50);
        }
    }
}

int findStr(char* str, int _vTaskTimeout)
{
	int vTaskTimeout = _vTaskTimeout;
	char buff;
	int cnt=0;
	int cnt2=0;
	int len = strlen(str);
	// Waiting for the first char...
	while (GSMBufferSize() == 0)
	{
		cnt++;
		vTaskDelay(1);
		if (cnt == vTaskTimeout)
			return -1;
	}
	
	//	Searching for OK answer
	while (GSMBufferSize() > 0)
	{
		GSMRead(&buff, 1);
			
		if (buff == str[cnt2])
			cnt2++;
		else
			cnt2 = 0;
		if (cnt2 == len)
		{
			return 0;
		}	
		while (GSMBufferSize() == 0)
		{
			vTaskDelay(1);
			cnt++;
			if (cnt == vTaskTimeout)
				break;
		}				
	}		
	return -1;
}

int SaraComTest()
{
	int cnt, i;
	char tofind[]="OK\r", buff;
	#if defined(STACK_USE_UART)
	char gab[55];
	#endif
	for (i=15; i>=0; i--)
	{
		cnt = 0;
		SARA_RTS_IO = 0;
		
		//	Waiting for CTS signal enabled (CTS_IO == 0 is for enabled state), timeout 10 secs.
		while (SARA_CTS_IO == 1)
		{
			cnt++;
			// vTaskDelay(50);
            DelayMs(500);
			if (cnt == 20)
			{
				return -1;
			}
		}
		
		//	Hilo UART initialization, without hardware flow control
		SaraUARTInit(baudComp[i]);
		#if defined(STACK_USE_UART)
		sprintf(gab, "Testing %ld baud...\r\n", baudComp[i]);
		_dbgwrite(gab);
		#endif
		while (GSMBufferSize() > 0)
		{
			GSMFlush();
			vTaskDelay(50);
		}
			
		//	Setting hardware flow control on Hilo module
		GSMWrite("AT&K3\r");
		vTaskDelay(50);
		//	Waiting for Modem answer, timeout 5 secs.
		cnt = 0;
#define CNT_TIMEOUT	500
		while (GSMBufferSize() < strlen(tofind))// == 0)
		{
			cnt++;
			vTaskDelay(1);
			if (cnt == CNT_TIMEOUT)
				break;		
		}

		//	If Hilo timed out, trying next baudrate
		if (cnt == CNT_TIMEOUT)
		{
			continue;
		}
		cnt = 0;
		//	Searching for OK answer
		while (GSMBufferSize() > 0)
		{
			GSMRead(&buff, 1);
				
			if (buff == tofind[cnt])
				cnt++;
			else
				cnt = 0;
			if (cnt == 3)
			{
				if(i != 7) // Restore 115200 bauds
				{
					GSMWrite("AT+IPR=115200\r");
					vTaskDelay(50);
					SaraUARTInit(115200);
					vTaskDelay(20);
					GSMFlush();
				}
				return 0;
			}	
			vTaskDelay(5);
			if (GSMBufferSize() == 0)
				vTaskDelay(100);				
		}
	}
	return -1;
}

//#define _SARA_NO_SIM_CONNECTED_

#define _SARA_PRINT_FW_VER_

#define _SARA_INIT_TIMEOUT_ 430
extern const long int baudComp[];
// Configures GSM modem to enter in "StandardMode", setting up desired parameters
int SaraStdModeOn(long int baud)
{
	// Hw init:
	SaraInit(baud);
	vTaskDelay(20);
	GSMFlush();

	// Set parameters
	_dbgwrite("setup parameters...\r\n");
	
	// ATE1
	sprintf(UnsolBuffer, "ATE1\r");
	GSMWrite(UnsolBuffer);
	if(!findStr("OK\r", _SARA_INIT_TIMEOUT_))
	{
		_dbgwrite("ATE1 OK\r\n");
	}
	else
	{
		_dbgwrite("ATE1 ERROR\r\n");
		return OP_SYNTAX_ERR;
	}

	// AT+CREG=0
	sprintf(UnsolBuffer,"AT+CREG=0\r");
	GSMWrite(UnsolBuffer);
	if(!findStr("OK\r", _SARA_INIT_TIMEOUT_))
	{
		_dbgwrite("AT+CREG=0 OK\r\n");
	}
	else
	{
		_dbgwrite("AT+CREG=0 ERROR\r\n");
		return OP_SYNTAX_ERR;
	}
	
	// AT+CRC=0
	sprintf(UnsolBuffer,"AT+CRC=0\r");
	GSMWrite(UnsolBuffer);
	if(!findStr("OK\r", _SARA_INIT_TIMEOUT_))
	{
		_dbgwrite("AT+CRC=0 OK\r\n");
	}
	else
	{
		_dbgwrite("AT+CRC=0 ERROR\r\n");
		return OP_SYNTAX_ERR;
	}

#ifndef _SARA_NO_SIM_CONNECTED_
	int atCmdTest = 5;
	while(atCmdTest > 0)
	{
		// AT+CAOC=0
		sprintf(UnsolBuffer,"AT+CAOC=0\r");
		GSMWrite(UnsolBuffer);
		if(!findStr("OK\r", _SARA_INIT_TIMEOUT_))
		{
			_dbgwrite("AT+CAOC=0 OK\r\n");
			break;
		}
		else
		{
			atCmdTest--;
			DelayMs(500);
			GSMFlush();
		}
	}
	if(atCmdTest == 0)
	{
		_dbgwrite("AT+CAOC=0 ERROR\r\n");
		return OP_SYNTAX_ERR;
	}
    atCmdTest = 5;
    while(atCmdTest > 0)
	{
        // AT+CCWA=0
        sprintf(UnsolBuffer,"AT+CCWA=0\r");
        GSMWrite(UnsolBuffer);
        if(!findStr("OK\r", _SARA_INIT_TIMEOUT_))
        {
            _dbgwrite("AT+CCWA=0 OK\r\n");
            break;
        }
        else
        {
            atCmdTest--;
            DelayMs(500);
            GSMFlush();
        }
    }
    if(atCmdTest == 0)
	{
		_dbgwrite("AT+CCWA=0 ERROR\r\n");
		return OP_SYNTAX_ERR;
	}
	
    atCmdTest = 5;
    while(atCmdTest > 0)
	{
        // AT+CNMI=2,1,0,1
        sprintf(UnsolBuffer,"AT+CNMI=2,1,0,1\r");
        GSMWrite(UnsolBuffer);
        if(!findStr("OK\r", _SARA_INIT_TIMEOUT_))
        {
            _dbgwrite("AT+CNMI=2,1,0,1 OK\r\n");
            break;
        }
        else
        {
            atCmdTest--;
            DelayMs(500);
            GSMFlush();
        }
    }
    if(atCmdTest == 0)
	{
		_dbgwrite("AT+CNMI=2,1,0,1 ERROR\r\n");
		return OP_SYNTAX_ERR;
	}
	
    atCmdTest = 5;
    while(atCmdTest > 0)
	{
        // AT+CSMP=17,167,0,0
        sprintf(UnsolBuffer,"AT+CSMP=17,167,0,0\r");
        GSMWrite(UnsolBuffer);
        if(!findStr("OK\r", _SARA_INIT_TIMEOUT_))
        {
            _dbgwrite("AT+CSMP=17,167,0,0 OK\r\n");
            break;
        }
        else
        {
            atCmdTest--;
            DelayMs(500);
            GSMFlush();
        }
    }
    if(atCmdTest == 0)
    {
        _dbgwrite("AT+CSMP=17,167,0,0 ERROR\r\n");
        return OP_SYNTAX_ERR;
    }
	 		
	// AT+CUSD=0		
	sprintf(UnsolBuffer,"AT+CUSD=0\r");
	GSMWrite(UnsolBuffer);
	if(!findStr("OK\r", _SARA_INIT_TIMEOUT_))
	{
		_dbgwrite("AT+CUSD=0 OK\r\n");
	}
	else
	{
		_dbgwrite("AT+CUSD=0 ERROR\r\n");
		return OP_SYNTAX_ERR;
	}
	
	// AT+CLIP=1 
	sprintf(UnsolBuffer,"AT+CLIP=1\r");
	GSMWrite(UnsolBuffer);
	if(!findStr("OK\r", _SARA_INIT_TIMEOUT_))
	{
		_dbgwrite("AT+CLIP=1 OK\r\n");
	}
	else
	{
		_dbgwrite("AT+CLIP=1 ERROR\r\n");
		return OP_SYNTAX_ERR;
	}

//	
//	// AT+KPATTERN=\"--EOF--Pattern--\"
//	sprintf(UnsolBuffer,"AT+KPATTERN=\"--EOF--Pattern--\"\r");
//	GSMWrite(UnsolBuffer);
//	if(!findStr("OK\r", _HILO_INIT_TIMEOUT_))
//	{
//		_dbgwrite("AT+KPATTERN=\"--EOF--Pattern--\" OK\r\n");
//	}
//	else
//	{
//		_dbgwrite("AT+KPATTERN=\"--EOF--Pattern--\" ERROR\r\n");
//		return OP_SYNTAX_ERR;
//	}
	
	// AT+CSDH=1 
	sprintf(UnsolBuffer,"AT+CSDH=1\r");
	GSMWrite(UnsolBuffer);
	if(!findStr("OK\r", _SARA_INIT_TIMEOUT_))
	{
		_dbgwrite("AT+CSDH=1 OK\r\n");
	}
	else
	{
		_dbgwrite("AT+CSDH=1 ERROR\r\n");
		return OP_SYNTAX_ERR;
	}
	
	// AT+CMGF=1
	sprintf(UnsolBuffer,"AT+CMGF=1\r");
	GSMWrite(UnsolBuffer);
	if(!findStr("OK\r\n", _SARA_INIT_TIMEOUT_))
	{
		_dbgwrite("AT+CMGF=1 OK\r\n");
	}
	else
	{
		_dbgwrite("AT+CMGF=1 ERROR\r\n");
		return OP_SYNTAX_ERR;
	}
	
	// AT+CREG=1
	sprintf(UnsolBuffer,"AT+CREG=1\r");
	GSMWrite(UnsolBuffer);
	if(!findStr("OK\r", _SARA_INIT_TIMEOUT_))
	{
		_dbgwrite("AT+CREG=1 OK\r\n");
	}
	else
	{
		_dbgwrite("AT+CREG=1 ERROR\r\n");
		return OP_SYNTAX_ERR;
	}
	    
    // AT+UPSV=0 - Remove previously power saving modes (if any)
    sprintf(UnsolBuffer, "AT+UPSV=0\r");
    GSMWrite(UnsolBuffer);
    if(!findStr("OK\r", _SARA_INIT_TIMEOUT_))
    {
        _dbgwrite("AT+UPSV=0 OK\r\n");
        if(mainGSMStateMachine == SM_GSM_LOW_POWER)
        {
            OnLowPowerDisabled();
        }
        
    }
    else
    {
        _dbgwrite("AT+UPSV=0 ERROR\r\n");
        return OP_SYNTAX_ERR;
    }
    
    // AT+CMEE=1 - allow numeric error codes
    sprintf(UnsolBuffer, "AT+CMEE=1\r");
    GSMWrite(UnsolBuffer);
    if(!findStr("OK\r", _SARA_INIT_TIMEOUT_))
    {
        _dbgwrite("AT+CMEE=1 OK\r\n");
    }
    else
    {
        _dbgwrite("AT+CMEE=1 ERROR\r\n");
        return OP_SYNTAX_ERR;
    }
#endif

    // ATI0 // ATI+GMM (to get Model Version)
	//sprintf(UnsolBuffer, "AT+GMM\r");
    sprintf(UnsolBuffer, "ATI0\r");
	GSMFlush();
	GSMWrite(UnsolBuffer);
	vTaskDelay(25);
	char modelVer[250];
//    if(!findStr("AT+GMM\r\r\n", _SARA_INIT_TIMEOUT_))
    if(!findStr("ATI0\r\r\n", _SARA_INIT_TIMEOUT_))
	{
		int lenRevStr = GSMRead((char*)modelVer, 240);
		if(lenRevStr > 0)
		{
            int limit = 0;
            for(limit = 0; limit < lenRevStr; limit++)
            {
                if(modelVer[limit] == '\r')
                {
                    modelVer[limit] = '\0';
                    lenRevStr = limit;
                }
            }
            if(lenRevStr > MODEL_VER_LEN)
                lenRevStr = MODEL_VER_LEN;
			modelVer[lenRevStr] = '\0';
            strncpy(mainGSM.ModelVersion, modelVer, lenRevStr);
#ifdef _SARA_PRINT_FW_VER_
            _dbgwrite("Model Version: "); _dbgline(modelVer);
#endif
		}
		else
		{
            sprintf(mainGSM.ModelVersion, "UNKNOWN");
#ifdef _SARA_PRINT_FW_VER_
			_dbgwrite("ERROR getting model version!");
#endif
		}
    }
    
    // ATI9 // AT+CGMR (to get Revision identification)
	sprintf(UnsolBuffer, "ATI9\r");//sprintf(UnsolBuffer, "AT+CGMR\r");
	GSMFlush();
	GSMWrite(UnsolBuffer);
	vTaskDelay(25);
	char revision[250];
//    if(!findStr("AT+CGMR\r\r\n", _SARA_INIT_TIMEOUT_))
    if(!findStr("ATI9\r\r\n", _SARA_INIT_TIMEOUT_))
	{
		int lenRevStr = GSMRead((char*)revision, 240);

		if(lenRevStr > 0)
		{
            int limit = 0;
            for(limit = 0; limit < lenRevStr; limit++)
            {
                if(revision[limit] == '\r')
                {
                    revision[limit] = '\0';
                    lenRevStr = limit;
                }
            }
            if(lenRevStr > MODEM_FW_REV_LEN)
                lenRevStr = MODEM_FW_REV_LEN;
			revision[lenRevStr] = '\0';
            strncpy(mainGSM.FwRevision, revision, lenRevStr);
#ifdef _SARA_PRINT_FW_VER_
			_dbgwrite("FW Revision: "); _dbgline(revision);
#endif
		}
		else
		{
#ifdef _SARA_PRINT_FW_VER_
			_dbgwrite("ERROR getting revision!");
#endif
		}
    }
    	
    // AT+CGSN (to get IMEI)
    GSMFlush();
	sprintf(UnsolBuffer, "AT+CGSN\r");
	GSMWrite(UnsolBuffer);
	if(!findStr("AT+CGSN\r\r\n", _SARA_INIT_TIMEOUT_))
	{
        GSMpRead((char*)mainGSM.IMEI, IMEI_LEN);
        if(isdigit((int)mainGSM.IMEI[0])==0)
        {
            GSMRead((char*)mainGSM.IMEI, 1); // remove first char
        }
        GSMRead((char*)mainGSM.IMEI, IMEI_LEN); // than read correct IMEI
        mainGSM.IMEI[IMEI_LEN] = '\0';
        int i;
        for(i = IMEI_LEN; i>0; i--)
        {
            if((mainGSM.IMEI[i] == '\r') || (mainGSM.IMEI[i] == '\n'))
                mainGSM.IMEI[i] = '\0';
        }
//        _dbgwrite("IMEI: "); _dbgline(mainGSM.IMEI);
	}
	if(findStr("OK\r\n", _SARA_INIT_TIMEOUT_))
	{
		sprintf((char*)mainGSM.IMEI, "ERROR!");
	}
	
	mainGSM.HWReady = TRUE;
	return 0;

}

// UART4 Rx Interrupt to store received chars from modem
void GSMRxInt()
{
	int port = SARA_UART - 1;
	
	while ((*USTAs[port] & 1)!=0)
	{
		if (bufind_w_GSM == bufind_r_GSM)
		{
			if (last_op == 1)
			{
				GSMbuffover = 1;
				bufind_w_GSM = 0;
				bufind_r_GSM = 0;
				last_op = 0;
			}
		}

		GSMBuffer[bufind_w_GSM] = *URXREGs[port];
        #if defined GSM_DEBUG_AT_UART_1
        UARTWriteCh(1, GSMBuffer[bufind_w_GSM]);
        #elif defined GSM_DEBUG_AT_UART_2
        UARTWriteCh(2, GSMBuffer[bufind_w_GSM]);
        #elif defined GSM_DEBUG_AT_UART_3
        UARTWriteCh(3, GSMBuffer[bufind_w_GSM]);
        #endif
		if (bufind_w_GSM == GSM_BUFFER_SIZE - 1)
		{
			bufind_w_GSM = 0;
		}
		else
			bufind_w_GSM++;
	}
	last_op = 1;
	*UIFSs[port] = *UIFSs[port] & (~URXIPos[port]);
}

// Writes to GSM Modem the chars contained on data2wr until a '\0' is reached
void GSMWrite(char* data2wr)
{
	int port = SARA_UART-1;
	int pdsel;
	int cnt = 0;	
	while(SARA_CTS_IO == 1)
	{
		vTaskDelay(10);
		cnt++;
		if(cnt == 50)
			return;
	}
	SARA_RTS_IO = 1;
	// transmits till NUL character is encountered 
	pdsel = (*UMODEs[port] & 6) >>1;
    if (pdsel == 3)                             // checks if TX is 8bits or 9bits
    {
        while(*data2wr != '\0') 
        {
            #if defined GSM_DEBUG_AT_UART_1
            UARTWriteCh(1, *data2wr);
            #elif defined GSM_DEBUG_AT_UART_2
//            UARTWriteCh(2, *data2wr);
            #elif defined GSM_DEBUG_AT_UART_3
            UARTWriteCh(3, *data2wr);
            #endif

            while((*USTAs[port] & 512)>0);	// waits if the buffer is full 
            *UTXREGs[port] = *data2wr++;         // sends char to TX reg
        }
    }
    else
    {
        while(*data2wr != '\0')
        {
            #if defined GSM_DEBUG_AT_UART_1
            UARTWriteCh(1, *data2wr);
            #elif defined GSM_DEBUG_AT_UART_2
//            UARTWriteCh(2, *data2wr);
            #elif defined GSM_DEBUG_AT_UART_3
            UARTWriteCh(3, *data2wr);
            #endif

            while((*USTAs[port] & 512)>0);      // sends char to TX reg
            *UTXREGs[port] = *data2wr++ & 0xFF;  // sends char to TX reg
        }
    }
    SARA_RTS_IO = 0;
}

void GSMWriteCh(char chr)
{
	int port = SARA_UART-1;
	int pdsel;
	pdsel = (*UMODEs[port] & 6) >>1;
	int cnt = 0;
	while(SARA_CTS_IO == 1)
	{
		vTaskDelay(10);
		cnt++;
		if(cnt == 50)
			return;
	}
	SARA_RTS_IO = 1;
    if(pdsel == 3)        /* checks if TX is 8bits or 9bits */
    {
        #if defined GSM_DEBUG_AT_UART_1
        UARTWriteCh(1, chr);
        #elif defined GSM_DEBUG_AT_UART_2
//        UARTWriteCh(2, chr);
        #elif defined GSM_DEBUG_AT_UART_3
        UARTWriteCh(3, chr);
        #endif
        while((*USTAs[port] & 512)>0);	/* waits if the buffer is full */
        *UTXREGs[port] = chr;    		/* transfer data to TX reg */
    }
    else
    {
        #if defined GSM_DEBUG_AT_UART_1
        UARTWriteCh(1, chr);
        #elif defined GSM_DEBUG_AT_UART_2
//        UARTWriteCh(2, chr);
        #elif defined GSM_DEBUG_AT_UART_3
        UARTWriteCh(3, chr);
        #endif
        while((*USTAs[port] & 512)>0); /* waits if the buffer is full */
        *UTXREGs[port] = chr & 0xFF;   /* transfer data to TX reg */
    }
    SARA_RTS_IO = 0;
}


void GSMFlush()
{
	bufind_w_GSM = 0;
	bufind_r_GSM = 0;
	last_op = 0;
	GSMBuffer[0] = '\0';
}


int GSMBufferSize()
{
	BYTE loc_last_op = last_op;
	int conf_buff;
	int bsize=0;

	conf_buff = bufind_r_GSM - bufind_w_GSM;
	if (conf_buff > 0)
		bsize = GSM_BUFFER_SIZE - bufind_r_GSM + bufind_w_GSM;
	else if (conf_buff < 0)
		bsize = bufind_w_GSM - bufind_r_GSM;
	else if (conf_buff == 0)
		if (loc_last_op == 1)
			bsize = GSM_BUFFER_SIZE;

	return bsize;
}

// Reads a max of count chars and puts them inside towrite char array, from GSMBuffer. This functions clears GSM buffer array
int GSMRead(char *towrite , int count)
{
	int rd,limit;
	limit = GSMBufferSize();
	if (count > limit)
		count=limit;
	int irx = 0;
	rd = 0;
	while (irx < count)
	{
        *(towrite+irx) = GSMBuffer[bufind_r_GSM];

		if (bufind_r_GSM == (GSM_BUFFER_SIZE-1))
			bufind_r_GSM = 0;
		else
			bufind_r_GSM++;		

		irx++;
	}
	
	if ( GSMbuffover != 0 )
	{
		rd = -count;
		GSMbuffover = 0;
	}
	else
		rd = count;

	last_op = 2;
	return rd;
}

// Reads a max of count chars inside towrite char array, from GSMBuffer. This functions DOES NOT clear GSM buffer array
int GSMpRead(char *towrite, int count)
{
	int rd,limit;
	int bufind_r_temp = bufind_r_GSM;
	
	limit = GSMBufferSize();
	if (count > limit)
		count=limit;
	int irx = 0;
	rd = 0;
	while (irx < count)
	{
        *(towrite+irx) = GSMBuffer[bufind_r_temp];

		if (bufind_r_temp == (GSM_BUFFER_SIZE-1))
			bufind_r_temp = 0;
		else
			bufind_r_temp++;		

		irx++;
	}
	
	if ( GSMbuffover != 0 )
	{
		rd = -count;
		GSMbuffover = 0;
	}
	else
		rd = count;
	return rd;
}

// Reads a max of num chars inside towrite char array, from GSMBuffer, starting from start relative position. 
// This functions DOES NOT clear GSM buffer array 
BOOL GSMpSeek(int start, int num, char* towrite)
{
	int limit = GSMBufferSize();
	// Check if we have a sufficient number of BYTEs on GSMBuffer
	if((start + num) > limit)
		return FALSE;
	else
	{
		limit = num;
		int bufind_r_temp = bufind_r_GSM + start;
		
		if(bufind_r_temp > GSM_BUFFER_SIZE)
			bufind_r_temp = bufind_r_temp - GSM_BUFFER_SIZE;
			
		int irx = 0;
		
		while(irx < limit)
		{
			*(towrite+irx) = GSMBuffer[bufind_r_temp];

			if (bufind_r_temp == (GSM_BUFFER_SIZE-1))
				bufind_r_temp = 0;
			else
				bufind_r_temp++;		
	
			irx++;
		}
		return TRUE;	
	}
}

/**
 * Reads a "maxlen" number of bytes from GSM BUFFER, stopping if a "occurr" times of "term" character is found
 * @param maxlen max length of BYTEs to read
 * @param term termination char to count
 * @param occurr max number of occurrence the "term" char should be found on buffer
 * @param destbuff destination buffer to use as copy buffer
 * @return number of times the term char was found, -1 if less than provided occurrence was found on buffer
 */
int GSMReadN(int maxlen, char term, int occurr, char* destbuff)
{
	int count = 0;
	int i, occ_count = 0;
	for(i = 0; i < maxlen; i++)
	{
		GSMRead(destbuff+count, 1);
		if( *(destbuff+count) == term)
			occ_count++;
		count++;
		if(occ_count == occurr)
		{
			return count;
		}
	}
	// Error (less occ_count than expected)
	return -1;
}

// Unsolicited Messages Parsing
void GSMUnsol(int errorType)
{
	static DWORD unsolTick = 0;
	BOOL dontEraseBuffer = FALSE;
	int startPoint = 0;
	int endPoint = GSMBufferSize();
	// Parse unsollecited message on GSMBuffer
	if((GSMBufferSize() <= 3)&&(GSMBufferSize() > 0)&&(mainOpStatus.Function != 0)&&(mainOpStatus.ExecStat == OP_EXECUTION))
	{
		if((TickGetDiv64K() - unsolTick) > 2) // wait at least 2 seconds...
		{
			unsolTick = TickGetDiv64K();
            
			
			char bbb[2];
			/* INDBG
			char ddd[50];
            sprintf(ddd, "found lost chars: ");
			_dbgwrite(ddd);
			// indbg */
			int l = GSMBufferSize();
			int r;
			for(r = 0; r < l; r++)
			{
				GSMpRead(bbb, 1);
				bbb[1] = '\0';
				if((bbb[0] == '\r') || (bbb[0] == '\n'))
				{
					GSMRead(bbb, 1);
					/* INDBG
					sprintf(ddd, "0x%x", bbb[0]);
					_dbgwrite(ddd);
					// indbg */
				}
				else
				{
					/* INDBG
					_dbgwrite(bbb);
					// indbg */

					/* INDBG
					_dbgline("\r\n\t WARNING FOUND valid chars on buffer!!!\r\n\t");
					// indbg */

					break;
				}
			}
			/* INDBG
			_dbgwrite("*** END OF LOST CHARS\r\n");
			// indbg */
		}

	}

	if (GSMBufferSize() > 3)
	{
		vTaskDelay(1);
		char unsolStr[5];
		if(dontEraseBuffer == TRUE)
			GSMpSeek(startPoint, 4, unsolStr);
		else
			GSMRead(unsolStr, 4);
		unsolStr[4] = '\0';
		
		while(GSMBufferSize() > 0)
		{
			// Scan for "RING" message
			if(strstr(unsolStr, "RING")!=NULL)
			{
				// Wait for message complete...
				int counterEx = 120;
				while((counterEx>0)&&(GSMBufferSize() < 10))
				{
					counterEx--;
					vTaskDelay(2);
				}	
				//RING
				int res = GSMReadN(150, '\n', 3, UnsolBuffer);
				
				if(res == -1)
				{
					//ERROR:
					_dbgwrite("ERROR GSMReadN RING:\r\n");
					
					// Execute Error Handler
					ErrorHandler(errorType);
				}
				else
				{
					UnsolBuffer[res] = '\0';
					
					// get phone number
					res = getfield('"', '"', 20, 1, UnsolBuffer, mainCall.CallerID, 500);
					
					if(res != 1)
					{
						_dbgwrite("ERROR getfield RING\r\n");
						
						// Execute Error Handler
						ErrorHandler(errorType);						
						break;
					}
					else
					{				
						// Set GSM Event:
						EventType = ON_RING;
						
						// Set mainCall status
						mainCall.Status = CALL_IN_PROG;
					}
				}
				
				break;
			}
			// Scan for "NO CARRIER" message
			else if(strstr(unsolStr, "NO C")!=NULL)
			{
				vTaskDelay(2);
				// NO CARRIER
				int res = GSMReadN(15, '\n', 1, UnsolBuffer);
				if(res == -1)
				{
					//ERROR:
					_dbgwrite("ERROR GSMReadN NO CARRIER\r\n");
										
					// Execute Error Handler
					ErrorHandler(errorType);
				}
				else
				{
					UnsolBuffer[res] = '\0';
					EventType = ON_NO_CARRIER;
					mainCall.Status = CALL_READY;
				}	
				break;
			}
			// Scan for "BUSY" message
			else if(strstr(unsolStr, "BUSY")!=NULL)
			{
				vTaskDelay(2);
				// BUSY
				int res = GSMReadN(15, '\n', 1, UnsolBuffer);
				if(res == -1)
				{
					// ERROR:
					_dbgwrite("ERROR GSMReadN BUSY\r\n");
										
					// Execute Error Handler
					ErrorHandler(errorType);
				}
				else
				{
					UnsolBuffer[res] = '\0';
					EventType = ON_BUSY;
					mainCall.Status = CALL_BUSY;
				}
				break;
			}
			// Scan for "+CMS ERROR" message
			else if(strstr(unsolStr, "+CMS")!=NULL)
			{
				vTaskDelay(2);
				//+CMS ERROR:<n>
				// Search the first LineFeed and put the string inside UnsolBuffer
				int res = GSMReadN(20, '\n', 1, UnsolBuffer);
				
				// Check result
				if(res == -1)
				{
					_dbgwrite("ERROR GSMReadN +CMS\r\n");
										
					// Execute Error Handler
					ErrorHandler(errorType);
				}
				else
				{
					UnsolBuffer[res] = '\0';
					char temp[10];
					// get error number
					res = getfield(':', '\r', 8, 1, UnsolBuffer, temp, 500);
					
					if(res != 1)
					{
						// Execute Error Handler
						ErrorHandler(errorType);
						
						break;
					}
					else
					{				
						// Set error code
						errorCode = atoi(temp);
						mainOpStatus.ErrorCode = errorCode;
						// Set GSM Event:
						EventType = ON_ERROR;	
					}
				}	
				break;
			}
			// Scan for "+CME ERROR" message
			else if(strstr(unsolStr, "+CME")!=NULL)
			{
				vTaskDelay(2);
				//+CME ERROR:<n>
				// Search the first LineFeed and put the string inside UnsolBuffer
				int res = GSMReadN(20, '\n', 1, UnsolBuffer);
				UnsolBuffer[res] = '\0';
				
				// Check result
				if(res == -1)
				{
					//ERROR:
					_dbgwrite("ERROR GSMReadN +CME\r\n");
					
					// Execute Error Handler
					ErrorHandler(errorType);
				}
				else
				{
					UnsolBuffer[res] = '\0';
					char temp[10];
					// get error number
					res = getfield(':', '\r', 8, 1, UnsolBuffer, temp, 500);
					
					if(res != 1)
					{
						// Execute Error Handler
						ErrorHandler(errorType);
						
						break;
					}
					else
					{				
						// Set error code
						errorCode = atoi(temp);
						mainOpStatus.ErrorCode = errorCode;
						// Set GSM Event:
						EventType = ON_ERROR;	
					}
				}	
				break;
			}
			// Scan for "+CMTI:..." message
			else if(strstr(unsolStr, "+CMT")!=NULL)
			{
				vTaskDelay(2);
				//+CMTI:<memory>,<index>
				// <memory> can be "SM" or "ME"
				// <index> is the int number of new SMS location in memory 
				
				// Search the first LineFeed and put the string inside UnsolBuffer
				int res = GSMReadN(20, '\n', 1, UnsolBuffer);
				
				// Check result
				if(res == -1)
				{
					//ERROR:
					_dbgwrite("ERROR GSMReadN +CMTI\r\n");
										
					// Execute Error Handler
					ErrorHandler(errorType);
				}
				else
				{
					UnsolBuffer[res] = '\0';
					char temp[10];
					// get Memory Type
					res = getfield('"', '"', 4, 1, UnsolBuffer, temp, 500);
					
					if(res != 1)
					{
						// Execute Error Handler
						ErrorHandler(errorType);
					
						break;
					}
					else
					{				
						// Set mainSMS.MemType
						if(strstr(temp, "SM")!=NULL)
							IncomingSMS_MemType = SM_MEM;
						else if(strstr(temp, "ME")!=NULL)
							IncomingSMS_MemType = ME_MEM;
							
						// Get also index if Memory type is correct (SM or ME)
						res = getfield(',','\r', 5, 1, UnsolBuffer, temp, 500);
						int indexSMS = atoi(temp);
						
						if(indexSMS > 0)
						{
							IncomingSMS_Index = indexSMS;
						}
						// Set GSM Event:
						EventType = ON_SMS_REC;	
					}
				}
				break;
			} 
			// Scan for ":..." message
			else if(strstr(unsolStr, "+CDS")!=NULL)
			{
				vTaskDelay(50);
				char temp[25];
				//+CDS: <fo>,<mr>,[<ra>],[<tora>],<scts>,<dt>, <st>(text mode enabled)
				
				// Search the first LineFeed and put the string inside UnsolBuffer
				int res = GSMReadN(145, '\n', 1, UnsolBuffer);
				
				// Check result
				if(res == -1)
				{
					//ERROR:
					_dbgwrite("ERROR GSMReadN +CDS\r\n");
					
					// Execute Error Handler
					ErrorHandler(errorType);
				}
				else
				{
					UnsolBuffer[res] = '\0';
					// get <mr>
					res = getfield(',', ',', 4, 1, UnsolBuffer, temp, 500);
					if(res != 1)
					{
						// Execute Error Handler
						ErrorHandler(errorType);
						break;
					}
					else
					{
						mainSMS.MessageReference = atoi(temp);
					}
					
					// get <ra> (destination)
					res = getfield('\"', '\"', 20, 1, UnsolBuffer, temp, 500);
					
					if(res != 1)
					{
						// Execute Error Handler
						ErrorHandler(errorType);
						break;
					}
					else
					{
						sprintf(mainSMS.Destination, temp);
					}
					
					// get <dt>
					res = getfield('\"', '\"', 22, 5, UnsolBuffer, temp, 500);
					if(res != 1)
					{
						// Execute Error Handler
						ErrorHandler(errorType);
						break;
					}
					else
					{
						sprintf(mainSMS.DateTime, temp);
						
						// Convert DateTime string to struct time:
						int countFirstSlash = strlen(mainSMS.DateTime);
						int dmp;
						char tmpval[25];
						// Get index of first slash:
						for(dmp = 0; dmp < countFirstSlash; dmp++)
						{
							if(mainSMS.DateTime[dmp] == '/')
								countFirstSlash = dmp;
						}
						// copy first fieldof DateTime inside tmpval char[]
						strncpy(tmpval, mainSMS.DateTime, countFirstSlash);
						
						// Convert YEAR:
						mainSMS.time.tm_year = atoi(tmpval)+100;
						
						// Get second field (month)
						getfield('/', '/', 5, 1, mainSMS.DateTime, tmpval, 500);
						// Convert MONTH:
						mainSMS.time.tm_mon = atoi(tmpval);
						
						// Get third field (day)
						getfield('/', ',', 5, 2, mainSMS.DateTime, tmpval, 500);
						// Convert DAY
						mainSMS.time.tm_mday = atoi(tmpval);
						
						// Get hour
						getfield(',', ':', 5, 1, mainSMS.DateTime, tmpval, 500);
						// convert HOUR
						mainSMS.time.tm_hour = atoi(tmpval);
						
						// Get minutes
						getfield(':', ':', 5, 1, mainSMS.DateTime, tmpval, 500);
						// convert MINUTES
						mainSMS.time.tm_min = atoi(tmpval);
						
						// Get seconds & GMT
						countFirstSlash = strlen(mainSMS.DateTime);
						int quithere = 0;
						// Get index of last ':' char to retrieve seconds:
						for(dmp = 0; dmp < countFirstSlash; dmp++)
						{
							// stop at second ':'
							if((mainSMS.DateTime[dmp] == ':')&&(quithere != 0))
							{
								// store index of second ':'
								countFirstSlash = dmp;
							}
							// get first ':'
							if(mainSMS.DateTime[dmp] == ':')
							{	
								quithere++;
							}	
							
						}
						// copy 5 chars (2 chars for seconds, up to 5 chars for GMT)
						strncpy(tmpval, &mainSMS.DateTime[countFirstSlash+1], 5);
						
						// copy seconds
						char secs[3];
						secs[0] = tmpval[0];
						secs[1] = tmpval[1];
						secs[2] = '\0';
						
						// copy gmt
						char gmt[4];
						gmt[0] = tmpval[2];
						gmt[1] = tmpval[3];
						gmt[2] = tmpval[4];
						gmt[3] = '\0';
						
						// Convert SECONDS
						mainSMS.time.tm_sec = atoi(secs);

						// Add GMT to hours
						int gmtVal = atoi(gmt);
						gmtVal = gmtVal + mainSMS.time.tm_hour;
						/*
						if(gmtVal < 0)
						{
							mainSMS.time.tm_hour = gmtVal + 24;
							mainSMS.time.tm_mday--; 
						}
						else if(gmtVal > 24)
						{
							mainSMS.time.tm_hour = gmtVal - 24;
							mainSMS.time.tm_mday++;
						}
						*/
					}
					
					// get <st>
					res = getfield(',', '\r', 4, 8, UnsolBuffer, temp, 500);
					if(res != 1)
					{
						// Execute Error Handler
						ErrorHandler(errorType);
						break;
					}
					else
					{
						mainSMS.ReportValue = atoi(temp);
					}
				}			
				
				// Set GSM Event:
				EventType = ON_SMS_SENT;
				
				break;
			}
			// Scan for "+CREG:..." message
			else if(strstr(unsolStr, "+CRE")!=NULL)
			{
				vTaskDelay(20);
				//+CREG
				// replies: +CREG:<stat>
				// <stat>
				// - 0: deregistration
				// - 1: registration
				// - 2: searching for new operator
				// - 3: registration failed
				// - 4: unknown
				// - 5: registered, roaming
				
				// Search the first LineFeed and put the string inside UnsolBuffer
				int res = GSMReadN(15, '\n', 1, UnsolBuffer);
				
				// Set GSM.ConnStatus = +CREG value
				if(res == -1)
				{
					//ERROR:
					_dbgwrite("ERROR GSMReadN +CREG\r\n");
					
					// Execute Error Handler
					ErrorHandler(errorType);
				}
				else
				{
					UnsolBuffer[res] = '\0';
					char temp[5];
					// get registration status number
					res = getfield(':', '\r', 4, 1, UnsolBuffer, temp, 500);
					
					if(res != 1)
					{
						// Execute Error Handler
						ErrorHandler(errorType);
					
						break;
					}
					else
					{				
						int regStatus = atoi(temp);
						BYTE _oldConnStatus = mainGSM.ConnStatus;
						switch(regStatus)
						{
							case 0:
								// Set mainGSM Connection Status
								mainGSM.ConnStatus = NO_REG;
								break;
							case 1:
								// Set mainGSM Connection Status
								mainGSM.ConnStatus = REG_SUCCESS;
								break;
							case 2:
								// Set mainGSM Connection Status
								mainGSM.ConnStatus = SEARCHING;
								break;
							case 3:
								// Set mainGSM Connection Status
								mainGSM.ConnStatus = REG_DENIED;
								break;
							case 4:
								// Set mainGSM Connection Status
								mainGSM.ConnStatus = UNKOWN;
								break;
							case 5:
								// Set mainGSM Connection Status
								mainGSM.ConnStatus = ROAMING;
								break;
							default:
								// Set mainGSM Connection Status
								mainGSM.ConnStatus = UNKOWN;
								break;
						}
						if(_oldConnStatus != mainGSM.ConnStatus)
						{
							// Set GSM Event:
							EventType = ON_REG;
						}
					}
				}
				break;
			}
			// Scan for "+KTCP_NOTIF:..." message or +KTCP_DATA
			else if(strstr(unsolStr, "+KTC")!=NULL)
			{
				vTaskDelay(20);
				
				// Search the first LineFeed and put the string inside UnsolBuffer
				int res = GSMReadN(20, '\n', 1, UnsolBuffer);
				
				if(strstr(UnsolBuffer, "P_NOTIF:") != NULL)
				{			
					//+KTCP_NOTIF
					// response: +KTCP_NOTIF:<session_id>,<tcp_notif>
					// <tcp_notif>
					// 0- Network error 
					// 1- No more sockets available; max. number already reached 
					// 2- Memory problem  
					// 3- DNS error 
					// 4-TCP disconnection by the server or remote client
					// 5-TCP connection error 
					// 6- Generic error 
					// 7- Fail to accept client request’s 
					// 8- Data sending is OK but KTCPSND was waiting more or less characters 
					// 9- Bad session ID 
					// 10- Session is already running 
					// 11- All sessions are used 

					
					// Set xSocket.notif = tcp_notif
					if(res == -1)
					{
						//ERROR:
						_dbgwrite("ERROR GSMReadN +KTCP_NOTIF\r\n");
						
						// Execute Error Handler
						ErrorHandler(errorType);
					}
					else
					{
						UnsolBuffer[res] = '\0';
						char temp[5];
						// get phone number
						res = getfield(',', '\r', 4, 1, UnsolBuffer, temp, 500);
						
						if(res != 1)
						{
							// Execute Error Handler
							ErrorHandler(errorType);
						
							break;
						}
						else
						{				
							xSocket->notif = atoi(temp);
						}
					}
				}
				else if(strstr(UnsolBuffer, "P_DATA:") != NULL)// Ignore +KTCP_DATA
				{
					//+KTCP_DATA
					// response: +KTCP_DATA:<session_id>,<rxLen>
					// <rxLen>
					
					// Set xSocket.notif = tcp_notif
					if(res == -1)
					{
						//ERROR:
						_dbgwrite("ERROR GSMReadN +KTCP_DATA\r\n");

						// Execute Error Handler
						ErrorHandler(errorType);
					}
					else
					{
						UnsolBuffer[res] = '\0';
						char temp[7];
						// get phone number
						res = getfield(',', '\r', 6, 1, UnsolBuffer, temp, 500);
						
						if(res != 1)
						{
							// Execute Error Handler
							ErrorHandler(errorType);
						
							break;
						}
						else
						{				
							xSocket->rxLen = atoi(temp);
                            if(xSocket->rxLen > 1024)
                                xSocket->rxLen= 1024;
						}
					}
				}
				break;
			}			
			// Scan for "+KFTP_ERROR:<session_id>,<ftpError>..." message or 
			// +KFTP_SND_DONE:<session_id> or +KFTP_SND_ERROR:<session_id>,<ftp_error> or
			// +KFTP_RCV_DONE:<session_id> or +KFTP_RCV_ERROR:<session_id>,<ftp_error> 
			else if(strstr(unsolStr, "+KFT")!=NULL)
			{
				vTaskDelay(20);

				// Search the first LineFeed and put the string inside UnsolBuffer
				int res = GSMReadN(20, '\n', 1, UnsolBuffer);

				if(strstr(UnsolBuffer, "P_ERROR :") != NULL)
				{			
					// response: +KFTP_ERROR:<session_id>,<ftp_error>
					// <ftp_error>
					
					// xFTPSocket
					// Set xFTPSocket.ftpError = ftp_error
					if(res == -1)
					{
						//ERROR:
						_dbgwrite("ERROR GSMReadN +KFTP_ERROR\r\n");
				
						// Execute Error Handler
						ErrorHandler(errorType);
					}
					else
					{
						UnsolBuffer[res] = '\0';
						char temp[5];
						// get phone number
						res = getfield(',', '\r', 4, 1, UnsolBuffer, temp, 500);
						
						if(res != 1)
						{
							// Execute Error Handler
							ErrorHandler(errorType);
						
							break;
						}
						else
						{				
							xFTPSocket->ftpError = atoi(temp);
						}
					}
				}
			}	
            // +UFTPER: <error_class>,<error_code>
            else if(strstr(unsolStr, "+UFT")!=NULL)
            {
                vTaskDelay(20);
				
				// Search the first LineFeed and put the string inside UnsolBuffer
				int res = GSMReadN(20, '\n', 1, UnsolBuffer);
				if(res == -1)
                {
                    //ERROR:
                    _dbgwrite("ERROR GSMReadN +UFTPER\r\n");

                    // Execute Error Handler
                    ErrorHandler(errorType);
                }
                else
                {
                    //+UFTPER: <error_class>,<error_code>
                    if(strstr(UnsolBuffer, "PER:") != NULL)
                    {			
						UnsolBuffer[res] = '\0';
                        char temp[10];
                        int err_class = 0;
                        // get error number
                        int res = getfield(' ', ',', 8, 1, UnsolBuffer, temp, 500);
                        if(res == 1)
                        {
                            err_class = atoi(temp);
                        }
                        res = getfield(',', '\r', 8, 1, UnsolBuffer, temp, 500);

                        if(res == 1)
                        {
                            if(err_class == 0) // No Errors
                            {
                                return;
                            }
                            else
                            {   
                                // Set error code
                                errorCode = atoi(temp);
                                xFTPSocket->ftpError = errorCode;
                                mainOpStatus.ErrorCode = errorCode;
                                // SET State Machine as IDLE
                                mainOpStatus.ExecStat = OP_FTP_ERR;
                                mainGSMStateMachine = SM_GSM_IDLE;
                                // Set GSM Event:
                                EventType = ON_ERROR;	
                            }
                        }	
					}
				}                
            }
            // Scan for "+UUS..." messages from SARA TCP-related URCs
			else if(strstr(unsolStr, "+UUS")!=NULL)
			{
				vTaskDelay(20);
				
				// Search the first LineFeed and put the string inside UnsolBuffer
				int res = GSMReadN(20, '\n', 1, UnsolBuffer);
				if(strstr(UnsolBuffer, "OCL:") != NULL)
				{			
					if(res == -1)
					{
						//ERROR:
						_dbgwrite("ERROR GSMReadN +UUSOCL\r\n");
						
						// Execute Error Handler
						ErrorHandler(errorType);
					}
					else
					{
						UnsolBuffer[res] = '\0';
						char temp[5];
						// get phone number
						res = getfield(' ', '\r', 4, 1, UnsolBuffer, temp, 500);
						
						if(res != 1)
						{
							// Execute Error Handler
							ErrorHandler(errorType);
						
							break;
						}
						else
						{				
							xSocket->status = SOCK_CLOSED;//atoi(temp);
							//xSocket->number = INVALID_SOCKET;

							EventType = ON_SOCK_CLOSE;
						}
					}
				}
				else if(strstr(UnsolBuffer, "ORD:") != NULL)// +UUSORD:
				{
					// +UUSORD: <socket_id>,<rxLen>
					// <rxLen>
					
					/* INDBG
					// Enabled this to receive the UNSOL notification also on debug UART
					_dbgwrite("+UUS"); _dbgline(UnsolBuffer);
					// indbg */

					if(res == -1)
					{
						//ERROR:
						_dbgwrite("ERROR GSMReadN +UUSORD\r\n");

						// Execute Error Handler
						ErrorHandler(errorType);
					}
					else
					{
						UnsolBuffer[res] = '\0';
						char temp[7];
						// get phone number
						res = getfield(',', '\r', 6, 1, UnsolBuffer, temp, 500);
						
						if(res != 1)
						{
							// Execute Error Handler
							ErrorHandler(errorType);
						
							break;
						}
						else
						{				
							int newLen = atoi(temp);
                            if(newLen > 1024)
                                newLen = 1024;
                            else if(newLen < 0)
                            	newLen = 0;
                            xSocket->rxLen = newLen;
						}
					}
				}
                // SMTP Related:
                // +UUSMTPCR
                else if(strstr(UnsolBuffer, "MTPCR:") != NULL)
				{
					// +UUSMTPCR: <smtp_command>,<smtp_result>[,<reject_rcpt_addr1>[,<reject_rcpt_addr2>[,...]]]
					
					if(res == -1)
					{
						//ERROR:
						_dbgwrite("ERROR GSMReadN +UUSMTPCR\r\n");

						// Execute Error Handler
						ErrorHandler(errorType);
					}
					else
					{
                        UnsolBuffer[res] = '\0';
						char temp[7];
                        int smtp_command = -1;
                        int smtp_result = -1;
						// get smtp_command
						res = getfield(' ', ',', 6, 1, UnsolBuffer, temp, 500);
						
						if(res != 1)
						{
							// Execute Error Handler
							ErrorHandler(errorType);
						
							break;
						}
						else
						{				
							smtp_command = atoi(temp);
						}
                        
                        res = getfield(',', '\r', 6, 1, UnsolBuffer, temp, 500);
                        
                        if(res != 1)
                        {
                            ErrorHandler(errorType);
                        }
                        else
                        {
                            smtp_result = atoi(temp); // 0=success, 1=fail, 2=partial success
                        }
                        
                        if(smtp_result > 0)
                        {
                            mainOpStatus.ErrorCode = smtp_command;
                            // SET State Machine as IDLE
                            mainOpStatus.ExecStat = OP_SMTP_ERR;
                            mainGSMStateMachine = SM_GSM_IDLE;
                            // Set GSM Event:
                            EventType = ON_ERROR;	
                        }
					}
				}
				break;
			}
			// Message not understood, shift one char over and tries again
			else
			{	
				if( (errorType == NO_ERR) || \
                    (errorType == CMD_UNEXPECTED) || \
                    (errorType == ECHO_EXPECTED_ERR) || \
                    (errorType == ANSW_EXPECTED_ERR))
				{

					if(dontEraseBuffer == TRUE)
					{
						startPoint++;
						endPoint = GSMBufferSize();
						if((endPoint - startPoint) >= 4)
						{
							GSMpSeek(startPoint, 4, unsolStr);
						}
						else
						{
							int i;
							for(i = 1; i < 4; i++)
								unsolStr[i-1] = unsolStr[i];
							GSMRead(&unsolStr[3], 1);
						}
					}
					else
					{
						int i;
						for(i = 1; i < 4; i++)
							unsolStr[i-1] = unsolStr[i];
						GSMRead(&unsolStr[3], 1);
					}
				}
                /*
				else if(errorType == CMD_UNEXPECTED)
				{
					int i;
					for(i = 1; i < 4; i++)
						unsolStr[i-1] = unsolStr[i];
					GSMRead(&unsolStr[3], 1);
				}
                 * */
			}
		}	
	}	
	// If the UnsolParsing generated a GSM Event, launch the event handler
	if (EventType != NO_EVENT)
	{
        if(mainGSMStateMachine == SM_GSM_LOW_POWER)
        {
            SARA_DTR_IO = TURN_SARA_DTR_ON;
            EventHandler();
            mainGSMStateMachine = SM_GSM_IDLE;
        }
        else
        {
            EventHandler();
        }
	}
}

void EventHandler()
{
    // TODO removed this condition here, since SMS Call and so on will no more remove low power (ublox "IDLE") status!
//    if((mainGSMStateMachine == SM_GSM_LOW_POWER) && (EventType != NO_EVENT))
//    {
//        OnLowPowerDisabled();
//    }
    
	switch(EventType)
	{
		case ON_RING:
			OnRing(mainCall.CallerID);
			EventType = NO_EVENT;
			break;
			
		case ON_NO_CARRIER:
			OnNoCarrier(mainCall.CallerID);
			EventType = NO_EVENT;
			break;
			
		case ON_ERROR:
			OnError(mainOpStatus.ExecStat, mainOpStatus.ErrorCode);
			EventType = NO_EVENT;
			break;
				
		case ON_SMS_SENT:
			OnSMSSentReport(mainSMS.MessageReference, mainSMS.ReportValue);
			EventType = NO_EVENT;
			break;
		
		case ON_SMS_REC:
			OnSMSReceived(IncomingSMS_MemType, IncomingSMS_Index);
			EventType = NO_EVENT;
			break;
			
		case ON_REG:
			OnRegistration(mainGSM.ConnStatus);
			EventType = NO_EVENT;
			break;
			
		case ON_BUSY:
			OnBusy(mainCall.CallerID);
			EventType = NO_EVENT;
			break;

		case ON_SOCK_CLOSE:
			OnSocketDisconnect(xSocket->number);
			EventType = NO_EVENT;
			break;
	}
}

// Executes actions related to the errorType received, and state machine status
void ErrorHandler(int errorType)
{
	// Check if state machine is IDLE
	if(mainGSMStateMachine == SM_GSM_IDLE)
	{
		#if defined(STACK_USE_UART)
		char temp[50];
		sprintf(temp, "State machine: IDLE, Error type: %d\r\n", errorType);
		_dbgwrite(temp);
		#endif
		
		// SET State Machine as FAULT	
		mainOpStatus.ExecStat = OP_SYNTAX_ERR;
		mainGSMStateMachine = SM_GSM_HW_FAULT;
		_dbgwrite("generic error on message received!\r\n");
	}
	
	// Check if state machine is CMD_PENDING
	else if(mainGSMStateMachine == SM_GSM_CMD_PENDING)
	{
		#if defined(STACK_USE_UART)
		char temp[50];
		sprintf(temp, "State machine: CMD_PENDING, Error type: %d\r\n", errorType);
		_dbgwrite(temp);
		#endif
		
		// SET State Machine as FAULT		
		mainOpStatus.ExecStat = OP_SYNTAX_ERR;
		mainGSMStateMachine = SM_GSM_HW_FAULT;
		_dbgwrite("generic error on message received!\r\n");
	}
	
	// Check if state machine is FAULT
	else if (mainGSMStateMachine == SM_GSM_HW_FAULT)
	{
		#if defined(STACK_USE_UART)
		char temp[50];
		sprintf(temp, "State machine: HW_FAULT, Error type: %d\r\n", errorType);
		_dbgwrite(temp);
		#endif
		// Execute HWFault_Test() until unlocked...
		SaraInit(115200);
		int a = 1;
		while(a != 0)
		{	
			/*HiloPok();
			vTaskDelay(50);
			HiloReset();
			vTaskDelay(50);
			
			a = HiloComTest();*/
            if(Sara_PwrOn(15))//2500))
            {
                vTaskDelay(50);
                SaraReset();
                vTaskDelay(50);

                a = SaraComTest();
            }
            else
            {
                _dbgwrite("\r\nSARA not ready after RESET!\r\n");
                SaraReset();
                vTaskDelay(300);
                SaraReset();
                _dbgwrite("Testing RESET again...\r\n");
            }
		}
	}
}

/****** Parsing Helper Functions ******/

/**
* getfield - Reads the "parnum" field of "srcbuff" buffer, and puts it on "destbuff".
* \param start - char symbol to use as start parsing marker
* \param stop - char symbol to use as stop parsing marker
* \param maxlen - max length of field (usefull to prevent array overflows)
* \param parnum - number of the occurrence of same field to be accepted
* \param srcbuff - char[] source buffer of the string to parse
* \param destbuff - char[] destinationbuffer of the string to be filled with field content
* \param timeout - max time before to exit with error result (ms, max 1000 ms)
* \return the result of operation.
*/
int getfield(char start, char stop, int maxlen, int parnum, char* srcbuff, char* destbuff, int timeout)
{
	// TODO: handle Tick overflow
	DWORD startTick = TickGetDiv256();
	DWORD thisTick = TickGetDiv256();
    int len = 0;
    int fieldCount = 0;
    int startInd = 0;
    int stopInd = 0;
    int startFound = 0;
    int length = strlen(srcbuff);
    // Transform timeout in ms...
    timeout = timeout*5/20;

    while(((thisTick - startTick) <= (timeout))&&(len < length))
    {
        len++;
        thisTick = TickGetDiv256();
        // Search start char
        if(startFound == 0)
        {
            if(srcbuff[startInd] == start)
            {
                fieldCount++;
                if(fieldCount == parnum)
                    startFound++;
            }
            startInd++;
        }
        // Search Stop char
        else
        {
            
            if(stopInd < maxlen)
            {            
	            if(srcbuff[stopInd + startInd] == stop)
	            {
	                strncpy(destbuff, &srcbuff[startInd], stopInd);
	                destbuff[stopInd] = '\0';
	                return 1;
	            }
	            stopInd++;
	        }
	        else
	        	return 2;
        }
    }
    if((thisTick - startTick) > timeout)
    {
		return 3;
    }
    return 0;
}

void waitForGsmMsg(const char* msg, int timeout)
{
	DWORD maxtimeout = TickGetDiv64K() + timeout+1;
	// Wait to have enough data on buffer...
	while((GSMBufferSize() < strlen(msg))&&(TickGetDiv64K() < maxtimeout))
	{
		vTaskDelay(1);
	}
	if(GSMBufferSize() < strlen(msg))
	{
//		_dbgline("[ERROR] waitForGsmMsg did not found enough chars on buffer...");
	}
}

void waitForGsmLines(int lines, int timeout)
{
	char linesStr[2] = {'\0', '\0'};
	int _lines = 0;
	int pos = 0;
	DWORD maxtimeout = TickGetDiv64K() + timeout+1;
	// Wait to have enough data on buffer...
	while((_lines < lines)&&(TickGetDiv64K() < maxtimeout))
	{
		if(GSMpSeek(pos, 1, linesStr) == TRUE)
		{
			pos++;
			if(linesStr[0] == '\n')
				_lines++;
		}
		else
		{
			vTaskDelay(1);
		}
	}
}

/**
 * echoFind - searches the echo reply from GSMBuffer. It clears echo from GSMBuffer only if string is found.
 			- (200-4) chars max echo can be found
 * \param echoStr - const char* that contains echo string to search
 * \return result of operation.
 */
int echoFind(const char* echoStr/*, int startPoint*/)
{
	// 200-4 max echo to find... to be defined!
	char echoBuff[201];
	int lenStr = strlen(echoStr); // + startPoint;

	// There could be some cases I receive a "\r\n" just before my ECHO,
	// to avoid problems just erase them
	BOOL trashUnwantedChars = FALSE;
	do
	{
		// Read buffer content without erasing
		int pReadCount = abs(GSMpRead(echoBuff, lenStr)); //lenStr);
		if(pReadCount > lenStr)
			pReadCount = lenStr;
		echoBuff[pReadCount] = '\0';
		// Check if first char if \r or \n
		if((echoBuff[0] == '\r') || (echoBuff[0] == '\n'))
		{
			GSMRead(echoBuff, 1);
			trashUnwantedChars = TRUE; // if yes continue loop
		}
		else
		{
			trashUnwantedChars = FALSE; // else start the echo parsing
		}
	} while(trashUnwantedChars);

	//if(strstr(echoBuff, echoStr) !=NULL)
	if(!strncmp(echoBuff, echoStr, lenStr))
	{
		// Flush GSMBuffer of the same size of string read...
		GSMRead(echoBuff, lenStr);
		echoBuff[lenStr] = '\0';
		return 0;
	}
	else if(strstr(echoBuff, "ERROR") != NULL)
	{
		// Reply present! Read until the lineNumber occurrence of '\n' char are found...
		int cc = GSMReadN(200, '\n', 2, echoBuff);
		echoBuff[cc] = '\0';
		
		// SET State Machine as IDLE						
		mainOpStatus.ExecStat = OP_SYNTAX_ERR;
		mainGSMStateMachine = SM_GSM_IDLE;
		return 1;
	}
	else if(strstr(echoBuff, "+CMS ") != NULL)
	{
		// Reply present! Read until the lineNumber occurrence of '\n' char are found...
		int cc = GSMReadN(200, '\n', 2, echoBuff);
		echoBuff[cc] = '\0';
		
		// SET State Machine as IDLE						
		mainOpStatus.ExecStat = OP_CMS_ERR;
		mainGSMStateMachine = SM_GSM_IDLE;
		
		//+CMS ERROR:<n>
		char temp[10];
		// get error number
		int res = getfield(':', '\r', 8, 1, echoBuff, temp, 500);
		
		if(res == 1)
		{
			// Set error code
			errorCode = atoi(temp);
			mainOpStatus.ErrorCode = errorCode;
			// Set GSM Event:
			EventType = ON_ERROR;	
		}	
		return 2;
	}
	else if(strstr(echoBuff, "+CME ") != NULL)
	{
		// Reply present! Read until the lineNumber occurrence of '\n' char are found...
		int cc = GSMReadN(200, '\n', 2, echoBuff);
		echoBuff[cc] = '\0';
		
		// SET State Machine as IDLE
		mainOpStatus.ExecStat = OP_CME_ERR;
		mainGSMStateMachine = SM_GSM_IDLE;
		
		//+CME ERROR:<n>
		char temp[10];
		// get error number
		int res = getfield(':', '\r', 8, 1, echoBuff, temp, 500);
		
		if(res == 1)
		{
			// Set error code
			errorCode = atoi(temp);
			mainOpStatus.ErrorCode = errorCode;
			// Set GSM Event:
			EventType = ON_ERROR;	
		}	
		return 3;	
	}
	else
	{
		return -1;
	}	
}

/**
 * getAnswer - searches the answer reply from GSMBuffer. It clears answer + lineNumber lines from GSMBuffer only if string is found.
 			 - The answer should be always in <CR><LF><response><CR><LF> format!
 * \param answer2src - const char* that contains answer string to search
 * \param lineNumber - how many lines to read if string is found
 * \param replyBuffer - char* buffer for destination
 * \return result of operation.
 */
int getAnswer(const char* answer2src, int lineNumber, char* replyBuffer)
{
	// Answer is in format:
	// <CR><LF><response><CR><LF>
	int lenStr = strlen(answer2src);
	GSMpRead(replyBuffer, lenStr+2);
	replyBuffer[lenStr+2] = '\0';

	if(strstr(replyBuffer, answer2src) != NULL)
	{
		memset(replyBuffer, '\0', lenStr+2);
		// Reply present! Read until the lineNumber occurrence of '\n' char are found...
		int cc = GSMReadN(200, '\n', lineNumber, replyBuffer);
		if(cc == -1)
		{
			return -1;
		}
		else
			replyBuffer[cc] = '\0';

		return 0;
	}
	else if(strstr(replyBuffer, "+CMS") != NULL)
	{
		// Reply present! Read until the lineNumber occurrence of '\n' char are found...
		int cc = GSMReadN(200, '\n', 2, replyBuffer);
		replyBuffer[cc] = '\0';
		
		// SET State Machine as IDLE						
		mainOpStatus.ExecStat = OP_CMS_ERR;
		mainGSMStateMachine = SM_GSM_IDLE;
		
		//+CMS ERROR:<n>
		char temp[10];
		// get error number
		int res = getfield(':', '\r', 8, 1, replyBuffer, temp, 500);
		
		if(res == 1)
		{
			// Set error code
			errorCode = atoi(temp);
			mainOpStatus.ErrorCode = errorCode;
			// Set GSM Event:
			EventType = ON_ERROR;	
		}	
		return 2;
	}
	else if(strstr(replyBuffer, "+CME") != NULL)
	{
		// Reply present! Read until the lineNumber occurrence of '\n' char are found...
		int cc = GSMReadN(200, '\n', 2, replyBuffer);
		replyBuffer[cc] = '\0';
		
		// SET State Machine as IDLE
		mainOpStatus.ExecStat = OP_CME_ERR;
		mainGSMStateMachine = SM_GSM_IDLE;
		
		//+CME ERROR:<n>
		char temp[10];
		// get error number
		int res = getfield(':', '\r', 8, 1, replyBuffer, temp, 500);
		
		if(res == 1)
		{
			// Set error code
			errorCode = atoi(temp);
			mainOpStatus.ErrorCode = errorCode;
			// Set GSM Event:
			EventType = ON_ERROR;	
		}	
		return 3;	
	}
	else if(strstr(replyBuffer, "NO C") != NULL)
	{
		// Reply present! Read until the lineNumber occurrence of '\n' char are found...
		int cc = GSMReadN(200, '\n', 2, replyBuffer);
		replyBuffer[cc] = '\0';
		
		// SET State Machine as IDLE						
		mainOpStatus.ExecStat = OP_NO_CARR_ERR;
		mainGSMStateMachine = SM_GSM_IDLE;
		return 4;
	}
	
	else if(strstr(replyBuffer, "+KSM")!=NULL)
	{
		// Reply present! Read until the lineNumber occurrence of '\n' char are found...
		int cc = GSMReadN(200, '\n', 2, replyBuffer);
		replyBuffer[cc] = '\0';
		
		// SET State Machine as IDLE
		mainOpStatus.ExecStat = OP_SMTP_ERR;
		mainGSMStateMachine = SM_GSM_IDLE;
		
		//+KSMTP ERROR:<n>
		char temp[10];
		// get error number
		int res = getfield(':', '\r', 8, 1, replyBuffer, temp, 500);
		
		if(res == 1)
		{
			// Set error code
			errorCode = atoi(temp);
			mainOpStatus.ErrorCode = errorCode;
			// Set GSM Event:
			EventType = ON_ERROR;	
		}	
		return 3;	
	}
	
	else if(strstr(replyBuffer, "+KFT")!=NULL)
	{
		// Reply present! Read until the lineNumber occurrence of '\n' char are found...
		int cc = GSMReadN(200, '\n', 2, replyBuffer);
		replyBuffer[cc] = '\0';
		
		// SET State Machine as IDLE
		mainOpStatus.ExecStat = OP_FTP_ERR;
		mainGSMStateMachine = SM_GSM_IDLE;
		
		//+KFTP ERROR:<session_id>,<n>
		char temp[10];
		// get error number
		int res = getfield(',', '\r', 8, 1, replyBuffer, temp, 500);
		
		if(res == 1)
		{
			// Set error code
			errorCode = atoi(temp);
			xFTPSocket->ftpError = errorCode;
			mainOpStatus.ErrorCode = errorCode;
			// Set GSM Event:
			EventType = ON_ERROR;	
		}	
		return 3;
	}
    // +UFTPER: <error_class>,<error_code>
    else if(strstr(replyBuffer, "+UFT")!=NULL)
	{
		// Reply present! Read until the lineNumber occurrence of '\n' char are found...
		int cc = GSMReadN(200, '\n', 2, replyBuffer);
		replyBuffer[cc] = '\0';
		
		//+UFTPER: <error_class>,<error_code>
		char temp[10];
        int err_class = 0;
		// get error number
		int res = getfield(' ', ',', 8, 1, replyBuffer, temp, 500);
        if(res == 1)
        {
            err_class = atoi(temp);
        }
		res = getfield(',', '\r', 8, 1, replyBuffer, temp, 500);
		
		if(res == 1)
		{
			// Set error code
			errorCode = atoi(temp);
			xFTPSocket->ftpError = errorCode;
			mainOpStatus.ErrorCode = errorCode;
            if(err_class == 0) // No Errors
            {
                return 0;
            }
            else
            {   
                // SET State Machine as IDLE
                mainOpStatus.ExecStat = OP_FTP_ERR;
                mainGSMStateMachine = SM_GSM_IDLE;
                // Set GSM Event:
                EventType = ON_ERROR;	
            }
		}	
		return 3;
	}
	// ERROR (generic error)
	else if(strstr(replyBuffer, "ERRO") != NULL)
	{
		// Reply present! Read until the lineNumber occurrence of '\n' char are found...
		int cc = GSMReadN(200, '\n', 2, replyBuffer);
		replyBuffer[cc] = '\0';
		
		// SET State Machine as IDLE						
		mainOpStatus.ExecStat = OP_SYNTAX_ERR;
		mainGSMStateMachine = SM_GSM_IDLE;
		return 1;
	}
	
	else
	{
		// NOT FOUND
		return -1;
	}
}


/**
 * CheckCmd - 	Check if GSM received a message (written inside msg2send).
 				This functions searches the first msg2send char inside GSMBuffer using GSMpSeek, and after uses getAnswer
 			 -  If yes set TRUE dataFound, if timeout occurs set TRUE timeout
 * \param countData - the starting position related to circular GSMBuffer (it uses GSMpSeek)
 * \param chars2read - the amount of lines to be read using getAnswer
 * \param tick - the starting tick value when the command was written to UART
 * \param cmdReply - char* to be used as buffer
 * \param msg2send - char* with echo to search
 * \param maxtimeout - max timeout seconds to track
 * \return result of operation
 */
int CheckCmd(int countData, int chars2read, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout)
{
    int _countDataBackup = countData;
    int _chars2readBackup = chars2read;
    // Add the wait for enough incoming data...
    waitForGsmMsg(msg2send, maxtimeout);

    char firstChar = msg2send[0];

	// Check timeout	
	while((TickGetDiv64K() - tick) < (maxtimeout + 2))
	{
		// Seek GSMBuffer to search firstChar character...
		if(GSMpSeek(countData, 1, cmdReply) == TRUE)
		{			
            BOOL checkNow = FALSE;
            if(cmdReply[0] == firstChar)
            {
            	while(countData > _countDataBackup)
            	{
                	// Discard unwanted chars from buffer (if \r or \n) - for example we want "+CREG: 0,1" but on buffer we have "<cr><lf>+CREG: 0,1<cr><lf>"
                	GSMpRead(cmdReply, 1);
                	if((cmdReply[0] == '\r')||(cmdReply[0] == '\n'))
                	{
                		countData--;
                		if(cmdReply[0] == '\n') // Remove a line number from getAnswer parameter since we already read one!
                			chars2read--;
                		GSMRead(cmdReply, 1);
                	}
                	else
                		break;
            	}
                checkNow = TRUE;
            }
            /*INDBG
            else
            {
                // If we don't need to check a line, but only a spare string, we have to constantly use strstr function!
                if((_countDataBackup == 0)&&(strstr(msg2send, cmdReply) != NULL))
                    checkNow = TRUE;
            } 
            //indbg*/
			if(checkNow == TRUE)
			{
				waitForGsmLines(chars2read, maxtimeout);
                int answRes = getAnswer(msg2send, chars2read, cmdReply);
                if((answRes == -1) && (GSMBufferSize() > 0))
                {
                    // Launch GSMUnsol and Try Again
                    GSMUnsol(ANSW_EXPECTED_ERR );
                    checkNow = FALSE;
                    countData = _countDataBackup;
                    chars2read = _chars2readBackup;
                }
                else
                    return answRes; 
			}	
			else
				countData++;
		}
		else
		{
			// do not increase buffer, but wait a while to increase buffer
			vTaskDelay(10);
		}
	}
	
	// Timeout
	return -2;
}

/**
 * CheckEcho - 	Check if GSM received echo message (written inside msg2send). 
 				This functions searches the '\r' char inside buffer using GSMpSeek, and after uses echoFind
 			 -  If yes set TRUE dataFound, if timeout occurs set TRUE timeout
 * \param countData - the starting position related to circular GSMBuffer (it uses GSMpSeek)
 * \param tick - the starting tick value when the command was written to UART
 * \param cmdReply - char* to be used as buffer
 * \param msg2send - char* with echo to search
 * \param maxtimeout - max timeout seconds to track
 * \return result of operation
 */
int CheckEcho(int countData, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout)
{
    int echoRes;
    int _countData = countData;
    int _countTotal = 0;
    char firstChar = msg2send[0];
    // Add the wait for enought incoming data...
    waitForGsmMsg(msg2send, maxtimeout);

	// Check timeout	
	while((TickGetDiv64K() - tick) < (maxtimeout + 2))
	{
		// Seek GSMBuffer to search first msg2send character...
		if(GSMpSeek(_countData, 1, cmdReply) == TRUE)
		{
			if(cmdReply[0] == firstChar)
			{
                echoRes = echoFind(msg2send/*, _countData*/);
                if((echoRes == -1) && (GSMBufferSize() > 0))
                {
                    // Launch GSMUnsol and Try Again
                    GSMUnsol(ECHO_EXPECTED_ERR);
                    _countData = 0; // restart counter since we may have parsed an unsol
                }
                else
                    return echoRes;
			}	
			else
			{
				_countData++;
				_countTotal++;
			}
		}
		else
		{
			// do not increase counter, but wait a while to increase buffer
			vTaskDelay(10);
		}	
	}
	// Timeout
	return -2;
}

#define HW_FAULT_COUNTER_MAX	3//10
static int gsmHwFaultCounter = 0;

/**
 * CheckErr  - 	Check if GSM received error message and updates mainGSMStateMachine, mainOpStatus and smInternal
 * \param result - result of CheckEcho/CheckCmd
 * \param smInt - reference of smInternal
 * \return none
 */
void CheckErr(int result, BYTE* smInt, DWORD* tickUpdate)
{
	switch(result)
	{
		case 0:
			// Operation Success
			mainGSMStateMachine = SM_GSM_IDLE;
			(*smInt)++;
			mainOpStatus.ExecStat = OP_EXECUTION;
			mainOpStatus.ErrorCode = 0;
			break;
		
		case 1:
			// ERROR
			mainGSMStateMachine = SM_GSM_IDLE;
			*smInt = 0;
			mainOpStatus.ExecStat = OP_SYNTAX_ERR;
			mainOpStatus.ErrorCode = 0;
			break;
			
		case 2:
			// +CMS
			mainGSMStateMachine = SM_GSM_IDLE;
			*smInt = 0;
			mainOpStatus.ExecStat = OP_CMS_ERR;
			break;
			
		case 3:
			// +CME
			mainGSMStateMachine = SM_GSM_IDLE;
			*smInt = 0;
			mainOpStatus.ExecStat = OP_CME_ERR;
			break;
			
		case 4:
			// NO CARRIER
			mainGSMStateMachine = SM_GSM_IDLE;
			*smInt = 0;
			mainOpStatus.ExecStat = OP_NO_CARR_ERR;
			break;
					
		case -1:
			// Need GSMUnsol test...
			mainGSMStateMachine = SM_GSM_CMD_PENDING;
			mainOpStatus.ExecStat = OP_EXECUTION;
			mainOpStatus.ErrorCode = 0;
			*tickUpdate = TickGetDiv64K();
			break;
		
		case -2:
			gsmHwFaultCounter++;
			if(gsmHwFaultCounter > HW_FAULT_COUNTER_MAX)
			{
				gsmHwFaultCounter = 0;
				// Timeout
				mainGSMStateMachine = SM_GSM_HW_FAULT;
				*smInt = 0;
				mainOpStatus.ExecStat = OP_TIMEOUT;
				mainOpStatus.ErrorCode = 0;
			}
			else
			{
				mainGSMStateMachine = SM_GSM_CMD_PENDING;
				mainOpStatus.ExecStat = OP_EXECUTION;
				mainOpStatus.ErrorCode = 0;
				*tickUpdate = TickGetDiv64K();
			}
			break;
	}
}

#endif
