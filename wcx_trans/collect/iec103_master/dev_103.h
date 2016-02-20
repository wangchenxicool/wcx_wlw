#ifndef __NR_103__
#define __NR_103__

#include <stdint.h>
#include <vector>
#include <queue>
#include <iostream>
#include "typedef.h"
#include "system.h"
#include "dev_mng.h"
#include "com.h"
/********************************************************/
/*                          							*/
/*	�ⲿ�ӿ�����		                    			*/
/*							                            */
/********************************************************/
/* 103�豸����ӿ� */
BOOL c103_service(T_IED *p_ied, T_MESSAGE *p_msg);

/********************************************************/
/*						                            	*/
/*	103�豸����ӿڼ��ڲ��ӿ�����			            */
/*							                            */
/********************************************************/

typedef struct tagT_C103_CODE_TABLE
{	
	WORD			dev_type;	/* �豸���� */	
	T_EVENT_ENTRY  *p_event;	/* �¼���� */
	T_ALARM_ENTRY  *p_alarm;	/* �澯��� */
	T_SET_ENTRY    *p_setting;	/* ��ֵ��� */
	WORD			w_even;		/* �¼����� */
	WORD			w_alarm;	/* �澯���� */
	WORD			w_setting;	/* ��ֵ���� */
}	T_C103_CODE_TABLE;

/* 103�Ự��ʩ���� */
#define C103_ASDU_TIMEOUT	(2*SYS_SECOND_TICKS)
#define C103_ASDU_SIZE		0x300

/* return value of callback function */
#define UART_PKT_FIN	0
#define UART_PKT_CON	1
#define UART_PKT_ERR	2

#define UART_PACKET_SIZE  500

//֡��ʽ����
#define FRAME_T_VOL	0x68	/* �ɱ�֡�� */
#define FRAME_T_CON	0x10	/* �̶�֡�� */

/* 103����֡��ʽ���� */
typedef struct tagT_C103_FRAME
{	
	BYTE	type;			/* ֡���� */
	BYTE	len;			/* ����   */
	BYTE	contral;		/* ������ */
	BYTE	address;		/* ��ַ�� */
	BYTE	crc;			/* ֡У��� */
	BYTE	endbyte;		/* �����ַ� */

} T_C103_FRAME;

/* 103Ӧ�÷������ݵ�Ԫ(ASDU)��ʽ */
typedef struct tagT_C103_ASDU
{
	BYTE	type;			/* ���ͱ�ʶ */
	BYTE	vsq;			/* �ɱ�ṹ�޶��� */
	BYTE	cot;			/* ����ԭ�� */
	BYTE	addr;			/* ������ַ */
	BYTE	fun;			/* �������� */
	BYTE	inf;			/* ��Ϣ��� */
	BYTE	data[C103_ASDU_SIZE-7];	/* ��Ϣ�屨�� */

} T_C103_ASDU;

/* 103����(��ϢԪ)��ʽ */
typedef struct tagT_C103_DATA
{
	BYTE	ginh;			/* ͨ�÷����ʶ��Ÿ� */
	BYTE	ginl;			/* ͨ�÷����ʶ��ŵ� */
	BYTE	kod;			/* ��������� */
	BYTE	type;			/* �������� */
	BYTE	size;			/* ���ݿ�� */
	BYTE	num;			/* ��Ŀ */
	BYTE	data[16];		/* ����ֵ */

} T_C103_DATA;

typedef struct _iec103_tag_tp
{
    WORD            tag_id;
    BYTE            device_addr;
    BYTE            iec103_type;
    volatile BYTE   fcb;
    WORD            dev_flag;
    BYTE            fun;
    BYTE            inf;
    BYTE            operation;
    float           factor;

} iec103_tag_tp;

struct iec103_tag_table
{
    char                        device[50];
    iec103_com_t                com;
    pthread_t                   com_th_id;
    BYTE                        tx_buf[UART_PACKET_SIZE];
    BYTE                        rx_buf[UART_PACKET_SIZE];
    pthread_mutex_t             com_mutex;
    WORD                        com_space_time;
    WORD                        com_time_out;
    std::vector<iec103_tag_tp>  tag;

};

/* ������Ŀ */
#define COM_NUM                 7
/* iec103 config file */
#define IEC103_CONF_FILE        "/etc/iec103_conf"
/* iec103 boot file */
#define IEC103_BOOT_FILE        "/var/log/iec103_boot.log"
/* iec103 err file */
#define IEC103_ERR_FILE         "/var/log/iec103_err.log"
/* tag_file */
#define TAG_FILE                "/mnt/disk/ort3000c/collect/bin/sysdb.db"
/* optimized_tag_file */
#define TAG_FILE_OPTIMIZED      "/mnt/disk/ort3000c/collect/bin/tags/optimized_tag_iec103_master.db"
#define ONLINE                  1
#define OFFLINE                 0

