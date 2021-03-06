#include "dev_mng.h"
#include "dev_103.h"

/*设备类型*/
#define RCS931A 0x01
#define RCS901A 0x02
#define RCS915A 0x03
#define RCS9607 0x04
#define RCS943A 0x05

/********************************************************
rcs9607遥测表       系数
1:ua---------------120/4096*pt
2:ub
3:uc
4:ux
5:uab
6:ubc
7:uca
8:3u0
9:ia---------------6/4096*ct
10:ib
11:ic
12:I0
13:p---------------1.732*120*6/4096*ct*pt
14:q
15:cos-------------1.2/4096
16:F---------------60/4096
17:Fx
************************************************************/

/* 设备创建部分 */
static T_IED g_ied_list[] =
{
//	{1,  c103_service, C103_FUN_931A, 0, {1,1,0,0,0,0}, RCS931A},
//	{2,  c103_service, C103_FUN_901A, 0, {2,1,0,0,0,0}, RCS901A},
//	{3,  c103_service, C103_FUN_915A, 0, {3,1,0,0,0,0}, RCS915A},
    {1,  c103_service, C103_FUN_943A, 0, {1, 1, 0, 0, 0, 0}, RCS943A},
    {2,  c103_service, C103_FUN_943A, 0, {2, 1, 0, 0, 0, 0}, RCS943A},
};

BOOL IED_Initialize()
{
    //return DEV_Create_IED (g_ied_list, sizeof (g_ied_list) / sizeof (T_IED));
}
/******************************* RCS-943a保护事件和告警********************************/
static T_EVENT_ENTRY nr_943a_event_table[] =
{
    {26, "重合闸动作      "},
    {168, "电流差动保护动作"},
    {164, "远动启动跳闸动作"},
    {54, "零序过流Ⅰ段动作"},
    {55, "零序过流Ⅱ段动作"},
    {56, "零序过流Ⅲ段动作"},
    {57, "零序过流Ⅳ段动作"},
    {78, "距离Ⅰ段动作    "},
    {79, "距离Ⅱ段动作    "},
    {80, "距离Ⅲ段动作    "},
    {116, "距离加速动作    "},
    {151, "零序加速动作    "},
    {82, "双回线相继速动  "},
    {83, "不对称相继速动  "},
    {66, "TV断线过流Ⅰ段  "},
    {67, "TV断线过流Ⅱ段  "},
    {25, "跳闸            "},
    {182, "启动            "},
};
#define m_943a_evSize (sizeof(nr_943a_event_table)/sizeof(nr_943a_event_table[0]))

static T_ALARM_ENTRY nr_943a_alarm_table[] =
{
    {194, "存储器出错      "},
    {195, "程序出错        "},
    {196, "定值出错        "},
    {49, "该区定值无效    "},
    {50, "CPU采样异常     "},
    {189, "零序长期启动    "},
    {214, "装置长期启动    "},
    {51, "DSP采样异常     "},
    {200, "跳合出口异常    "},
    {222, "定值校验出错    "},
    {44, "直流电源异常    "},
    {202, "光耦电源异常    "},
    {241, "母线电压回路断线"},
    {240, "线路电压回路断线"},
    {206, "TA断线          "},
    {210, "TWJ异常	      "},
    {203, "控制回路回路断线"},
    {184, "过负荷          "},
    {192, "通道异常	      "},
    {227, "长期有差流      "},
    {226, "无对侧数据      "},
    {42, "监视方向闭锁    "},
    {31, "差动保护压板    "},
    {1,  "距离保护压板    "},
    {2,  "零序Ⅰ段压板    "},
    {3,  "零序Ⅱ段压板    "},
    {68, "零序Ⅲ段压板    "},
    {69, "零序Ⅳ段压板    "},
    {7,  "不对称相继速动  "},
    {6,  "双回线相继速动  "},
    {8,  "闭锁重合        "},
    {12, "双回线通道试验  "},
    {87, "合后位置        "},
    {204, "跳闸压力        "},
    {205, "合闸压力        "},
    {91, "Ⅰ母电压        "},
    {92, "Ⅱ母电压        "},
    {9,  "跳闸位置        "},
    {41, "合闸位置1       "},
    {46, "合闸位置2       "},
    {5,  "收相邻线        "},
    {32, "发远跳          "},
    {74, "发远传1         "},
    {75, "发远传2         "},
    {76, "收远传1         "},
    {77, "收远传2         "},

};
#define m_943a_alSize (sizeof(nr_943a_alarm_table)/sizeof(nr_943a_alarm_table[0]))

