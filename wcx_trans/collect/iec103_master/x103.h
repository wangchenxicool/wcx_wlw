#ifndef __X103__
#define __X103__
#include "typedef.h"
#include "nucleus.h"
#include "inforec.h"
#include "system.h"
#include "dev_mng.h"
/********************************************************/
/*														*/
/*	外部接口描述										*/
/*														*/
/********************************************************/
/* 103设备服务接口 */
BOOL x103_service(T_IED *p_ied, T_MESSAGE *p_msg);

/* 打开103会话 */
BOOL x103_Open_Session(IPADDR x103_ip);

/* 关闭103会话 */
BOOL x103_Close_Session(IPADDR x103_ip);

/* 103主站端协议解释器 */
BOOL x103_Interpret(IPADDR x103_ip, const BYTE *p_data, WORD data_len);

/********************************************************/
/*														*/
/*	103设备服务接口及内部接口描述						*/
/*														*/
/********************************************************/
/* 103会话设施描述 */
#define X103_ASDU_TIMEOUT	(10*SYS_SECOND_TICKS)
#define X103_ASDU_SIZE		0x600
typedef struct tagT_SESSION
{
	BOOL		 opened;		/* 设备通讯打开标志 */
	BYTE		 x103_sn;		/* ASDU扫描序号 */
	NU_MAILBOX	 x103_mail;		/* 会话消息传递设施 */
	NU_SEMAPHORE x103_sema;		/* 会话资源保护设施 */
	WORD		 tx_len;		/* 会话发送缓冲区 */
	BYTE		 tx_buf[X103_ASDU_SIZE];
	WORD		 rx_len;		/* 会话接收缓冲区 */
	BYTE		 rx_buf[X103_ASDU_SIZE];
}	T_SESSION;

/* 103设备描述(等价于T_IED描述) */
typedef struct tagT_103_ENTRY
{
	WORD	    x103_id;		/* 系统标识号 */
	T_SERVICE   x103_if;
	WORD		x103_type;
	WORD		x103_flag;
	IPADDR	    x103_ip;		/* 通讯链路描述 */
	BYTE	    x103_addr;		/* 103通讯公共地址 */
	BYTE	    pad_0;
	T_SESSION  *x103_com;		/* 会话设施 */
}	T_103_ENTRY;

/* 103应用服务数据单元(ASDU)格式 */
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

/* 103数据(信息元)格式 */
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

/* 103带参数的事件数据格式 */
typedef struct tagT_EVENT_PARA	//modify by linjun 2003.8.27
{
	BYTE	type;
	BYTE	size;
	BYTE	num;
	BYTE	data[16];	
}	T_EVENT_PARA;				//end modify 2003.8.27.

/* 通用分类标识码(GINH:组号) */
#define X103_GIN_SYS0	0	//系统组0
#define X103_GIN_SYS1	1	//系统组1
#define X103_GIN_SET0	2	//定值组0
#define X103_GIN_SET1	3	//定值组1
#define X103_GIN_EVENT	4	//保护动作组
#define X103_GIN_ALARM	5	//保护告警组
#define X103_GIN_CHN	6	//保护测量组
#define X103_GIN_MS		7	//遥测组
#define X103_GIN_DI		8	//遥信组
#define X103_GIN_PS		10	//遥脉组
#define X103_GIN_DO		11	//遥控组
#define X103_GIN_TP		12	//分头组(tap position)
#define X103_GIN_YT		13	//遥调组
#define X103_GIN_SFC	14	//软压板组
#define X103_GIN_SOE	24	//遥信SOE

/* kind of description(KOD) */
#define X103_KOD_VAL	1	//value:	实际值
#define X103_KOD_DEF	2	//default:	缺省值
#define X103_KOD_RAN	3	//range:	量程（最小值、最大值、步长）
#define X103_KOD_PRE	5	//precision:精度（n，m）
#define X103_KOD_FAC	6	//factor:	因子
#define X103_KOD_UNI	9	//unit:		单位(量纲)
#define X103_KOD_NAM	10	//name:		名称(描述)

/* type of data(TOD) */
#define X103_TOD_NIL	0	//无数据
#define X103_TOD_ASC	1	//ASCII字符
#define X103_TOD_UINT	3	//无符号整数
#define X103_TOD_SINT	4	//有符号整数
#define X103_TOD_FLOAT	6	//浮点数
#define X103_TOD_R3223	7	//IEEE标准754短实数
#define X103_TOD_R6453	8	//IEEE标准754实数
#define X103_TOD_DPI	9	//双点信息
#define X103_TOD_MSQ	12	//带品质描述的测量值
#define X103_TOD_SOE	18	//带时标的报文

