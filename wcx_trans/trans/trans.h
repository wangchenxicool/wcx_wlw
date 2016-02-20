#ifndef _TRANS_H
#define _TRANS_H

#define RTU_PORT        9527
#define CLIENT_PORT     9528

#define  ERROR_LOG_FILE    "/var/log/trans_err.log"
#define  RUN_LOG_FILE      "/var/log/trans_run.log"
#define  DEBUG_MODE        0X01    // 打印调试信息

/* IEC104 struct */
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef void  VOID;
typedef bool  BOOL;

#define C104_APDU_SATRT     0x68
#define C104_APDU_LEN       253
#define C104_APDU_SIZE      C104_APDU_LEN + 2
#define C104_ASDU_SIZE      (C104_APDU_LEN - 4)
#define C104_DATA_SIZE      (C104_ASDU_SIZE - 9)

/* 104报文帧格式描述 */
typedef struct tagT_C104_FRAME
{
    BYTE	start;			/* 启动字符 */
    BYTE	len;			/* APDU长度   */
    BYTE	c1;     		/* 控制域 */
    BYTE	c2;	        	/* 控制域 */
    BYTE	c3;     		/* 控制域 */
    BYTE	c4;	        	/* 控制域 */

} T_C104_FRAME;

/* 104应用服务数据单元(ASDU)格式 */
typedef struct tagT_C104_ASDU
{
    BYTE	type;       			/* 类型标识 */
    BYTE	vsq;	        		/* 可变结构限定词 */
    BYTE	cot_low;	        	/* 传送原因 */
    BYTE	cot_high;
    BYTE	addr_asdu_low;			/* 公共地址 */
    BYTE	addr_asdu_high;
    BYTE	data[C104_DATA_SIZE];	/* 信息体报文 */

} T_C104_ASDU;


/* 用户登录相关结构 */
#define TYPE_LOG_IN      108

typedef struct
{
    char user_name[20];
    char passwd[20];
    int sn;

} T_LOG_IN_INF;


static inline int login (sqlite3 *db, const char *user, const char *passwd, int *sn);

#endif