static T_SET_ENTRY nr_943a_set_table[] =
{
    {0, SET_T_UINT, "保护定值个数    ", " "},
    {1, SET_T_FLOAT, "电流变化量起动值", " "},
    {2, SET_T_FLOAT, "零序启动电流    ", " "},
    {3, SET_T_FLOAT, "负序启动电流    ", " "},
    {4, SET_T_FLOAT, "TA变比系数      ", " "},
    {5, SET_T_FLOAT, "差动电流高定值  ", " "},
    {6, SET_T_FLOAT, "差动电流低定值  ", " "},
    {7, SET_T_FLOAT, "TA断线差流定值  ", " "},
    {8, SET_T_FLOAT, "零序补偿系数    ", " "},
    {9, SET_T_FLOAT, "振荡闭锁过流    ", " "},
    {10, SET_T_FLOAT, "接地距离Ⅰ段定值", " "},
    {11, SET_T_FLOAT, "距离Ⅰ段时间    ", " "},
    {12, SET_T_FLOAT, "接地距离Ⅱ段定值", " "},
    {13, SET_T_FLOAT, "接地距离Ⅱ段时间", " "},
    {14, SET_T_FLOAT, "接地距离Ⅲ段定值", " "},
    {15, SET_T_FLOAT, "接地Ⅲ段四边形  ", " "},
    {16, SET_T_FLOAT, "接地距离Ⅲ段时间", " "},
    {17, SET_T_FLOAT, "相间距离Ⅰ段定值", " "},
    {18, SET_T_FLOAT, "相间距离Ⅱ段定值", " "},
    {19, SET_T_FLOAT, "相间距离Ⅱ段时间", " "},
    {20, SET_T_FLOAT, "相间距离Ⅲ段定值", " "},
    {21, SET_T_FLOAT, "相间Ⅲ段四边形  ", " "},
    {22, SET_T_FLOAT, "相间距离Ⅲ段时间", " "},
    {23, SET_T_FLOAT, "正序灵敏角      ", " "},
    {24, SET_T_FLOAT, "零序灵敏角      ", " "},
    {25, SET_T_FLOAT, "接地距离偏移角  ", " "},
    {26, SET_T_FLOAT, "相间距离偏移角  ", " "},
    {27, SET_T_FLOAT, "零序过流Ⅰ段定值", " "},
    {28, SET_T_FLOAT, "零序过流Ⅰ段时间", " "},
    {29, SET_T_FLOAT, "零序过流Ⅱ段定值", " "},
    {30, SET_T_FLOAT, "零序过流Ⅱ段时间", " "},
    {31, SET_T_FLOAT, "零序过流Ⅲ段定植", " "},
    {32, SET_T_FLOAT, "零序过流Ⅲ段时间", " "},
    {33, SET_T_FLOAT, "零序过流Ⅳ段定植", " "},
    {34, SET_T_FLOAT, "零序过流Ⅳ段时间", " "},
    {35, SET_T_FLOAT, "零序过流加速段  ", " "},
    {36, SET_T_FLOAT, "相电流过负荷定值", " "},
    {37, SET_T_FLOAT, "相电流过负荷时间", " "},
    {38, SET_T_FLOAT, "TV断线过流Ⅰ定值", " "},
    {39, SET_T_FLOAT, "TV断线过流Ⅰ时间", " "},
    {40, SET_T_FLOAT, "TV断线过流Ⅱ定值", " "},
    {41, SET_T_FLOAT, "TV断线过流Ⅱ时间", " "},
    {42, SET_T_FLOAT, "固定角度差定值  ", " "},
    {43, SET_T_FLOAT, "重合闸时间      ", " "},
    {44, SET_T_FLOAT, "同期合闸角      ", " "},
    {45, SET_T_FLOAT, "线路正序电抗    ", " "},
    {46, SET_T_FLOAT, "线路正序电阻    ", " "},
    {47, SET_T_FLOAT, "线路零序电抗    ", " "},
    {48, SET_T_FLOAT, "线路零序电阻    ", " "},
    {49, SET_T_FLOAT, "线路总长度      ", " "},
    {50, SET_T_FLOAT, "线路编号        ", " "},
    {51, SET_T_FLOAT, "投纵联差动保护  ", " "},
    {52, SET_T_FLOAT, "投TV断线闭锁差动", " "},
    {53, SET_T_FLOAT, "投主机方式      ", " "},
    {54, SET_T_FLOAT, "投专用光纤      ", " "},
    {55, SET_T_FLOAT, "投通道自环试验  ", " "},
    {56, SET_T_FLOAT, "投远跳受本侧控制", " "},
    {57, SET_T_FLOAT, "投振荡闭锁	 ", " "},
    {58, SET_T_FLOAT, "投Ⅰ段接地距离  ", " "},
    {59, SET_T_FLOAT, "投Ⅱ段接地距离  ", " "},
    {60, SET_T_FLOAT, "投Ⅲ段接地距离  ", " "},
    {61, SET_T_FLOAT, "投Ⅰ段相间距离  ", " "},
    {62, SET_T_FLOAT, "投Ⅱ段相间距离  ", " "},
    {63, SET_T_FLOAT, "投Ⅲ段相间距离  ", " "},
    {64, SET_T_FLOAT, "重合加速Ⅱ段距离", " "},
    {65, SET_T_FLOAT, "重合加速Ⅲ段距离", " "},
    {66, SET_T_FLOAT, "双回线相继速动  ", " "},
    {67, SET_T_FLOAT, "不对称相继速动  ", " "},
    {68, SET_T_FLOAT, "投Ⅰ段零序方向  ", " "},
    {69, SET_T_FLOAT, "投Ⅱ段零序方向  ", " "},
    {70, SET_T_FLOAT, "投Ⅲ段零序方向  ", " "},
    {71, SET_T_FLOAT, "投Ⅳ段零序方向  ", " "},
    {72, SET_T_FLOAT, "投相电流过负荷  ", " "},
    {73, SET_T_FLOAT, "投重合闸        ", " "},
    {74, SET_T_FLOAT, "投检同期方式    ", " "},
    {75, SET_T_FLOAT, "投线无压母有压  ", " "},
    {76, SET_T_FLOAT, "投母无压线有压  ", " "},
    {77, SET_T_FLOAT, "投线无压母无压  ", " "},
    {78, SET_T_FLOAT, "投重合闸不检    ", " "},
    {79, SET_T_FLOAT, "TV断线留零Ⅰ段  ", " "},
    {80, SET_T_FLOAT, "TV断线闭锁重合  ", " "},
    {81, SET_T_FLOAT, "Ⅲ段及以上闭重  ", " "},
    {82, SET_T_FLOAT, "投多相故障闭重  ", " "},
};
#define m_943a_setSize (sizeof(nr_943a_set_table)/sizeof(nr_943a_set_table[0]))

