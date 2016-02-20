#ifndef __DEV_MNG__
#define __DEV_MNG__

#include "typedef.h"
#include "system.h"

/* IED(Intelligent Electronic Device)设备及服务接口描述 */
typedef struct tagT_MESSAGE
{
	WORD	m_type;		/* message type */
	WORD	m_flag;		/* message flag */
	DWORD	m_data;		/* message data */
}	T_MESSAGE;

/* definitions of message type */
#define MSG_T_INIT		0		/* 初始化协议 */
#define MSG_T_OPEN		50		/* 初始化设备，(T_IED    *)m_data */
#define MSG_T_LIST		100		/* 描述表服务，(T_LIST   *)m_data */
#define MSG_T_ANALOG	200		/* 保护测量值，(T_CHANNEL*)m_data */
#define MSG_T_SET		300		/* 定值操作类，(T_SET    *)m_data */
#define MSG_T_ZONE		400		/* 定值区操作，读[m_data:区号指针]，写[m_data:区号] */
#define MSG_T_SFC		500		/* 软压板设置，LOWORD(m_data):压板编号，HIWORD(m_data):压板状态 */
#define MSG_T_CTRL		600		/* 遥控类操作，m_data:遥控点号 */
#define MSG_T_SIGNAL	700		/* 保护信号复归 */
#define MSG_T_CLOCK		800		/* 时间类服务，(T_DATE   *)m_data */

/* definitions of message flag */
#define MSG_F_READ		0x55	/* 读操作 */
#define MSG_F_WRITE		0xAA	/* 执行写操作 */
#define MSG_F_CHECK		0x88	/* 选择写操作 */

typedef BOOL (*T_SERVICE)(struct t_ied_struct *p_ied, T_MESSAGE *p_msg);

typedef struct t_ied_struct
{
	WORD		dev_id;			/* 设备编号 */
    T_SERVICE	dev_if;			/* 设备服务接口 */
	WORD		dev_type;		/* 设备类型 */
	WORD		dev_flag;		/* 设备标志 */
	BYTE		dev_data[6];	/* 设备描述用数据 */
	DWORD		user_defined_1; /* 用户自定义数据 */
}	T_IED;

/* IED数据描述 */
//保护采样数据
typedef struct tagT_CHANNEL
{
	WORD	chn_num;
	float	chn_val[SYS_MAX_CHANNEL];
}	T_CHANNEL;

//保护定值项
typedef struct tagT_SET_ITEM
{
	WORD	type;
	union
	{
		WORD	u_val;
		float   f_val;
	} un_val;
}	T_SET_ITEM;

//保护定值项类型
#define SET_T_UINT	0x55	/* 16位整数   */
#define SET_T_FLOAT	0xAA	/* 仿真浮点数 */

//保护定值
typedef struct tagT_SET
{
	WORD		set_no;
	WORD		set_num;
	T_SET_ITEM	set_val[SYS_MAX_SET];
}	T_SET;

//设备描述表
typedef struct tagT_LIST
{
	WORD	l_type;
	WORD	l_size;
	void   *l_ptr;
}	T_LIST;

//描述表类型
#define LIST_T_ANALOG	0  //模拟量描述表
#define LIST_T_SET		1  //定值定义表
#define LIST_T_SFC		2  //软压板描述表
#define LIST_T_EVENT	3  //事件描述表
#define LIST_T_ALARM	4  //告警描述表

#define NAME_SIZE       128

//描述表条目定义(e_key用于IED设备)
typedef struct tagT_ANALOG_ENTRY
{
	WORD	e_key;
	char	name[NAME_SIZE];
	char	unit[4];
}	T_ANALOG_ENTRY;

