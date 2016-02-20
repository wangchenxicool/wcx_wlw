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

/* 104����֡��ʽ���� */
typedef struct tagST_C104_FRAME
{
    BYTE	start;			/* �����ַ� */
    BYTE	len;			/* APDU����   */
    BYTE	c1;     		/* ������ */
    BYTE	c2;	        	/* ������ */
    BYTE	c3;     		/* ������ */
    BYTE	c4;	        	/* ������ */

} ST_C104_FRAME;

/* 104Ӧ�÷������ݵ�Ԫ(ASDU)��ʽ */
typedef struct tagST_C104_ASDU
{
    BYTE	type;       			/* ���ͱ�ʶ */
    BYTE	vsq;	        		/* �ɱ�ṹ�޶��� */
    BYTE	cot_low;	        	/* ����ԭ�� */
    BYTE	cot_high;
    BYTE	addr_asdu_low;			/* ������ַ */
    BYTE	addr_asdu_high;
    BYTE	data[C104_DATA_SIZE];	/* ��Ϣ�屨�� */

} ST_C104_ASDU;

struct IEC104_SLAVE
{
    int             fd;
    char            IpAddr_Client[50];

    volatile int    is_connect; //���ӱ�־ TRUE: �Ѿ����� FALSE: δ����
    volatile char   is_idlesse; //IEC104ͨ�����б�־, �����1,����ʱ���Ͳ���֡
    volatile char   is_send_sframe; //S֡���ͱ�־

    struct event    ev_read;
    struct event    ev_write;
    struct event    ev_timer_tframe; //���Ͳ���֡�¼�
    struct event    ev_timer_sframe; //����S֡�¼�

    struct timeval  tv_timer_tframe; //����֡��ʱ��
    struct timeval  tv_timer_sframe; //S֡��ʱ��

    unsigned char   RecvBuf[C104_APDU_SIZE];
    volatile unsigned short  RecvLen;  //�Ѿ����յ������ݳ���
    unsigned char   SendBuf[C104_APDU_SIZE];
    volatile unsigned short  SendLen;  //�Ѿ������˵����ݳ���

    volatile unsigned short  RecvNum;  //�Ѿ����յ���֡
    volatile unsigned short  SendNum;  //�Ѿ����ͳ���֡

    volatile unsigned char   k; //����I��ʽӦ�ù�Լ���ݵ�Ԫ��δ�Ͽ�֡��
    volatile unsigned char   w; //����I��ʽӦ�ù�Լ���ݵ�Ԫ��֡��

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