/******************************* RCS-901a保护事件和告警********************************/
static T_EVENT_ENTRY nr_901a_event_table[] =
{
    {26, "重合闸动作      "},
    {169, "纵联变化量方向  "},
    {166, "纵联零序方向    "},
    {113, "工频变化量阻抗  "},
    {78, "距离Ⅰ段动作	  "},
    {79, "距离Ⅱ段动作	  "},
    {80, "距离Ⅲ段动作	  "},
    {55, "零序过流Ⅱ段	  "},
    {56, "零序过流Ⅲ段	  "},
    {114, "非全相运行故障  "},
    {116, "距离加速		  "},
    {151, "零序加速		  "},
    {88, "选相无效三跳	  "},
    {89, "单跳失败三跳	  "},
    {90, "单相运行三跳	  "},
    {63, "TV断线过流	  "},
    {20, "跳A		  "},
    {21, "跳B		  "},
    {22, "跳C		  "},
    {182, "起动		  "},

};
#define m_901a_evSize (sizeof(nr_901a_event_table)/sizeof(nr_901a_event_table[0]))

static T_ALARM_ENTRY nr_901a_alarm_table[] =
{
    {194, "存储器出错  "},
    {195, "程序出错      "},
    {196, "定值出错      "},
    {49, "该区定值无效    "},
    {50, "CPU采样异常     "},
    {189, "零序长期起动    "},
    {214, "装置长期起动    "},
    {51, "DSP采样异常     "},
    {200, "跳合出口异常    "},
    {222, "定值校验出错    "},
    {44, "直流电源异常    "},
    {202, "光耦电源异常    "},
    {241, "母线电压回路断线"},
    {240, "线路电压回路断线"},
    {206, "TA断线	      "},
    {210, "TWJ异常	      "},
    {212, "跳闸开入异常    "},
    {192, "通道异常	      "},
    {42 , "监视方向闭锁    "},

};
#define m_901a_alSize (sizeof(nr_901a_alarm_table)/sizeof(nr_901a_alarm_table[0]))

