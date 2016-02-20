#ifndef __X103__
#define __X103__
#include "typedef.h"
#include "nucleus.h"
#include "inforec.h"
#include "system.h"
#include "dev_mng.h"
/********************************************************/
/*														*/
/*	�ⲿ�ӿ�����										*/
/*														*/
/********************************************************/
/* 103�豸����ӿ� */
BOOL x103_service(T_IED *p_ied, T_MESSAGE *p_msg);

/* ��103�Ự */
BOOL x103_Open_Session(IPADDR x103_ip);

/* �ر�103�Ự */
BOOL x103_Close_Session(IPADDR x103_ip);

/* 103��վ��Э������� */
BOOL x103_Interpret(IPADDR x103_ip, const BYTE *p_data, WORD data_len);

/********************************************************/
/*														*/
/*	103�豸����ӿڼ��ڲ��ӿ�����						*/
/*														*/
/********************************************************/
/* 103�Ự��ʩ���� */
#define X103_ASDU_TIMEOUT	(10*SYS_SECOND_TICKS)
#define X103_ASDU_SIZE		0x600
typedef struct tagT_SESSION
{
	BOOL		 opened;		/* �豸ͨѶ�򿪱�־ */
	BYTE		 x103_sn;		/* ASDUɨ����� */
	NU_MAILBOX	 x103_mail;		/* �Ự��Ϣ������ʩ */
	NU_SEMAPHORE x103_sema;		/* �Ự��Դ������ʩ */
	WORD		 tx_len;		/* �Ự���ͻ����� */
	BYTE		 tx_buf[X103_ASDU_SIZE];
	WORD		 rx_len;		/* �Ự���ջ����� */
	BYTE		 rx_buf[X103_ASDU_SIZE];
}	T_SESSION;

/* 103�豸����(�ȼ���T_IED����) */
typedef struct tagT_103_ENTRY
{
	WORD	    x103_id;		/* ϵͳ��ʶ�� */
	T_SERVICE   x103_if;
	WORD		x103_type;
	WORD		x103_flag;
	IPADDR	    x103_ip;		/* ͨѶ��·���� */
	BYTE	    x103_addr;		/* 103ͨѶ������ַ */
	BYTE	    pad_0;
	T_SESSION  *x103_com;		/* �Ự��ʩ */
}	T_103_ENTRY;

/* 103Ӧ�÷������ݵ�Ԫ(ASDU)��ʽ */
typedef struct tagT_103_ASDU
{
	BYTE	type;
	BYTE	vsq;
	BYTE	cot;
	BYTE	addr;
	BYTE	fun;
	BYTE	inf;
	BYTE	data[X103_ASDU_SIZE-6];
}	T_103_ASDU;

/* 103����(��ϢԪ)��ʽ */
typedef struct tagT_103_DATA
{
	BYTE	ginh;
	BYTE	ginl;
	BYTE	kod;
	BYTE	type;
	BYTE	size;
	BYTE	num;
//	BYTE	data[16];	//modify by linjun 2003.8.27
	BYTE	data[80];	//end modify 2003.8.27.
}	T_103_DATA;

/* 103���������¼����ݸ�ʽ */
typedef struct tagT_EVENT_PARA	//modify by linjun 2003.8.27
{
	BYTE	type;
	BYTE	size;
	BYTE	num;
	BYTE	data[16];	
}	T_EVENT_PARA;				//end modify 2003.8.27.

/* ͨ�÷����ʶ��(GINH:���) */
#define X103_GIN_SYS0	0	//ϵͳ��0
#define X103_GIN_SYS1	1	//ϵͳ��1
#define X103_GIN_SET0	2	//��ֵ��0
#define X103_GIN_SET1	3	//��ֵ��1
#define X103_GIN_EVENT	4	//����������
#define X103_GIN_ALARM	5	//�����澯��
#define X103_GIN_CHN	6	//����������
#define X103_GIN_MS		7	//ң����
#define X103_GIN_DI		8	//ң����
#define X103_GIN_PS		10	//ң����
#define X103_GIN_DO		11	//ң����
#define X103_GIN_TP		12	//��ͷ��(tap position)
#define X103_GIN_YT		13	//ң����
#define X103_GIN_SFC	14	//��ѹ����
#define X103_GIN_SOE	24	//ң��SOE

/* kind of description(KOD) */
#define X103_KOD_VAL	1	//value:	ʵ��ֵ
#define X103_KOD_DEF	2	//default:	ȱʡֵ
#define X103_KOD_RAN	3	//range:	���̣���Сֵ�����ֵ��������
#define X103_KOD_PRE	5	//precision:���ȣ�n��m��
#define X103_KOD_FAC	6	//factor:	����
#define X103_KOD_UNI	9	//unit:		��λ(����)
#define X103_KOD_NAM	10	//name:		����(����)

