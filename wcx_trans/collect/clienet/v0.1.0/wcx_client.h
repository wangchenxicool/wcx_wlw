#ifndef _IEC104_MASTER_H
#define _IEC104_MASTER_H

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#if defined(__FreeBSD__ ) && __FreeBSD__ < 5
#include <netinet/in_systm.h>
#endif
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <termios.h>
#include <sys/timeb.h>

/* Libevent. */
#include <event.h>

#include <vector>
#include <queue>
#include <iostream>

/* */
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef OFF
#define OFF 0
#endif
#ifndef ON
#define ON 1
#endif

#define DBSERVER_PORT     9528

#define IEC104_SLAVE_IP_NUMS_MAX              256
#define CONNECT_CHECK_TIME      5
#define IEC104MASTER_LOG        "/var/log/IEC104_MASTER.log"
#define IEC104MASTER_RCV_LOG    "/var/log/iec104master_rcv.log"
#define IEC104MASTER_CONF_FILE  "/etc/iec104_master.conf"
#define CSV_FILE_PATH           "/var/log"
#define TAG_FILE                "/mnt/disk/mnt/disk/ort3000c/collect/bin/sysdb.db"
#define TAG_FILE_OPTIMIZED      "/mnt/disk/mnt/disk/ort3000c/collect/bin/tags/optimized_tag_iec104_master.db"
#define MAX_DATA_LEN            512
#define MaxListDsp              20
#define TIME_OUT_WRITEDB        2

/* err code */
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#define INVALID_DATA            -0x10
#define INVALID_CRC             -0x11
#define INVALID_EXCEPTION_CODE  -0x12

#define SELECT_TIMEOUT          -0x13
#define SELECT_FAILURE          -0x14
#define SOCKET_FAILURE          -0x15
#define CONNECTION_CLOSED       -0x16
#define MB_EXCEPTION            -0x17

/* IEC104 asdu_type */
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#define  M_SP_NA               1
#define  M_DP_NA               3
#define  M_ST_NA               5
#define  M_BO_NA               7
#define  M_ME_NA               9
#define  M_ME_NB               11
#define  M_ME_NC               13
#define  M_IT_NA               15
#define  M_PS_NA               20
#define  M_ME_ND               21
#define  M_EI_NA               70

#define  M_SP_TB               30
#define  M_DP_TB               31
#define  M_ST_TB               32
#define  M_BO_TB               33
#define  M_ME_TD               34
#define  M_ME_TE               35
#define  M_ME_TF               36
#define  M_IT_TB               37
#define  M_EP_TD               38
#define  M_EP_TE               39
#define  M_EP_TF               40

#define C_SC_NA                45
#define C_DC_NA                46
#define C_SE_NA                48
#define C_IC_NA                100
#define C_CI_NA                101
#define C_CS_NA                103

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

typedef struct _iec104_tag_tp
{
    uint16_t tag_id;
    uint8_t data_type;
    uint32_t info_addr;
    uint8_t operate;
    float k;

} iec104_tag_tp;

struct IEC104_MASTER
{
    int             i;
    int             did;

    int             fd;
    char            ip_addr[250];
    int             port;
    int             asdu_addr;

    int     info_addr_status;
    int     info_addr_coils;
    int     info_addr_analog;
    int     info_addr_analog_argument;
    int     info_addr_control;
    int     info_addr_agc;
    int     info_addr_energy;
    int     info_addr_position;

    int     bits_cos;
    int     bits_asdu_addrs;
    int     bits_info_addrs;

    volatile int    is_connect; //连接标志 TRUE: 已经连接 FALSE: 未连接
    volatile char   is_idlesse; //IEC104通道空闲标志, 如果置1,将定时发送测试帧
    volatile char   is_send_sframe; //S帧发送标志

    struct event    ev_read;
    struct event    ev_write;
    struct event    ev_timer_tframe; //发送测试帧事件
    struct event    ev_timer_sframe; //发送S帧事件
    struct event    ev_timer_C_IC_NA; //总召唤事件
    struct event    ev_timer_C_CI_NA; //电度总召唤事件

    struct timeval  tv_timer_tframe; //测试帧定时器
    struct timeval  tv_timer_sframe; //S帧定时器
    struct timeval  tv_timer_C_IC_NA; //总召唤定时器
    struct timeval  tv_timer_C_CI_NA; //电度总召唤定时器

