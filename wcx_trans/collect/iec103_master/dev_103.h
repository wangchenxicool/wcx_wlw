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
/*	外部接口描述		                    			*/
/*							                            */
/********************************************************/
/* 103设备服务接口 */
BOOL c103_service(T_IED *p_ied, T_MESSAGE *p_msg);

/********************************************************/
/*						                            	*/
/*	103设备服务接口及内部接口描述			            */
/*							                            */
/********************************************************/

typedef struct tagT_C103_CODE_TABLE
{	
	WORD			dev_type;	/* 设备类型 */	
	T_EVENT_ENTRY  *p_event;	/* 事件码表 */
	T_ALARM_ENTRY  *p_alarm;	/* 告警码表 */
	T_SET_ENTRY    *p_setting;	/* 定值码表 */
	WORD			w_even;		/* 事件个数 */
	WORD			w_alarm;	/* 告警个数 */
	WORD			w_setting;	/* 定值个数 */
}	T_C103_CODE_TABLE;

/* 103会话设施描述 */
#define C103_ASDU_TIMEOUT	(2*SYS_SECOND_TICKS)
#define C103_ASDU_SIZE		0x300

/* return value of callback function */
#define UART_PKT_FIN	0
#define UART_PKT_CON	1
#define UART_PKT_ERR	2

#define UART_PACKET_SIZE  500

//帧格式类型
#define FRAME_T_VOL	0x68	/* 可变帧长 */
#define FRAME_T_CON	0x10	/* 固定帧长 */

/* 103报文帧格式描述 */
typedef struct tagT_C103_FRAME
{	
	BYTE	type;			/* 帧类型 */
	BYTE	len;			/* 长度   */
	BYTE	contral;		/* 控制域 */
	BYTE	address;		/* 地址域 */
	BYTE	crc;			/* 帧校验和 */
	BYTE	endbyte;		/* 结束字符 */

} T_C103_FRAME;

/* 103应用服务数据单元(ASDU)格式 */
typedef struct tagT_C103_ASDU
{
	BYTE	type;			/* 类型标识 */
	BYTE	vsq;			/* 可变结构限定词 */
	BYTE	cot;			/* 传送原因 */
	BYTE	addr;			/* 公共地址 */
	BYTE	fun;			/* 功能类型 */
	BYTE	inf;			/* 信息序号 */
	BYTE	data[C103_ASDU_SIZE-7];	/* 信息体报文 */

} T_C103_ASDU;

/* 103数据(信息元)格式 */
typedef struct tagT_C103_DATA
{
	BYTE	ginh;			/* 通用分类标识序号高 */
	BYTE	ginl;			/* 通用分类标识序号低 */
	BYTE	kod;			/* 描述的类别 */
	BYTE	type;			/* 数据类型 */
	BYTE	size;			/* 数据宽度 */
	BYTE	num;			/* 数目 */
	BYTE	data[16];		/* 数据值 */

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

/* 串口数目 */
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

/* 类型标识 */
#define	ASDU1		1
#define ASDU2		2
#define ASDU5		5
#define ASDU8		8
#define ASDU10		10
#define ASDU40		40		//遥信变位
#define ASDU41		41		//SOE
#define ASDU44		44		//全遥信
#define ASDU50		50		//全遥测
#define ASDU64		64		//遥控

/* 起始点号 */
#define MEA_FIRST_FUN	0x01
#define MEA_FIRST_INF	0x5b
#define DI_FIRST_FUN	0x01
#define DI_FIRST_INF	0x94
#define CTRL_FIRST_FUN	0x01
#define CTRL_FIRST_INF	0x2f

/* 功能类型 */
#define C103_FUN_931A		178
#define C103_FUN_901A		178
#define C103_FUN_902A		178
#define C103_FUN_923A		178
#define C103_FUN_943A		178
#define C103_FUN_915A		210
#define C103_FUN_9607		1

#define C103_FUN_GEN		254		//通用分类功能类型
#define C103_FUN_GLB		255		//全局功能类型

/* 通用分类标识码(GINH:组号) */
#define C103_GIN_SYS0	0	//系统组0
#define C103_GIN_SYS1	1	//系统组1
#define C103_GIN_SET0	2	//定值组0
#define C103_GIN_SET1	3	//定值组1
#define C103_GIN_EVENT	4	//保护动作组
#define C103_GIN_ALARM	5	//保护告警组
#define C103_GIN_CHN	6	//保护测量组
#define C103_GIN_MS		7	//遥测组
#define C103_GIN_DI		8	//遥信组
#define C103_GIN_PS		10	//遥脉组
#define C103_GIN_DO		11	//遥控组
#define C103_GIN_TP		12	//分头组(tap position)
#define C103_GIN_YT		13	//遥调组
#define C103_GIN_SFC	14	//软压板组
#define C103_GIN_SOE	24	//遥信SOE

/* kind of description(KOD) */
#define C103_KOD_VAL	1	//value:	实际值
#define C103_KOD_DEF	2	//default:	缺省值
#define C103_KOD_RAN	3	//range:	量程（最小值、最大值、步长）
#define C103_KOD_PRE	5	//precision:精度（n，m）
#define C103_KOD_FAC	6	//factor:	因子
#define C103_KOD_UNI	9	//unit:		单位(量纲)
#define C103_KOD_NAM	10	//name:		名称(描述)

/* type of data(TOD) */
#define C103_TOD_NIL	0	//无数据
#define C103_TOD_ASC	1	//ASCII字符
#define C103_TOD_UINT	3	//无符号整数
#define C103_TOD_SINT	4	//有符号整数
#define C103_TOD_FLOAT	6	//浮点数
#define C103_TOD_R3223	7	//IEEE标准754短实数
#define C103_TOD_R6453	8	//IEEE标准754实数
#define C103_TOD_DPI	9	//双点信息
#define C103_TOD_MSQ	12	//带品质描述的测量值
#define C103_TOD_SOE	18	//带时标的报文

/* 系统组0下的条目定义 */
#define C103_CUR_ZONE	2	//当前定值区
#define C103_RUN_ZONE	3	//运行定值区
#define C103_PLS_STS	5	//脉冲状态(冻结/解冻)
#define C103_SIG_STS	6	//信号状态(复归/未复归)

/* 103设备总查询任务 */
void task_c103(int argc, void *argv);

/* 保护采样通道值 */
BOOL c103_get_channel(T_IED *p_ied, T_CHANNEL *p_chn);

/* 定值类服务 */
BOOL c103_get_setting(T_IED *p_ied, T_SET *p_set);

BOOL c103_get_zone(T_IED *p_ied, WORD *p_set_no);

/* 软压板类服务 */
BOOL c103_get_sfc(T_IED *p_ied);

/* 遥控类服务[最小遥控点号 =1] */
BOOL c103_check_control (int sn, iec103_tag_tp *p_tag, WORD ctrl_no);

BOOL c103_remote_control (int sn, iec103_tag_tp *p_tag, WORD ctrl_no);

/* 信号复归 */
BOOL c103_reset_signal(T_IED *p_ied);

/* 时间服务 */
static BOOL c103_set_clock(T_IED *p_ied, const T_DATE *p_date);

/* 描述表服务 */
BOOL c103_get_list(T_IED *p_ied, T_LIST *p_list);

/* 取103条目号 */
//WORD C103_Get_Code(T_IED* p_ied, T_LIST* p_list, WORD code);

static BOOL c103_device_initialize(T_IED* p_ied);

int iec103_master_run (void *arg);
int iec103_master_exit (void *arg);

#endif