/* type of data(TOD) */
#define X103_TOD_NIL	0	//������
#define X103_TOD_ASC	1	//ASCII�ַ�
#define X103_TOD_UINT	3	//�޷�������
#define X103_TOD_SINT	4	//�з�������
#define X103_TOD_FLOAT	6	//������
#define X103_TOD_R3223	7	//IEEE��׼754��ʵ��
#define X103_TOD_R6453	8	//IEEE��׼754ʵ��
#define X103_TOD_DPI	9	//˫����Ϣ
#define X103_TOD_MSQ	12	//��Ʒ�������Ĳ���ֵ
#define X103_TOD_SOE	18	//��ʱ��ı���

/* ϵͳ��0�µ���Ŀ���� */
#define X103_CUR_ZONE	2	//��ǰ��ֵ��
#define X103_RUN_ZONE	3	//���ж�ֵ��
#define X103_PLS_STS	5	//����״̬(����/�ⶳ)
#define X103_SIG_STS	6	//�ź�״̬(����/δ����)

/* 103Э���ʼ�� */
BOOL x103_initialize();

/* 103�豸��ʼ�� */
BOOL x103_open_ied(T_IED *p_ied);

/* 103�豸�ܲ�ѯ���� */
VOID Task_Polling(UNSIGNED argc, VOID *argv);

/* 103�豸�ܲ�ѯ */
BOOL x103_general_polling(T_103_ENTRY *p_ent);

/* ��ȡ�������� */
BOOL x103_get_pulse(T_103_ENTRY *p_ent, T_PULSE *p_ps);

/* ��������ͨ��ֵ */
BOOL x103_get_channel(T_103_ENTRY *p_ent, T_CHANNEL *p_chn);

/* ��ֵ����� */
BOOL x103_get_setting(T_103_ENTRY *p_ent, T_SET *p_set);

BOOL x103_chk_setting(T_103_ENTRY *p_ent, const T_SET *p_set);

BOOL x103_set_setting(T_103_ENTRY *p_ent, const T_SET *p_set);

BOOL x103_get_zone(T_103_ENTRY *p_ent, WORD *p_set_no);

BOOL x103_chk_zone(T_103_ENTRY *p_ent, WORD set_no);

BOOL x103_set_zone(T_103_ENTRY *p_ent, WORD set_no);

/* ��ѹ������� */
BOOL x103_get_sfc(T_103_ENTRY *p_ent, T_SFC *p_sfc);

BOOL x103_chk_sfc(T_103_ENTRY *p_ent, WORD sfc_no, WORD sfc_state);

BOOL x103_set_sfc(T_103_ENTRY *p_ent, WORD sfc_no, WORD sfc_state);

/* ң�������[��Сң�ص�� =1] */
BOOL x103_check_control(T_103_ENTRY *p_ent, WORD ctrl_no);

BOOL x103_remote_control(T_103_ENTRY *p_ent, WORD ctrl_no);

/* �źŸ��� */
BOOL x103_reset_signal(T_103_ENTRY *p_ent);

/* ʱ����� */
BOOL x103_get_clock(T_103_ENTRY *p_ent, T_DATE *p_date);

BOOL x103_set_clock(T_103_ENTRY *p_ent, const T_DATE *p_date);

/* ��������� */
BOOL x103_get_list(T_103_ENTRY *p_ent, T_LIST *p_list);

/****************************************************************************/
/*																			*/
/*    ����¼������		added by linjun 2002.9.5.							*/
/*																			*/
/****************************************************************************/

//¼��ͨ��������
BOOL x103_get_rchannel_list(T_103_ENTRY *p_ent, T_LIST *p_list);

//����־��״̬��λ������
BOOL x103_get_change_list(T_103_ENTRY *p_ent, T_LIST *p_list);

//���󱻼�¼���Ŷ���
BOOL x103_Get_Wave_Table(T_103_ENTRY *p_ent, T_WAVE_TABLE *p_table);

//���ϵ�ѡ��
BOOL x103_Get_Wave_Ready(T_103_ENTRY *p_ent, T_WAVE *p_wave);

//����־��״̬��λ����׼������
BOOL x103_Get_Wave_ReadyState(T_103_ENTRY *p_ent, T_WAVE *p_wave);

//�������־��״̬��λ��ԭʼ״̬����
BOOL x103_Get_Wave_Originality(T_103_ENTRY *p_ent, T_WAVE *p_wave);

//�������־��״̬��λ�ĵ�ǰ״̬����
BOOL x103_Get_Wave_Nonce(T_103_ENTRY *p_ent, T_WAVE *p_wave);

//���󱻼�¼��ͨ��
BOOL x103_Get_Wave_Channel(T_103_ENTRY *p_ent, T_WAVE *p_wave);

//�����Ŷ�ֵ
BOOL x103_Get_Wave_Data(T_103_ENTRY *p_ent, T_WAVE *p_wave);
#endif