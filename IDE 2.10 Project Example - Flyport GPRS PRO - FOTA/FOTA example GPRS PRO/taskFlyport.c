#include "taskFlyport.h"
#include "FWUpdate_GPRS_PRO.h"

#define FTP_SERV_NAME	"put.here.your.ftp.server"
#define FTP_USER_NAME	"username"
#define FTP_PASSWORD	"password"
#define FTP_PORT		21
#define APN_SERV_NAME	"put.here.your.apn.server"

//#define _FW_1_FOTA_GPRS_
#define _FW_2_FOTA_GPRS_

int i = 0;
char inBuff[250];
BOOL toProcess = FALSE;
BOOL toExecute = FALSE;
BOOL newConnMsg = TRUE;
int connStatOld = 0;
DWORD tickRef;
unsigned long int readSize;

typedef	enum {
	APN_SETUP = 0,
	GSM_NET_UP = 1,
	FOTA_INIT = 2,
	FOTA_EXECUTE = 3,
    FILE_COPY = 4,
	FILE_DELETE = 5,
    MD5_READ = 6,
	MD5_CALCULATION = 7,
	FLASH_ERASE_SECTORS
}ActionStateMachine;
ActionStateMachine actionState = APN_SETUP;

void FlyportTask()
{
	vTaskDelay(20);
    _dbgwrite("Flyport Task Started...\r\n");
	IOInit(p31, out);
	IOPut(p31, off);
    
	vTaskSuspendAll();
#ifdef _FW_1_FOTA_GPRS_
    _dbgline("\r\n\tFlyport GPRS PRO FOTA Example FW1\r\n");
#else
    _dbgline("\r\n\tFlyport GPRS PRO FOTA Example FW2\r\n");
#endif
	xTaskResumeAll();

	// Erase Flash for FOTA execution...
	FWUpdateInit_GprsPro();

	tickRef = TickGetDiv256(); 			// Tick used for LED blink

	_dbgline("Starting FOTA example...");
	vTaskDelay(20);  
    
    while(1)
    {
		/// USER APPLICATION : BLINK LED!
		if(( TickGetDiv256() - tickRef) > 125) //(500ms /4) = 125 ms
		{
			IOPut(p31, toggle);
			tickRef = TickGetDiv256();
		}
		
		if(toProcess != TRUE) // No more Action is pending (to be executed...)
			toExecute = TRUE;
		
		// In case of TIMEOUT, Hilo Module is reset on GSM Task.
        // MUST be sure to reset Application state machine too:
        if(toExecute == TRUE)
        {
            if(LastExecStat() == OP_TIMEOUT)
            {
                if(actionState > APN_SETUP)
                    actionState = APN_SETUP;
            }
        }
		
		if((toExecute == TRUE) && (LastExecStat() != OP_TIMEOUT))
		{
			//	GSM COMMANDS STATE MACHINE
			switch(actionState)
			{
				case APN_SETUP: // Setup APN
                    _dbgwrite("Setting up APN...\r\n");
					APNConfig(APN_SERV_NAME,"","",DYNAMIC_IP,DYNAMIC_IP,DYNAMIC_IP);
					toExecute = FALSE;
					toProcess = TRUE;
					break;
                    
				case GSM_NET_UP: // Check GSM Connection
					if(newConnMsg)
					{
						_dbgwrite("Updating connection status...\r\n");
						newConnMsg = FALSE;
					}
					UpdateConnStatus();
					toExecute = FALSE;
					toProcess = TRUE;
					break;
					
				case FOTA_INIT:
					_dbgline("Configuring FTP");
					FTPConfig(&ftpSocket, FTP_SERV_NAME, FTP_USER_NAME, FTP_PASSWORD, FTP_PORT);
					while(LastExecStat() == OP_EXECUTION)
						vTaskDelay(20);
					// Choose next step:
					if(LastExecStat() != OP_SUCCESS)
					{
						actionState = GSM_NET_UP; // Repeat from case GSM_NET_UP
					}
					else
					{
						actionState = FOTA_EXECUTE; // execute next step
					}
					break;

                case FOTA_EXECUTE:
                    if(fileCnt < 9)
                        sprintf(fileName, "fota.bin.%d", fileCnt);
					_dbgwrite("Receiving file from FTP: ");
					
#ifdef _FW_1_FOTA_GPRS_
					_dbgwrite("/FOTA_GPRS2/");
					_dbgline(fileName);
					FTPReceive(&ftpSocket, fileName, "/FOTA_GPRS2", fileName);
#else
					_dbgwrite("/FOTA_GPRS1/");
					_dbgline(fileName);
					FTPReceive(&ftpSocket, fileName, "/FOTA_GPRS1", fileName);
#endif
					
                    _dbgwrite("Waiting for file download...");
					toExecute = FALSE;
					toProcess = TRUE;
					break;

                case FILE_COPY:
					_dbgline("Read FTP Data...");
                    __eds__ BYTE* fileEdsPointer = fileDataEds;
					readSize = EDS_FILE_SIZE;
					if(fileCnt == 8) // Last file...
					{
						readSize = FW_LAST_SIZE + MD5_SIZE; // last file has a shorter size, so change it!
					}
					
					FSReadEDS(fileName, fileEdsPointer, readSize);
					
					while(LastExecStat() == OP_EXECUTION)
						vTaskDelay(20);
					// Choose next step:
					if(LastExecStat() != OP_SUCCESS)
					{
						_dbgline("no data read or size do not match!\r\nFlyport will try again soon...");
						actionState = GSM_NET_UP;
					}
					else
					{
						_dbgline("Read Completed!");
                        _dbgline("Writing on Flash...");
                        unsigned long int a = 0;
						flash_ind = FLASH_START_ADD;
                        flash_ind += FW_SIZE*(unsigned long int)(fileCnt);
						long int remainingBytes = readSize - MD5_SIZE;
						
                        // write all data that complete a PAGE_SIZE write:
                        do
                        {
							vTaskSuspendAll();
                            SPIFlashBeginWrite(flash_ind);
                            for(i = 0; i < PAGE_SIZE; i++)
                            {
                                flashDataW[i] = fileDataEds[a];
                                a++;
                            }
                            SPIFlashWriteArray(flashDataW, PAGE_SIZE);
							xTaskResumeAll();
							flash_ind += PAGE_SIZE;
							remainingBytes -= PAGE_SIZE;
                        } while(remainingBytes >= PAGE_SIZE);
						
                        if(remainingBytes > 0)
						{
							vTaskSuspendAll();
							SPIFlashBeginWrite(flash_ind);
							for(i = 0; i < remainingBytes; i++)
							{
								flashDataW[i] = fileDataEds[a];
								a++;
							}
							SPIFlashWriteArray(flashDataW, remainingBytes);
							xTaskResumeAll();
						}
						
						// Check for errors if some errors changed current state machine value:
						if(actionState != FILE_COPY)
							break;
						
                        actionState = FILE_DELETE;
					}
					break;
					
				case FILE_DELETE:
					FSDelete(fileName);
					while(LastExecStat() == OP_EXECUTION)
						vTaskDelay(20);
					// Choose next step:
					if(LastExecStat() != OP_SUCCESS)
					{
						_dbgline("File not deleted!");
						actionState = GSM_NET_UP;
					}
					else
					{
						_dbgline("File deleted!");
						
						actionState = MD5_READ;
					}
					break;
                    
                case MD5_READ:
					// Read Last 16 BYTEs from fota file RAM content:
					readSize = FW_SIZE;
					if(fileCnt == 8) // Last file...
					{
						readSize = FW_LAST_SIZE; // change size for last file
					}
					
					for(i = 0; i < 16; i++)
					{
						md5fileContent[i] = fileDataEds[readSize+i];
					}
					actionState = MD5_CALCULATION;
                    break;
					
				case MD5_CALCULATION:
					_dbgline("MD5 Integrity Check");
                    if(MD5IntegrityCheck(md5fileContent, fileCnt) == 0)
                    {
                        _dbgline("FOTA file correctly downloaded!");
						
						if(fileCnt == 8) // Last file downloaded
						{
							_dbgwrite("New valid firmware found: ");
							if(FWNewEnable_GprsPro()==TRUE)
							{
								_dbgline("Flyport will be restarted with new firmware");
								vTaskDelay(100);
								Reset();
							}
						}
						else
						{
							_dbgline("MD5 Verified... start downloading next file!");
							// Increase fileCnt
							fileCnt++;
						}
                    }
                    else
                    {
                        _dbgline("Corrupted MD5 from FOTA files... Flyport will try again!");
						// Don't increase fileCnt, so we try again with same file...
						actionState = FLASH_ERASE_SECTORS;
						break;
                    }
			
					// Set flash_ind to new position and start a new file downloading
					flash_ind = FLASH_START_ADD + FW_SIZE*fileCnt;
					actionState = FOTA_INIT;
					break;
					
				case FLASH_ERASE_SECTORS:
					if(fileCnt < 9)
					{
						DWORD fl_ind;
						sprintf(inBuff, "Erasing flash sectors related to file %s...", fileName);
						_dbgwrite(inBuff);
						for (fl_ind = FLASH_START_ADD + ((DWORD)FW_SIZE*(DWORD)fileCnt); fl_ind < FLASH_START_ADD + ((DWORD)FW_SIZE*((DWORD)fileCnt+1)); fl_ind += (DWORD)PAGE_SIZE)
						{
							vTaskSuspendAll();
							SPIFlashEraseSector(fl_ind);
							xTaskResumeAll();
						}
						_dbgline("Done!");
						actionState = FOTA_INIT;
					}
					else
						actionState = GSM_NET_UP;
					
					break;
					
				default:
					break;
			}
		}
		
		// GSM COMMANDS RESULT PROCESS
		if(( LastExecStat() != OP_EXECUTION) && (toProcess == TRUE))
		{
			switch(actionState)
			{
				case APN_SETUP: // Setup APN
					// This instruction checks the result of operation
					if(LastExecStat() == OP_SUCCESS)
					{
						_dbgwrite("APN correctly set!\n");
						toProcess = FALSE;
						toExecute = TRUE;
						newConnMsg = TRUE;
						actionState = GSM_NET_UP;
					}
					else
					{
						_dbgwrite("Problems on APNConfig...\n");
						toProcess = FALSE;
						toExecute = TRUE;
						actionState = APN_SETUP; // Try again!
					}
					break;

				case GSM_NET_UP: // Check GSM Connection
					if(LastExecStat() == OP_SUCCESS)
					{
						if( connStatOld != LastConnStatus())
						{
							newConnMsg = FALSE;
							connStatOld = LastConnStatus();
							sprintf(inBuff, "Connection Status: %d\r\n", LastConnStatus());
							_dbgwrite(inBuff);
						}
						toProcess = FALSE;
						toExecute = TRUE;
						if((LastConnStatus() == REG_SUCCESS) || (LastConnStatus() == ROAMING))
						{
							actionState = FOTA_INIT;
						}
						else
						{
							actionState = GSM_NET_UP; // try Again...
                            vTaskDelay(150);
						}
					}
					else
					{
						_dbgwrite("Problems on UpdateConnStatus...\n");
						toProcess = FALSE;
						toExecute = TRUE;
						actionState = GSM_NET_UP; // Try again!
					}
					break;
					
				case FOTA_EXECUTE:
					if(LastExecStat() == OP_SUCCESS)
					{
						_dbgline("Completed!");
                        actionState++;
						toProcess = FALSE;
						toExecute = TRUE;
                    }
                    else
                    {
						vTaskDelay(50);
                        _dbgwrite("Problems during FTP file transfer, try again soon...\r\n");
						toProcess = FALSE;
						toExecute = TRUE;
						actionState = GSM_NET_UP; // Try again!
                    }
					break;
				
				default:
					break;
			}
		}
	}
}
