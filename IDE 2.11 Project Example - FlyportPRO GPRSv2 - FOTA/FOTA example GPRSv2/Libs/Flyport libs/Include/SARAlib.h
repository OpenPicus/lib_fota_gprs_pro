#ifndef _SARALIB_H
#define _SARALIB_H


// GSM Registration Status
#define		NO_REG			0
#define		REG_SUCCESS		1
#define		SEARCHING		2
#define		REG_DENIED		3
#define		UNKOWN			4
#define		ROAMING			5

// Event Type Defines
#define		NO_EVENT		0
#define		ON_RING			1
#define		ON_NO_CARRIER	2
#define		ON_SMS_SENT		3
#define		ON_SMS_REC		4
#define		ON_ERROR		5
#define		ON_REG			6
#define		ON_BUSY			7
#define		ON_SOCK_CLOSE	8

// ErrorHandler error type definitions
#define		NO_ERR			0
#define		IDLE_UNEXPECTED 1
#define		CMD_UNEXPECTED	2
#define		TIMEOUT_ERR		3
// Probably we received an UnsolMessage during Echo Check
#define         ECHO_EXPECTED_ERR       4
#define         ANSW_EXPECTED_ERR       5

// ----- OPERATIONS STATUS DEFINES AND VARIABLES -----

// Operation Status ExecStat values
#define		OP_SUCCESS		0
#define		OP_EXECUTION	-1
#define		OP_LL			-2
#define		OP_TIMEOUT		1  	// no reply
#define		OP_SYNTAX_ERR	2  	// reply: "ERROR"
#define 	OP_CMS_ERR		3 	// reply: "+CMS :..."
#define		OP_CME_ERR		4 	// reply: "+CME :..."
#define		OP_NO_CARR_ERR	5	// reply: "NO CARRIER" (for example TCP Server not reached...)
#define		OP_SMTP_ERR		6
#define		OP_FTP_ERR		7
#define		OP_HIB_ERR		8
#define     OP_LOW_POW_ERR  9


// Call Values Defines
#define		CALL_READY		0
#define		CALL_IN_PROG	1
#define		CALL_BUSY		2

// SMS Values Defines
// MemType
#define		SM_MEM			0
#define		ME_MEM			1

// TCP/HTTP socket <status> values
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


extern BYTE LastConnStatus();
extern int	LastExecStat();
extern int  LastErrorCode();
extern int  LastSignalRssi();

char* GSMGetIMEI();
char* GSMGetOperatorName();
BOOL  GSMGetGprsPacketServiceStatus();
char* GSMGetFwRevision();
char* GSMGetModelVersion();

int cGSMHibernate(void);
void GSMHibernate();

int cGSMLowPowerEnable(void);
void GSMLowPowerEnable();

void GSMSleep();

int cGSMOn(void);
void GSMOn();

int cUpdateConnStatus(void);
void UpdateConnStatus();

int cGSMSignalQualityUpdate(void);
void GSMSignalQualityUpdate();


#endif
