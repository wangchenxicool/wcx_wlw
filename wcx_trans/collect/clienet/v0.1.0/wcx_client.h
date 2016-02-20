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

/* 104����֡��ʽ���� */
typedef struct tagT_C104_FRAME
{
    BYTE	start;			/* �����ַ� */
    BYTE	len;			/* APDU����   */
    BYTE	c1;     		/* ������ */
    BYTE	c2;	        	/* ������ */
    BYTE	c3;     		/* ������ */
    BYTE	c4;	        	/* ������ */

} T_C104_FRAME;

/* 104Ӧ�÷������ݵ�Ԫ(ASDU)��ʽ */
typedef struct tagT_C104_ASDU
{
    BYTE	type;       			/* ���ͱ�ʶ */
    BYTE	vsq;	        		/* �ɱ�ṹ�޶��� */
    BYTE	cot_low;	        	/* ����ԭ�� */
    BYTE	cot_high;
    BYTE	addr_asdu_low;			/* ������ַ */
    BYTE	addr_asdu_high;
    BYTE	data[C104_DATA_SIZE];	/* ��Ϣ�屨�� */

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

    volatile int    is_connect; //���ӱ�־ TRUE: �Ѿ����� FALSE: δ����
    volatile char   is_idlesse; //IEC104ͨ�����б�־, �����1,����ʱ���Ͳ���֡
    volatile char   is_send_sframe; //S֡���ͱ�־

    struct event    ev_read;
    struct event    ev_write;
    struct event    ev_timer_tframe; //���Ͳ���֡�¼�
    struct event    ev_timer_sframe; //����S֡�¼�
    struct event    ev_timer_C_IC_NA; //���ٻ��¼�
    struct event    ev_timer_C_CI_NA; //������ٻ��¼�

    struct timeval  tv_timer_tframe; //����֡��ʱ��
    struct timeval  tv_timer_sframe; //S֡��ʱ��
    struct timeval  tv_timer_C_IC_NA; //���ٻ���ʱ��
    struct timeval  tv_timer_C_CI_NA; //������ٻ���ʱ��

    unsigned char   TimeOut_t0;  //���ӽ�����ʱֵ,��λs
    unsigned char   TimeOut_t1;  //APDU�ķ��ͻ���Եĳ�ʱʱ��,s
    unsigned char   TimeOut_t2;  //�����ݱ���t2<t1������Ͽɵĳ�ʱʱ��,s
    unsigned char   TimeOut_t3;  //�ݳ�ʱ��Idle״̬t3>t1����·���S-֡�ĳ�ʱʱ��,s
    unsigned char   TimeOut_t4;  //????
    unsigned char   TimeOut_C_IC_NA;  //���ٻ���������
    unsigned char   TimeOut_C_CI_NA;  //������ٻ���������
    unsigned char   TimeOut_zu;  //���ٻ�����
    unsigned char   TimeOut_clock;  //ʱ��ͬ������

    volatile unsigned short  RecvLen;  //�Ѿ����յ������ݳ���
    unsigned char   RecvBuf[MAX_DATA_LEN];
    volatile unsigned short  SendLen;  //�Ѿ������˵����ݳ���
    unsigned char   SendBuf[MAX_DATA_LEN];

    volatile unsigned short  RecvNum;  //�Ѿ����յ���֡
    volatile unsigned short  SendNum;  //�Ѿ����ͳ���֡

    volatile unsigned char   k;             //����I��ʽӦ�ù�Լ���ݵ�Ԫ��δ�Ͽ�֡��
    volatile unsigned char   w;             //����I��ʽӦ�ù�Լ���ݵ�Ԫ��֡��
    unsigned char   Max_k;       //����״̬���������ͬ�Ľ������
    unsigned char   Max_w;       //����w��I��ʽAPDUs֮��������Ͽ�

    std::vector<iec104_tag_tp> tag;

};
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


/* WRITE_REAL_VAL_TAG */
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#define REAL_VAL_TAG_SIZE      14  // 3�ֽ���Ϣ���ַ + 4�ֽ����� + 7�ֽ�ʱ��
#define REAL_VAL_TAG_COUNT     (C104_DATA_SIZE / REAL_VAL_TAG_SIZE)  // һ��IEC104֡�ܴ�17��LongData
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
    BYTE    sec_l; /* ��-��8λ */
    BYTE    sec_h; /* ��-��8λ */
    BYTE    min; /* ���� */
    BYTE    hour; /* Сʱ */
    BYTE    week_day; /* ����-�� */
    BYTE    month; /* �� */
    BYTE    year; /* �� */

} REAL_VAL_TAG;

/* �û���¼��ؽṹ */
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
