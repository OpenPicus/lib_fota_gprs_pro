#include "FWUpdate_GPRS_PRO.h"

BYTE flashDataW[PAGE_SIZE+1];
__eds__ BYTE fileDataEds[EDS_FILE_SIZE+1] __attribute__((space(eds)));
//static __eds__ BYTE* fPointer;
unsigned long int flash_ind = FLASH_START_ADD;
BYTE md5fileContent[MD5_SIZE+1];
char fileName[50];
BYTE fileCnt = 0;
HASH_SUM myHash;
FTP_SOCKET ftpSocket;

/**********************************************************************************************
 * 	void FWUpdateInit_GprsPro(): 
 *	The function simply initializes the external SPI flash memory.
 *						 
 * 	Parameters:
 *	-
 *
 * 	Returns:
 *	-
************************************************************************************************/
void FWUpdateInit_GprsPro()
{
	SPIFlashInit();
	flash_ind = FLASH_START_ADD;
	FOTAEraseFlash();
}

void FOTAEraseFlash()
{
    unsigned long int mem_ind;
	_dbgwrite("Erasing flash... ");
	for (mem_ind = FLASH_START_ADD; mem_ind < 0x1FFFFF; mem_ind += PAGE_SIZE)
	{
		vTaskSuspendAll();
		SPIFlashEraseSector(mem_ind);
		xTaskResumeAll();
	}
	_dbgwrite("Done!\r\n");
}

BOOL FWNewEnable_GprsPro()
{
	BOOL enable_low = FALSE, enable_high = FALSE;
	BYTE i,fread[1];
	//	Check on low word of reset vector
	for (i = 0; i < 3;i++)
	{
		vTaskSuspendAll();
		SPIFlashReadArray(FLASH_START_ADD + i, fread, 1);
		xTaskResumeAll();
		if (fread[0] != 0xFF)
			enable_low = TRUE;
	}
	//	Check on high word of reset vector
	for (i = 3; i < 6;i++)
	{
		vTaskSuspendAll();
		SPIFlashReadArray(FLASH_START_ADD + i, fread, 1);
		xTaskResumeAll();
		if (fread[0] != 0xFF)
			enable_high = TRUE;
	}
	if ( (enable_low == TRUE) && (enable_high == TRUE) )
	{
		vTaskSuspendAll();
		_erase_flash(0x29800);
		xTaskResumeAll();
		return TRUE;
	}
	else
		return FALSE;
}

BYTE MD5IntegrityCheck(BYTE* buffer, BYTE fileCount)
{
    BYTE operationRep = 0;
     
    //	MD5 INTEGRITY CHECK ON MEMORY
    BYTE resmd[16];
    HASH_SUM Hash;
    _dbgline("Calculating md5 from memory...");
    MD5Initialize(&Hash);
    long unsigned int f_ind = 0;
    BYTE b_read[2];
    unsigned long int md5_ind = FLASH_START_ADD;
    md5_ind += FW_SIZE*(unsigned long int)(fileCnt);
	unsigned long int stopSize = FW_SIZE;
	if(fileCount == 8) // Last file...
	{
		stopSize = (unsigned long int)FW_LAST_SIZE;
	}
	for (f_ind = 0; f_ind < stopSize; f_ind++)
	{
		vTaskSuspendAll();
		SPIFlashReadArray(md5_ind+f_ind, b_read, 1);
		xTaskResumeAll();
		HashAddData(&Hash, b_read, 1);
	}
	
    MD5Calculate(&Hash, resmd);
    BYTE i;
    char rr[3];

    _dbgline("MD5:");
    for (i=0; i<16; i++)
    {
        sprintf(rr,"%X ",resmd[i]);
        _dbgwrite(rr);
        if (resmd[i] != (BYTE) buffer[i])
        {
            operationRep = 1;
        }
    }
    return operationRep;
}
