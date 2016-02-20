/*   IEC104_MASTER_TEST.c  4.2  04/15/99
 ***********************************************************************
 *                          NJSK   Software                            *
 *            Automatic Realtime Remote Interface software             *
 *           Copyright (c) 1998,1999 NARI P.R.C                        *
 *                        All Rights Reserved                          *
 *                                                                     *
 *                                                                     *
 * Name        : iec104_master_test.cpp                                *
 *                                                                     *
 * Programmer  : wangchenxicool                                        *
 *                                                                     *
 * Decsription : the server process of net104 protocol test            *
 *                                                                     *
 ***********************************************************************/

#include <fcntl.h>
#include <sys/wait.h>
#include <netinet/in.h>    /* for sockaddr_in */
#include <sys/types.h>    /* for sockeat */
#include <sys/socket.h>    /* for socket */
#include <netinet/tcp.h>     /* for TCP_NODELAY */
#include <stdio.h>        /* for printf */
#include <stdlib.h>        /* for exit */
#include <string.h>
#include <pthread.h>
#include <sys/errno.h>
#include <semaphore.h>  /* for sem */
#include <netdb.h>
#include <arpa/inet.h> /* inet_ntoa */
#include <netinet/in.h>
#include <unistd.h> /* fork, close,  access */
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

#include "wcx_utils.h"
#include "iec104_master.h"

#define NAME_SIZE   256

#define DEBUG

#ifdef DEBUG
#define wprintf(format, arg...)  \
    printf( format , ## arg)
#else
#define wprintf(format, arg...) {}
#endif

//--->Global var
char thread_exit; //thread exit flg

static void handle_pipe (int sig)
{
    wprintf ("\033[0;31;40mSIGPIPE!\a\033[0m\n");
}

/**
 * @brief    quit handle
 *
 * @param   sig: signal
 */
static void handle_quit (int   sig)
{
    wprintf ("\033[0;31;40mSIGTERM!\a\033[0m\n");
}

/**
 * @brief    segv error handle
 *
 * @param   sig: signal
 */
static void handle_segv (int   sig)
{
    wprintf ("\033[0;31;40mSIGSEGV!\a\033[0m\n");
    exit (1);
}

/**
 * @brief
 *
 * @param   sig
 */
static void handle_abrt (int   sig)
{
    wprintf ("\033[0;31;40mSIGABRT!\a\033[0m\n");
}

/**
 * @brief    sigint handle
 *
 * @param   sig: signal
 */
static void handle_int (int   sig)
{
    /*system ("clear");*/
    wprintf ("\033[0;31;40mrcv SIGINT!\a\033[0m\n");
    thread_exit = 1;
    IEC104MASTER_Exit (NULL);
}

/**
 * @brief    alarm handle
 *
 * @param   sig: signal
 */
#if 0
static void handle_alarm (int sig)
{
    g_flg_thread_exit = 1;
}
#endif


/**
 * @brief    init time
 *
 * @param   time: time
 */
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

    /*SIGALRM*/
#if 0
    struct sigaction action_alarm;
    action_alarm.sa_handler = handle_alarm;
    action_alarm.sa_flags = 0;
    sigemptyset (&action_alarm.sa_mask);
    sigaction (SIGALRM, &action_alarm, NULL);
#endif
}

#define MAXLINE     80
#define CMDPORT     55555

static void  *read_cmd (void *data)
{
    int sock_fd;
    struct sockaddr_in sin;
    struct sockaddr_in rin;
    socklen_t address_size;
    char buf[MAXLINE];
    char str[INET_ADDRSTRLEN];
    int i, ret, len;


    pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);

    bzero (&sin, sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons (CMDPORT);

    sock_fd = socket (AF_INET, SOCK_DGRAM, 0);
    if (-1 == sock_fd)
    {
        perror ("call to socket");
        goto exit;
    }
    ret = bind (sock_fd, (struct sockaddr *) &sin, sizeof (sin));
    if (-1 == ret)
    {
        perror ("call to bind");
        goto exit;
    }

    while (1)
    {
again:
        if (thread_exit)
        {
            break;
        }
        address_size = sizeof (rin);
        ret = recvfrom (sock_fd, buf, MAXLINE, MSG_DONTWAIT, (struct sockaddr *) &rin, &address_size);
        if (-1 == ret)
        {
            if (EAGAIN == errno) //time_out
            {
                MySleep (2, 0);
                goto again;
            }
            perror ("call to recvfrom.\n");
            break;
        }

        /*wprintf ("You ip is %s at port %d: %s\n", inet_ntop (AF_INET, &rin.sin_addr, str, sizeof (str)), ntohs (rin.sin_port), buf);*/
        if (strstr (buf, "cmd0") != NULL)
        {
            system ("clear");
            printf ("recv cmd: 发送总召唤激活请求命令...\n");
            //IEC104_Master_sC_IC_NA (0);
        }
        else if (strstr (buf, "cmd1") != NULL)
        {
            system ("clear");
            printf ("recv cmd: 时钟同步激活请求命令...\n");
            //IEC104_Master_sC_CS_NA (0);
        }
        else if (strstr (buf, "cmd2") != NULL)
        {
            system ("clear");
            printf ("recv cmd: 双点遥控命令...合\n");
            //IEC104_Master_sC_DC_NA_PreSet (0, 0x82);
        }
        else if (strstr (buf, "cmd3") != NULL)
        {
            system ("clear");
            printf ("recv cmd: 双点遥控命令...开\n");
            //IEC104_Master_sC_DC_NA_PreSet (0, 0x81);
        }
        else if (strstr (buf, "cmd8") != NULL)
        {
            printf ("recv cmd: 关闭显示命令！\n");
            //IEC104_Master_DisplayOff ();
        }
        else if (strstr (buf, "cmd9") != NULL)
        {
            /*system ("clear");*/
            printf ("recv cmd: 打开显示命令！\n");
        }
        else
        {
            system ("clear");
            strlcpy (buf, "unknow command!", sizeof (buf));
        }
#if 0
        ret = sendto (sock_fd, buf, strlen (buf) + 1, 0, (struct sockaddr *) &rin, address_size);
        if (-1 == ret)
        {
            perror ("call to sendto\n");
        }
#endif
        MySleep (2, 0);
    }

exit:
    close (sock_fd);
    pthread_exit (NULL);
}

/**
 * @brief    模拟数据采集
 *
 * @param   data
 *
 * @return  
 */
static void  *collect (void *data)
{
    int i = 0;
    float var;

    while (1)
    {
        if (thread_exit) break;
        var = 1 + (int)(1000.0*rand() / (RAND_MAX + 1.0));
        REAL_VAL_TAG tag;
        MAKE_REAL_TAG (1001, 100 + var / 100, &tag);
        PUSH_REAL_VAL_TAG (tag);
        MySleep (0, 500);
    }
}

int main (int argc, char *argv[])
{
    pthread_t thread_read_cmd;
    pthread_t thread_collect;

    thread_exit = 0;

    init_sigaction ();

    /* create cmd receive thread */
    if (pthread_create (&thread_read_cmd, NULL, read_cmd, NULL) < 0)
    {
        perror ("pthread_creat error");
        exit (1);
    }

    /* create collect  thread */
    if (pthread_create (&thread_collect, NULL, collect, NULL) < 0)
    {
        perror ("pthread_creat error");
        exit (1);
    }

    IEC104_Master_Display ((void*)1);

    IEC104MASTER_Run (NULL);
}
