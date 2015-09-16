//#ifndef __TASKFLY_H
//#define __TASKFLY_H
#include "HWlib.h"
#include "INTlib.h"
#include "string.h"
#include "RTCClib.h"
#include "HILOlib.h"
#include "SMSlib.h"
#include "CALLlib.h"
#include "LowLevelLib.h"
#include "DATAlib.h"
#include "TCPlib.h"
#include "HTTPlib.h"
#include "SMTPlib.h"
#include "FTPlib.h"
#include "FSlib.h"

#include "SPIFlash.h"

//	RTOS components - Semaphore and queues
extern xQueueHandle xQueue;
extern xSemaphoreHandle xSemFrontEnd;
extern xSemaphoreHandle xSemHW;

//	FrontEnd variables
extern int xFrontEndStat;
extern int xErr;

void FlyportTask();
//#endif	// #ifndef __TASKFLY_