    unsigned char   TimeOut_t0;  //连接建立超时值,单位s
    unsigned char   TimeOut_t1;  //APDU的发送或测试的超时时间,s
    unsigned char   TimeOut_t2;  //无数据报文t2<t1情况下认可的超时时间,s
    unsigned char   TimeOut_t3;  //茌长时间Idle状态t3>t1情况下发送S-帧的超时时间,s
    unsigned char   TimeOut_t4;  //????
    unsigned char   TimeOut_C_IC_NA;  //总召唤请求周期
    unsigned char   TimeOut_C_CI_NA;  //电度总召唤请求周期
    unsigned char   TimeOut_zu;  //组召唤周期
    unsigned char   TimeOut_clock;  //时钟同步周期

    volatile unsigned short  RecvLen;  //已经接收到的数据长度
    unsigned char   RecvBuf[MAX_DATA_LEN];
    volatile unsigned short  SendLen;  //已经发送了的数据长度
    unsigned char   SendBuf[MAX_DATA_LEN];

    volatile unsigned short  RecvNum;  //已经接收到的帧
    volatile unsigned short  SendNum;  //已经发送出的帧

    volatile unsigned char   k;             //发送I格式应用规约数据单元的未认可帧数
    volatile unsigned char   w;             //接收I格式应用规约数据单元的帧数
    unsigned char   Max_k;       //发送状态变量的最大不同的接收序号
    unsigned char   Max_w;       //接收w个I格式APDUs之后的最后的认可

    std::vector<iec104_tag_tp> tag;

};
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


/* WRITE_REAL_VAL_TAG */
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#define REAL_VAL_TAG_SIZE      14  // 3字节信息体地址 + 4字节数据 + 7字节时间
#define REAL_VAL_TAG_COUNT     (C104_DATA_SIZE / REAL_VAL_TAG_SIZE)  // 一个IEC104帧能传17个LongData
//#define REAL_VAL_TAG_COUNT     2
typedef struct
{
    BYTE    info_l;
    BYTE    info_m;
    BYTE    info_h;
    BYTE    val_0;
    BYTE    val_1;
    BYTE    val_2;
    BYTE    val_3;
    BYTE    sec_l; /* 秒-低8位 */
    BYTE    sec_h; /* 秒-高8位 */
    BYTE    min; /* 分钟 */
    BYTE    hour; /* 小时 */
    BYTE    week_day; /* 星期-日 */
    BYTE    month; /* 月 */
    BYTE    year; /* 年 */

} REAL_VAL_TAG;

/* 用户登录相关结构 */
#define TYPE_LOG_IN      108

typedef struct
{
    char user_name[20];
    char passwd[20];

} T_LOG_IN_INF;


/* IEC104 function */
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static void iec104_master_error_treat (int code, const char *string, void *arg);
static void IEC104MASTER_Display (struct IEC104_MASTER *IEC104);
static int  Socket (int i);
static int  Connect (int i);
static void iec104_master_connect();
static void iec104_send_msg (int i);
static void on_read (int fd, short ev, void *arg);
static void on_write (int fd, short ev, void *arg);
static void on_timer (int fd, short ev, void *arg);
static void on_timer_tframe (int fd, short event, void *arg);
static void on_timer_sframe (int fd, short event, void *arg);
static void on_timer_C_IC_NA (int fd, short event, void *arg);
static void on_timer_C_CI_NA (int fd, short event, void *arg);
static void IEC104_Master_sAssociateAct (int i);
static void IEC104_Master_LOGIN (int i);
static void IEC104_Master_sAssociateAck (int i);
static void IEC104_Master_sTestAct (int i);
static void IEC104_Master_sTestAck (int i);
static void IEC104_Master_sSFrame (int i);
static inline uint16_t GetPid (struct IEC104_MASTER *IEC104, uint32_t info_addr, float *K);

int IEC104MASTER_Run (void *arg);
int IEC104MASTER_Exit (void *arg);
int IEC104_Master_Display (void *arg);

void IEC104_Master_sC_IC_NA (int i, int coa, uint32_t info_addr, int qoi);
void IEC104_Master_sC_CI_NA (int i, int coa, uint32_t info_addr, int qoi);
void IEC104_Master_sC_CS_NA (int i);
void IEC104_Master_sC_DC_NA_PreSet (int i, int coa, uint32_t info_addr, unsigned char dco);
void IEC104_Master_sC_DC_NA_Execute (int i, int coa, uint32_t info_addr, unsigned char co);
void IEC104_Master_sC_DC_NA_Term (int i, int coa, uint32_t info_addr, unsigned char co);

#ifdef __cplusplus
}
#endif
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#endif