static T_SET_ENTRY nr_901a_set_table[] =
{
    {0, SET_T_UINT, "保护定值个数    ", " "},
    {1, SET_T_FLOAT, "电流变化量起动值", " "},
    {2, SET_T_FLOAT, "零序启动电流    ", " "},
    {3, SET_T_FLOAT, "工频变化量阻抗  ", " "},
    {4, SET_T_FLOAT, "超范围变化量阻抗", " "},
    {5, SET_T_FLOAT, "零序方向过流定值", " "},
    {6, SET_T_FLOAT, "通道交换时间定值", " "},
    {7, SET_T_FLOAT, "零序补偿系数    ", " "},
    {8, SET_T_FLOAT, "振荡闭锁过流    ", " "},
    {9, SET_T_FLOAT, "接地距离I段定值 ", " "},
    {10, SET_T_FLOAT, "接地距离II段定值", " "},
    {11, SET_T_FLOAT, "接地距离II段时间", " "},
    {12, SET_T_FLOAT, "接地距离Ⅲ段定值", " "},
    {13, SET_T_FLOAT, "接地距离Ⅲ段时间", " "},
    {14, SET_T_FLOAT, "相间距离I段定值", " "},
    {15, SET_T_FLOAT, "相间距离II段定值", " "},
    {16, SET_T_FLOAT, "相间距离II段时间", " "},
    {17, SET_T_FLOAT, "相间距离Ⅲ段定值", " "},
    {18, SET_T_FLOAT, "相间距离Ⅲ段时间", " "},
    {19, SET_T_FLOAT, "负荷限制电阻定值", " "},
    {20, SET_T_FLOAT, "正序灵敏角      ", " "},
    {21, SET_T_FLOAT, "零序灵敏角    ", " "},
    {22, SET_T_FLOAT, "接地距离偏移角  ", " "},
    {23, SET_T_FLOAT, "相间距离偏移角  ", " "},
    {24, SET_T_FLOAT, "零序过流II段定值", " "},
    {25, SET_T_FLOAT, "零序过流II段时间", " "},
    {26, SET_T_FLOAT, "零序过流Ⅲ段定植", " "},
    {27, SET_T_FLOAT, "零序过流Ⅲ段时间", " "},
    {28, SET_T_FLOAT, "零序过流加速段", " "},
    {29, SET_T_FLOAT, "TV断线相过流定值", " "},
    {30, SET_T_FLOAT, "TV断线时零序过流", " "},
    {31, SET_T_FLOAT, "TV断线时过流时间", " "},
    {32, SET_T_FLOAT, "单相重合闸时间", " "},
    {33, SET_T_FLOAT, "三相重合闸时间  ", " "},
    {34, SET_T_FLOAT, "同期合闸角      ", " "},
    {35, SET_T_FLOAT, "线路正序电抗", " "},
    {36, SET_T_FLOAT, "线路正序电阻", " "},
    {37, SET_T_FLOAT, "线路零序电抗", " "},
    {38, SET_T_FLOAT, "线路零序电阻    ", " "},
    {39, SET_T_FLOAT, "线路总长度      ", " "},
    {40, SET_T_FLOAT, "线路编号        ", " "},
    {41, SET_T_FLOAT, "投工频变化量阻抗", " "},
    {42, SET_T_FLOAT, "投纵联变化量方向", " "},
    {43, SET_T_FLOAT, "投纵联零序方向  ", " "},
    {44, SET_T_FLOAT, "投方向补偿阻抗  ", " "},
    {45, SET_T_FLOAT, "允许式通道      ", " "},
    {46, SET_T_FLOAT, "投自动通道交换  ", " "},
    {47, SET_T_FLOAT, "弱电源侧	 ", " "},
    {48, SET_T_FLOAT, "电压接线路TV  ", " "},
    {49, SET_T_FLOAT, "投振荡闭锁元件  ", " "},
    {50, SET_T_FLOAT, "投I段接地距离   ", " "},
    {51, SET_T_FLOAT, "投II段接地距离  ", " "},
    {52, SET_T_FLOAT, "投III段接地距离 ", " "},
    {53, SET_T_FLOAT, "投I段相间距离", " "},
    {54, SET_T_FLOAT, "投II段相间距离", " "},
    {55, SET_T_FLOAT, "投III段相间距离", " "},
    {56, SET_T_FLOAT, "投负荷限制距离", " "},
    {57, SET_T_FLOAT, "三重加速II段距离", " "},
    {58, SET_T_FLOAT, "三重加速Ⅲ段距离", " "},
    {59, SET_T_FLOAT, "零序Ⅲ段经方向", " "},
    {60, SET_T_FLOAT, "零Ⅲ跳闸后加速 ", " "},
    {61, SET_T_FLOAT, "投三相跳闸方式 ", " "},
    {62, SET_T_FLOAT, "投重合闸 ", " "},
    {63, SET_T_FLOAT, "投检同期方式 ", " "},
    {64, SET_T_FLOAT, "投检无压方式  ", " "},
    {65, SET_T_FLOAT, "投重合闸不检", " "},
    {66, SET_T_FLOAT, "不对应启动重合", " "},
    {67, SET_T_FLOAT, "相间距离II闭重", " "},
    {68, SET_T_FLOAT, "接地距离II闭重", " "},
    {69, SET_T_FLOAT, "零II段三跳闭重", " "},
    {70, SET_T_FLOAT, "投选相无效闭重", " "},
    {71, SET_T_FLOAT, "非全相故障闭重", " "},
    {72, SET_T_FLOAT, "投多相故障闭重", " "},
    {73, SET_T_FLOAT, "投三相故障闭重  ", " "},
    {74, SET_T_FLOAT, "内重合把手有效  ", " "},
    {75, SET_T_FLOAT, "投单重方式	 ", " "},
    {76, SET_T_FLOAT, "投三重方式      ", " "},
    {77, SET_T_FLOAT, "投综重方式      ", " "},
};
#define m_901a_setSize (sizeof(nr_901a_set_table)/sizeof(nr_901a_set_table[0]))

