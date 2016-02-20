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
#include <dlfcn.h> /* dlopen��dlsym��dlerror��dlclose��ͷ�ļ� */

#include "modbus.h"

#define     MAX_DATE_LEN            100 // modbusһ�βɼ�������ݳ���(���ֽڸ���)
#define     MYSELF_MAX_TAGID        100 // ort3000cԤ�������TAGID
#define     MODBUSFRAME_LENGH_MAX   50  // Modbus֡��󳤶�
#define     SERIES_MAX              7   // �����������
#define     SHM_MAX                 50  // �����������ֽ���

#define     DATABASE_CHANGED_CMD	   2	// ֪ͨ�ɼ������ݿ������Ѿ��ı䣬��Ҫ���¶�ȡ���

#define    	MAX_TIMES_USED_LOW_SPEED   5*60*1000	// ����ģʽ�������ʱ��5����
#define    	MODBUS_SPACE_LEVEL0        3	// ���߿���ʱ��ȼ�
#define    	MODBUS_SPACE_LEVEL1        4	// ���߿���ʱ��ȼ�
#define    	MODBUS_SPACE_LEVEL2        5	// ���߿���ʱ��ȼ�
#define    	MODBUS_SPACE_LEVEL3        6	// ���߿���ʱ��ȼ�
#define    	MODBUS_SPACE_LEVEL4        7	// ���߿���ʱ��ȼ�

#define  DEBUG_MODE        0X01    // ��ӡ������Ϣ
#define  DEBUG_MODBUS      0X02    // ��ӡmodbus֡
#define  DEBUG_TAG         0X04    // ��ӡ�����Ϣ
#define  DEBUG_STEP        0X10    // ����ģʽ
#define  DEBUG_CSV         0X20    // дCSV�ļ�

//#define    	MODBUS_TYPE_STANDARD       1	// ��׼modbus 
//#define    	MODBUS_TYPE_RPC3FXC        2	// RPC3FXC(���ǹ�����)modbus
//#define    	MODBUS_TYPE_LD-B10-220     3	// LD-B10-220ϵ�и�ʽ��ѹ���¶ȿ�����modbus

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
    struct timeb time_response; // ��¼�豸��Ӧʱ��ʱ���
    struct timeb time_timeout; // ��¼�豸��ʱʱ��ʱ���
} TagTp;

struct CollectTagDataTp
{
    char                 fd[50]; //serial file node
    modbus_param_t       mb_param;
    pthread_t            thread_id; //thread ID
    pthread_mutex_t      mutex; //����ң�ز���ʱ��ͣmodbus�ɼ�����
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
