#include "taskFlyport.h"
#include "libpic30.h"
#include "Hashes.h"
#include "Helpers.h"

#ifndef FWUPDATE_GPRS_PRO_H
#define FWUPDATE_GPRS_PRO_H

#define PAGE_SIZE		4096    // 0x1000
#define FW_SIZE			28672   // 4096 * 7 => 0x7000
#define FW_LAST_SIZE	27136   // 0x6a00
#define MD5_SIZE		16
#define EDS_FILE_SIZE	(FW_SIZE+MD5_SIZE) // 28688 => 0x7010
#define FLASH_START_ADD 0x1C0000UL

extern BYTE flashDataW[PAGE_SIZE+1];
extern __eds__ BYTE fileDataEds[EDS_FILE_SIZE+1] __attribute__((space(eds)));
extern unsigned long int flash_ind;
extern char fileName[50];
extern BYTE fileCnt;
extern BYTE md5fileContent[MD5_SIZE+1];
extern HASH_SUM myHash;
extern FTP_SOCKET ftpSocket;

void FWUpdateInit_GprsPro();
BYTE MD5IntegrityCheck(BYTE* buffer, BYTE fileCount);
BOOL FWNewEnable_GprsPro();
void FOTAEraseFlash();

#endif // FWUPDATE_GPRS_PRO_H