/******************************* 915A保护事件和告警********************************/
static T_EVENT_ENTRY nr_915a_event_table[] =
{
    {16 , "线路跟跳        "},
    {17 , "跟跳线路1       "},
    {18 , "跟跳线路2       "},
    {19 , "跟跳线路3       "},
    {21 , "跟跳线路4       "},
    {22 , "跟跳线路5       "},
    {23 , "跟跳线路6       "},
    {24 , "跟跳线路7       "},
    {25 , "跟跳线路8       "},
    {26 , "跟跳线路9       "},
    {27 , "跟跳线路10      "},
    {28 , "跟跳线路11      "},
    {29 , "跟跳线路12      "},
    {30 , "跟跳线路13      "},
    {31 , "跟跳线路14      "},
    {32 , "跟跳线路15      "},
    {33 , "跟跳线路16      "},
    {34 , "跟跳线路17      "},
    {35 , "跟跳线路18      "},
    {36 , "跟跳线路19      "},
    {37 , "跟跳线路20      "},
    {14 , "母差后备动作    "},
    {116, "失灵跳一母      "},
    {117, "失灵跳二母      "},
    {69 , "A相跳闸         "},
    {70 , "B相跳闸         "},
    {71 , "C相跳闸         "},
    {153, "母差跳一母      "},
    {154, "母差跳一母      "},
    {158, "充电跳母联      "},
    {157, "过流跳母联      "},
    {163, "母联失灵        "},

};
#define m_915a_evSize (sizeof(nr_915a_event_table)/sizeof(nr_915a_event_table[0]))

