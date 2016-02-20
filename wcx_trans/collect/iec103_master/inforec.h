#ifndef __INFOREC__
#define __INFOREC__

#include "typedef.h"
#include "system.h"

/* 通用状态类型 */
typedef BOOL T_STATE;

/* 动作事件参数状态 */
typedef BOOL T_PARAFLAG;				

#define MAX_EVENT_PARA		20			//参数个数

typedef struct tagT_EVENT_PARAITEM		//单个参数结构
{
	WORD	para_index;					//参数索引号
	WORD	para_type;					//参数数据类型
	union								//参数值
	{
		BYTE	para_byte;
		float	para_float;
		float	para_complex[2];
	}un_val;
}	T_EVENT_PARAITEM;

typedef struct tagT_EVENT_PARATABLE		//事件参数表
{
	BYTE	para_num;					// 实际使用参数个数
	BYTE	struct_lenth;				// 所有数据结构字节总长度
	T_EVENT_PARAITEM	para_table[MAX_EVENT_PARA];	// 参数表 
}	T_EVENT_PARATABLE;

typedef struct tagT_EVENT
{
	WORD		dev_id;					/* 设备标识号 */
	WORD		e_code;					/* 事件条目号(最小值为1) */
	T_STATE		e_state;				/* 事件状态 */
	T_DATE		e_date;					/* 事件日期 */
	T_PARAFLAG	e_paraflag;				/* 事件参数有效标志 */
	T_EVENT_PARATABLE	e_eventpara;	/* 事件参数 */
}	T_EVENT;

/* 告警事件记录及检索 */	
typedef struct tagT_ALARM   
{
	WORD	dev_id;			/* 设备标识号 */
	WORD	e_code;			/* 事件条目号(最小值为1) */
	T_STATE	e_state;		/* 事件状态 */
	T_DATE	e_date;			/* 事件日期 */	
}	T_ALARM;				//end modify 2003.8.27.

/* 遥信SOE记录及检索 */
typedef T_ALARM	T_SOE;		//end modify 2003.8.27

/* 遥信变位记录及检索 */
typedef struct tagT_DIC
{
	WORD	dev_id;
	WORD	e_code;
	T_STATE	e_state;
}	T_DIC;

/* 遥信量数据及接口定义 */
#define MAX_DI_GROUPS  ((SYS_MAX_DI+31)/32)
typedef struct tagT_DI
{
	WORD  di_num;
	DWORD di_val[MAX_DI_GROUPS];
}	T_DI;

/* 脉冲电度量数据及接口定义 */
typedef struct tagT_PULSE
{
	WORD  ps_num;
	DWORD ps_val[SYS_MAX_PULSE];
}	T_PULSE;

/* 遥测量数据及接口定义 */
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

/* 遥测越限量记录及检索 */
typedef struct tagT_OVERLINE
{
	WORD dev_id;
	WORD ovl_no;
	WORD ovl_val;
}	T_OVERLINE;

/* 软压板变位记录及检索 */
typedef T_DIC T_SFCC;

/* 软压板数据及接口定义 */
#define MAX_SFC_GROUPS  ((SYS_MAX_SFC+31)/32)
typedef struct tagT_SFC
{
	WORD  sfc_num;
	DWORD sfc_val[MAX_SFC_GROUPS];
}	T_SFC;

#endif
