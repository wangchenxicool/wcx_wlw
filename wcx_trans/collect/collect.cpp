/*================================================================================
              _  _              _
  ___   ___  | || |  ___   ___ | |_
 / __| / _ \ | || | / _ \ / __|| __|
| (__ | (_) || || ||  __/| (__ | |_
 \___| \___/ |_||_| \___| \___| \__|  --- v0.1.0

================================================================================*/
#include <fcntl.h>
#include <sys/wait.h>
#include <netinet/in.h> /* for sockaddr_in */
#include <sys/types.h> /* for sockeat */
#include <sys/socket.h> /* for socket */
#include <netinet/tcp.h> /* for TCP_NODELAY */
#include <stdio.h> /* for printf */
#include <stdlib.h> /* for exit */
#include <string.h>
#include <pthread.h>
#include <sys/errno.h>
#include <semaphore.h> /* for sem */
#include <netdb.h>
#include <arpa/inet.h> /* inet_ntoa */
#include <netinet/in.h>
#include <unistd.h> /* fork, close, access */
#include <sys/types.h>
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
#include <sqlite3.h>
#include <dlfcn.h> /* dlopen、dlsym、dlerror、dlclose的头文件 */
#include <asm/ioctl.h>
#include <sys/ioctl.h>

#include "TimeUse.h"
#include "INIFileOP.h"
#include "collect.h"
#include "crc16.h"
#include "wcx_log.h"
#include "wcx_utils.h"
#include "dmalloc.h"
//#include "memwatch.h"

#include "myself_id.h"
#include "modbus_type.h"
#include "data_type.h"

#include "iec104_master.h"
#include "iec104_slave.h"
#include "modbus.h"
#include "modbus_RPC3FxC.h"
#include "modbus_LD_B10_220.h"

using namespace std;

#define ADC_INPUT_PIN       _IOW('S', 0x0c, unsigned long)
#define WCX_ARR_LEN(arr)    (sizeof(arr)/sizeof(arr[0]))

//#define  WRITE_LOG
//#define  IEC104_MASTER_CMD_RCV
//#define  PRINT_TIME_USE
//#define  PRINT_VALUE
//#define  PRINT_TAG

