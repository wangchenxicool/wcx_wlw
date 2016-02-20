/*   IEC104_MASTER_TEST.c  4.2  04/15/99
 ***********************************************************************
 *                          NARI   Software                            *
 *            Automatic Realtime Remote Interface software             *
 *           Copyright (c) 1998,1999 NARI P.R.C                        *
 *                        All Rights Reserved                          *
 *                                                                     *
 *                                                                     *
 * Name        : dlserver.c                                            *
 *                                                                     *
 * Programmer  : NARI                                                  *
 *                                                                     *
 * Decsription : the server process of net104 protocol                 *
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

#include "dev_103.h"

#define DEBUG

#ifdef DEBUG
#define wprintf(format, arg...)  \
    printf( format , ## arg)
#else
#define wprintf(format, arg...) {}
#endif

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
    wprintf ("\033[0;31;40mrcv SIGINT!\a\033[0m\n");
    iec103_master_exit (NULL);
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

static void  *read_cmd (void *data)
{
    iec103_master_run (NULL);
    pthread_exit (NULL);
}

int main (int argc, char *argv[])
{
    pthread_t thread_read_cmd;

    init_sigaction ();

    /*create cmd receive thread*/
    if (pthread_create (&thread_read_cmd, NULL, read_cmd, NULL) < 0)
    {
        perror ("pthread_creat error");
        exit (1);
    }

    pthread_join (thread_read_cmd, NULL);
    printf ("test exited!\n");
}