static T_ALARM_ENTRY nr_915a_alarm_table[] =
{
    {194, "内存出错  "},
    {195, "程序区出错      "},
    {196, "定值区出错      "},
    {201, "跳闸出口报警    "},
    {202, "光耦失电        "},
    {211, "内部通讯出错    "},
    {214, "装置长期启动报警"},
    {231, "一母TV断线      "},
    {232, "二母TV断线      "},
    {210, "TWJ报警         "},
    {206, "TA断线	      "},
    {209, "刀闸位置报警    "},
    {103, "母联TA断线报警  "},
    {104, "该区定值无效    "},
    {81 , "修改定值        "},
    {82 , "DSP报警         "},
    {135, "I母电压闭锁开放 "},
    {136, "II母电压闭锁开放"},
};
#define m_915a_alSize (sizeof(nr_915a_alarm_table)/sizeof(nr_915a_alarm_table[0]))

static T_SET_ENTRY nr_915a_set_table[] =
{

    {0, SET_T_UINT, "保护定值个数  ", " "},
    {1, SET_T_FLOAT, "差动启动电流高值", " "},
    {2, SET_T_FLOAT, "差动启动电流低值", " "},
    {3, SET_T_FLOAT, "大差比率制动系数", " "},
    {4, SET_T_FLOAT, "比率制动系数高值", " "},
    {5, SET_T_FLOAT, "比率制动系数低值", " "},
    {6, SET_T_FLOAT, "充电保护电流定值", " "},
    {7, SET_T_FLOAT, "母联过流电流定值", " "},
    {8, SET_T_FLOAT, "母联过流零序定值", " "},
    {9, SET_T_FLOAT, "母联过流时间定值", " "},
    {10, SET_T_FLOAT, "非全相零序定值  ", " "},
    {11, SET_T_FLOAT, "非全相负序定值  ", " "},
    {12, SET_T_FLOAT, "非全相时间定值  ", " "},
    {13, SET_T_FLOAT, "TA断线电流定值  ", " "},
    {14, SET_T_FLOAT, "TA异常电流定值  ", " "},
    {15, SET_T_FLOAT, "母差相低电压闭锁", " "},
    {16, SET_T_FLOAT, "母差零序电压闭锁", " "},
    {17, SET_T_FLOAT, "母差负序电压闭锁", " "},
    {18, SET_T_FLOAT, "母联失灵电压定值", " "},
    {19, SET_T_FLOAT, "母联失灵电压定值", " "},
    {20, SET_T_FLOAT, "投入母差保护    ", " "},
    {21, SET_T_FLOAT, "投入失灵保护    ", " "},
    {22, SET_T_FLOAT, "投入母联过流    ", " "},
    {23, SET_T_FLOAT, "投入单母方式    ", " "},
    {24, SET_T_FLOAT, "投I母TV         ", " "},
    {25, SET_T_FLOAT, "投II母TV        ", " "},
    {26, SET_T_FLOAT, "投充电闭锁母差  ", " "},
    {27, SET_T_FLOAT, "投TV断线自动恢复", " "},
};
#define m_915a_setSize (sizeof(nr_915a_set_table)/sizeof(nr_915a_set_table[0]))

/************************************************************************/
/*                  RCS-931A                                            */
/************************************************************************/
static T_EVENT_ENTRY nr_931a_event_table[] =
{
    {26, "重合闸动作	  "},
    {168, "电流差动保护	  "},
    {164, "远方起动跳闸	  "},
    {113, "工频变化量阻抗  "},
    {78, "距离Ⅰ段动作	  "},
    {79, "距离Ⅱ段动作	  "},
    {80, "距离Ⅲ段动作	  "},
    {55, "零序过流Ⅱ段	  "},
    {56, "零序过流Ⅲ段	  "},
    {114, "非全相运行故障  "},
    {116, "距离加速		  "},
    {151, "零序加速		  "},
    {88, "选相无效三跳	  "},
    {89, "单跳失败三跳	  "},
    {90, "单相运行三跳	  "},
    {63, "TV断线过流	  "},
    {20, "跳A		  "},
    {21, "跳B		  "},
    {22, "跳C		  "},
    {182, "起动		  "},
};
#define m_931a_evSize (sizeof(nr_931a_event_table)/sizeof(nr_931a_event_table[0]))

