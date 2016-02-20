#ifndef _COLLECT_H
#define _COLLECT_H

#include <fcntl.h>
#include <sys/wait.h>
#include <netinet/in.h>  /* for sockaddr_in */
#include <sys/types.h>  /* for sockeat */
#include <sys/socket.h>  /* for socket */
#include <netinet/tcp.h>  /* for TCP_NODELAY */
#include <stdio.h>  /* for printf */
#include <stdlib.h>  /* for exit */
#include <string.h>
#include <pthread.h>
#include <sys/errno.h>
#include <semaphore.h>  /* for sem */
#include <netdb.h>
#include <arpa/inet.h>  /* inet_ntoa */
#include <netinet/in.h>
#include <unistd.h>  /* fork, close,  access */
#include <dirent.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/timeb.h>
#include <ctype.h>
#include <signal.h>
#include <vector>
#include <queue>
#include <iostream>
#include <stdint.h>
#include <getopt.h>
#include <dlfcn.h> /* dlopen、dlsym、dlerror、dlclose的头文件 */

#include "modbus.h"

#define     MAX_DATE_LEN            100 // modbus一次采集最大数据长度(非字节个数)
#define     MYSELF_MAX_TAGID        100 // ort3000c预留的最大TAGID
#define     MODBUSFRAME_LENGH_MAX   50  // Modbus帧最大长度
#define     SERIES_MAX              7   // 串口最大数量
#define     SHM_MAX                 50  // 共享类存最大字节数

#define     DATABASE_CHANGED_CMD	   2	// 通知采集器数据库配置已经改变，需要重新读取点表

#define    	MAX_TIMES_USED_LOW_SPEED   5*60*1000	// 低速模式运行最大时间5分钟
#define    	MODBUS_SPACE_LEVEL0        3	// 总线空闲时间等级
#define    	MODBUS_SPACE_LEVEL1        4	// 总线空闲时间等级
#define    	MODBUS_SPACE_LEVEL2        5	// 总线空闲时间等级
#define    	MODBUS_SPACE_LEVEL3        6	// 总线空闲时间等级
#define    	MODBUS_SPACE_LEVEL4        7	// 总线空闲时间等级

#define  DEBUG_MODE        0X01    // 打印调试信息
#define  DEBUG_MODBUS      0X02    // 打印modbus帧
#define  DEBUG_TAG         0X04    // 打印测点信息
#define  DEBUG_STEP        0X10    // 单步模式
#define  DEBUG_CSV         0X20    // 写CSV文件

//#define    	MODBUS_TYPE_STANDARD       1	// 标准modbus 
//#define    	MODBUS_TYPE_RPC3FXC        2	// RPC3FXC(华星功补仪)modbus
//#define    	MODBUS_TYPE_LD-B10-220     3	// LD-B10-220系列干式变压器温度控制器modbus

#define COLLECT_CONF_FILE   "/etc/collect.conf"


/* collect struct */
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
typedef unsigned char  BYTE;
typedef unsigned long  DWORD;
typedef unsigned short WORD;
typedef void  VOID;
typedef bool  BOOL;

typedef struct _TagTp
{
    uint16_t        Id; /* tag ID */
    uint8_t         ModBusQueryFrame[MODBUSFRAME_LENGH_MAX];
    uint16_t        QueryFrameLen;
    int             ModbusType;
    uint8_t         DataType;
    uint16_t        TimeOut;
    uint16_t        SpaceTime;
    uint8_t         Opertion;
    float           K;
    struct timeb time_response; // 记录设备响应时的时间戳
    struct timeb time_timeout; // 记录设备超时时的时间戳
} TagTp;

struct CollectTagDataTp
{
    char                 fd[50]; //serial file node
    modbus_param_t       mb_param;
    pthread_t            thread_id; //thread ID
    pthread_mutex_t      mutex; //用于遥控操作时暂停modbus采集动作
    std::vector<TagTp>   Tag;
};

#define MAX_DL 500
struct dl_type
{
    void *handle;
    int (*run) (void *arg);
    int (*exit) (void *arg);
    int (*write_to_rtu) (void *arg);
    int (*set_display) (void *arg);
    bool is_display;
    pthread_t thread;
};

typedef struct auto_control
{
    int condition_id;
    int condition_operator; /* 0: ==, 1: >, 2: >=, 3: <, 4: <= */
    int condition_value;
    int control_id;
    int control_value;

} auto_control_tp;
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static void handle_pipe (int sig);
static int init_collect();
static int load_series_config();
static int load_taglist();
static void *modbus_collect (void * SeriesNo);
static void *read_cmd (void *data);
static int db1_connect();
static int db2_connect();
static int db1_disconnect();
static int db2_disconnect();
static int cha_biao (unsigned char series, unsigned char device_addr,
                     unsigned char tag_type, unsigned int data_addr, unsigned int *nTagNo,
                     float *modulus, uint8_t *DataType, uint8_t *Opertion);
static void init_sigaction();
static void handle_pipe (int sig);
static void handle_quit (int sig);
static void handle_alarm (int sig);
static void pthread_quit();
static void handle_segv (int sig);
static void handle_int (int sig);
static void printf_msg (void);
static void init_time (int time);
static int DataType (int TagID, int TagType);

int get_comno_slave_addr (int TagID, int *com, int *slave, int *fun, int *addr, int *value);
int get_ort3000_switch_attr (int PID, int *switch_type, int *delay_time);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
