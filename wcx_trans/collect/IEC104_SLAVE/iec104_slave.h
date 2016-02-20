/*===================================================================================================

_________ _______  _______   __    _______     ___      _______  _        _______           _______
\__   __/(  ____ \(  ____ \ /  \  (  __   )   /   )    (  ____ \( \      (  ___  )|\     /|(  ____ \
   ) (   | (    \/| (    \/ \/) ) | (  )  |  / /) |    | (    \/| (      | (   ) || )   ( || (    \/
   | |   | (__    | |         | | | | /   | / (_) (_   | (_____ | |      | (___) || |   | || (__
   | |   |  __)   | |         | | | (/ /) |(____   _)  (_____  )| |      |  ___  |( (   ) )|  __)
   | |   | (      | |         | | |   / | |     ) (          ) || |      | (   ) | \ \_/ / | (
___) (___| (____/\| (____/\ __) (_|  (__) |     | |    /\____) || (____/\| )   ( |  \   /  | (____/\
\_______/(_______/(_______/ \____/(_______)     (_)    \_______)(_______/|/     \|   \_/   (_______/

===================================================================================================*/
#ifndef _IEC104_SLAVE_H
#define _IEC104_SLAVE_H

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <termios.h>
#include <sys/timeb.h>
#include <event.h>

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

#define CONF_FILE                "/etc/iec104_slave.conf"
#define IEC104SLAVE_RCV_LOG      "/var/log/iec104slave_rcv.log"
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* err code */
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#define INVALID_DATA            -0x10
#define INVALID_EXCEPTION_CODE  -0x12

#define SELECT_TIMEOUT          -0x13
#define SELECT_FAILURE          -0x14
#define SOCKET_FAILURE          -0x15
#define CONNECTION_CLOSED       -0x16
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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
#define C_CS_NA                103
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


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
typedef struct tagST_C104_FRAME
{
    BYTE	start;			/* 启动字符 */
    BYTE	len;			/* APDU长度   */
    BYTE	c1;     		/* 控制域 */
    BYTE	c2;	        	/* 控制域 */
    BYTE	c3;     		/* 控制域 */
    BYTE	c4;	        	/* 控制域 */

} ST_C104_FRAME;

/* 104应用服务数据单元(ASDU)格式 */
typedef struct tagST_C104_ASDU
{
    BYTE	type;       			/* 类型标识 */
    BYTE	vsq;	        		/* 可变结构限定词 */
    BYTE	cot_low;	        	/* 传送原因 */
    BYTE	cot_high;
    BYTE	addr_asdu_low;			/* 公共地址 */
    BYTE	addr_asdu_high;
    BYTE	data[C104_DATA_SIZE];	/* 信息体报文 */

} ST_C104_ASDU;

struct IEC104_SLAVE
{
    int             fd;
    char            IpAddr_Client[50];

    volatile int    is_connect; //连接标志 TRUE: 已经连接 FALSE: 未连接
    volatile char   is_idlesse; //IEC104通道空闲标志, 如果置1,将定时发送测试帧
    volatile char   is_send_sframe; //S帧发送标志

    struct event    ev_read;
    struct event    ev_write;
    struct event    ev_timer_tframe; //发送测试帧事件
    struct event    ev_timer_sframe; //发送S帧事件

    struct timeval  tv_timer_tframe; //测试帧定时器
    struct timeval  tv_timer_sframe; //S帧定时器

    unsigned char   RecvBuf[C104_APDU_SIZE];
    volatile unsigned short  RecvLen;  //已经接收到的数据长度
    unsigned char   SendBuf[C104_APDU_SIZE];
    volatile unsigned short  SendLen;  //已经发送了的数据长度

    volatile unsigned short  RecvNum;  //已经接收到的帧
    volatile unsigned short  SendNum;  //已经发送出的帧

    volatile unsigned char   k; //发送I格式应用规约数据单元的未认可帧数
    volatile unsigned char   w; //接收I格式应用规约数据单元的帧数

};
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


/* IEC104 function */
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static void iec104_error_treat (int code, const char *string, void *arg);
static void Display (struct IEC104_SLAVE *IEC104);
static void on_read (int fd, short ev, void *arg);
static void on_write (int fd, short ev, void *arg);
static void on_timer (int fd, short ev, void *arg);
static void on_timer_tframe (int fd, short event, void *arg);
static void on_timer_sframe (int fd, short event, void *arg);
static void IEC104SLAVE_AssociateAck (struct IEC104_SLAVE *IEC104);
static void IEC104SLAVE_M_EI_NA (struct IEC104_SLAVE *IEC104);
static void IEC104SLAVE_TestAct (struct IEC104_SLAVE *IEC104);
static void IEC104SLAVE_TestAck (struct IEC104_SLAVE *IEC104);
static void IEC104SLAVE_Sframe (struct IEC104_SLAVE *IEC104);

void IEC104SLAVE_C_DC_NA_PreAck (struct IEC104_SLAVE *IEC104);
void IEC104SLAVE_C_DC_NA_ExeAck (struct IEC104_SLAVE *IEC104);
void IEC104SLAVE_C_DC_NA_TermAck (struct IEC104_SLAVE *IEC104);
void IEC104SLAVE_C_SE_NA_OkAck (struct IEC104_SLAVE *IEC104);
void IEC104SLAVE_C_SE_NA_NoAck (struct IEC104_SLAVE *IEC104);
void IEC104SLAVE_C_SE_NA_EndAck (struct IEC104_SLAVE *IEC104);
int IEC104SLAVE_Run (void *arg);
int IEC104SLAVE_Exit (void *arg);
int IEC104SLAVE_Display (void *arg);

#ifdef __cplusplus
}
#endif
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#endif