/* ���ͱ�ʶ */
#define	ASDU1		1
#define ASDU2		2
#define ASDU5		5
#define ASDU8		8
#define ASDU10		10
#define ASDU40		40		//ң�ű�λ
#define ASDU41		41		//SOE
#define ASDU44		44		//ȫң��
#define ASDU50		50		//ȫң��
#define ASDU64		64		//ң��

/* ��ʼ��� */
#define MEA_FIRST_FUN	0x01
#define MEA_FIRST_INF	0x5b
#define DI_FIRST_FUN	0x01
#define DI_FIRST_INF	0x94
#define CTRL_FIRST_FUN	0x01
#define CTRL_FIRST_INF	0x2f

/* �������� */
#define C103_FUN_931A		178
#define C103_FUN_901A		178
#define C103_FUN_902A		178
#define C103_FUN_923A		178
#define C103_FUN_943A		178
#define C103_FUN_915A		210
#define C103_FUN_9607		1

#define C103_FUN_GEN		254		//ͨ�÷��๦������
#define C103_FUN_GLB		255		//ȫ�ֹ�������

/* ͨ�÷����ʶ��(GINH:���) */
#define C103_GIN_SYS0	0	//ϵͳ��0
#define C103_GIN_SYS1	1	//ϵͳ��1
#define C103_GIN_SET0	2	//��ֵ��0
#define C103_GIN_SET1	3	//��ֵ��1
#define C103_GIN_EVENT	4	//����������
#define C103_GIN_ALARM	5	//�����澯��
#define C103_GIN_CHN	6	//����������
#define C103_GIN_MS		7	//ң����
#define C103_GIN_DI		8	//ң����
#define C103_GIN_PS		10	//ң����
#define C103_GIN_DO		11	//ң����
#define C103_GIN_TP		12	//��ͷ��(tap position)
#define C103_GIN_YT		13	//ң����
#define C103_GIN_SFC	14	//��ѹ����
#define C103_GIN_SOE	24	//ң��SOE

/* kind of description(KOD) */
#define C103_KOD_VAL	1	//value:	ʵ��ֵ
#define C103_KOD_DEF	2	//default:	ȱʡֵ
#define C103_KOD_RAN	3	//range:	���̣���Сֵ�����ֵ��������
#define C103_KOD_PRE	5	//precision:���ȣ�n��m��
#define C103_KOD_FAC	6	//factor:	����
#define C103_KOD_UNI	9	//unit:		��λ(����)
#define C103_KOD_NAM	10	//name:		����(����)

/* type of data(TOD) */
#define C103_TOD_NIL	0	//������
#define C103_TOD_ASC	1	//ASCII�ַ�
#define C103_TOD_UINT	3	//�޷�������
#define C103_TOD_SINT	4	//�з�������
#define C103_TOD_FLOAT	6	//������
#define C103_TOD_R3223	7	//IEEE��׼754��ʵ��
#define C103_TOD_R6453	8	//IEEE��׼754ʵ��
#define C103_TOD_DPI	9	//˫����Ϣ
#define C103_TOD_MSQ	12	//��Ʒ�������Ĳ���ֵ
#define C103_TOD_SOE	18	//��ʱ��ı���

/* ϵͳ��0�µ���Ŀ���� */
#define C103_CUR_ZONE	2	//��ǰ��ֵ��
#define C103_RUN_ZONE	3	//���ж�ֵ��
#define C103_PLS_STS	5	//����״̬(����/�ⶳ)
#define C103_SIG_STS	6	//�ź�״̬(����/δ����)

/* 103�豸�ܲ�ѯ���� */
void task_c103(int argc, void *argv);

/* ��������ͨ��ֵ */
BOOL c103_get_channel(T_IED *p_ied, T_CHANNEL *p_chn);

/* ��ֵ����� */
BOOL c103_get_setting(T_IED *p_ied, T_SET *p_set);

BOOL c103_get_zone(T_IED *p_ied, WORD *p_set_no);

/* ��ѹ������� */
BOOL c103_get_sfc(T_IED *p_ied);

/* ң�������[��Сң�ص�� =1] */
BOOL c103_check_control (int sn, iec103_tag_tp *p_tag, WORD ctrl_no);

BOOL c103_remote_control (int sn, iec103_tag_tp *p_tag, WORD ctrl_no);

/* �źŸ��� */
BOOL c103_reset_signal(T_IED *p_ied);

/* ʱ����� */
static BOOL c103_set_clock(T_IED *p_ied, const T_DATE *p_date);

/* ��������� */
BOOL c103_get_list(T_IED *p_ied, T_LIST *p_list);

/* ȡ103��Ŀ�� */
//WORD C103_Get_Code(T_IED* p_ied, T_LIST* p_list, WORD code);

static BOOL c103_device_initialize(T_IED* p_ied);

int iec103_master_run (void *arg);
int iec103_master_exit (void *arg);

#endif
