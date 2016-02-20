#ifndef __DEV_MNG__
#define __DEV_MNG__

#include "typedef.h"
#include "system.h"

/* IED(Intelligent Electronic Device)�豸������ӿ����� */
typedef struct tagT_MESSAGE
{
	WORD	m_type;		/* message type */
	WORD	m_flag;		/* message flag */
	DWORD	m_data;		/* message data */
}	T_MESSAGE;

/* definitions of message type */
#define MSG_T_INIT		0		/* ��ʼ��Э�� */
#define MSG_T_OPEN		50		/* ��ʼ���豸��(T_IED    *)m_data */
#define MSG_T_LIST		100		/* ���������(T_LIST   *)m_data */
#define MSG_T_ANALOG	200		/* ��������ֵ��(T_CHANNEL*)m_data */
#define MSG_T_SET		300		/* ��ֵ�����࣬(T_SET    *)m_data */
#define MSG_T_ZONE		400		/* ��ֵ����������[m_data:����ָ��]��д[m_data:����] */
#define MSG_T_SFC		500		/* ��ѹ�����ã�LOWORD(m_data):ѹ���ţ�HIWORD(m_data):ѹ��״̬ */
#define MSG_T_CTRL		600		/* ң���������m_data:ң�ص�� */
#define MSG_T_SIGNAL	700		/* �����źŸ��� */
#define MSG_T_CLOCK		800		/* ʱ�������(T_DATE   *)m_data */

/* definitions of message flag */
#define MSG_F_READ		0x55	/* ������ */
#define MSG_F_WRITE		0xAA	/* ִ��д���� */
#define MSG_F_CHECK		0x88	/* ѡ��д���� */

typedef BOOL (*T_SERVICE)(struct t_ied_struct *p_ied, T_MESSAGE *p_msg);

typedef struct t_ied_struct
{
	WORD		dev_id;			/* �豸��� */
    T_SERVICE	dev_if;			/* �豸����ӿ� */
	WORD		dev_type;		/* �豸���� */
	WORD		dev_flag;		/* �豸��־ */
	BYTE		dev_data[6];	/* �豸���������� */
	DWORD		user_defined_1; /* �û��Զ������� */
}	T_IED;

/* IED�������� */
//������������
typedef struct tagT_CHANNEL
{
	WORD	chn_num;
	float	chn_val[SYS_MAX_CHANNEL];
}	T_CHANNEL;

//������ֵ��
typedef struct tagT_SET_ITEM
{
	WORD	type;
	union
	{
		WORD	u_val;
		float   f_val;
	} un_val;
}	T_SET_ITEM;

//������ֵ������
#define SET_T_UINT	0x55	/* 16λ����   */
#define SET_T_FLOAT	0xAA	/* ���渡���� */

//������ֵ
typedef struct tagT_SET
{
	WORD		set_no;
	WORD		set_num;
	T_SET_ITEM	set_val[SYS_MAX_SET];
}	T_SET;

//�豸������
typedef struct tagT_LIST
{
	WORD	l_type;
	WORD	l_size;
	void   *l_ptr;
}	T_LIST;

//����������
#define LIST_T_ANALOG	0  //ģ����������
#define LIST_T_SET		1  //��ֵ�����
#define LIST_T_SFC		2  //��ѹ��������
#define LIST_T_EVENT	3  //�¼�������
#define LIST_T_ALARM	4  //�澯������

#define NAME_SIZE       128

//��������Ŀ����(e_key����IED�豸)
typedef struct tagT_ANALOG_ENTRY
{
	WORD	e_key;
	char	name[NAME_SIZE];
	char	unit[4];
}	T_ANALOG_ENTRY;

typedef struct tagT_SET_ENTRY
{
	WORD	e_key;
	WORD	type;		//��ֵ������
	char	name[NAME_SIZE];
	char	unit[4];
}	T_SET_ENTRY;

typedef struct tagT_EVENT_ENTRY
{
	WORD	e_key;
	char	name[NAME_SIZE];
}	T_EVENT_ENTRY;

typedef struct tagT_ALARM_ENTRY
{
	WORD	e_key;
	char	name[NAME_SIZE];
}	T_ALARM_ENTRY;

typedef struct tagT_SFC_ENTRY
{
	WORD	e_key;
	char	name[NAME_SIZE];
}	T_SFC_ENTRY;

/* �豸������ά������������ */
BOOL DEV_Create_IED(T_IED *p_dev, WORD dev_num);

T_IED *DEV_Search_IED(WORD dev_id);

T_IED *DEV_First_IED(void);

T_IED *DEV_Next_IED(WORD dev_id);

/* Ϊ��λ���ṩ��ͨ�÷���ӿ� */
//���������
BOOL DEV_Get_List(WORD dev_id, WORD l_type, T_LIST *p_list);

//��������ͨ��ֵ
BOOL DEV_Get_Channel(WORD dev_id, T_CHANNEL *p_chn);

//��ֵ�����
BOOL DEV_Get_Setting(WORD dev_id, WORD set_no, T_SET *p_set);

BOOL DEV_Check_Setting(WORD dev_id, WORD set_no, T_SET *p_set);

BOOL DEV_Set_Setting(WORD dev_id, WORD set_no, T_SET *p_set);

