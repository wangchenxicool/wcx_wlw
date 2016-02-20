#ifndef __SYSTEM__
#define __SYSTEM__

#include "typedef.h"

/* 系统定义 */
#define SYS_NUCLEUS_TICK	5   //5ms/tick(time slice)

#define SYS_MAX_IED	 100	//IED总数目
#define SYS_IED_NIL	 0		//无效IED编号
#define SYS_IED_MIN	 1      //IED最小编号
#define SYS_IED_RSV  100	//保留,用作虚拟设备(本系统)
#define SYS_IED_ANY	 0xFFFF //IED广播编号

#define SYS_ASSERT_IED(id) ((id >= SYS_IED_MIN) && (id <= SYS_MAX_IED))

#define SYS_MAX_CHANNEL 40
#define SYS_MAX_DI		64
#define SYS_MAX_SFC		32
#define SYS_MAX_SET     120
#define SYS_MAX_MEASURE 40
#define SYS_MAX_PULSE	40
#define SYS_MAX_RCHANNEL 40
#define SYS_MAX_CHANGE	100

/* 系统日期 */
typedef struct tagT_DATE
{
	WORD year;
	BYTE month;
	BYTE day;
	BYTE week;
	BYTE hour;
	BYTE minute;
	WORD msec;
} T_DATE;

/* 闰年判断 */
#define IS_LEAP_YEAR(year) ((year%400 == 0) || ((year%4 == 0) && (year%100 != 0)))

/* 软件版本信息 */
typedef struct tagTSOFTINFO
{
	char name[16];
	WORD wType;
	WORD wVersion;
	WORD wCRC;
} TSOFTINFO;

/* 通信口类型 */
#define	TY_PORT	0
#define	UDP_CLI	1
#define	UDP_SVR 2
#define	TCP_CLI 3
#define	TCP_SVR 4

typedef struct tag_SYS_MON_COM
{
    BYTE	CommType;	
    BYTE	Port_Param[6];		/* IP address and port number in UDP/TCP. */
    BYTE	PreTx_SN;			/* SN of the previous Tx in the pipe. */
    BYTE	CurTx_SN;			/* SN of current Tx message in the pipe. */
    BYTE	PreRx_SN;			/* SN of the previous Rx in the pipe. */
    BYTE	CurRx_SN;			/* SN of current Rx message in the pipe. */
								/* Format of message in pipe:
								* Byte0=Tx/Rx flag. 'T'=Tx, 'R'=Rx.
								* Byte1=SN to indicate if a message is lost.
								* Byte2-n=Trx message in HEX code. */
    //NU_PIPE	pipMsg;
    BYTE	*pmonPipe;
    WORD	MaxLen;
    struct	tag_SYS_MON_COM	*pmonNext;
} SYS_MON_COM;

/* 系统配置 */
typedef struct tagT_SYS_CONFIG
{
	unsigned char	net1_ip[4];
	unsigned char	net1_mask[4];
	unsigned char	net2_ip[4];
	unsigned char	net2_mask[4];
	unsigned char	net3_ip[4];
	unsigned char	net3_mask[4];
}	T_SYS_CONFIG;

#endif