/* 系统组0下的条目定义 */
#define X103_CUR_ZONE	2	//当前定值区
#define X103_RUN_ZONE	3	//运行定值区
#define X103_PLS_STS	5	//脉冲状态(冻结/解冻)
#define X103_SIG_STS	6	//信号状态(复归/未复归)

/* 103协议初始化 */
BOOL x103_initialize();

/* 103设备初始化 */
BOOL x103_open_ied(T_IED *p_ied);

/* 103设备总查询任务 */
VOID Task_Polling(UNSIGNED argc, VOID *argv);

/* 103设备总查询 */
BOOL x103_general_polling(T_103_ENTRY *p_ent);

/* 读取脉冲电度量 */
BOOL x103_get_pulse(T_103_ENTRY *p_ent, T_PULSE *p_ps);

/* 保护采样通道值 */
BOOL x103_get_channel(T_103_ENTRY *p_ent, T_CHANNEL *p_chn);

/* 定值类服务 */
BOOL x103_get_setting(T_103_ENTRY *p_ent, T_SET *p_set);

BOOL x103_chk_setting(T_103_ENTRY *p_ent, const T_SET *p_set);

BOOL x103_set_setting(T_103_ENTRY *p_ent, const T_SET *p_set);

BOOL x103_get_zone(T_103_ENTRY *p_ent, WORD *p_set_no);

BOOL x103_chk_zone(T_103_ENTRY *p_ent, WORD set_no);

BOOL x103_set_zone(T_103_ENTRY *p_ent, WORD set_no);

/* 软压板类服务 */
BOOL x103_get_sfc(T_103_ENTRY *p_ent, T_SFC *p_sfc);

BOOL x103_chk_sfc(T_103_ENTRY *p_ent, WORD sfc_no, WORD sfc_state);

BOOL x103_set_sfc(T_103_ENTRY *p_ent, WORD sfc_no, WORD sfc_state);

/* 遥控类服务[最小遥控点号 =1] */
BOOL x103_check_control(T_103_ENTRY *p_ent, WORD ctrl_no);

BOOL x103_remote_control(T_103_ENTRY *p_ent, WORD ctrl_no);

/* 信号复归 */
BOOL x103_reset_signal(T_103_ENTRY *p_ent);

/* 时间服务 */
BOOL x103_get_clock(T_103_ENTRY *p_ent, T_DATE *p_date);

BOOL x103_set_clock(T_103_ENTRY *p_ent, const T_DATE *p_date);

/* 描述表服务 */
BOOL x103_get_list(T_103_ENTRY *p_ent, T_LIST *p_list);

/****************************************************************************/
/*																			*/
/*    故障录波服务		added by linjun 2002.9.5.							*/
/*																			*/
/****************************************************************************/

//录波通道描述表
BOOL x103_get_rchannel_list(T_103_ENTRY *p_ent, T_LIST *p_list);

//带标志的状态变位描述表
BOOL x103_get_change_list(T_103_ENTRY *p_ent, T_LIST *p_list);

//请求被记录的扰动表
BOOL x103_Get_Wave_Table(T_103_ENTRY *p_ent, T_WAVE_TABLE *p_table);

//故障的选择
BOOL x103_Get_Wave_Ready(T_103_ENTRY *p_ent, T_WAVE *p_wave);

//带标志的状态变位传输准备就绪
BOOL x103_Get_Wave_ReadyState(T_103_ENTRY *p_ent, T_WAVE *p_wave);

//请求带标志的状态变位的原始状态数据
BOOL x103_Get_Wave_Originality(T_103_ENTRY *p_ent, T_WAVE *p_wave);

//请求带标志的状态变位的当前状态数据
BOOL x103_Get_Wave_Nonce(T_103_ENTRY *p_ent, T_WAVE *p_wave);

//请求被记录的通道
BOOL x103_Get_Wave_Channel(T_103_ENTRY *p_ent, T_WAVE *p_wave);

//请求扰动值
BOOL x103_Get_Wave_Data(T_103_ENTRY *p_ent, T_WAVE *p_wave);
#endif