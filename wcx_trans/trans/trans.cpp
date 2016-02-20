/*================================================================================
  _
 | |_   _ __    __ _   _ __    ___
 | __| | '__|  / _` | | '_ \  / __|
 | |_  | |    | (_| | | | | | \__ \
  \__| |_|     \__,_| |_| |_| |___/

================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/errno.h>
#include <time.h>
#include <sys/timeb.h>
#include <ctype.h>
#include <signal.h>
#include <vector>
#include <queue>
#include <iostream>
#include <stdint.h>
#include <sqlite3.h>
#include <dlfcn.h> // dlopen、dlsym、dlerror、dlclose的头文件
#include <fcntl.h>
#include <sys/wait.h>
#include <netinet/in.h> // for sockaddr_in
#include <netinet/tcp.h> // for TCP_NODELAY 
#include <sys/types.h> // for sockeat 
#include <sys/socket.h> // for socket
#include <string.h>
#include <pthread.h>
#include <semaphore.h> // for sem
#include <netdb.h>
#include <arpa/inet.h> // inet_ntoa
#include <unistd.h> // fork, close, access
#include <dirent.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "trans.h"
#include "iop_obj_pool.h"
#include "iop.h"
#include "iop_service.h"
#include "iop_log_service.h"

#include "iop_queue.h"
#include "TimeUse.h"
#include "INIFileOP.h"
#include "crc16.h"
#include "wcx_log.h"
#include "wcx_utils.h"

/* 全局变量 */
static iop_queue m_client;
static iop_queue m_rtu;
static sqlite3 *sysdb; /* 权限管理数据库 */
static int DEBUG_FLG;
static struct log_tp error_log;
static struct log_tp run_log;


