#ifndef _TRANS_H
#define _TRANS_H

#define RTU_PORT        9527
#define CLIENT_PORT     9528

#define  ERROR_LOG_FILE    "/var/log/trans_err.log"
#define  RUN_LOG_FILE      "/var/log/trans_run.log"
#define  DEBUG_MODE        0X01    // ��ӡ������Ϣ

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


/* �û���¼��ؽṹ */
#define TYPE_LOG_IN      108

typedef struct
{
    char user_name[20];
    char passwd[20];
    int sn;

} T_LOG_IN_INF;


static inline int login (sqlite3 *db, const char *user, const char *passwd, int *sn);

#endif