typedef struct tagT_SET_ENTRY
{
	WORD	e_key;
	WORD	type;		//定值项类型
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

/* 设备创建、维护及迭代搜索 */
BOOL DEV_Create_IED(T_IED *p_dev, WORD dev_num);

T_IED *DEV_Search_IED(WORD dev_id);

T_IED *DEV_First_IED(void);

T_IED *DEV_Next_IED(WORD dev_id);

/* 为上位机提供的通用服务接口 */
//描述表服务
BOOL DEV_Get_List(WORD dev_id, WORD l_type, T_LIST *p_list);

//保护采样通道值
BOOL DEV_Get_Channel(WORD dev_id, T_CHANNEL *p_chn);

//定值类服务
BOOL DEV_Get_Setting(WORD dev_id, WORD set_no, T_SET *p_set);

BOOL DEV_Check_Setting(WORD dev_id, WORD set_no, T_SET *p_set);

BOOL DEV_Set_Setting(WORD dev_id, WORD set_no, T_SET *p_set);

BOOL DEV_Get_Zone(WORD dev_id, WORD *p_set_no);

BOOL DEV_Check_Zone(WORD dev_id, WORD set_no);

BOOL DEV_Set_Zone(WORD dev_id, WORD set_no);

//软压板类服务[最小软压板号 =1]
BOOL DEV_Check_SFC(WORD dev_id, WORD sfc_no, WORD sfc_state);

BOOL DEV_Set_SFC(WORD dev_id, WORD sfc_no, WORD sfc_state);

//遥控类服务[最小遥控点号 =1]
BOOL DEV_Check_Control(WORD dev_id, WORD ctrl_no);

BOOL DEV_Remote_Control(WORD dev_id, WORD ctrl_no);

//信号复归
BOOL DEV_Reset_Signal(WORD dev_id);

//时间服务
BOOL DEV_Get_Clock(WORD dev_id, T_DATE *p_date);

BOOL DEV_Set_Clock(WORD dev_id, const T_DATE *p_date);

/*******************************************************/
/*                                                     */
/*	保护故障录波数据  2002年9月12日 林俊               */ 
/*                                                     */
/*******************************************************/

//定义服务类型
#define MSG_T_WAVE			900		// 故障录波类服务, (T_WAVE *)m_data 

//定义服务标志
#define MSG_F_TABLE			0x100	// 被记录的扰动表					
#define MSG_F_READY			0x200	// 扰动数据传输准备就绪	
#define MSG_F_STATE			0x250			
#define MSG_F_ORIGINALITY	0x300	// 带标志的状态变位的原始状态数据   
#define MSG_F_NONCE			0x400	// 带标志的状态变位的当前状态数据   
#define MSG_F_CHANNEL		0x500	// 被记录的通道传输					
#define MSG_F_DATA			0x600	// 传输扰动值						

//描述表的类型
#define LIST_T_CHANNEL	5			// 录波通道号的描述表				
#define LIST_T_CHANGE	6			// 录波带标志的变位遥信的描述表		

//最大变位数目
#define MAX_CHANGE_NUM	40

//最大扰动数值
#define MAX_WAVE_DATA	500

//最大记录录波数目
#define MAX_WAVE_NUM	50

//描述表的条目定义
typedef struct tagT_CHANNEL_ENTRY
{
	WORD	e_key;
	char	name[NAME_SIZE];
}	T_CHANNEL_ENTRY;				// 录波通道号的描述表定义			

typedef struct tagT_CHANGE_ENTRY
{
	WORD	e_key;
	char	name[NAME_SIZE];
}	T_CHANGE_ENTRY;					// 录波带标志的变位遥信的描述表定义	

//被记录的扰动数据表
typedef struct tagT_WAVE_TABLE
{
	BYTE	wave_num;					//录波条目数
	WORD	wave_fan[MAX_WAVE_NUM];		//故障序号
	BYTE	wave_sof[MAX_WAVE_NUM];		//故障的状态
	T_DATE	wave_date[MAX_WAVE_NUM];	//七个八位组的二进制时间
}	T_WAVE_TABLE;

//带标志的状态变位信息表
typedef struct tagT_WAVE_STATE
{
	WORD	wave_tap;				//标志的位置
	BYTE	wave_fun;				//功能类型	
	BYTE	wave_inf;				//信息序号	
	BYTE	wave_dpi;				//双点信息
}	T_WAVE_STATE;

//扰动值数据
typedef struct tagT_WAVE_DATA
{
	BYTE	wave_tov;				//扰动值的类型
	BYTE	wave_acc;				//实际通道号
	WORD	wave_ndv;				//每个ASDU有关联扰动值的数目
	WORD	wave_nfe;				//ASDU第一个信息元素的序号
	float	wave_rfa;				//参比因子
	float	wave_rpv;				//额定一次值
	float	wave_rsv;				//额定二次值
	WORD	*p_wave_data;			//单个扰动值
}	T_WAVE_DATA;

//故障录波数据
typedef struct tagT_DEV_WAVE
{
	WORD	wave_fan;					//故障序号
	BYTE	wave_acc;					//实际通道号
	BYTE	wave_sof;					//故障的状态
	T_DATE	wave_date;					//七个八位组的二进制时间
	DWORD	wave_time;					//四个八位组的二进制时间
	WORD	wave_noe;					//一个通道信息元素的数目
	WORD	wave_nof;					//电网故障序号
	BYTE	wave_noc;					//通道数目
	WORD	wave_int;					//信息元素之间的间隔
	BYTE	wave_not;					//带标志的状态变位的数目
	BOOL	wave_end;					//数据结束
	T_WAVE_STATE	*p_wave_orstate;	//带标志的原始状态信息
	T_WAVE_STATE	*p_wave_nostate;	//带标志的状态变位信息
	T_WAVE_DATA		wave_data;			//扰动值
}	T_WAVE;

#endif