#define DEBUG
#ifdef DEBUG
#define wprintf(format, arg...)           \
    if (DEBUG_FLG & DEBUG_MODE)           \
        printf (format , ##arg)
#else
#define wprintf(format, arg...) {}
#endif

#define tprintf(format, arg...)           \
    if (DEBUG_FLG & DEBUG_TAG)            \
        printf (format , ##arg)


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/* Global Variable */
static struct CollectTagDataTp      Serial[SERIES_MAX];
struct CollectTagDataTp             OptSerial[SERIES_MAX];
static struct CollectTagDataTp      myself;
uint8_t                             kaichu_value; //ort3000开出量初值

static char                         g_boot_log[50];
static char                         g_err_log[50];
static char                         g_taglist_log[50];

static char                         g_db_ip[50];
static char                         g_db_port[20];
static char                         g_path_fast_tag[128];
static char                         g_path_db[128];
static unsigned int                 g_space_time;
static volatile char                g_thread_exit_flg;
static int                          g_modbus_space_level;
static struct                       dl_type dl[MAX_DL];
static int                          dl_nums = 0;

static char *shm_head_watch_dog, *shm_pos_watch_dog;
static char *shm_head_myself_check, *shm_pos_myself_check;

static struct log_tp    error_log;
static struct log_tp    boot_log;

TimeUse low_speed; /* 记录低速模式时间 */

std::vector<auto_control_tp>   auto_control; /* 自动控制 */

/* conf file */
static int      DEBUG_FLG;
static int      DEVICE_OFFLINE_TIME = 5 * 50; /* 当设备超过此时间无响应, 则写设备掉线报警! */

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#define WCX_DEBUG(fmt, ...)     wcx_debug(__FILE__, __LINE__, fmt, __VA_ARGS__)
static void wcx_debug (const char *file, int line, const char *fmt, ...)
{
    va_list ap;

    if (DEBUG_FLG & DEBUG_MODE)
    {
        fprintf (stderr, "\033[0;31;40mDEBUG\a\033[0m(%s:%d:%d): ", file, line, getpid());
        va_start (ap, fmt);
        vfprintf (stderr, fmt, ap);
        va_end (ap);
    }
}

static void pthread_quit()
{
    int i;
    write_log (&error_log, -1, "pthread_quit()", "pthread_quit");
#if 0
    //-->退出线程: (方式1)
    //pthread_kill(g_thread_read_cmd, SIGKILL);
    pthread_kill (modbus_collect_thread, SIGKILL);
    pthread_kill (global_thread_receive, SIGKILL);
#endif

#if 0
    //-->退出线程: (方式2)
    //pthread_cancel(g_thread_read_cmd);
    pthread_cancel (modbus_collect_thread);
    pthread_cancel (global_thread_receive);
#endif

#if 1
    //-->退出线程: (方式3)
    for (i = 0; i < dl_nums; i++)
    {
        dl[i].exit (NULL);
    }
    g_thread_exit_flg = 1; // 通知线程退出
    MySleep (2, 0);
    exit (1);
#endif
}

static int cha_biao (unsigned char series, unsigned char device_addr, unsigned char tag_type,
                     unsigned int data_addr, unsigned int *nTagNo, float *modulus, uint8_t *DataType, uint8_t *Opertion)
{
    if (series > SERIES_MAX - 1)
        return -1;
    for (size_t i = 0; i < Serial[series].Tag.size(); i++)
    {
        BYTE device_addr_cur = Serial[series].Tag[i].ModBusQueryFrame[0];
        BYTE tag_type_cur = Serial[series].Tag[i].ModBusQueryFrame[1];
        WORD data_addr_cur = (WORD) Serial[series].Tag[i].ModBusQueryFrame[2] << 8 |
                             (WORD) Serial[series].Tag[i].ModBusQueryFrame[3];
        if (device_addr == device_addr_cur && tag_type == tag_type_cur && data_addr == data_addr_cur)
        {
            *nTagNo = Serial[series].Tag[i].Id;
            *modulus = Serial[series].Tag[i].K;
            *DataType = Serial[series].Tag[i].DataType;
            *Opertion = Serial[series].Tag[i].Opertion;
            return (1);
        }
    }
    return -2;
}

static void handle_pipe (int sig)
{
    WCX_DEBUG ("\033[0;31;40mSIGPIPE!\a\033[0m%s", "\n");
    write_log (&error_log, -1, "handle_pipe()", "receive SIGPIPE");
    pthread_quit();
}

static void handle_quit (int   sig)
{
    printf ("\033[0;31;40mSIGTERM!\a\033[0m\n");
    write_log (&error_log, -1, "handle_quit()", "receive SIGTERM");
    pthread_quit();
}

static void handle_segv (int   sig)
{
    WCX_DEBUG ("\033[0;31;40mSIGSEGV!\a\033[0m%s", "\n");
    write_log (&error_log, -1, "handle_segv()", "receive SIGSEGV");
    pthread_quit();
    MySleep (5, 0);
    exit (1);
}

static void handle_abrt (int   sig)
{
    WCX_DEBUG ("\033[0;31;40mSIGABRT!\a\033[0m%s", "\n");
    write_log (&error_log, -1, "handle_abrt()", "receive SIGABRT");
    pthread_quit();
}

static void handle_int (int   sig)
{
    printf ("\033[0;31;40mSIGINT!\a\033[0m\n");
    write_log (&error_log, -1, "handle_int()", "receive SIGINT");
    pthread_quit();
}

static void handle_usr1 (int   sig)
{
    printf ("\033[0;31;40mSIGUSR1!\a\033[0m\n");
    write_log (&error_log, -1, "handle_usr1()", "receive SIGUSR1");
    //pthread_quit();
}

static void handle_usr2 (int   sig)
{
    printf ("\033[0;31;40mSIGUSR1!\a\033[0m\n");
    write_log (&error_log, -1, "handle_usr2()", "receive SIGUSR2");
    //pthread_quit();
}

#if 0
static void handle_alarm (int sig)
{
    write_log (&error_log, -1, "handle_alarm()", "receive SIGALARM");
    g_thread_exit_flg = 1;
}
#endif

#if 0
static void init_time (int time)
{
    struct itimerval value;

    value.it_value.tv_sec = time;
    value.it_value.tv_usec = 0;
    value.it_interval = value.it_value;
    setitimer (ITIMER_REAL, &value, NULL);
}
#endif

/**
 * @brief    init sigaction
 */
static void init_sigaction()
{
    /*SIGPIPE*/
    struct   sigaction   action;
    action.sa_handler   =   handle_pipe;
    sigemptyset (&action.sa_mask);
    action.sa_flags   =   0;
    sigaction (SIGPIPE, &action, NULL);

    /*SIGTERM*/
    struct   sigaction   action_term;
    action_term.sa_handler   =   handle_quit;
    sigemptyset (&action_term.sa_mask);
    action_term.sa_flags   =   0;
    sigaction (SIGTERM, &action_term, NULL);

    /*SIGSEGV*/
    struct   sigaction   action_segv;
    action_segv.sa_handler   =   handle_segv;
    sigemptyset (&action_segv.sa_mask);
    action_segv.sa_flags   =   0;
    sigaction (SIGSEGV, &action_segv, NULL);

    /*SIGABRT*/
    struct   sigaction   action_abrt;
    action_abrt.sa_handler   =   handle_abrt;
    sigemptyset (&action_abrt.sa_mask);
    action_abrt.sa_flags   =   0;
    sigaction (SIGABRT, &action_abrt, NULL);

    /*SIGINT*/
    struct   sigaction   action_int;
    action_int.sa_handler   =   handle_int;
    sigemptyset (&action_int.sa_mask);
    action_int.sa_flags   =   0;
    sigaction (SIGINT, &action_int, NULL);

    /*SIGUSR1*/
    struct   sigaction   action_usr1;
    action_usr1.sa_handler   =   handle_usr1;
    sigemptyset (&action_usr1.sa_mask);
    action_usr1.sa_flags   =   0;
    sigaction (SIGUSR1, &action_usr1, NULL);

    /*SIGUSR2*/
    struct   sigaction   action_usr2;
    action_usr2.sa_handler   =   handle_usr2;
    sigemptyset (&action_usr2.sa_mask);
    action_usr2.sa_flags   =   0;
    sigaction (SIGUSR2, &action_usr2, NULL);

    /*SIGALRM*/
#if 0
    struct sigaction action_alarm;
    action_alarm.sa_handler = handle_alarm;
    action_alarm.sa_flags = 0;
    sigemptyset (&action_alarm.sa_mask);
    sigaction (SIGALRM, &action_alarm, NULL);
#endif
}

static int init_collect()
{
    for (size_t i = 0; i < SERIES_MAX; i++)
    {
        pthread_mutex_init (&Serial[i].mutex, NULL);
        pthread_mutex_init (&OptSerial[i].mutex, NULL);
        //for (size_t j = 0; j < OptSerial[i].Tag.size(); j++)
    }
    g_thread_exit_flg = 0;
    g_modbus_space_level = -1;
    return 0;
}

#define MAXLINE     80
#define CMDPORT     55555

static void  *thread_read_cmd (void *data)
{
    int sock_fd;
    struct sockaddr_in sin;
    struct sockaddr_in rin;
    socklen_t address_size;
    char buf[MAXLINE];
    char str[INET_ADDRSTRLEN];
    int i, n, len;
    int reuseaddr_on = 1;

    write_log (&boot_log, 1, "thread_read_cmd()", "thread_read_cmd start....");

    pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);

    bzero (&sin, sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons (CMDPORT);

    sock_fd = socket (AF_INET, SOCK_DGRAM, 0);
    if (-1 == sock_fd)
    {
        perror ("call to socket");
        write_log (&error_log, -1, "thread_read_cmd()", "socket:%d", sock_fd);
        pthread_quit ();
    }
    if (setsockopt (sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, sizeof (reuseaddr_on)) == -1)
    {
        write_log (&error_log, -1, "thread_read_cmd()", "setsockopt:%s", strerror (errno));
        pthread_quit ();
    }
    n = bind (sock_fd, (struct sockaddr *) &sin, sizeof (sin));
    if (-1 == n)
    {
        perror ("call to bind");
        write_log (&error_log, -1, "thread_read_cmd()", "%>_< %bind:%d", n);
        pthread_quit ();
    }

    while (1)
    {
again:
        if (g_thread_exit_flg == 1)
            break;
        address_size = sizeof (rin);
        //n = recvfrom ( sock_fd, buf, MAXLINE, 0, ( struct sockaddr * ) &rin, &address_size );
        n = recvfrom (sock_fd, buf, MAXLINE, MSG_DONTWAIT, (struct sockaddr *) &rin, &address_size);
        if (-1 == n)
        {
            if (EAGAIN == errno)
            {
                MySleep (5, 0);
                goto again;
            }
            perror ("call to recvfrom.\n");
            write_log (&error_log, -1, "thread_read_cmd()", "recvfrom:%d, errno:%d", n, errno);
            pthread_quit ();
        }
        //wprintf ("You ip is %s at port %d:%s\n",
        //inet_ntop (AF_INET, &rin.sin_addr, str, sizeof (str)), ntohs (rin.sin_port), buf);

        //-->分析处理...
        if (buf[0] == DATABASE_CHANGED_CMD || strstr (buf, "reload") != NULL)
        {
            //reload();
            //strlcpy(buf, "reload OK!", sizeof(buf));
            write_log (&error_log, 1, "thread_read_cmd()", "rcv reload!");
            WCX_DEBUG ("!!!!!!!!!!!!!!!!!!!!!!rcv reload!%s", "\n");
            /* delet the optimized_tags */
            char *cmd;
            cmd = new char[128];
            strlcpy (cmd, "rm -rf ", 128);
            strlcat (cmd, g_path_fast_tag, 128);
            strlcat (cmd, "/*", 128);
            system (cmd);
            delete[] cmd;
            pthread_quit();
        }
        else if (buf[0] == MODBUS_SPACE_LEVEL0)
        {
            g_modbus_space_level = -1;
        }
        else if (buf[0] == MODBUS_SPACE_LEVEL1)
        {
            g_modbus_space_level = 100;
            low_speed.time_start ();
        }
        else if (buf[0] == MODBUS_SPACE_LEVEL2)
        {
            g_modbus_space_level = 200;
            low_speed.time_start ();
        }
        else if (buf[0] == MODBUS_SPACE_LEVEL3)
        {
            g_modbus_space_level = 500;
            low_speed.time_start ();
        }
        else if (buf[0] == MODBUS_SPACE_LEVEL4)
        {
            g_modbus_space_level = 1000;
            low_speed.time_start ();
        }
#ifdef IEC104_MASTER_CMD_RCV
        else if (strstr (buf, "cmd0") != NULL)
        {
            system ("clear");
            printf ("recv cmd: 发送总召唤激活请求命令...\n");
            IEC104_Master_DisplayOn ();
            IEC104_Master_sC_IC_NA (0, 1, 1793, 20);
        }
        else if (strstr (buf, "cmd1") != NULL)
        {
            system ("clear");
            printf ("recv cmd: 时钟同步激活请求命令...\n");
            IEC104_Master_DisplayOn ();
            IEC104_Master_sC_CS_NA (0);
        }
        else if (strstr (buf, "cmd2") != NULL)
        {
            system ("clear");
            printf ("recv cmd: 双点遥控命令...合\n");
            IEC104_Master_DisplayOn ();
            IEC104_Master_sC_DC_NA_PreSet (0, 1, 1793, 0x82);
        }
        else if (strstr (buf, "cmd3") != NULL)
        {
            system ("clear");
            printf ("recv cmd: 双点遥控命令...开\n");
            IEC104_Master_sC_DC_NA_PreSet (0, 1, 1793, 0x81);
        }
        else if (strstr (buf, "cmd8") != NULL)
        {
            printf ("recv cmd: 关闭显示命令！\n");
            IEC104_Master_DisplayOff ();
        }
        else if (strstr (buf, "cmd9") != NULL)
        {
            /*system ("clear");*/
            printf ("recv cmd: 打开显示命令！\n");
            IEC104_Master_DisplayOn ();
        }
#endif
        else if (strstr (buf, "lock") != NULL)
        {
            if ( (strstr (buf, "all")) != NULL)
            {
                for (size_t i = 0; i < 7; i++)
                {
                    int ret = pthread_mutex_lock (&OptSerial[i].mutex);
                    if (ret != 0)
                    {
                        WCX_DEBUG ("pthread_mutex_lock err!%s", "\n");
                        write_log (&error_log, -1, "thread_read_cmd", "pthread_mutex_lock err!");
                    }
                }
            }
            else
            {
                int sn = strtol (buf, NULL, 10);
                if (sn >= 0 && sn <= 6)
                {
                    WCX_DEBUG ("------ mutex com%d\n", sn + 1);
                    /* mutex RS485 port */
                    int ret = pthread_mutex_lock (&OptSerial[sn].mutex);
                    if (ret != 0)
                    {
                        WCX_DEBUG ("pthread_mutex_lock err!%s", "\n");
                        write_log (&error_log, -1, "thread_read_cmd", "pthread_mutex_lock err!");
                    }
                }
            }
        }
        else if (strstr (buf, "free") != NULL)
        {
            if ( (strstr (buf, "all")) != NULL)
            {
                for (size_t i = 0; i < 7; i++)
                {
                    int ret = pthread_mutex_unlock (&OptSerial[i].mutex);
                    if (ret != 0)
                    {
                        WCX_DEBUG ("pthread_mutex_lock err!%s", "\n");
                        write_log (&error_log, -1, "thread_read_cmd", "pthread_mutex_lock err!");
                    }
                }
            }
            else
            {
                int sn = strtol (buf, NULL, 10);
                if (sn >= 0 && sn <= 6)
                {
                    WCX_DEBUG ("------ unmutex com%d\n", sn + 1);
                    /* unmutex RS485 port */
                    int ret = pthread_mutex_unlock (&OptSerial[sn].mutex);
                    if (ret != 0)
                    {
                        WCX_DEBUG ("unmutex com%d\n", sn);
                        write_log (&error_log, -1, "thread_modbus_collect()", "pthread_mutex_lock err!");
                    }
                }
            }
        }
        else
        {
            strlcpy (buf, "unknow command!", sizeof (buf));
        }

        n = sendto (sock_fd, buf, strlen (buf) + 1, 0, (struct sockaddr *) &rin, address_size);
        if (-1 == n)
        {
            perror ("call to sendto\n");
            write_log (&error_log, -1, "thread_read_cmd()", "sendto:%d", n);
        }

        MySleep (5, 0);
    }

    close (sock_fd);
    pthread_exit (NULL);
    write_log (&boot_log, 1, "thread_read_cmd()", "thread_read_cmd exit!");
}

#define N 100

static void *thread_modbus_collect (void * SeriesNo)
{
    int ret, i, j;
    int sn = (int)SeriesNo;

    int fd_io;

    int slave, fun, addr, quantity;
    uint8_t DataType, Opertion;
    float xi_shu;
    double value = 0;
    int16_t value_count;
    unsigned int nTagId;
    unsigned int loop_count;

    /* read buf */
    uint8_t uint8_buf[512];
    uint32_t uint32_buf[512];

#ifdef PRINT_TIME_USE
    struct timeb time_start; // 记录发送时间
    struct timeb time_end; // 记录接收时间
#endif
    float time_use;

    /* 用于滑动平均值滤波 */
    double m_value_buf[N];
    int m_i = 0;

    write_log (&boot_log, 1, "thread_modbus_collect()", "modbus_collect_thread[%d] startting...", sn);

#if 0
    /* set pthread Attribute */
    pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif

    /* open kaichu fd  */
    fd_io = open ("/dev/ort3000c_io0", O_RDWR);
    if (fd_io < 0)
    {
        printf ("/dev/ort3000c_io0 can not open!\n");
        exit (1);
    }

    /* modbus initialization */
    modbus_init_rtu (&OptSerial[sn].mb_param, OptSerial[sn].fd, 9600, "none", 8, 1, 1);
    if (DEBUG_FLG & DEBUG_MODBUS)
    {
        modbus_set_debug (&OptSerial[sn].mb_param, TRUE);
    }
    if (modbus_connect (&OptSerial[sn].mb_param) == -1)
    {
        write_log (&error_log, -1, "thread_modbus_collect()", "modbus_connect<%d> error", sn);
        loop_count = 0;
        goto modbus_collect_again;
    }

    loop_count = OptSerial[sn].Tag.size();
    write_log (&boot_log, 1, "thread_modbus_collect()", "sn:%d,loop:%d", sn, loop_count);

    for (i = 0; i < loop_count; i++)
    {
        ftime (&OptSerial[sn].Tag[i].time_response);
        ftime (&OptSerial[sn].Tag[i].time_timeout);
    }

    while (1)
    {
modbus_collect_again:
        if (g_thread_exit_flg == 1)
        {
            shm_head_watch_dog[sn] = 0;
            goto modbus_collect_exit;
        }
        if (loop_count == 0)
        {
            //shm_head_watch_dog[sn] = 1; [> feed watch dog <]
            //printf ("loop_count:%d\n", loop_count);
            //MySleep (2, 0);
            //continue;
            break;
        }
        for (i = 0; i < loop_count; i++)
        {
            if (g_thread_exit_flg == 1)
            {
                shm_head_watch_dog[sn] = 0;
                goto modbus_collect_exit;
            }
            shm_head_watch_dog[sn] = 1; /* feed watch dog */

            /* mutex RS485 port */
            ret = pthread_mutex_lock (&OptSerial[sn].mutex);
            if (ret != 0)
            {
                write_log (&error_log, -1, "thread_modbus_collect()", "pthread_mutex_lock err!");
            }

            slave = OptSerial[sn].Tag[i].ModBusQueryFrame[0];
            fun = OptSerial[sn].Tag[i].ModBusQueryFrame[1];
            addr = (uint16_t) OptSerial[sn].Tag[i].ModBusQueryFrame[2] << 8
                   | OptSerial[sn].Tag[i].ModBusQueryFrame[3];
            quantity = (uint16_t) OptSerial[sn].Tag[i].ModBusQueryFrame[4] << 8
                       | OptSerial[sn].Tag[i].ModBusQueryFrame[5];
            OptSerial[sn].mb_param.slave = slave;

            //printf ("@@@@slave:%d, fun:%d, addrH:%d, addrL:%d, quaH:%d, quaL:%d\n",
            //slave, fun,
            //OptSerial[sn].Tag[i].ModBusQueryFrame[2], OptSerial[sn].Tag[i].ModBusQueryFrame[3],
            //OptSerial[sn].Tag[i].ModBusQueryFrame[4],OptSerial[sn].Tag[i].ModBusQueryFrame[5]);

            switch (fun)
            {
            case FC_READ_COIL_STATUS:
            {
                switch (OptSerial[sn].Tag[i].ModbusType)
                {
                case MODBUS_TYPE_STANDARD:
                {
                    value_count = read_coil_status (&OptSerial[sn].mb_param, addr, quantity, uint8_buf,
                                                    OptSerial[sn].Tag[i].TimeOut);
                    if (value_count != quantity | value_count > REAL_VAL_TAG_COUNT)
                    {
#ifdef WRITE_LOG
                        write_log (&error_log, -1, "thread_modbus_collect()",
                                   "read_input_status err, value_count:%d, slave:%d, addr:%d",
                                   value_count, slave, addr);
#endif
                    }
                    else /* write que */
                    {
                        for (j = 0; j < value_count; j++)
                        {
                            ret = cha_biao (sn, slave, fun, addr++, &nTagId, &xi_shu, &DataType, &Opertion);   /* computer tag ID */
                            if (ret < 0)
                            {
                                write_log (&error_log, -1, "thread_modbus_collect()",
                                           "cha_biao err,sn:%d,nTagId:%d,slave_cur:%d,fun_cur:%d,addr_cur:%d,value_count:%d,dataType:%d",
                                           sn, nTagId, slave, fun, addr, quantity, OptSerial[sn].Tag[j].DataType);
                                MySleep (0, 100 * 1000);
                                ret = pthread_mutex_unlock (&OptSerial[sn].mutex);
                                if (ret != 0)
                                {
                                    write_log (&error_log, -1, "thread_modbus_collect()", "pthread_mutex_lock err!");
                                }
                                goto modbus_collect_again;
                            }
                            REAL_VAL_TAG tag;
                            MAKE_REAL_TAG (nTagId, uint8_buf[j], &tag);
                            PUSH_REAL_VAL_TAG (tag);
                            tprintf ("!tag_id:% 5d, value: %.5f\n", nTagId, uint8_buf[j]);
                        } /* end for */
                    }
                }
                break;

                case MODBUS_TYPE_RPC3FXC:
                {
                    value_count = read_coil_status_RPC3FxC (&OptSerial[sn].mb_param, addr, quantity, uint8_buf,
                                                            OptSerial[sn].Tag[i].TimeOut);
                    if (value_count != quantity | value_count > REAL_VAL_TAG_COUNT)
                    {
#ifdef WRITE_LOG
                        write_log (&error_log, -1, "thread_modbus_collect()", "read_input_status err, value_count:%d, slave:%d, addr:%d",
                                   value_count, slave, addr);
#endif
                    }
                    else /* write que */
                    {
                        for (j = 0; j < value_count; j++)
                        {
                            ret = cha_biao (sn, slave, fun, addr++, &nTagId, &xi_shu, &DataType, &Opertion);   /* computer tag ID */
                            if (ret < 0)
                            {
                                write_log (&error_log, -1, "thread_modbus_collect()",
                                           "cha_biao err,sn:%d,nTagId:%d,slave_cur:%d,fun_cur:%d,addr_cur:%d,value_count:%d,dataType:%d",
                                           sn, nTagId, slave, fun, addr, quantity, OptSerial[sn].Tag[j].DataType);
                                MySleep (0, 100 * 1000);
                                ret = pthread_mutex_unlock (&OptSerial[sn].mutex);
                                if (ret != 0)
                                {
                                    write_log (&error_log, -1, "thread_modbus_collect()", "pthread_mutex_lock err!");
                                }
                                goto modbus_collect_again;
                            }
                            REAL_VAL_TAG tag;
                            MAKE_REAL_TAG (nTagId, uint8_buf[j], &tag);
                            PUSH_REAL_VAL_TAG (tag);
                            tprintf ("!tag_id:% 5d, value: %.5f\n", nTagId, uint8_buf[j]);
                        } /* end for */
                    }
                }
                break;

                case MODBUS_TYPE_LD_B10_220:
                {
                    value_count = read_coil_status_LD_B10_220 (&OptSerial[sn].mb_param,
                                  uint8_buf, OptSerial[sn].Tag[i].TimeOut);
                    if (value_count != quantity | value_count > REAL_VAL_TAG_COUNT)
                    {
#ifdef WRITE_LOG
                        printf ("COIL quantity:%d, value_count:%d, value:\n", quantity, value_count);
                        write_log (&error_log, -1, "thread_modbus_collect()", "read_coil_status err, value_count:%d, slave:%d, addr:%d",
                                   value_count, slave, addr);
#endif
                    }
                    else /* write que */
                    {
                        for (j = 0; j < value_count; j++)
                        {
                            ret = cha_biao (sn, slave, fun, addr++, &nTagId, &xi_shu, &DataType, &Opertion);   /* computer tag ID */
                            if (ret < 0)
                            {
                                write_log (&error_log, -1, "thread_modbus_collect()",
                                           "cha_biao err,sn:%d,nTagId:%d,slave_cur:%d,fun_cur:%d,addr_cur:%d,value_count:%d,dataType:%d",
                                           sn, nTagId, slave, fun, addr, quantity, OptSerial[sn].Tag[j].DataType);
                                MySleep (0, 100 * 1000);
                                ret = pthread_mutex_unlock (&OptSerial[sn].mutex);
                                if (ret != 0)
                                {
                                    write_log (&error_log, -1, "thread_modbus_collect()", "pthread_mutex_lock err!");
                                }
                                goto modbus_collect_again;
                            }
                            REAL_VAL_TAG tag;
                            MAKE_REAL_TAG (nTagId, uint8_buf[j], &tag);
                            PUSH_REAL_VAL_TAG (tag);
                            tprintf ("!tag_id:% 5d, value: %.5f\n", nTagId, uint8_buf[j]);
                        } /* end for */
                    }
                }
                break;

                default:
                    break;
                }
            }
            break;

            case FC_READ_INPUT_STATUS:
            {
#ifdef PRINT_TIME_USE
                ftime (&time_start);
#endif
                value_count = read_input_status (&OptSerial[sn].mb_param, addr, quantity, uint8_buf, OptSerial[sn].Tag[i].TimeOut);
#ifdef PRINT_TIME_USE
                ftime (&time_end);
                time_use = (time_end.time - time_start.time) * 1000 + (time_end.millitm - time_start.millitm);
                wprintf ("Serial<%d>,slave<%d> ------ time_use[%4dms]\n", sn, slave, (int) time_use);
#endif
                if (value_count != quantity | value_count > REAL_VAL_TAG_COUNT)
                {
#ifdef WRITE_LOG
                    write_log (&error_log, -1, "thread_modbus_collect()",
                               "read_input_status err, value_count:%d, slave:%d, addr:%d",
                               value_count, slave, addr);
#endif
                }
                else /* write que */
                {
                    for (j = 0; j < value_count; j++)
                    {
                        ret = cha_biao (sn, slave, fun, addr++, &nTagId, &xi_shu, &DataType, &Opertion);   /* computer tag ID */
                        if (ret < 0)
                        {
                            write_log (&error_log, -1, "thread_modbus_collect()",
                                       "cha_biao err,sn:%d,nTagId:%d,slave_cur:%d,fun_cur:%d,addr_cur:%d,value_count:%d,dataType:%d",
                                       sn, nTagId, slave, fun, addr, quantity, OptSerial[sn].Tag[j].DataType);
                            MySleep (0, 100 * 1000);
                            ret = pthread_mutex_unlock (&OptSerial[sn].mutex);
                            if (ret != 0)
                            {
                                write_log (&error_log, -1, "thread_modbus_collect()", "pthread_mutex_lock err!");
                            }
                            goto modbus_collect_again;
                        }
                        REAL_VAL_TAG tag;
                        MAKE_REAL_TAG (nTagId, uint8_buf[j], &tag);
                        PUSH_REAL_VAL_TAG (tag);
                        tprintf ("!tag_id:% 5d, value: %.5f\n", nTagId, uint8_buf[j]);
                    } /* end for */
                }
            }
            break;

            case FC_READ_HOLDING_REGISTERS:
            {
                switch (OptSerial[sn].Tag[i].ModbusType)
                {
                case MODBUS_TYPE_STANDARD:
                case MODBUS_TYPE_PSD:
                {
#if 0
                    /* test , write rand value */
                    srand ( (int) time (0));
                    int rand_num = 1 + (int) (10.0 * rand() / (RAND_MAX + 1.0));
                    REAL_VAL_TAG tag;
                    MAKE_REAL_TAG (1001, (double) rand_num, &tag);
                    PUSH_REAL_VAL_TAG (tag);
                    break;
#endif

#ifdef PRINT_TIME_USE
                    ftime (&time_start);
#endif
                    value_count = read_holding_registers (&OptSerial[sn].mb_param, addr, quantity, uint32_buf,
                                                          OptSerial[sn].Tag[i].DataType,
                                                          OptSerial[sn].Tag[i].TimeOut);
#ifdef PRINT_TIME_USE
                    ftime (&time_end);
                    time_use = (time_end.time - time_start.time) * 1000 + (time_end.millitm - time_start.millitm);
                    wprintf ("Serial<%d>,slave<%d> ------ time_use[%4dms]\n", sn, slave, (int) time_use);
#endif
                    if (value_count != quantity | value_count > REAL_VAL_TAG_COUNT)
                    {
                        ftime (&OptSerial[sn].Tag[i].time_timeout);
                        time_use = OptSerial[sn].Tag[i].time_timeout.time - OptSerial[sn].Tag[i].time_response.time;
                        if (time_use > DEVICE_OFFLINE_TIME)
                        {
                            REAL_VAL_TAG tag;
                            MAKE_REAL_TAG (sn + 29, slave, &tag);
                            PUSH_REAL_VAL_TAG (tag);
                        }
#ifdef WRITE_LOG
                        write_log (&error_log, -1, "thread_modbus_collect()",
                                   "read holding_registers err,value_count:-0X%x,slave:%d,addr:%d",
                                   -value_count, slave, addr);
#endif
                    }
                    else /* write que */
                    {
                        ftime (&OptSerial[sn].Tag[i].time_response);
                        for (j = 0; j < value_count; j++)
                        {
                            ret = cha_biao (sn, slave, fun, addr++, &nTagId, &xi_shu, &DataType, &Opertion);   /* computer tag ID and xishu */
                            if (ret < 0)
                            {
                                write_log (&error_log, -1, "thread_modbus_collect()",
                                           "cha_biao err when read holding register,sn:%d,salve:%d,fun:%d,addr:%d,nTagId:%d",
                                           sn, slave, fun, addr, nTagId);
                                MySleep (0, 100 * 1000);
                                ret = pthread_mutex_unlock (&OptSerial[sn].mutex);
                                if (ret != 0)
                                {
                                    write_log (&error_log, -1, "thread_modbus_collect()", "pthread_mutex_lock err!");
                                }
                                goto modbus_collect_again;
                            }
                            /* compute quotiety */
                            double Cur_Value = 0.0;
                            switch (Opertion)
                            {
                            case    0: // NULL
                            {
                                switch (DataType)
                                {
                                case    0:      // 数据类型为8位整数
                                    Cur_Value = (int8_t) uint32_buf[j];
                                    break;
                                case    1:      // 数据类型为8位无符号整数
                                    Cur_Value = (uint8_t) uint32_buf[j];
                                    break;
                                case    2:      // 数据类型为16位整数
                                    Cur_Value = (int16_t) uint32_buf[j];
                                    break;
                                case    3:      // 数据类型为16位无符号整数
                                    Cur_Value = (uint16_t) uint32_buf[j];
                                    break;
                                case    4:      // 数据类型为32位整数
                                    Cur_Value = (int32_t) uint32_buf[j];
                                    break;
                                case    5:      // 数据类型为32位无符号整数
                                    Cur_Value = (uint32_t) uint32_buf[j];
                                    break;
                                case    6:      // 数据类型为64位整数
                                case    7:      // 数据类型为64位无符号整数
                                case    8:      // 单精度浮点数
                                case    9:      // 双精度浮点数
                                default:
                                    Cur_Value = (double) uint32_buf[j];
                                    break;
                                }
                            }
                            break;

                            case    1: // +
                            {
                                switch (DataType)
                                {
                                case    0:      // 数据类型为8位整数
                                    Cur_Value = (int8_t) uint32_buf[j] + xi_shu;
                                    break;
                                case    1:      // 数据类型为8位无符号整数
                                    Cur_Value = (uint8_t) uint32_buf[j] + xi_shu;
                                    break;
                                case    2:      // 数据类型为16位整数
                                    Cur_Value = (int16_t) uint32_buf[j] + xi_shu;
                                    break;
                                case    3:      // 数据类型为16位无符号整数
                                    Cur_Value = (uint16_t) uint32_buf[j] + xi_shu;
                                    break;
                                case    4:      // 数据类型为32位整数
                                    Cur_Value = (int32_t) uint32_buf[j] + xi_shu;
                                    break;
                                case    5:      // 数据类型为32位无符号整数
                                    Cur_Value = (uint32_t) uint32_buf[j] + xi_shu;
                                    break;
                                case    6:      // 数据类型为64位整数
                                case    7:      // 数据类型为64位无符号整数
                                case    8:      // 单精度浮点数
                                case    9:      // 双精度浮点数
                                default:
                                    Cur_Value = (double) uint32_buf[j] + xi_shu;
                                    break;
                                }
                            }
                            break;

                            case    2: // -
                            {
                                switch (DataType)
                                {
                                case    0:      // 数据类型为8位整数
                                    Cur_Value = (int8_t) uint32_buf[j] - xi_shu;
                                    break;
                                case    1:      // 数据类型为8位无符号整数
                                    Cur_Value = (uint8_t) uint32_buf[j] - xi_shu;
                                    break;
                                case    2:      // 数据类型为16位整数
                                    Cur_Value = (int16_t) uint32_buf[j] - xi_shu;
                                    break;
                                case    3:      // 数据类型为16位无符号整数
                                    Cur_Value = (uint16_t) uint32_buf[j] - xi_shu;
                                    break;
                                case    4:      // 数据类型为32位整数
                                    Cur_Value = (int32_t) uint32_buf[j] - xi_shu;
                                    break;
                                case    5:      // 数据类型为32位无符号整数
                                    Cur_Value = (uint32_t) uint32_buf[j] - xi_shu;
                                    break;
                                case    6:      // 数据类型为64位整数
                                case    7:      // 数据类型为64位无符号整数
                                case    8:      // 单精度浮点数
                                case    9:      // 双精度浮点数
                                default:
                                    Cur_Value = (double) uint32_buf[j] - xi_shu;
                                    break;
                                }
                            }
                            break;

                            case    4: // /
                            {
                                switch (DataType)
                                {
                                case    0:      // 数据类型为8位整数
                                    Cur_Value = (int8_t) uint32_buf[j] / xi_shu;
                                    break;
                                case    1:      // 数据类型为8位无符号整数
                                    Cur_Value = (uint8_t) uint32_buf[j] / xi_shu;
                                    break;
                                case    2:      // 数据类型为16位整数
                                    Cur_Value = (int16_t) uint32_buf[j] / xi_shu;
                                    break;
                                case    3:      // 数据类型为16位无符号整数
                                    Cur_Value = (uint16_t) uint32_buf[j] / xi_shu;
                                    break;
                                case    4:      // 数据类型为32位整数
                                    Cur_Value = (int32_t) uint32_buf[j] / xi_shu;
                                    break;
                                case    5:      // 数据类型为32位无符号整数
                                    Cur_Value = (uint32_t) uint32_buf[j] / xi_shu;
                                    break;
                                case    6:      // 数据类型为64位整数
                                case    7:      // 数据类型为64位无符号整数
                                case    8:      // 单精度浮点数
                                case    9:      // 双精度浮点数
                                default:
                                    Cur_Value = (double) uint32_buf[j] / xi_shu;
                                    break;
                                }
                            }
                            break;

                            case    3: // x
                            default:
                            {
                                switch (DataType)
                                {
                                case    0:      // 数据类型为8位整数
                                    Cur_Value = (int8_t) uint32_buf[j] * xi_shu;
                                    break;
                                case    1:      // 数据类型为8位无符号整数
                                    Cur_Value = (uint8_t) uint32_buf[j] * xi_shu;
                                    break;
                                case    2:      // 数据类型为16位整数
                                    Cur_Value = (int16_t) uint32_buf[j] * xi_shu;
                                    break;
                                case    3:      // 数据类型为16位无符号整数
                                    Cur_Value = (uint16_t) uint32_buf[j] * xi_shu;
                                    break;
                                case    4:      // 数据类型为32位整数
                                    Cur_Value = (int32_t) uint32_buf[j] * xi_shu;
                                    break;
                                case    5:      // 数据类型为32位无符号整数
                                    Cur_Value = (uint32_t) uint32_buf[j] * xi_shu;
                                    break;
                                case    6:      // 数据类型为64位整数
                                case    7:      // 数据类型为64位无符号整数
                                case    8:      // 单精度浮点数
                                case    9:      // 双精度浮点数
                                default:
                                    Cur_Value = (double) uint32_buf[j] * xi_shu;
                                    break;
                                }
                            }
                            break;
                            }
                            /* 滑动平均值滤波 */
                            if (OptSerial[sn].Tag[i].ModbusType == MODBUS_TYPE_PSD)
                            {
                                int count;
                                double sum = 0;
                                m_value_buf [m_i++] = Cur_Value;
                                if (m_i == N)
                                    m_i = 0;
                                for (count = 0; count < N; count++)
                                    sum += m_value_buf [count];
                                Cur_Value = (double) (sum / N);
                            }
                            /* write db */
                            tprintf ("!#tag_id:% 5d, value: %.5f\n", nTagId, Cur_Value);
                            REAL_VAL_TAG tag;
                            MAKE_REAL_TAG (nTagId, Cur_Value, &tag);
                            PUSH_REAL_VAL_TAG (tag);
                        } /* end for */
                        /* 清空设备掉线报警 */
                        REAL_VAL_TAG tag;
                        MAKE_REAL_TAG (sn + 29, 0, &tag);
                        PUSH_REAL_VAL_TAG (tag);
                    }
                }
                break;

                default:
                    break;
                }
            }
            break;

            case FC_READ_INPUT_REGISTERS:
            case FC_FORCE_SINGLE_COIL:
            case FC_PRESET_SINGLE_REGISTER:
            case FC_READ_EXCEPTION_STATUS:
            case FC_FORCE_MULTIPLE_COILS:
            case FC_PRESET_MULTIPLE_REGISTERS:
            case FC_REPORT_SLAVE_ID:
            default:
                write_log (&error_log, -1, "thread_modbus_collect()", "funtion code default:%d", fun);
            } /* end switch(fun) */

            /* umutex RS450 port */
            ret = pthread_mutex_unlock (&OptSerial[sn].mutex);
            if (ret != 0)
            {
                write_log (&error_log, -1, "thread_modbus_collect()", "pthread_mutex_lock err!");
            }

            if (g_modbus_space_level == -1)
            {
                if (DEBUG_FLG != 0)
                    MySleep (0, 1000 * g_space_time);
                else
                    MySleep (0, 1000 * (long) OptSerial[sn].Tag[i].SpaceTime);
            }
            else
            {
                MySleep (0, 1000 * g_modbus_space_level);
                if (low_speed.time_used () > MAX_TIMES_USED_LOW_SPEED)
                    g_modbus_space_level = -1;
            }
            if (DEBUG_FLG & DEBUG_STEP)
            {
                printf ("\n--------------- [ push enter key to next step...]---------------\n");
                getchar();
            }
        } /* end for */
    } /* end while(1) */

modbus_collect_exit:
    write_log (&boot_log, 1, "thread_modbus_collect()", "modbus_collect_thread[%d] exit!", sn);
    pthread_exit (NULL);
}

static void  *thread_myself_check (void * data)
{
    int i, j, ret;
    int fd_adc, fd_tm, fd_battery_detect, fd_io;
    double value;
    float pow_voltage, bat_voltage;
    char buffer[30];
    uint32_t tag_id;
    uint32_t loop_count;
    int temperature_check = 1;
    REAL_VAL_TAG tag;

    write_log (&boot_log, 1, "myself_check()", "start...");

    fd_adc = open ("/dev/adc", 0);
    if (fd_adc < 0)
    {
        printf ("/dev/adc open err!!!");
        write_log (&error_log, -1, "myself_check()", "/dev/adc open err!");
    }
    else
    {
        printf ("/dev/adc open sucess!\n");
    }

    fd_tm = open ("/dev/TEM0", 0);
    if (fd_tm < 0)
    {
        write_log (&error_log, -1, "myself_check()", "/dev/TEM0 open err!");
    }

    fd_battery_detect = open ("/dev/ort3000c_battery_detect0", O_RDWR);
    if (fd_battery_detect < 0)
    {
        write_log (&boot_log, -1, "myself_check()", "/dev/ort3000c_battery_detect open err!");
    }

    fd_io = open ("/dev/ort3000c_io0", O_RDWR);
    if (fd_io < 0)
    {
        write_log (&boot_log, -1, "myself_check()", "/dev/ort3000c_io open err!");
    }

    loop_count = myself.Tag.size();

    write_log (&boot_log, 1, "myself_check()", "loop_count:%d", loop_count);

    while (1)
    {
        if (g_thread_exit_flg == 1)
            goto myself_check_exit;
        for (i = 0; i < loop_count; i++)
        {
            //printf ("i:%d\n", i);
            //getchar ();
            if (g_thread_exit_flg == 1)
                goto myself_check_exit;
            tag_id = myself.Tag[i].Id;
            //write_log ( &boot_log, 1, "myself_check()", "tag_id:%d", tag_id );
            switch (tag_id)
            {
            case    POWER_TYPE:  //获取前置机供电方式
            {
                memset (buffer, 0, sizeof (buffer));
                ioctl (fd_adc, ADC_INPUT_PIN, 1);
                ret = read (fd_adc, buffer, sizeof (buffer - 1)); //read Power Voltage
                if (ret > 0)
                {
                    pow_voltage = 0;
                    pow_voltage = atol (buffer);
                    pow_voltage *= 3.3 / 1023;
                    //printf ("Power Voltage: %f\n", pow_voltage);
                }
                else
                {
                    //write_log (&error_log, -1, "myself_check()", "read Power Voltage err!");
                    printf ("read Power Voltage err!\n");
                    continue;
                }
                memset (buffer, 0, sizeof (buffer));
                ioctl (fd_adc, ADC_INPUT_PIN, 0);
                ret = read (fd_adc, buffer, sizeof (buffer - 1)); //read Battery Voltage
                if (ret > 0)
                {
                    bat_voltage = 0;
                    bat_voltage = atol (buffer);
                    bat_voltage *= 3.3 / 1023;
                    //printf ("Power Voltage: %f\n", bat_voltage);
                }
                else
                {
                    //write_log (&error_log, -1, "myself_check()", "read Battery Voltage err!");
                    printf ("read Battery Voltage err!\n");
                    continue;
                }
                if (pow_voltage > 1.5)
                {
                    value = 255; //是电源供电
                    //wprintf ("PowerTpye: Power!\n");
                }
                else
                {
                    if (bat_voltage < 1.5)   //电池电压低于3.5v,关机
                    {
                        //write_log (&boot_log, 1, "myself_check()", "battery(%dv) \
                        //is too lower! halt!", bat_value);
                        //wprintf ("Battery(%dv) is too lower!\n", bat_value);
                    }
                    value = bat_voltage;
                }
            }
            break;

            case    TEMPERATURE:  //获取本机温度
            {
                if (temperature_check == 1) //ten minutes a second look
                {
                    //printf ("temperature check...\n");
                    temperature_check = -300;
                    ret = pthread_mutex_lock (&OptSerial[0].mutex);
                    if (ret != 0)
                        write_log (&error_log, -1, "thread_myself_check()", "pthread_mutex_lock err!");
                    value = 0;
                    memset (buffer, 0, sizeof (buffer));
                    ret = read (fd_tm, buffer, sizeof (buffer - 1));
                    if (ret > 0)
                        value = (int) buffer[1] << 8 | buffer[0];
                    else
                    {
                        write_log (&error_log, -1, "myself_check()", "read TEM err!");
                        ret = pthread_mutex_unlock (&OptSerial[0].mutex);
                        if (ret != 0)
                            write_log (&error_log, -1, "thread_myself_check()", "pthread_mutex_lock err!");
                        continue;
                    }
                    ret = pthread_mutex_unlock (&OptSerial[0].mutex);
                    if (ret != 0)
                        write_log (&error_log, -1, "thread_myself_check()", "pthread_mutex_lock err!");
                }
                else
                {
                    temperature_check++;
                    continue;
                }
            }
            break;

            case    NETTYPE:  //获取当前使用哪种网络
            {
                value = shm_head_myself_check[0];
            }
            break;

            case    QUALITY:  //获取信号质量
            {
                value = shm_head_myself_check[1];
            }
            break;

            case    NETSUPPLY:  //获取当前使用哪个网络运营商
            {
                value = shm_head_myself_check[2];
            }
            break;

            case BATTERY_IN: //Battery is in?
            {
                ret = read (fd_battery_detect, buffer, 1);
                if (ret == 1)
                {
                    value = buffer[0];
                    //wprintf ("BatteryDetect: %d\n", value);
                }
            }
            break;

            case KAIRU_0: //read KaiRu
            {
                ret = read (fd_io, buffer, 3);
                if (ret == 3)
                {
                    for (j = 0; j < 18; j++)
                    {
                        int TagID;
                        double Cur_Value;
                        value = (buffer[j / 8] << j % 8) & 0x80;
                        //wprintf ("------开入%d = %d\n", j, value);
                        /* write queue */
                        TagID = KAIRU_0 + j;
                        if (value == 0x80)
                            Cur_Value = 0;
                        else
                            Cur_Value = 1;
                        MAKE_REAL_TAG (TagID, Cur_Value, &tag);
                        PUSH_REAL_VAL_TAG (tag);
                    }
                }
                i += 17;
            }

            case KAICU_0:
            {
                //wprintf ("kaichu_value:%d\n", kaichu_value);
                for (j = 0; j < 4; j++)
                {
                    int TagID;
                    double Cur_Value;
                    value = (kaichu_value >> j) & 0x01;
                    TagID = KAICU_0 + j;
                    if (value == 0x00)
                        Cur_Value = 1;
                    else
                        Cur_Value = 0;
                    MAKE_REAL_TAG (TagID, Cur_Value, &tag);
                    PUSH_REAL_VAL_TAG (tag);
                }
                i += 3;
            }
            continue;

            default:
            {
                MySleep (0, 500 * 1000);
            }
            continue;
            } /*end switch*/

            /* write queue */
            MAKE_REAL_TAG (tag_id, value, &tag);
            PUSH_REAL_VAL_TAG (tag);
        } /*end for*/
        MySleep (2, 0);
    } /*end while*/

myself_check_exit:
    close (fd_adc);
    close (fd_tm);
    close (fd_battery_detect);
    close (fd_io);
    write_log (&boot_log, 1, "myself_check()", "exit!");
    pthread_exit (NULL);
}

static void  *thread_iec104_master (void * data)
{
    IEC104MASTER_Run (NULL);
    pthread_exit (NULL);
}

void print_usage (const char * prog)
{
    system ("clear");
    printf ("Usage: %s [-pwscmdt]\n", prog);
    puts ("  -p  --设置本地路径\n"
          "  -w  --write taglist to csv file\n"
          "  -s  --设置modbus总线空闲时间\n"
          "  -c  --单步执行\n"
          "  -d  --打印调试信息\n"
          "  -m  --打印modbus帧\n"
          "  -t  --打印测点信息\n");
}

int parse_opts (int argc, char * argv[])
{
    int ret, ch;
    opterr = 0;
    char key_value[128];

    DEBUG_FLG = 0;
    g_space_time = 50;

    while ( (ch = getopt (argc, argv, "dmts:p:cw")) != EOF)
    {
        switch (ch)
        {
        case 'd':
            DEBUG_FLG |= DEBUG_MODE;
            break;
        case 'm':
            DEBUG_FLG |= DEBUG_MODBUS;
            break;
        case 't':
            DEBUG_FLG |= DEBUG_TAG;
            break;
        case 'w':
            DEBUG_FLG |= DEBUG_MODE;
            DEBUG_FLG |= DEBUG_CSV;
            break;
        case 's':
            DEBUG_FLG |= DEBUG_MODBUS;
            g_space_time = (unsigned int) strtol (optarg, NULL, 10);
            break;
        case 'c':
            DEBUG_FLG |= DEBUG_MODBUS;
            DEBUG_FLG |= DEBUG_STEP;
            break;
        case 'h':
        case '?':
            print_usage (argv[0]);
            return -1;
        default:
            break;
        }
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Read DBConfig

static inline int QueryKaiChuAccordingPid (sqlite3 * db, int pid)
{
    int result;
    char *errmsg = NULL;
    char **dbResult;
    int nRow, nColumn;
    char SQL[128];
    int value;

    sprintf (SQL, "select DATATYPE from TDIGITALT where tagid=%d", pid);

    result = sqlite3_get_table (db, SQL, &dbResult, &nRow, &nColumn, &errmsg);
    if (SQLITE_OK == result)
    {
        if (nRow == 1 && nColumn == 1)
        {
            value = strtol (dbResult[1], NULL, 10);
        }
        else
        {
            write_log (&error_log, -1, "QueryKaiChuAccordingPid()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
            value = -1;
        }
    }
    else
    {
        write_log (&error_log, -1, "QueryKaiChuAccordingPid()", "select DATATYPE from TDIGITALT where tagid=?");
        value = -1;
    }
    sqlite3_free_table (dbResult);
    return value;
}

static inline int QueryAttAccordingPid (sqlite3 * db, int ID, int TYPE, char *result, int result_size)
{
    int ret;
    char *errmsg = NULL;
    char **dbResult;
    int nRow, nColumn;
    int index;
    char SQL[256];

    sprintf (SQL, "select A.value from T_POT_COL as A,T_PCOLE as B \
            where A.PID=%d and B.dp_name=A.seq and B.ID=%d;", ID, TYPE);
    ret = sqlite3_get_table (db, SQL, &dbResult, &nRow, &nColumn, &errmsg);
    if (SQLITE_OK == ret)
    {
        if (nRow == 1 && nColumn == 1)
        {
            strlcpy (result, dbResult[1], result_size);
            ret = 0;
        }
        else
        {
            printf ("SQL:%s\n", SQL);
            write_log (&error_log, -1, "QueryAttAccordingPid()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
            ret = -1;
        }
    }
    else
    {
        printf ("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        write_log (&error_log, -1, "QueryAttAccordingPid()", "sqlite3_get_table err!-0");
        ret = -1;
    }
    sqlite3_free_table (dbResult);
    return ret;
}

static inline int QueryProtocolAccordingPid (sqlite3 * db, int ID)
{
    int result;
    char *errmsg = NULL;
    char **dbResult;
    int nRow, nColumn;
    int value;
    char SQL[256];

    sprintf (SQL, "select DISTINCT t_cole.id from TNODET,t_dev_col,t_cole \
            where TNODET.tagid=%d and TNODET.pid=t_dev_col.did and t_dev_col.cid=t_cole.id", ID);
    result = sqlite3_get_table (db, SQL, &dbResult, &nRow, &nColumn, &errmsg);
    if (SQLITE_OK == result)
    {
        if (nRow == 1 && nColumn == 1)
        {
            value = strtol (dbResult[1], NULL, 10);
        }
        else
        {
            printf ("SQL:%s\n", SQL);
            write_log (&error_log, -1, "QueryProtocolAccordingPid()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
            value = -1;
        }
    }
    else
    {
        write_log (&error_log, -1, "QueryProtocolAccordingPid()", "sqlite3_get_table err!-1");
        value = -1;
    }
    sqlite3_free_table (dbResult);
    return value;
}

static inline int Get_DID_From_PID (sqlite3 * db, int PID)
{
    int result;
    char *errmsg = NULL;
    char **dbResult;
    int nRow, nColumn;
    int index;
    char SQL[256];
    int dID;

    sprintf (SQL, "select PID from TNODET where TAGID=%d;", PID);

    result = sqlite3_get_table (db, SQL, &dbResult, &nRow, &nColumn, &errmsg);
    if (SQLITE_OK == result)
    {
        if (nRow == 1 && nColumn == 1)
        {
            dID = strtol (dbResult[1], NULL, 10);
        }
        else
        {
            write_log (&error_log, -1, "Get_DID_From_PID()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
            dID = -1;
        }
    }
    else
    {
        write_log (&error_log, -1, "Get_DID_From_PID()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
        dID = -1;
    }
    sqlite3_free_table (dbResult);
    return dID;
}

static inline float Get_K_From_PID (sqlite3 * db, int PID)
{
    int result;
    char *errmsg = NULL;
    char **dbResult;
    int nRow, nColumn;
    int index;
    char SQL[256];
    float K;

    sprintf (SQL, "select mods,is_use,n_mods from T_PCT_T where tid=%d", PID);

    result = sqlite3_get_table (db, SQL, &dbResult, &nRow, &nColumn, &errmsg);
    if (SQLITE_OK == result)
    {
        if (nRow == 1 && nColumn == 3)
        {
            float mods = atof (dbResult[3]);
            int is_use = strtol (dbResult[4], NULL, 10);
            float n_mods = atof (dbResult[5]);
            if (is_use == 1)
                K = mods;
            else
                K = n_mods;
        }
        else
        {
            write_log (&error_log, -1, "Get_K_From_PID()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
            K = -1;
        }
    }
    else
    {
        write_log (&error_log, -1, "Get_K_From_PID()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
        K = -1;
    }
    sqlite3_free_table (dbResult);
    return K;
}

static inline int QueryAttAccordingDid (sqlite3 * db, int ID, int TYPE, char *result, int result_size)
{
    int ret;
    char *errmsg = NULL;
    char **dbResult;
    int nRow, nColumn;
    int index;
    char SQL[256];

    sprintf (SQL, "select A.value from T_DEV_COL as A,T_DCOLE as B \
            where A.did=%d and A.seq=B.dc_name and B.ID=%d;", ID, TYPE);
    ret = sqlite3_get_table (db, SQL, &dbResult, &nRow, &nColumn, &errmsg);
    if (SQLITE_OK == ret)
    {
        if (nRow == 1 && nColumn == 1)
        {
            strlcpy (result, dbResult[1], result_size);
            ret = 0;
        }
        else
        {
            write_log (&error_log, -1, "QueryAttAccordingDid()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
            ret = -1;
        }
    }
    else
    {
        write_log (&error_log, -1, "QueryAttAccordingDid()", "sqlite3_get_table err!-2");
        ret = -1;
    }
    sqlite3_free_table (dbResult);
    return ret;
}

//PID         integer not null primary key,
static inline int CreateTempModbusTable (sqlite3 * db)
{
    int result;
    char *errmsg = NULL;
    char SQL[] = "create table if not exists WCX_COLLECT_TMP    \
                (   \
                    PID         integer not null primary key,   \
                    Com         integer not null,   \
                    Slave       integer not null,   \
                    GAP         integer not null,   \
                    TimeOut     integer not null,   \
                    Times       integer not null,   \
                    DataType    integer not null,   \
                    PType       integer not null,   \
                    Addr        integer not null,   \
                    Number      integer not null,   \
                    Operator    integer not null,   \
                    XiShu       double not null,    \
                    ModbusType  integer not null    \
                )";

    result = sqlite3_exec (db, SQL, NULL, 0, &errmsg);
    if (result != SQLITE_OK)
    {
        write_log (&error_log, -1, "CreateTempModbusTable()", "sqlite3_exec err!");
        return -1;
    }
    else
        return result;
}

static inline int creat_ort3000c_table (sqlite3 * db)
{
    int result;
    char *errmsg = NULL;
    char SQL[] = "create table if not exists ort3000c   \
                (   \
                    kaichu         integer not null   \
                )";

    result = sqlite3_exec (db, SQL, NULL, 0, &errmsg);
    if (result != SQLITE_OK)
    {
        write_log (&error_log, -1, "creat_ort3000c_table()", "sqlite3_exec err!");
        return -1;
    }
    else
        return result;
}

static inline int insert_ort3000c_table (sqlite3 * db, int kaichu)
{
    int result;
    char *errmsg = NULL;
    char SQL[512];

    sprintf (SQL, "INSERT INTO ort3000c VALUES (%d)", kaichu);

    result = sqlite3_exec (db, SQL, NULL, 0, &errmsg);
    if (result != SQLITE_OK)
    {
        write_log (&error_log, -1, "insert_ort3000c_table()", "sqlite3_exec err!");
        return -1;
    }
    else
        return result;
}

static inline int InsertTempModbusTable (sqlite3 * db,
        int PID, int COM, int SLAVE, int GAP, int TIMEOUT, int TIMES, int DTYPE, int PTYPE, int ADDR, int NUBBER,
        int OPERATOR, double XISHU, int MODBUSTYPE)
{
    int result;
    char *errmsg = NULL;
    char SQL[512];

    //printf ("InsertTempModbusTable, pid:%d\n", PID);
    sprintf (SQL, "INSERT INTO WCX_COLLECT_TMP VALUES (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%lf,%d)",
             PID, COM, SLAVE, GAP, TIMEOUT, TIMES, DTYPE, PTYPE, ADDR, NUBBER, OPERATOR, XISHU, MODBUSTYPE);

    result = sqlite3_exec (db, SQL, NULL, 0, &errmsg);
    if (result != SQLITE_OK)
    {
        write_log (&error_log, -1, "InsertTempModbusTable()", "sqlite3_exec err:%s, pid:%d", errmsg, PID);
        return -1;
    }
    else
        return result;
}

int LoadTagList ()
{
    TagTp tmp;
    sqlite3 * db;
    sqlite3 * db_optimized_tag_modbus;
    char * errmsg = NULL;
    char **dbResult;
    int nRow, nColumn;
    int result;
    int i, j;
    int index;
    char SQL[512];
    uint8_t fun[] = {0x03, 0x04, 0x06, 0x10, 0x01, 0x02, 0x05, 0x0f};
    int PID, DID, COM, SLAVE, GAP, TIMEOUT, TIMES, MODBUSTYPE, DTYPE, PTYPE, ADDR, NUMBER, OPERATOR;
    double XISHU;
    char str[128];
    int fd_io;
    char buffer[30];

    /* clear Tag */
    myself.Tag.clear();
    for (i = 0; i < SERIES_MAX; i++)
    {
        Serial[i].Tag.clear();
    }

    //printf ("load tag............................ 0\n");
    /* get optimized_tag_modbus filename */
    char optimized_tag_modbus[128];
    strlcpy (optimized_tag_modbus, g_path_fast_tag, sizeof (optimized_tag_modbus));
    strlcat (optimized_tag_modbus, "/optimized_tag_modbus.db", sizeof (optimized_tag_modbus));

    //printf ("load tag............................ 1\n");
    /* if optimized_tag_modbus is exist goto read_db_optimized_tag_modbus */
    int fd = open (optimized_tag_modbus, O_RDONLY, 0600);
    if (fd != -1)
    {
        close (fd);
        /* open optimized_tag */
        result = sqlite3_open (optimized_tag_modbus, &db_optimized_tag_modbus);
        if (result != SQLITE_OK)
        {
            printf ("sqlite3_open err!!!!!!!!!!!!!!!!!!!!!!!!!path_db!!!!!!!!:%s\n", g_path_db);
            exit (1);
        }
        goto read_db_optimized_tag_modbus;
    }

    /* open sysdb */
    //printf ("load tag............................ 2\n");
    result = sqlite3_open (g_path_db, &db);
    if (result != SQLITE_OK)
    {
        printf ("sqlite3_open err!!!!!!!!!!!!!!!!!!!!!!!!!path_db!!!@@:%s\n", g_path_db);
        exit (1);
    }

    /* open optimized_tag */
    //printf ("load tag............................ 3\n");
    result = sqlite3_open (optimized_tag_modbus, &db_optimized_tag_modbus);
    if (result != SQLITE_OK)
    {
        printf ("open optimized_tag_modbus err! path_db:%s\n", g_path_db);
        exit (1);
    }

    /* 读出ort3000c开出量初值 */
    //printf ("load tag............................ 4\n");
    kaichu_value = 0xff; //0000---默认全为开 (0:继电器闭合, 1:继电器打开)
    result = QueryKaiChuAccordingPid (db, KAICU_0);
    if (result < 0)
    {
        exit (1);
    }
    else if (result == 1) //is 合
    {
        kaichu_value &= 0x0e; //1110
    }
    result = QueryKaiChuAccordingPid (db, KAICU_1);
    if (result < 0)
    {
        exit (1);
    }
    else if (result == 1)
    {
        kaichu_value &= 0x0d; //1101
    }
    result = QueryKaiChuAccordingPid (db, KAICU_2);
    if (result < 0)
    {
        exit (1);
    }
    else if (result == 1)
    {
        kaichu_value &= 0x0b; //1011
    }
    result = QueryKaiChuAccordingPid (db, KAICU_3);
    if (result < 0)
    {
        exit (1);
    }
    else if (result == 1)
    {
        kaichu_value &= 0x07; //0111
    }
    //write_log (&boot_log, -1, "LoadTagList()", "kaichu_value:%d", kaichu_value);
    //printf ("!!!!!!!!!!!!!!!!!!kaichu_value:%d\n", kaichu_value);
    //getchar ();

    //--->write the kaichu init value
    buffer[0] = kaichu_value;
    write (fd_io, buffer, 1);

    /* Create ort3000c_table */
    //printf ("load tag............................ 5\n");
    result = creat_ort3000c_table (db_optimized_tag_modbus);
    if (result < 0)
    {
        printf ("creat_ort3000c_table err!\n");
        exit (1);
    }

    /* insert ort3000c table */
    //printf ("load tag............................ 6\n");
    result = insert_ort3000c_table (db_optimized_tag_modbus, kaichu_value);
    if (result < 0)
    {
        printf ("insert_ort3000c_table err!\n");
        exit (1);
    }

    /* Create TempModbusTable */
    //printf ("load tag............................ 7\n");
    result = CreateTempModbusTable (db_optimized_tag_modbus);
    if (result < 0)
    {
        printf ("CreateTempModbusTable err!\n");
        exit (1);
    }

    /* 读取本机测点信息到临时表 */
    //printf ("load tag............................ 8\n");
    wprintf ("read myself tag to temp table...\n");
    //串口号(0)
    //测点ID(1)
    //设备地址(2)
    //测点类型(3)
    //数据地址(4)
    //数据数量(5)
    //系数(6)
    //运算符(7)
    //超时周期(8)
    //数据类型(9)
    //总线空闲时间(10)
    //失败后重试(11)
    result = sqlite3_get_table (db, "select A.COMNO,B.TID,A.DADDR,B.TYPE,B.DAADR,B.DANUM,B.MDU,B.OPE,\
            B.STIME,B.TTYPE,A.RETIME,A.STIME from DCSET as A,PCSET as B where A.DID=B.DID and B.TID < 1000\
            order by A.DADDR,B.TYPE,B.DAADR", &dbResult, &nRow, &nColumn, &errmsg);
    if (SQLITE_OK == result)
    {
        index = nColumn;
#ifdef PRINT_TAG
        wprintf ("查到%d条记录\n", nRow);
#endif
        for (i = 0; i < nRow ; i++)
        {
            //--->print to screen
#ifdef PRINT_TAG
            wprintf ("第 %d 条记录\n", i + 1);
#endif
            for (j = 0 ; j < nColumn; j++)
            {
#ifdef PRINT_TAG
                wprintf (" %s | %s |", dbResult[j], dbResult [index++]);
#endif
            }
#ifdef PRINT_TAG
            wprintf ("\n");
#endif
            //--->insert to temp table
            int base = nColumn * (i + 1);
            PID = strtol (dbResult[base + 1], NULL, 10);
            COM = strtol (dbResult[base + 0], NULL, 10) + 1;
            SLAVE = strtol (dbResult[base + 2], NULL, 10);
            GAP = strtol (dbResult[base + 10], NULL, 10);
            TIMEOUT = strtol (dbResult[base + 8], NULL, 10);
            TIMES = strtol (dbResult[base + 11], NULL, 10);
            //DTYPE = strtol (dbResult[base + 9], NULL, 10);
            if (PID >= 101 && PID <= 106)
                DTYPE = UINT16;
            else if (PID >= 107 && PID <= 114)
                DTYPE = INT16;
            else if (PID >= 115 && PID <= 120)
                DTYPE = UINT16;
            else if (PID >= 121 && PID <= 126)
                DTYPE = INT16;
            else if (PID >= 127 && PID <= 128)
                DTYPE = UINT32;
            else if (PID >= 129 && PID <= 314)
                DTYPE = UINT16;
            else if (PID >= 315 && PID <= 318)
                DTYPE = UINT8;
            //else
            //{
            //printf ("err: PID:%d, DTYPE:%d\n", PID, DTYPE);
            //write_log (&error_log, -1, "LoadTagList()", "DTYPE err!");
            //exit (1);
            //}
            //printf ("PID:%d, DTYPE:%d\n", PID, DTYPE);
            PTYPE = strtol (dbResult[base + 3], NULL, 10);
            ADDR = strtol (dbResult[base + 4], NULL, 10);
            NUMBER = strtol (dbResult[base + 5], NULL, 10);
            OPERATOR = strtol (dbResult[base + 7], NULL, 10);
            //XISHU = atof (dbResult[base + 6]);
            XISHU = Get_K_From_PID (db, PID);
            if (XISHU == -1)
            {
                exit (1);
            }

            int ret = InsertTempModbusTable (db_optimized_tag_modbus, PID, COM, SLAVE, GAP, TIMEOUT, TIMES, DTYPE,
                                             PTYPE, ADDR, NUMBER, OPERATOR, XISHU, 1);
            if (ret < 0)
            {
                printf ("InsertTempModbusTable err!\n");
                exit (1);
            }
        } /* end for nRow */
    }
    else
    {
        printf ("sqlite3_get_table err!-3\n");
        write_log (&error_log, -1, "LoadTagList()", "sqlite3_get_table err!-3");
        exit (1);
    }
    sqlite3_free_table (dbResult);

    /* 读取用户配置测点信息到临时表 */
    //printf ("load tag............................ 9\n");
    wprintf ("read user tag to temp table...\n");
    result = sqlite3_get_table (db, "select DISTINCT pid from T_POT_COL", &dbResult, &nRow, &nColumn, &errmsg);
    if (SQLITE_OK == result)
    {
        index = nColumn;
        for (i = 0; i < nRow ; i++)
        {
            for (j = 0 ; j < nColumn; j++)
            {
                PID = strtol (dbResult[index++], NULL, 10);
                int ret = QueryProtocolAccordingPid (db, PID);
                if (ret < 0)
                {
                    printf ("QueryProtocolAccordingPid err!\n");
                    exit (1);
                }
                else if (ret == 1)
                {
#ifdef PRINT_TAG
                    wprintf ("PID:%d\n", PID);
#endif
                    //--->get DID
                    DID = Get_DID_From_PID (db, PID);
                    if (DID < 0)
                    {
                        printf ("Get_DID_From_PID err!\n");
                        exit (1);
                    }
                    else
                    {
#ifdef PRINT_TAG
                        wprintf ("%d | ", PID);
#endif
                    }
                    //--->get COM number
                    result = QueryAttAccordingDid (db, DID, 1, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("QueryAttAccordingDid err!\n");
                        exit (1);
                    }
                    else
                    {
                        COM = strtol (str, NULL, 10);
#ifdef PRINT_TAG
                        wprintf ("%d | ", COM);
#endif
                    }
                    //--->get SLAVE
                    result = QueryAttAccordingDid (db, DID, 2, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("QueryAttAccordingDid err!\n");
                        exit (1);
                    }
                    else
                    {
                        SLAVE = strtol (str, NULL, 10);
#ifdef PRINT_TAG
                        wprintf ("%d | ", SLAVE);
#endif
                    }
                    //--->get GAP (modbus 总线空闲时间)
                    result = QueryAttAccordingDid (db, DID, 3, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("QueryAttAccordingDid err!\n");
                        exit (1);
                    }
                    else
                    {
                        GAP = strtol (str, NULL, 10);
#ifdef PRINT_TAG
                        wprintf ("%d | ", GAP);
#endif
                    }
                    //--->get TIMEOUT (modbus 超时时间)
                    result = QueryAttAccordingDid (db, DID, 4, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("QueryAttAccordingDid err!\n");
                        exit (1);
                    }
                    else
                    {
                        TIMEOUT = strtol (str, NULL, 10);
#ifdef PRINT_TAG
                        wprintf ("%d | ", TIMEOUT);
#endif
                    }
                    //--->get TIMES (modbus 失败重试次数)
                    result = QueryAttAccordingDid (db, DID, 5, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("get TIMES err!\n");
                        exit (1);
                    }
                    else
                    {
                        TIMES = strtol (str, NULL, 10);
#ifdef PRINT_TAG
                        wprintf ("%d | ", TIMES);
#endif
                    }
                    //--->get ModbusType (modbus 类型)
                    result = QueryAttAccordingDid (db, DID, 6, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("get ModbusType err! exit!\n");
                        exit (1);
                    }
                    else
                    {
                        MODBUSTYPE = strtol (str, NULL, 10);
#ifdef PRINT_TAG
                        wprintf ("%d | ", MODBUSTYPE);
#endif
                    }
                    //--->get DTYPE (数据类型)
                    result = QueryAttAccordingPid (db, PID, 1, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("get DTYPE err!\n");
                        exit (1);
                    }
                    else
                    {
                        DTYPE = strtol (str, NULL, 10);
                        if (DTYPE > 9)
                        {
                            printf ("DTYPE(数据类型) out range!\n");
                            exit (1);
                        }
#ifdef PRINT_TAG
                        wprintf ("%d | ", DTYPE);
#endif
                    }
                    //--->get PTYPE (测点类型)
                    result = QueryAttAccordingPid (db, PID, 2, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("get PTYPE err!\n");
                        exit (1);
                    }
                    else
                    {
                        PTYPE = strtol (str, NULL, 10);
#ifdef PRINT_TAG
                        wprintf ("%d | ", PTYPE);
#endif
                    }
                    //--->get ADDR (数据地址)
                    result = QueryAttAccordingPid (db, PID, 3, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("get ADDR err!\n");
                        exit (1);
                    }
                    else
                    {
                        ADDR = strtol (str, NULL, 10);
#ifdef PRINT_TAG
                        wprintf ("%d | ", ADDR);
#endif
                    }
                    //--->get NUMBER (数据数量)
                    result = QueryAttAccordingPid (db, PID, 4, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("get Number err!\n");
                        exit (1);
                    }
                    else
                    {
                        NUMBER = strtol (str, NULL, 10);
#ifdef PRINT_TAG
                        wprintf ("%d | ", NUMBER);
#endif
                    }
                    //--->get OPERATOR (运算符)
                    result = QueryAttAccordingPid (db, PID, 5, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("get Opertor err!\n");
                        exit (1);
                    }
                    else
                    {
                        OPERATOR = strtol (str, NULL, 10);
#ifdef PRINT_TAG
                        wprintf ("%d | ", OPERATOR);
#endif
                    }
                    //--->get XISHU
#if 0
                    result = QueryAttAccordingPid (db, PID, 6, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("get xishu err!\n");
                        exit (1);
                    }
                    else
                    {
                        XISHU = atof (str);
#ifdef PRINT_TAG
                        wprintf ("%d | ", COM);
#endif
                    }
#else
                    XISHU = Get_K_From_PID (db, PID);
                    if (XISHU == -1)
                    {
                        exit (1);
                    }
#endif
                    //--->插入临时表
                    int ret = InsertTempModbusTable (db_optimized_tag_modbus, PID, COM, SLAVE, GAP, TIMEOUT,
                                                     TIMES, DTYPE, PTYPE, ADDR, NUMBER, OPERATOR, XISHU, MODBUSTYPE);
                    if (ret < 0)
                    {
                        printf ("InsertTempModbusTable err!\n");
                        exit (1);
                    }
                }
            }
#ifdef PRINT_TAG
            wprintf ("\n");
#endif
        }
    }
    else
    {
        printf ("sqlite3_get_table err!-4\n");
        write_log (&error_log, -1, "LoadTagList()", "sqlite3_get_table err!-4");
        exit (1);
    }
    sqlite3_free_table (dbResult);
    sqlite3_close (db);

read_db_optimized_tag_modbus:

    /* read ort3000c table */
    //printf ("load tag............................ 10\n");
    result = sqlite3_get_table (db_optimized_tag_modbus,
                                "select kaichu from ort3000c",
                                &dbResult, &nRow, &nColumn, &errmsg);
    //printf ("load tag............................ 10.0\n");
    if (SQLITE_OK == result)
    {
        //printf ("load tag............................ 10.1\n");
        if (nRow == 1 && nColumn == 1)
        {
            kaichu_value = strtol (dbResult[1], NULL, 10);
            //--->write the kaichu init value
            buffer[0] = kaichu_value;
            write (fd_io, buffer, 1);
        }
        else
        {
            write_log (&error_log, -1, "LoadTagList()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
            printf ("read ort3000c table err!\n");
            MySleep (2, 0);
            exit (1);
        }
    }
    else
    {
        //printf ("load tag............................ 10.2\n");
        printf ("sqlite3_get_table err! when read ort3000c table!\n");
        write_log (&error_log, -1, "LoadTagList()", "sqlite3_get_table err!-5");
        exit (1);
    }
    sqlite3_free_table (dbResult);

    //printf ("load tag............................ 11\n");
    /* 从临时表按一定顺序读出所有信息到内存 */
    wprintf ("read tag from temp table...\n");
    result = sqlite3_get_table (db_optimized_tag_modbus,
                                "select PID, Com, Slave, GAP, TimeOut, Times, DataType, PType, Addr, Number,Operator,XiShu,ModbusType\
                                from WCX_COLLECT_TMP\
                                order by Slave, PType, Addr",
                                &dbResult, &nRow, &nColumn, &errmsg);
    if (SQLITE_OK == result)
    {
        index = nColumn;
#ifdef PRINT_TAG
        wprintf ("查到%d条记录\n", nRow);
#endif
        for (i = 0; i < nRow ; i++)
        {
            //--->print to screen
#ifdef PRINT_TAG
            wprintf ("第 %d 条记录\n", i + 1);
#endif
            for (j = 0 ; j < nColumn; j++)
            {
#ifdef PRINT_TAG
                wprintf (" %s | %s |", dbResult[j], dbResult [index++]);
#endif
            }
#ifdef PRINT_TAG
            wprintf ("\n");
#endif
            //--->print to vector
            int base = nColumn * (i + 1);
            COM = strtol (dbResult[base + 1], NULL, 10);
            if (COM <= 7 && COM >= 1)
                tmp.Id = (uint16_t) strtol (dbResult[base + 0], NULL, 10);
            else
            {
                write_log (&error_log, -1, "load_taglist()", "COM Error:%d", COM);
                exit (1);
            }
            tmp.SpaceTime = (unsigned int) strtol (dbResult[base + 3], NULL, 10);
            tmp.K = atof (dbResult[base + 11]);
            tmp.ModbusType = (unsigned int) strtol (dbResult[base + 12], NULL, 10);
            tmp.Opertion = (uint16_t) strtol (dbResult[base + 10], NULL, 10);
            tmp.TimeOut = (uint16_t) strtol (dbResult[base + 4], NULL, 10);
            tmp.DataType = (uint16_t) strtol (dbResult[base + 6], NULL, 10);
            tmp.ModBusQueryFrame[0] = (uint8_t) strtol (dbResult[base + 2], NULL, 10);
            //tmp.ModBusQueryFrame[1] = (uint16_t) strtol (dbResult[base + 7], NULL, 10);
            uint8_t fun_no = strtol (dbResult[base + 7], NULL, 10);
            if (fun_no <= FC_REPORT_SLAVE_ID)
                tmp.ModBusQueryFrame[1] = fun[fun_no];
            else
            {
                write_log (&error_log, -1, "load_taglist()", "fun Error:%d", fun_no);
                exit (1);
            }
            uint16_t DataAddr = (uint16_t) strtol (dbResult[base + 8], NULL, 10);
            tmp.ModBusQueryFrame[2] = (DataAddr >> 8) & 0xff;
            tmp.ModBusQueryFrame[3] = DataAddr & 0xff;
            uint16_t RegisterQuantity = (uint16_t) strtol (dbResult[base + 9], NULL, 10);
            tmp.ModBusQueryFrame[4] = (RegisterQuantity >> 8) & 0xff;
            tmp.ModBusQueryFrame[5] = RegisterQuantity & 0xff;

            switch (tmp.ModBusQueryFrame[1])
            {
            case    0x01:
            case    0x02:
            case    0x03:
            case    0x04:
            case    0x05:
            case    0x06:
                tmp.QueryFrameLen = 6;
                break;
            case    0x0F:
            case    0x10:
            case    0x14:
            case    0x16:
            case    0x17:
            case    0x2B:
                break;
            default:
                tmp.QueryFrameLen = 6;
                write_log (&error_log, -1, "load_taglist()", "fun code error:%d", tmp.ModBusQueryFrame[1]);
            } /* end switch */

            uint16_t nCrc16 = crc16 (tmp.ModBusQueryFrame, tmp.QueryFrameLen);
            tmp.ModBusQueryFrame[tmp.QueryFrameLen] = (nCrc16 >> 8) & 0xff;
            tmp.QueryFrameLen += 1;
            tmp.ModBusQueryFrame[tmp.QueryFrameLen] = nCrc16 & 0xff;
            tmp.QueryFrameLen += 1;

            if (tmp.Id <= MYSELF_MAX_TAGID)
            {
                myself.Tag.push_back (tmp);
            }
            else
            {
                Serial[COM - 1].Tag.push_back (tmp);
                OptSerial[COM - 1].Tag.push_back (tmp);
            }
        } /* end for nRow */
    }
    else
    {
        write_log (&error_log, -1, "LoadTagList()", "sqlite3_get_table err!-6");
        /* delet the optimized_tags */
        char *cmd;
        cmd = new char[128];
        strlcpy (cmd, "rm -rf ", 128);
        strlcat (cmd, g_path_fast_tag, 128);
        strlcat (cmd, "/optimized_tag_modbus.db", 128);
        system (cmd);
        delete[] cmd;
        pthread_quit ();
    }
    sqlite3_free_table (dbResult);

    /* write taglist.csv */
    //printf ("load tag............................ 12\n");
    if (DEBUG_FLG & DEBUG_CSV)
    {
        printf ("写csv文件...\n");
        char msg[512];
        for (i = 0; i < SERIES_MAX; i++)
        {
            sprintf (msg, "@@@@@@ Series %d @@@@@@", i);
            write_to_log2 (g_taglist_log, msg,  LOG_APPEND | LOG_NOTIME);
            sprintf (msg, "TagNums: \t%d", Serial[i].Tag.size());
            write_to_log2 (g_taglist_log, msg,  LOG_APPEND | LOG_NOTIME);
            write_to_log2 (g_taglist_log, "Id, Length, K, Ope, Time, DataType, Addr,fun_code, DAdrHi, DAdrLo, DNumHi, DNumLo, SpaceTime, ModbusType",  LOG_APPEND | LOG_NOTIME);
            for (j = 0; j < Serial[i].Tag.size(); j++)
            {
                sprintf (msg, "%3d, %d, %f, %d, %d, %d, %2d, %d, %2d, %2d, %2d, %2d, %d, %d",
                         Serial[i].Tag[j].Id,
                         Serial[i].Tag[j].QueryFrameLen,
                         Serial[i].Tag[j].K,
                         Serial[i].Tag[j].Opertion,
                         (int) Serial[i].Tag[j].TimeOut,
                         Serial[i].Tag[j].DataType,
                         Serial[i].Tag[j].ModBusQueryFrame[0], // device_addr
                         Serial[i].Tag[j].ModBusQueryFrame[1], // fun_code
                         Serial[i].Tag[j].ModBusQueryFrame[2], // data_addr_hi
                         Serial[i].Tag[j].ModBusQueryFrame[3], // data_addr_lo
                         Serial[i].Tag[j].ModBusQueryFrame[4], // data_quantity_hi
                         Serial[i].Tag[j].ModBusQueryFrame[5], // data_quantity_lo
                         Serial[i].Tag[j].SpaceTime,
                         Serial[i].Tag[j].ModbusType);
                write_to_log2 (g_taglist_log, msg,  LOG_APPEND | LOG_NOTIME);
            }
            write_to_log2 (g_taglist_log, " ", LOG_APPEND | LOG_NOTIME);
        } /*end for*/
    }

    /* 产生优化点表 */
    //printf ("load tag............................ 13\n");
    wprintf ("生成优化点表....^_^\n");
    int device_addr_pre, fun_code_pre, data_addr_pre, data_quantity_pre, data_type_pre;
    int device_addr_cur, fun_code_cur, data_addr_cur, data_quantity_cur, data_type_cur;
    for (i = 0; i < SERIES_MAX; i++)
    {
        if (Serial[i].Tag.size() <= 0)
        {
            continue;
        }
        OptSerial[i].Tag.clear();
        OptSerial[i].Tag.push_back (Serial[i].Tag[0]);

        device_addr_pre = Serial[i].Tag[0].ModBusQueryFrame[0];
        fun_code_pre = Serial[i].Tag[0].ModBusQueryFrame[1];
        data_addr_pre = (unsigned int) Serial[i].Tag[0].ModBusQueryFrame[2] << 8 | (unsigned int) Serial[i].Tag[0].ModBusQueryFrame[3];
        data_quantity_pre = (unsigned int) Serial[i].Tag[0].ModBusQueryFrame[4] << 8 | (unsigned int) Serial[i].Tag[0].ModBusQueryFrame[5];
        data_type_pre = (int) Serial[i].Tag[0].DataType;

        unsigned long nDataLen = data_quantity_pre;
        for (j = 1; j < Serial[i].Tag.size(); j++)
        {
            device_addr_cur = Serial[i].Tag[j].ModBusQueryFrame[0];
            fun_code_cur = Serial[i].Tag[j].ModBusQueryFrame[1];
            data_addr_cur = (unsigned int) Serial[i].Tag[j].ModBusQueryFrame[2] << 8 | (unsigned int) Serial[i].Tag[j].ModBusQueryFrame[3];
            data_quantity_cur = (unsigned int) Serial[i].Tag[j].ModBusQueryFrame[4] << 8 | (unsigned int) Serial[i].Tag[j].ModBusQueryFrame[5];
            data_type_cur = (int) Serial[i].Tag[j].DataType;
            int bits_pre = 0;

            switch (data_type_pre)
            {
            case    0:      // 数据类型为8位整数
            case    1:      // 数据类型为8位无符号整数
                bits_pre = 8;
                break;
            case    2:      // 数据类型为16位整数
            case    3:      // 数据类型为16位无符号整数
                bits_pre = 16;
                break;
            case    4:      // 数据类型为32位整数
            case    5:      // 数据类型为32位无符号整数
                bits_pre = 32;
                break;
            case    6:      // 数据类型为64位整数
            case    7:      // 数据类型为64位无符号整数
                bits_pre = 64;
                break;
            case    8:      // 单精度浮点数
                bits_pre = 128;
                break;
            case    9:      // 双精度浮点数
                bits_pre = 256;
                break;
            default:
                write_log (&error_log, -1, "load_taglist()",
                           "data_type_pre default: %d", data_type_pre);
                pthread_quit ();
            }

            int bits_cur = 0;

            switch (data_type_cur)
            {
            case    0:      // 数据类型为8位整数
            case    1:      // 数据类型为8位无符号整数
                bits_cur = 8;
                break;
            case    2:      // 数据类型为16位整数
            case    3:      // 数据类型为16位无符号整数
                bits_cur = 16;
                break;
            case    4:      // 数据类型为32位整数
            case    5:      // 数据类型为32位无符号整数
                bits_cur = 32;
                break;
            case    6:      // 数据类型为64位整数
            case    7:      // 数据类型为64位无符号整数
                bits_cur = 64;
                break;
            case    8:      // 单精度浮点数
                bits_cur = 128;
                break;
            case    9:      // 双精度浮点数
                bits_cur = 256;
                break;
            default:
                write_log (&error_log, -1, "load_taglist()",
                           "data_type_cur default: %d", data_type_cur);
                pthread_quit ();
            }

            if ( (device_addr_cur == device_addr_pre) && (fun_code_cur == fun_code_pre) && (data_addr_cur == data_addr_pre + data_quantity_pre)
                    && (bits_cur == bits_pre) && (nDataLen < MAX_DATE_LEN))
            {
                nDataLen += data_quantity_cur;
                data_addr_pre = data_addr_cur;
                data_quantity_pre = data_quantity_cur;

                if (j == Serial[i].Tag.size() - 1)
                {
                    /*修改上一条记录的部分字段: 数据长度+Crc6校验*/
                    std::vector<TagTp>::iterator  iter;
                    iter = OptSerial[i].Tag.end() - 1;
                    iter->ModBusQueryFrame[4] = (nDataLen >> 8) & 0xff;
                    iter->ModBusQueryFrame[5] = nDataLen & 0xff;
                    uint16_t nCrc16 = crc16 (iter->ModBusQueryFrame, iter->QueryFrameLen - 2);
                    iter->ModBusQueryFrame[iter->QueryFrameLen - 2] = (nCrc16 >> 8) & 0xff;     // crc16Hi
                    iter->ModBusQueryFrame[iter->QueryFrameLen - 1] = nCrc16 & 0xff;       // crc16Lo
                }
            }
            else
            {
                device_addr_pre = device_addr_cur;
                fun_code_pre = fun_code_cur;
                data_addr_pre = data_addr_cur;
                data_quantity_pre = data_quantity_cur;
                data_type_pre = data_type_cur;
                /*修改上一条记录的部分字段: 数据长度+ Crc6校验*/
                std::vector<TagTp>::iterator  iter;
                iter = OptSerial[i].Tag.end() - 1;

                switch (iter->DataType)
                {
                case    0:      // 数据类型为8位整数
                case    1:      // 数据类型为8位无符号整数
                case    2:      // 数据类型为16位整数
                case    3:      // 数据类型为16位无符号整数
                    break;
                case    4:      // 数据类型为32位整数
                case    5:      // 数据类型为32位无符号整数
                    //nDataLen <<= 1;
                    break;
                case    6:      // 数据类型为64位整数
                case    7:      // 数据类型为64位无符号整数
                case    8:      // 单精度浮点数
                case    9:      // 双精度浮点数
                default:
                    write_log (&error_log, -1, "load_taglist()",
                               "iter->DataType default: %d", iter->DataType);
                    pthread_quit ();
                } /*end switch*/

                iter->ModBusQueryFrame[4] = (nDataLen >> 8) & 0xff;
                iter->ModBusQueryFrame[5] = nDataLen & 0xff;
                uint16_t nCrc16 = crc16 (iter->ModBusQueryFrame, iter->QueryFrameLen - 2);
                iter->ModBusQueryFrame[iter->QueryFrameLen - 2] = (nCrc16 >> 8) & 0xff;     // crc16Hi
                iter->ModBusQueryFrame[iter->QueryFrameLen - 1] = nCrc16 & 0xff;       // crc16Lo
                /*将当前记录写入优化点表尾部*/
                OptSerial[i].Tag.push_back (Serial[i].Tag[j]);
                nDataLen = data_quantity_pre;
            }
        } /*end for*/
    } /*end for*/

    /* 打印优化后的数据 */
    if (DEBUG_FLG & DEBUG_CSV)
    {
        printf ("写csv文件...\n");
        char msg[512];
        write_to_log2 (g_taglist_log, " ",  LOG_APPEND | LOG_NOTIME);
        write_to_log2 (g_taglist_log, " ",  LOG_APPEND | LOG_NOTIME);
        write_to_log2 (g_taglist_log, "@@@@@@~Optimize~@@@@@@",  LOG_APPEND | LOG_NOTIME);
        write_to_log2 (g_taglist_log, " ",  LOG_APPEND | LOG_NOTIME);

        for (i = 0; i < SERIES_MAX; i++)
        {
            sprintf (msg, "@@@@@@ Series %d @@@@@@", i);
            write_to_log2 (g_taglist_log, msg,  LOG_APPEND | LOG_NOTIME);
            sprintf (msg, "TagNums: \t%d", OptSerial[i].Tag.size());
            write_to_log2 (g_taglist_log, msg,  LOG_APPEND | LOG_NOTIME);
            write_to_log2 (g_taglist_log, "Id, Length, K, Ope, Time, DataType, Addr,fun_code, DAdrHi, DAdrLo, DNumHi, DNumLo, SpaceTime",  LOG_APPEND | LOG_NOTIME);

            for (j = 0; j < OptSerial[i].Tag.size(); j++)
            {
                sprintf (msg, "%3d, %d, %f, %d, %d, %d, %2d, %d, %2d, %2d, %2d, %2d, %d, %d",
                         OptSerial[i].Tag[j].Id,
                         OptSerial[i].Tag[j].QueryFrameLen,
                         OptSerial[i].Tag[j].K,
                         OptSerial[i].Tag[j].Opertion,
                         (int) OptSerial[i].Tag[j].TimeOut,
                         OptSerial[i].Tag[j].DataType,
                         OptSerial[i].Tag[j].ModBusQueryFrame[0], // device_addr
                         OptSerial[i].Tag[j].ModBusQueryFrame[1], // fun_code
                         OptSerial[i].Tag[j].ModBusQueryFrame[2], // data_addr_hi
                         OptSerial[i].Tag[j].ModBusQueryFrame[3], // data_addr_lo
                         OptSerial[i].Tag[j].ModBusQueryFrame[4], // data_quantity_hi
                         OptSerial[i].Tag[j].ModBusQueryFrame[5], // data_quantity_lo
                         OptSerial[i].Tag[j].SpaceTime,
                         OptSerial[i].Tag[j].ModbusType);
                write_to_log2 (g_taglist_log, msg,  LOG_APPEND | LOG_NOTIME);
            }
            write_to_log2 (g_taglist_log, " ",  LOG_APPEND | LOG_NOTIME);
        } /*end for*/
    }

    sqlite3_close (db_optimized_tag_modbus);
    return 0;
}

/**
 * @brief             根据测点ID查串口号 设备地址 数据地址
 *
 * @param   sn        测点ID
 * @param   com       串口号
 * @param   slave     设备地址
 * @param   addr      数据地址
 *
 * @return 0：OK,  -1：ERROR
 */
int get_comno_slave_addr (int TagID, int * com, int * slave, int * fun, int * addr, int * value)
{
    sqlite3 *db;
    int result;
    char str[128];
    int DID;

    result = sqlite3_open (g_path_db, &db);
    if (result != SQLITE_OK)
    {
        return -1;
    }

    //--->get DID
    DID = Get_DID_From_PID (db, TagID);
    if (DID < 0)
    {
        write_log (&error_log, -1, "get_comno_slave_addr()", "get DID err!");
        return -1;
    }

    //--->get com
    result = QueryAttAccordingDid (db, DID, 1, str, sizeof (str));
    if (result < 0)
    {
        write_log (&error_log, -1, "get_comno_slave_addr()", "get COM err!");
        return -1;
    }
    else
    {
        *com = strtol (str, NULL, 10) - 1;
    }

    //--->get slave
    result = QueryAttAccordingDid (db, DID, 2, str, sizeof (str));
    if (result < 0)
    {
        write_log (&error_log, -1, "get_comno_slave_addr()", "get slave err!");
        return -1;
    }
    else
        *slave = strtol (str, NULL, 10);

#if 1
    //--->get fun
    result = QueryAttAccordingPid (db, TagID, 2, str, sizeof (str));
    if (result < 0)
    {
        write_log (&error_log, -1, "get_comno_slave_addr()", "get fun err!");
        return -1;
    }
    else
        *fun = strtol (str, NULL, 10);
#endif

    //--->get addr
    result = QueryAttAccordingPid (db, TagID, 3, str, sizeof (str));
    if (result < 0)
    {
        write_log (&error_log, -1, "get_comno_slave_addr()", "get addr err!");
        return -1;
    }
    else
        *addr = strtol (str, NULL, 10);

    //--->get value
    result = QueryAttAccordingPid (db, TagID, 4, str, sizeof (str));
    if (result < 0)
    {
        write_log (&error_log, -1, "get_comno_slave_addr()", "get addr err!");
        return -1;
    }
    else
        *value = strtol (str, NULL, 10);

    sqlite3_close (db);
    return 0;
}

int get_ort3000_switch_attr (int PID, int *switch_type, int *delay_time)
{
    int result;
    sqlite3 *db;
    char *errmsg = NULL;
    char **dbResult;
    int nRow, nColumn;
    int index;
    char SQL[256];

    /* open db */
    result = sqlite3_open (g_path_db, &db);
    if (result != SQLITE_OK)
    {
        return -1;
    }

    sprintf (SQL, "select TYPE, DAADR from PCSET where TID=%d", PID);

    /* exec sqlite cmd */
    result = sqlite3_get_table (db, SQL, &dbResult, &nRow, &nColumn, &errmsg);
    if (SQLITE_OK == result)
    {
        if (nRow == 1 && nColumn == 2)
        {
            *switch_type = strtol (dbResult[2], NULL, 10);
            *delay_time = strtol (dbResult[3], NULL, 10);
        }
        else
        {
            write_log (&error_log, -1, "Get_K_From_PID()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
            result = -1;
        }
    }
    else
    {
        write_log (&error_log, -1, "Get_K_From_PID()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
        result = -1;
    }
    sqlite3_free_table (dbResult);
    sqlite3_close (db);
    return result;
}

static void get_config_from_file ()
{
    int i, ret;
    char serial[10];
    char key_value[256];

    /* get serial node */
    for (i = 0; i < SERIES_MAX; i++)
    {
        memset (OptSerial[i].fd, 0, sizeof (OptSerial[i].fd));
        sprintf (serial, "Serial:%d", i);
        ret = ConfigGetKey (COLLECT_CONF_FILE, serial, "file_node", key_value);
        if (ret == 0)
            strncpy (OptSerial[i].fd, key_value, sizeof (OptSerial[i].fd) - 1);
        else
            strncpy (OptSerial[i].fd, "/dev/ttyNULL", sizeof (OptSerial[i].fd) - 1);
    }
    /* get boot_log */
    ret = ConfigGetKey (COLLECT_CONF_FILE, "log", "bootlog", key_value);
    if (ret == 0)
        strlcpy (g_boot_log, key_value, sizeof (g_boot_log));
    else
        strlcpy (g_boot_log, "/var/log/collect_boot.log", sizeof (g_boot_log));
    /* get err_log */
    ret = ConfigGetKey (COLLECT_CONF_FILE, "log", "errlog", key_value);
    if (ret == 0)
        strlcpy (g_err_log, key_value, sizeof (g_err_log));
    else
        strlcpy (g_err_log, "/var/log/collect_err.log", sizeof (g_err_log));
    /* get taglist_log */
    ret = ConfigGetKey (COLLECT_CONF_FILE, "log", "taglistlog", key_value);
    if (ret == 0)
        strlcpy (g_taglist_log, key_value, sizeof (g_taglist_log));
    else
        strlcpy (g_taglist_log, "/var/log/collect_taglist.log", sizeof (g_taglist_log));
    /* get db ip */
    ret = ConfigGetKey (COLLECT_CONF_FILE, "db", "ip", key_value);
    if (ret == 0)
        strlcpy (g_db_ip, key_value, sizeof (g_db_ip));
    else
        strlcpy (g_db_ip, "127.0.0.1", sizeof (g_db_ip));
    /* get db port */
    ret = ConfigGetKey (COLLECT_CONF_FILE, "db", "port", key_value);
    if (ret == 0)
        strlcpy (g_db_port, key_value, sizeof (g_db_port));
    else
        strlcpy (g_db_port, "7000", sizeof (g_db_port));
    /* get g_path_fast_tag */
    ret = ConfigGetKey (COLLECT_CONF_FILE, "db", "path_fast_tag_db", key_value);
    if (ret == 0)
        strlcpy (g_path_fast_tag, key_value, sizeof (g_path_fast_tag));
    else
    {
        printf ("can not get g_path_fast_tag!\n");
        exit (1);
    }
    /* copy sysdb to my folder */
    char syscmd[128];
    memset (syscmd, 0, sizeof (syscmd));
    strlcpy (syscmd, "cp ", sizeof (syscmd));
    ret = ConfigGetKey (COLLECT_CONF_FILE, "db", "path_wyx_db", key_value);
    if (ret == 0)
        strlcat (syscmd, key_value, sizeof (syscmd));
    else
    {
        printf ("can not get path_wyx_db!\n");
        exit (1);
    }
    ret = ConfigGetKey (COLLECT_CONF_FILE, "db", "path_wcx_db", key_value);
    if (ret == 0)
        strlcpy (g_path_db, key_value, sizeof (g_path_db));
    else
    {
        printf ("get path_wcx_db err!\n");
        exit (1);
    }
    strlcat (syscmd, " ", sizeof (syscmd));
    strlcat (syscmd, g_path_db, sizeof (syscmd));
    system (syscmd);

    /* get auto_conf */

    // get condition_id
    ret = ConfigGetKey (COLLECT_CONF_FILE, "auto", "condition_id", key_value);
    if (ret == 0)
    {
        char value[16];
        const char *lps = NULL, *lpe = NULL;
        lpe = lps = key_value;
        auto_control_tp auto_tmp;
        while (lps)
        {
            lps = strchr (lps, ',');
            if (lps)
            {
                memset (value, 0, sizeof (value));
                strncpy (value, lpe, lps - lpe);
                auto_tmp.condition_id = (int) strtol (value, NULL, 10);
                auto_tmp.condition_operator = -1;
                auto_tmp.condition_value = -1;
                auto_tmp.control_id = -1;
                auto_tmp.control_value = -1;
                auto_control.push_back (auto_tmp);
                lps++;
                lpe = lps;
            }
        }
    }
    else
    {
        printf ("can not get condition_id!\n");
        exit (1);
    }
    int auto_control_size = auto_control.size ();
    // get condition_operator
    ret = ConfigGetKey (COLLECT_CONF_FILE, "auto", "condition_operator", key_value);
    if (ret == 0)
    {
        int i = 0;
        char value[16];
        char a_operator = -1;
        const char *lps = NULL, *lpe = NULL;
        lpe = lps = key_value;
        while (lps)
        {
            lps = strchr (lps, ',');
            if (lps)
            {
                memset (value, 0, sizeof (value));
                strncpy (value, lpe, lps - lpe);
                if (strstr (value, "=") != NULL)
                    a_operator = 0;
                else if (strstr (value, ">") != NULL)
                    a_operator = 1;
                else if (strstr (value, ">=") != NULL)
                    a_operator = 2;
                else if (strstr (value, "<") != NULL)
                    a_operator = 3;
                else if (strstr (value, "<=") != NULL)
                    a_operator = 4;
                if ( (i <= auto_control_size) && (a_operator >= 0))
                    auto_control[i].condition_operator = a_operator;
                i++;
                lps++;
                lpe = lps;
            }
        }
    }
    else
    {
        printf ("can not get condition_operator!\n");
        exit (1);
    }
    // get condition_value
    ret = ConfigGetKey (COLLECT_CONF_FILE, "auto", "condition_value", key_value);
    if (ret == 0)
    {
        int i = 0;
        char value[16];
        int condition_value = -1;
        const char *lps = NULL, *lpe = NULL;
        lpe = lps = key_value;
        while (lps)
        {
            lps = strchr (lps, ',');
            if (lps)
            {
                memset (value, 0, sizeof (value));
                strncpy (value, lpe, lps - lpe);
                condition_value = (int) strtol (value, NULL, 10);
                if (i <= auto_control_size)
                    auto_control[i].condition_value = condition_value;
                i++;
                lps++;
                lpe = lps;
            }
        }
    }
    else
    {
        printf ("can not get condition_value!\n");
        exit (1);
    }
    // get control_id
    ret = ConfigGetKey (COLLECT_CONF_FILE, "auto", "control_id", key_value);
    if (ret == 0)
    {
        int i = 0;
        char value[16];
        int control_id = -1;
        const char *lps = NULL, *lpe = NULL;
        lpe = lps = key_value;
        while (lps)
        {
            lps = strchr (lps, ',');
            if (lps)
            {
                memset (value, 0, sizeof (value));
                strncpy (value, lpe, lps - lpe);
                control_id = (int) strtol (value, NULL, 10);
                if (i <= auto_control_size)
                    auto_control[i].control_id = control_id;
                i++;
                lps++;
                lpe = lps;
            }
        }
    }
    else
    {
        printf ("can not get control_id!\n");
        exit (1);
    }
    // get control_value
    ret = ConfigGetKey (COLLECT_CONF_FILE, "auto", "control_value", key_value);
    if (ret == 0)
    {
        int i = 0;
        char value[16];
        int control_value = -1;
        const char *lps = NULL, *lpe = NULL;
        lpe = lps = key_value;
        while (lps)
        {
            lps = strchr (lps, ',');
            if (lps)
            {
                memset (value, 0, sizeof (value));
                strncpy (value, lpe, lps - lpe);
                control_value = (int) strtol (value, NULL, 10);
                if (i <= auto_control_size)
                    auto_control[i].control_value = control_value;
                i++;
                lps++;
                lpe = lps;
            }
        }
    }
    else
    {
        printf ("can not get control_value!\n");
        exit (1);
    }
    /* print the auto_conf */
#if 0
    for (size_t i = 0; i < auto_control_size; i++)
    {
        printf ("condition_id:%3d, condition_operator:%3d, condition_value:%3d, control_id:%3d, control_value:%3d\n",
                auto_control[i].condition_id,
                auto_control[i].condition_operator,
                auto_control[i].condition_value,
                auto_control[i].control_id,
                auto_control[i].control_value
               );
    }
#endif

}

int main (int argc, char **argv)
{
    unsigned int i, j;
    int ret, fd, shmid_watch_dog, shmid_myself_check;
    key_t shmkey_watch_dog;
    key_t shmkey_myself_check;
    pthread_t g_thread_read_cmd;
    pthread_t th_IEC104SLAVE;
    pthread_t th_IEC104MASTER;

    ret = parse_opts (argc, argv);
    if (ret < 0)
    {
        exit (0);
    }

    /* get config from config file */
    wprintf ("get config from file..\n");
    get_config_from_file ();

    /* create tag follder */
    DIR *dp = opendir (g_path_fast_tag);
    if (dp == NULL)
        mkdir (g_path_fast_tag, S_IWRITE);
    else
        closedir (dp);

    /* inite log */
    log_init (&error_log, g_err_log, "ort3000c_modbus_collect", "v0.10.1", "wang_chen_xi", "Error log");
    log_init (&boot_log, g_boot_log, "ort3000c_modbus_collect", "v0.10.1", "wang_chen_xi", "Boot log");

    /* creat log file */
    wprintf ("create csv file...\n");
    write_to_log2 (g_taglist_log, "[TagListFile] -- - recording tag_file", LOG_NEW | LOG_TIME);
    write_to_log2 (g_taglist_log, " ", LOG_APPEND | LOG_NOTIME);

    /* initalization signal handle */
    wprintf ("init signal handle...\n");
    init_sigaction();

    /* get share memory */
    wprintf ("get share memory...\n");
    //watch_dog use
    shmkey_watch_dog = ftok ("/" , 'a');
    shmid_watch_dog = shmget (shmkey_watch_dog , SHM_MAX * sizeof (char) , IPC_CREAT | 0666) ;
    shm_head_watch_dog = shm_pos_watch_dog = (char *) shmat (shmid_watch_dog , 0 , 0);
    //myself_check use
    shmkey_myself_check = ftok ("/" , 'b');
    shmid_myself_check = shmget (shmkey_myself_check , SHM_MAX * sizeof (char) , IPC_CREAT | 0666) ;
    shm_head_myself_check = shm_pos_myself_check = (char *) shmat (shmid_myself_check , 0 , 0);

    write_log (&boot_log, 1, "main()", "demo_start...");

#if 1
    wprintf ("load taglist...\n");
    LoadTagList ();
#endif

    /* create cmd receive thread */
#if 1
    wprintf ("create cmd receive thread...\n");
    //pthread_attr_t  thread_read_cmd_attr;
    //pthread_attr_init ( &thread_read_cmd_attr );
    //pthread_attr_setdetachstate ( &thread_read_cmd_attr, PTHREAD_CREATE_DETACHED );
    //if ( pthread_create ( &g_thread_read_cmd, &thread_read_cmd_attr, read_cmd, NULL ) < 0 )
    if (pthread_create (&g_thread_read_cmd, NULL, thread_read_cmd, NULL) < 0)
    {
        write_log (&error_log, -1, "main()", "pthread_creat(thread_read_cmd) error");
        pthread_quit ();
    }
    write_log (&boot_log, -1, "main()", "thread_read_cmd_attr finished!");
#endif

    wprintf ("init collect...\n");
    init_collect();

    /* create collect threads */
#if 1
    wprintf ("create collect threads...\n");
    for (i = 0; i < SERIES_MAX; i++)
    {
        if (pthread_create (&Serial[i].thread_id, NULL, thread_modbus_collect, (void *) i) != 0)
        {
            write_log (&boot_log, -1, "main()", "thread_send[%d] creat error", i);
            pthread_quit ();
        }
        else
            write_log (&boot_log, -1, "main()", "thread_send[%d] creat OK!", i);
        MySleep (0, 50 * 1000);
    }
#endif

    /* create iec104_master thread */
#if 1
    wprintf ("create iec104_master thread...\n");
    if (pthread_create (&th_IEC104MASTER, NULL, thread_iec104_master, NULL) != 0)
    {
        write_log (&boot_log, -1, "main()", "thread iec104_master create error!");
        pthread_quit ();
        MySleep (5, 1);
        exit (1);
    }
    else
        write_log (&boot_log, -1, "main()", "thread iec104_master create finished!");
#endif

    /* create myself_check thread */
#if 1
    wprintf ("create myself check thread...\n");
    if (pthread_create (&myself.thread_id, NULL, thread_myself_check, NULL) != 0)
    {
        write_log (&boot_log, -1, "main()", "thread_myself_check create error!");
        pthread_quit ();
        MySleep (5, 1);
        exit (1);
    }
    else
        write_log (&boot_log, -1, "main()", "thread_myself_check create finished!");
#endif


exit:
    for (i = 0; i < SERIES_MAX; i++)
    {
        pthread_join (Serial[i].thread_id, NULL);
    }
    pthread_join (myself.thread_id, NULL);
    pthread_join (g_thread_read_cmd, NULL);
    shmdt (shm_head_watch_dog);
    shmdt (shm_head_myself_check);
    wprintf ("collect will exit...\n");
    return 0;
}