#define err_exit(num,fmt,args...)   \
    do{printf("[%s:%d]"fmt"\n",__FILE__,__LINE__,##args);exit(num);}while(0)
#define err_return(num,fmt,args...)   \
    do{printf("[%s:%d]"fmt"\n",__FILE__,__LINE__,##args);return(num);}while(0)

#define WCX_PRINT(format, arg...)   \
    if (DEBUG_FLG & DEBUG_MODE)     \
        printf (format , ##arg)

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

//测试写日志功能。日志服务是一个单独的线程
//当达到缓存大小或一定时间时会自动写入磁盘中。
#if 0
void test_log()
{
    iop_log_service_start (NULL, NULL, 3);
    char buf[1024];

    iop_log_fmt (buf, sizeof (buf), 0, "test log 1.\n");
    WCX_PRINT ("test 1.\n");
    iop_msleep (1000);
    iop_log_fmt (buf, sizeof (buf), 0, "test log 2.\n");
    WCX_PRINT ("test 2.\n");
}
#endif

/* rtu */
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

//解析客户端请求过来的数据
static int rtu_protocol_parse (const void *buf, unsigned int len)
{
    return len;
}

/* 处理前置机发来的信息 */
int rtu_process (const void *buf, unsigned int len, iop_t *iop)
{
    int ret, i;
    BYTE            *rcv_buf = (BYTE*) buf;
    T_C104_FRAME    *p_frame;
    T_C104_ASDU     *p_asdu;

    uint8_t          INFO_SQ; //信息顺序 0：非顺序 1：顺序
    uint16_t         INFO_NUMS; //信息数目

    p_frame = (T_C104_FRAME*) (&rcv_buf[0]);
    p_asdu = (T_C104_ASDU*) (&rcv_buf[6]);

    /* print the rcv date */
    if (DEBUG_FLG & DEBUG_MODE)
    {
        printf ("\033[32;40;1m+rtu:rcv from [%s]:\n\033[0m", "???");
        for (i = 0; i < len; i++)
        {
            printf ("%.2X ", rcv_buf[i]);
            if ( (i + 1) % 50 == 0)
                printf ("\n");
        }
        printf ("\n");
    }

    /* IEC104 Analyse */
    if (len == 6) // U_FORMAT or S_FORMAT
    {
        switch (p_frame->c1)
        {
        case 0x01: // S_Format
            WCX_PRINT ("\033[38;40;1m$ [S帧]\n\033[0m");
            break;
        case 0x0b:
            WCX_PRINT ("\033[38;40;1m$ [建立连接确认帧]\n\033[0m");
            break;
        case 0x43:
            WCX_PRINT ("\033[38;40;1m$ [测试帧]\n\033[0m");
            break;
        default:
            break;
        }
    }
    else //I_format
    {
        switch (p_asdu->type)
        {
        case TYPE_LOG_IN:
        {
            T_LOG_IN_INF *pLogInf = (T_LOG_IN_INF*) (&p_asdu->data[0]);
            /* register client */
            client_tp client;
            client.iop = iop;
            client.sn = pLogInf->sn;
            m_rtu.push (client);
            WCX_PRINT ("\033[38;40;1mrtu connected!\n\033[0m");
            WCX_LOG (&run_log, -1, "rtu_process()", "(%s:%d): rtu connected!", __FILE__, __LINE__);
        }
        break;

        default:
            break;
        }
    }
    WCX_PRINT ("\n\n");

    /* write history */

    /* trans the rtu date to clients */
    int rtu_sn = (unsigned int) p_asdu->addr_asdu_high << 8 | p_asdu->addr_asdu_low;
    for (i = 0; i < m_client.get_size(); i++)
    {
        if (m_client.client[i].iop && m_client.client[i].sn == rtu_sn)
        {
            WCX_PRINT ("### trans rtu_sn(%d) to client..\n", rtu_sn);
            ret = iop_buf_send (m_client.client[i].iop, buf, len);
            if (ret <= 0)
            {
                client_tp client;
                client.iop = m_client.client[i].iop;
                client.sn = m_client.client[i].sn;
                m_client.erase (client);
                WCX_DEBUG ("!!!!!!!!!!!!!!!!! delect client !!!!!!!!!!!!!!!!%s", "\n");
                WCX_LOG (&run_log, -1, "rtu_process()", "(%s:%d): iop_buf_send err!, delect this client!", __FILE__, __LINE__);
            }
        }
    }

    return 0;
}

/**
 * @brief    多线程模式下的转发服务器(这个模型依然能保证消息是有序处理的)
 *
 * @param   num_thread: 开多少个线程(linux下是epoll, win32下是select)
 *
 * @return  
 */
int rtu_server (int num_thread)
{
    iop_protocol_t proto = IOP_PROTOCOL_INIT;
    proto.protocol_parse = rtu_protocol_parse;
    proto.process = rtu_process;
    iop_task_pool_t *tp = iop_task_pool_create (num_thread);
    WCX_PRINT ("trans server(mt model) start. listen on %d.\n", RTU_PORT);
    iop_task_pool_start (tp, "0.0.0.0", RTU_PORT, &proto);
    return 0;
}

static void  *rtu_thread_server (void *data)
{
    //启动服务（多线程模型），10个线程。
    rtu_server (10);
}





/* client */
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

//解析客户端请求过来的数据
static int client_protocol_parse (const void * buf, unsigned int len)
{
    return len;
}

//处理客户端的消息
int client_process (const void *buf, unsigned int len, iop_t *iop)
{
    int ret, i;
    BYTE            *rcv_buf = (BYTE*) buf;
    T_C104_FRAME    *p_frame;
    T_C104_ASDU     *p_asdu;

    uint8_t          INFO_SQ; //信息顺序 0：非顺序 1：顺序
    uint16_t         INFO_NUMS; //信息数目

    p_frame = (T_C104_FRAME*) (&rcv_buf[0]);
    p_asdu = (T_C104_ASDU*) (&rcv_buf[6]);

    /* print the rcv date */
    if (DEBUG_FLG & DEBUG_MODE)
    {
        printf ("\033[32;40;1m+@@@@@@@@@@@@@@client:rcv from [%s]:\n\033[0m", "???");
        for (i = 0; i < len; i++)
        {
            printf ("%.2X ", rcv_buf[i]);
            if ( (i + 1) % 50 == 0)
                printf ("\n");
        }
        printf ("\n");
    }

    /* IEC104 Analyse */
    if (len == 6) // U_FORMAT or S_FORMAT
    {
        switch (p_frame->c1)
        {
        case 0x01: // S_Format
            WCX_PRINT ("\033[38;40;1m$ [S帧]\n\033[0m");
            break;
        case 0x07:
        {
            /* register client */
            client_tp client;
            client.iop = iop;
            client.sn = 1;
            m_client.push (client);
            WCX_PRINT ("\033[38;40;1mclient connected!\n\033[0m");
            WCX_LOG (&run_log, -1, "client_process()", "(%s:%d): client connected!", __FILE__, __LINE__);
        }
        break;
        case 0x0b:
            WCX_PRINT ("\033[38;40;1m$ [建立连接确认帧]\n\033[0m");
            break;
        case 0x43:
            WCX_PRINT ("\033[38;40;1m$ [测试帧]\n\033[0m");
            break;
        default:
            break;
        }
    }
    else //I_format
    {
        INFO_NUMS = p_asdu->vsq & 0x7F; //信息数目
        INFO_SQ = p_asdu->vsq & 0x80; //顺序, 0: 非顺序 1：顺序

        switch (p_asdu->type)
        {
        case TYPE_LOG_IN:
        {
            T_LOG_IN_INF *pLogInf = (T_LOG_IN_INF*) (&p_asdu->data[0]);
            //WCX_PRINT ("\033[38;40;1m<username: %s, passwd: %s>\n\033[0m", pLogInf->user_name, pLogInf->passwd);
            int sn;
            int ret = login (sysdb, pLogInf->user_name, pLogInf->passwd, &sn);
            if (ret == 1)
            {
                /* register client */
                client_tp client;
                client.iop = iop;
                client.sn = sn;
                m_client.push (client);
                WCX_PRINT ("\033[38;40;1mclient connected!\n\033[0m");
                WCX_LOG (&run_log, -1, "client_process()", "(%s:%d): client connected!", __FILE__, __LINE__);
            }
        }
        break;

        default:
            break;
        }
    }
    WCX_PRINT ("\n\n");

    /* get client rtu_sn */
    int rtu_sn = -1;
    for (i = 0; i < m_client.get_size(); i++)
    {
        if (m_client.client[i].iop == iop)
        {
            rtu_sn = m_client.client[i].sn;
            break;
        }
    }

    if (rtu_sn != -1)
    {
        /* trans the client date to rtu */
        for (i = 0; i < m_rtu.get_size(); i++)
        {
            if (m_rtu.client[i].iop && m_rtu.client[i].sn == rtu_sn)
            {
                WCX_PRINT ("$$$ trans client(%d) to rtu..\n", rtu_sn);
                ret = iop_buf_send (m_rtu.client[i].iop, buf, len);
                if (ret <= 0)
                {
                    client_tp rtu;
                    rtu.iop = m_rtu.client[i].iop;
                    rtu.sn = m_rtu.client[i].sn;
                    m_rtu.erase (rtu);
                    WCX_DEBUG ("!!!!!!!!!!!!!!!!! delect rtu !!!!!!!!!!!!!!!!%s", "\n");
                    WCX_LOG (&run_log, -1, "rtu_process()", "(%s:%d): iop_buf_send err!, delect this rtu!", __FILE__, __LINE__);
                }
            }
        }
    }

    return 0;
}

//多线程模式下的echo服务器，这个是在基础模型上的封装。
//num_thread：开多少个线程。
//这种模型依然能保证消息是有序处理的。
//linux下是epoll, win32下是select
int client_server (int num_thread)
{
    iop_protocol_t proto = IOP_PROTOCOL_INIT;
    proto.protocol_parse = client_protocol_parse;
    proto.process = client_process;
    iop_task_pool_t *tp = iop_task_pool_create (num_thread);

    /* 打开权限管理数据库 */
    int result = sqlite3_open ("/etc/trans_sysdb.db", &sysdb);
    if (result != SQLITE_OK)
    {
        WCX_DEBUG ("open sysdb err!%s", "\n");
        WCX_LOG (&error_log, -1, "main()", "(%s:%d): qlite3_open err!", __FILE__, __LINE__);
        exit (1);
    }
    else
    {
        WCX_PRINT ("open sysdb succeed!%s", "\n");
        WCX_LOG (&run_log, -1, "main()", "(%s:%d): qlite3_open succeed!", __FILE__, __LINE__);
    }

    WCX_PRINT ("trans server(mt model) start. listen on %d.\n", CLIENT_PORT);
    iop_task_pool_start (tp, "0.0.0.0", CLIENT_PORT, &proto);

    /* 关闭权限管理数据库 */
    sqlite3_close (sysdb);
    return 0;
}

static void  *client_thread_server (void *data)
{
    //启动服务（多线程模型），10个线程。
    client_server (10);
}

#if 0
//单线程模式下的echo服务器，这个是基本模型。
//linux下是epoll, win32下是select
int st_echo_server()
{
    iop_protocol_t proto = IOP_PROTOCOL_INIT;
    proto.protocol_parse = rtu_protocol_parse;
    proto.process = rtu_process;
    iop_base_t *base = iop_new (1024);
    iop_add_tcp_server (base, NULL, 7777, &proto);
    WCX_PRINT ("echo server(st model) start. listen on 7777.\n");
    WCX_PRINT ("try: telnet 127.0.0.1 7777\n");
    iop_dispatch (base);
    return 0;
}
#endif

void print_usage (const char * prog)
{
    system ("clear");
    printf ("Usage: %s [-d]\n", prog);
    puts ("  -d  --打印调试信息!\n");
}

/**
 * @brief    根据用户名和密码查询用户需查看的前置机的序列号
 *
 * @param   db: 数据库
 * @param   user: 用户名
 * @param   passwd: 密码
 * @param   sn: 前置机序列号
 *
 * @return: 成功: 返回1, 失败: 返回-1
 */
static inline int login (sqlite3 *db, const char *user, const char *passwd, int *sn)
{
    int result;
    char **dbResult;
    int nRow, nColumn;
    char *errmsg = NULL;
    int index;
    char SQL[250];

    sprintf (SQL, "select passwd,rtu_sn from USER where user_name='%s';", user);

    result = sqlite3_get_table (db, SQL, &dbResult, &nRow, &nColumn, &errmsg);
    if (SQLITE_OK == result)
    {
        if (nRow == 1 && nColumn == 2)
        {
            //printf ("passwd: %s, rtu_sn: %s\n", dbResult[2], dbResult[3]);
            if (strcmp (dbResult[2], passwd) == 0)
            {
                result = 1;
                *sn = atoi (dbResult[3]);
            }
            else
            {
                WCX_PRINT ("\033[38;40;1m[passwd1:%s, passwd2: %s]\n\033[0m", passwd, dbResult[2]);
                result = -1;
            }
        }
        else
        {
            //printf ("result is not one!\n");
            result = -1;
        }
    }
    else
    {
        //printf ("sqlite3_get_table err:%s\n", errmsg);
        result = -1;
    }

    sqlite3_free_table (dbResult);

    return result;
}

int parse_opts (int argc, char * argv[])
{
    int ret, ch;

    DEBUG_FLG = 0;

    while ( (ch = getopt (argc, argv, "d")) != EOF)
    {
        switch (ch)
        {
        case 'd':
            DEBUG_FLG |= DEBUG_MODE;
            break;
        case 'h':
        case '?':
            print_usage (argv[0]);
            exit (1);
        default:
            break;
        }
    }
    return 0;
}

int main (int argc, char **argv)
{
    int i, result;
    pthread_t th_server_rtu;
    pthread_t th_server_client;

    system ("clear");
    printf ("@@@@@@@@@@@@@@@@@@@@@@@@@@@@ wcx_trans @@@@@@@@@@@@@@@@@@@@@@@@@@\n");

    parse_opts (argc, argv);

    log_init (&error_log, ERROR_LOG_FILE, "trans", "v0.1.0", "wang_chen_xi", "Error log");
    log_init (&run_log, RUN_LOG_FILE, "trans", "v0.1.0", "wang_chen_xi", "Run log");

    /* 初始化系统配置。最大支持10240个连接 */
    iop_sys_init (10240);

    /* create rtu thread */
    if (pthread_create (&th_server_rtu, NULL, rtu_thread_server, NULL) != 0)
    {
        WCX_DEBUG ("pthread_create:%s\n", strerror (errno));
        WCX_LOG (&error_log, -1, "main()", "(%s:%d): pthread_creat:%s", __FILE__, __LINE__, strerror (errno));
        exit (1);
    }
    else
    {
        WCX_PRINT ("thread server rtu create successed!\n");
    }

    /* create client thread */
    if (pthread_create (&th_server_client, NULL, client_thread_server, NULL) != 0)
    {
        WCX_DEBUG ("pthread_create:%s\n", strerror (errno));
        WCX_LOG (&error_log, -1, "main()", "(%s:%d): pthread_creat:%s", __FILE__, __LINE__, strerror (errno));
        exit (1);
    }
    else
    {
        WCX_PRINT ("thread server client create successed!\n");
    }

    /* 回收系统资源 */
    iop_sys_destroy();
    pthread_join (th_server_rtu, NULL);
    pthread_join (th_server_client, NULL);
    return 0;
}
