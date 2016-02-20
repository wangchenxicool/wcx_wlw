#ifndef __INFOREC__
#define __INFOREC__

#include "typedef.h"
#include "system.h"

/* ͨ��״̬���� */
typedef BOOL T_STATE;

/* �����¼�����״̬ */
typedef BOOL T_PARAFLAG;				

#define MAX_EVENT_PARA		20			//��������

typedef struct tagT_EVENT_PARAITEM		//���������ṹ
{
	WORD	para_index;					//����������
	WORD	para_type;					//������������
	union								//����ֵ
	{
		BYTE	para_byte;
		float	para_float;
		float	para_complex[2];
	}un_val;
}	T_EVENT_PARAITEM;

typedef struct tagT_EVENT_PARATABLE		//�¼�������
{
	BYTE	para_num;					// ʵ��ʹ�ò�������
	BYTE	struct_lenth;				// �������ݽṹ�ֽ��ܳ���
	T_EVENT_PARAITEM	para_table[MAX_EVENT_PARA];	// ������ 
}	T_EVENT_PARATABLE;

typedef struct tagT_EVENT
{
	WORD		dev_id;					/* �豸��ʶ�� */
	WORD		e_code;					/* �¼���Ŀ��(��СֵΪ1) */
	T_STATE		e_state;				/* �¼�״̬ */
	T_DATE		e_date;					/* �¼����� */
	T_PARAFLAG	e_paraflag;				/* �¼�������Ч��־ */
	T_EVENT_PARATABLE	e_eventpara;	/* �¼����� */
}	T_EVENT;

/* �澯�¼���¼������ */	
typedef struct tagT_ALARM   
{
	WORD	dev_id;			/* �豸��ʶ�� */
	WORD	e_code;			/* �¼���Ŀ��(��СֵΪ1) */
	T_STATE	e_state;		/* �¼�״̬ */
	T_DATE	e_date;			/* �¼����� */	
}	T_ALARM;				//end modify 2003.8.27.

/* ң��SOE��¼������ */
typedef T_ALARM	T_SOE;		//end modify 2003.8.27

/* ң�ű�λ��¼������ */
typedef struct tagT_DIC
{
	WORD	dev_id;
	WORD	e_code;
	T_STATE	e_state;
}	T_DIC;

/* ң�������ݼ��ӿڶ��� */
#define MAX_DI_GROUPS  ((SYS_MAX_DI+31)/32)
typedef struct tagT_DI
{
	WORD  di_num;
	DWORD di_val[MAX_DI_GROUPS];
}	T_DI;

/* �����������ݼ��ӿڶ��� */
typedef struct tagT_PULSE
{
	WORD  ps_num;
	DWORD ps_val[SYS_MAX_PULSE];
}	T_PULSE;

/* ң�������ݼ��ӿڶ��� */
#define INF_MS_OVERFLOW    0x8000
#define INF_MS_ERROR	   0x4000
#define INF_MS_RESERVED	   0x2000
#define INF_MS_NEGATIVE    0x1000
#define INF_MS_VALUE_MASK  0x0FFF

typedef struct tagT_MEASURE
{
	WORD ms_num;
	WORD ms_val[SYS_MAX_MEASURE];
}	T_MEASURE;

/* ң��Խ������¼������ */
typedef struct tagT_OVERLINE
{
	WORD dev_id;
	WORD ovl_no;
	WORD ovl_val;
}	T_OVERLINE;

/* ��ѹ���λ��¼������ */
typedef T_DIC T_SFCC;

/* ��ѹ�����ݼ��ӿڶ��� */
#define MAX_SFC_GROUPS  ((SYS_MAX_SFC+31)/32)
typedef struct tagT_SFC
{
	WORD  sfc_num;
	DWORD sfc_val[MAX_SFC_GROUPS];
}	T_SFC;

#endif