BOOL DEV_Get_Zone(WORD dev_id, WORD *p_set_no);

BOOL DEV_Check_Zone(WORD dev_id, WORD set_no);

BOOL DEV_Set_Zone(WORD dev_id, WORD set_no);

//��ѹ�������[��С��ѹ��� =1]
BOOL DEV_Check_SFC(WORD dev_id, WORD sfc_no, WORD sfc_state);

BOOL DEV_Set_SFC(WORD dev_id, WORD sfc_no, WORD sfc_state);

//ң�������[��Сң�ص�� =1]
BOOL DEV_Check_Control(WORD dev_id, WORD ctrl_no);

BOOL DEV_Remote_Control(WORD dev_id, WORD ctrl_no);

//�źŸ���
BOOL DEV_Reset_Signal(WORD dev_id);

//ʱ�����
BOOL DEV_Get_Clock(WORD dev_id, T_DATE *p_date);

BOOL DEV_Set_Clock(WORD dev_id, const T_DATE *p_date);

/*******************************************************/
/*                                                     */
/*	��������¼������  2002��9��12�� �ֿ�               */ 
/*                                                     */
/*******************************************************/

//�����������
#define MSG_T_WAVE			900		// ����¼�������, (T_WAVE *)m_data 

//��������־
#define MSG_F_TABLE			0x100	// ����¼���Ŷ���					
#define MSG_F_READY			0x200	// �Ŷ����ݴ���׼������	
#define MSG_F_STATE			0x250			
#define MSG_F_ORIGINALITY	0x300	// ����־��״̬��λ��ԭʼ״̬����   
#define MSG_F_NONCE			0x400	// ����־��״̬��λ�ĵ�ǰ״̬����   
#define MSG_F_CHANNEL		0x500	// ����¼��ͨ������					
#define MSG_F_DATA			0x600	// �����Ŷ�ֵ						

//�����������
#define LIST_T_CHANNEL	5			// ¼��ͨ���ŵ�������				
#define LIST_T_CHANGE	6			// ¼������־�ı�λң�ŵ�������		

//����λ��Ŀ
#define MAX_CHANGE_NUM	40

//����Ŷ���ֵ
#define MAX_WAVE_DATA	500

//����¼¼����Ŀ
#define MAX_WAVE_NUM	50

//���������Ŀ����
typedef struct tagT_CHANNEL_ENTRY
{
	WORD	e_key;
	char	name[NAME_SIZE];
}	T_CHANNEL_ENTRY;				// ¼��ͨ���ŵ���������			

typedef struct tagT_CHANGE_ENTRY
{
	WORD	e_key;
	char	name[NAME_SIZE];
}	T_CHANGE_ENTRY;					// ¼������־�ı�λң�ŵ���������	

//����¼���Ŷ����ݱ�
typedef struct tagT_WAVE_TABLE
{
	BYTE	wave_num;					//¼����Ŀ��
	WORD	wave_fan[MAX_WAVE_NUM];		//�������
	BYTE	wave_sof[MAX_WAVE_NUM];		//���ϵ�״̬
	T_DATE	wave_date[MAX_WAVE_NUM];	//�߸���λ��Ķ�����ʱ��
}	T_WAVE_TABLE;

//����־��״̬��λ��Ϣ��
typedef struct tagT_WAVE_STATE
{
	WORD	wave_tap;				//��־��λ��
	BYTE	wave_fun;				//��������	
	BYTE	wave_inf;				//��Ϣ���	
	BYTE	wave_dpi;				//˫����Ϣ
}	T_WAVE_STATE;

//�Ŷ�ֵ����
typedef struct tagT_WAVE_DATA
{
	BYTE	wave_tov;				//�Ŷ�ֵ������
	BYTE	wave_acc;				//ʵ��ͨ����
	WORD	wave_ndv;				//ÿ��ASDU�й����Ŷ�ֵ����Ŀ
	WORD	wave_nfe;				//ASDU��һ����ϢԪ�ص����
	float	wave_rfa;				//�α�����
	float	wave_rpv;				//�һ��ֵ
	float	wave_rsv;				//�����ֵ
	WORD	*p_wave_data;			//�����Ŷ�ֵ
}	T_WAVE_DATA;

//����¼������
typedef struct tagT_DEV_WAVE
{
	WORD	wave_fan;					//�������
	BYTE	wave_acc;					//ʵ��ͨ����
	BYTE	wave_sof;					//���ϵ�״̬
	T_DATE	wave_date;					//�߸���λ��Ķ�����ʱ��
	DWORD	wave_time;					//�ĸ���λ��Ķ�����ʱ��
	WORD	wave_noe;					//һ��ͨ����ϢԪ�ص���Ŀ
	WORD	wave_nof;					//�����������
	BYTE	wave_noc;					//ͨ����Ŀ
	WORD	wave_int;					//��ϢԪ��֮��ļ��
	BYTE	wave_not;					//����־��״̬��λ����Ŀ
	BOOL	wave_end;					//���ݽ���
	T_WAVE_STATE	*p_wave_orstate;	//����־��ԭʼ״̬��Ϣ
	T_WAVE_STATE	*p_wave_nostate;	//����־��״̬��λ��Ϣ
	T_WAVE_DATA		wave_data;			//�Ŷ�ֵ
}	T_WAVE;

#endif