static T_ALARM_ENTRY nr_931a_alarm_table[] =
{
    {194, "存储器出错      "},
    {195, "程序出错        "},
    {196, "定值出错        "},
    {49, "该区定值无效    "},
    {50, "CPU采样异常     "},
    {189, "零序长期起动    "},
    {214, "装置长期起动    "},
    {51, "DSP采样异常  "},
    {200, "跳合出口异常    "},
    {222, "定值校验出错    "},
    {44, "直流电源异常    "},
    {202, "光耦电源异常  "},
    {241, "母线电压回路断线"},
    {240, "线路电压回路断线"},
    {206, "TA断线	  "},
    {210, "TWJ异常	  "},
    {212, "跳闸开入异常  "},
    {192, "通道异常	  "},
    {247, "容抗整定出错  "},
    {227, "长期有差流  "},
    {226, "无对侧数据  "},
    {42 , "监视方向闭锁  "},
};
#define m_931a_alSize (sizeof(nr_931a_alarm_table)/sizeof(nr_931a_alarm_table[0]))

static T_SET_ENTRY nr_931a_set_table[] =
{
    {0, SET_T_UINT, "保护定值个数    ", " "},
    {1, SET_T_FLOAT, "电流变化量起动值", " "},
    {2, SET_T_FLOAT, "零序启动电流", " "},
    {3, SET_T_FLOAT, "工频变化量阻抗", " "},
    {4, SET_T_FLOAT, "TA变比系数", " "},
    {5, SET_T_FLOAT, "差动电流高定值", " "},
    {6, SET_T_FLOAT, "差动电流低定值", " "},
    {7, SET_T_FLOAT, "TA断线差流定值", " "},
    {8, SET_T_FLOAT, "零序补偿系数", " "},
    {9, SET_T_FLOAT, "振荡闭锁过流", " "},
    {10, SET_T_FLOAT, "接地距离I段定值", " "},
    {11, SET_T_FLOAT, "接地距离II段定值", " "},
    {12, SET_T_FLOAT, "接地距离II段时间", " "},
    {13, SET_T_FLOAT, "接地距离Ⅲ段定值", " "},
    {14, SET_T_FLOAT, "接地距离Ⅲ段时间", " "},
    {15, SET_T_FLOAT, "相间距离I段定值", " "},
    {16, SET_T_FLOAT, "相间距离II段定值", " "},
    {17, SET_T_FLOAT, "相间距离II段时间", " "},
    {18, SET_T_FLOAT, "相间距离Ⅲ段定值", " "},
    {19, SET_T_FLOAT, "相间距离Ⅲ段时间", " "},
    {20, SET_T_FLOAT, "负荷限制电阻定值", " "},
    {21, SET_T_FLOAT, "正序灵敏角", " "},
    {22, SET_T_FLOAT, "零序灵敏角    ", " "},
    {23, SET_T_FLOAT, "接地距离偏移角 ", " "},
    {24, SET_T_FLOAT, "相间距离偏移角 ", " "},
    {25, SET_T_FLOAT, "零序过流II段定值", " "},
    {26, SET_T_FLOAT, "零序过流II段时间", " "},
    {27, SET_T_FLOAT, "零序过流Ⅲ段定植", " "},
    {28, SET_T_FLOAT, "零序过流Ⅲ段时间", " "},
    {29, SET_T_FLOAT, "零序过流加速段", " "},
    {30, SET_T_FLOAT, "TV断线相过流定值", " "},
    {31, SET_T_FLOAT, "TV断线时零序过流", " "},
    {32, SET_T_FLOAT, "TV断线时过流时间", " "},
    {33, SET_T_FLOAT, "单相重合闸时间", " "},
    {34, SET_T_FLOAT, "三相重合闸时间  ", " "},
    {35, SET_T_FLOAT, "同期合闸角      ", " "},
    {36, SET_T_FLOAT, "线路正序电抗", " "},
    {37, SET_T_FLOAT, "线路正序电阻", " "},
    {38, SET_T_FLOAT, "线路正序容抗", " "},
    {39, SET_T_FLOAT, "线路零序电抗", " "},
    {40, SET_T_FLOAT, "线路零序电阻", " "},
    {41, SET_T_FLOAT, "线路零序容抗    ", " "},
    {42, SET_T_FLOAT, "线路总长度    ", " "},
    {43, SET_T_FLOAT, "线路编号    ", " "},
    {44, SET_T_FLOAT, "工频变化阻抗投入", " "},
    {45, SET_T_FLOAT, "纵联差动投入    ", " "},
    {46, SET_T_FLOAT, "TA断闭锁差动投入", " "},
    {47, SET_T_FLOAT, "主机方式	", " "},
    {48, SET_T_FLOAT, "专用光纤", " "},
    {49, SET_T_FLOAT, "通道自环试验", " "},
    {50, SET_T_FLOAT, "远跳受本侧控制", " "},
    {51, SET_T_FLOAT, "电压接线路TV  ", " "},
    {52, SET_T_FLOAT, "投振荡闭锁元件  ", " "},
    {53, SET_T_FLOAT, "投I段接地距离   ", " "},
    {54, SET_T_FLOAT, "投II段接地距离  ", " "},
    {55, SET_T_FLOAT, "投III段接地距离 ", " "},
    {56, SET_T_FLOAT, "投I段相间距离", " "},
    {57, SET_T_FLOAT, "投II段相间距离", " "},
    {58, SET_T_FLOAT, "投III段相间距离", " "},
    {59, SET_T_FLOAT, "投负荷限制距离", " "},
    {60, SET_T_FLOAT, "三重加速II段距离", " "},
    {61, SET_T_FLOAT, "三重加速Ⅲ段距离", " "},
    {62, SET_T_FLOAT, "零序Ⅲ段经方向", " "},
    {63, SET_T_FLOAT, "零Ⅲ跳闸后加速 ", " "},
    {64, SET_T_FLOAT, "投三相跳闸方式 ", " "},
    {65, SET_T_FLOAT, "投重合闸 ", " "},
    {66, SET_T_FLOAT, "投检同期方式 ", " "},
    {67, SET_T_FLOAT, "投检无压方式  ", " "},
    {68, SET_T_FLOAT, "投重合闸不检", " "},
    {69, SET_T_FLOAT, "不对应启动重合", " "},
    {70, SET_T_FLOAT, "相间距离II闭重", " "},
    {71, SET_T_FLOAT, "接地距离II闭重", " "},
    {72, SET_T_FLOAT, "零II段三跳闭重", " "},
    {73, SET_T_FLOAT, "投选相无效闭重", " "},
    {74, SET_T_FLOAT, "非全相故障闭重", " "},
    {75, SET_T_FLOAT, "投多相故障闭重", " "},
    {76, SET_T_FLOAT, "投三相故障闭重  ", " "},
    {77, SET_T_FLOAT, "内重合把手有效  ", " "},
    {78, SET_T_FLOAT, "投单重方式	 ", " "},
    {79, SET_T_FLOAT, "投三重方式      ", " "},
    {80, SET_T_FLOAT, "投综重方式      ", " "},
};
#define m_931a_setSize (sizeof(nr_931a_set_table)/sizeof(nr_931a_set_table[0]))

/* 保护类型 */
T_C103_CODE_TABLE g_c103_code[] =
{
    {RCS943A, nr_943a_event_table, nr_943a_alarm_table, nr_943a_set_table, m_943a_evSize, m_943a_alSize, m_943a_setSize},
    {RCS915A, nr_915a_event_table, nr_915a_alarm_table, nr_915a_set_table, m_915a_evSize, m_915a_alSize, m_915a_setSize},
    {RCS901A, nr_901a_event_table, nr_901a_alarm_table, nr_901a_set_table, m_901a_evSize, m_901a_alSize, m_901a_setSize},
    {RCS931A, nr_931a_event_table, nr_931a_alarm_table, nr_931a_set_table, m_931a_evSize, m_931a_alSize, m_931a_setSize},
    {RCS9607},
};
WORD g_c103_code_size = sizeof (g_c103_code) / sizeof (T_C103_CODE_TABLE);

