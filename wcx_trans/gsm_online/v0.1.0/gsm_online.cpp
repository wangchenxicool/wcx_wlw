/***********************************************************************************
Project:          collect�ػ�����
Filename:        collect_demo.c -- ������

Prozessor:        linux
Compiler:          gcc

Autor:              ������
Copyrigth:        �Ͼ����������ܿƼ����޹�˾
***********************************************************************************/
#include <fcntl.h>
#include <sys/wait.h>
#include <netinet/in.h>    // for sockaddr_in
#include <sys/types.h>    // for socket
#include <sys/socket.h>    // for socket
#include <netinet/tcp.h>     // for TCP_NODELAY
#include <stdio.h>        // for printf
#include <stdlib.h>        // for exit
#include <string.h>        // for bzero
#include <pthread.h>    // for thread
#include <sys/errno.h>    // for errno
#include <semaphore.h>  // for sem
#include <netdb.h>
#include <arpa/inet.h> // inet_ntoa
#include <netinet/in.h>
#include <unistd.h> // fork, close,  access
#include <sys/types.h>
#include <dirent.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <ctype.h>
#include <signal.h>
#include <vector>
#include <iostream>
#include "wcx_log.h"
#include "wcx_utils.h"

#define MAXFILE 65535
#define SHM_MAX     50 // �����������ֽ���

#define     CONNECT_OK                        1
#define     CONNECT_NG                       -7

/*#define DEBUG*/

#ifdef DEBUG
#define wprintf(format, arg...)  \
    printf( format , ## arg)
#else
#define wprintf(format, arg...) {}
#endif

/* ȫ�ֱ��� */
static struct log_tp run_log;

int is_online (void)
{
    time_t end_time;

    end_time = time (NULL) + 5;
    while (time (NULL) < end_time)
    {
        if (gethostbyname ("www.baidu.cn") != NULL)
            return CONNECT_OK;
    }

    return CONNECT_NG;
}

int gsm_connect (void)
{
    int i;
    FILE *pp;
    char shell_ret_buf[552];
    long shell_ret_num;
    time_t end_time;

    if (is_online() == CONNECT_OK)
    {
        wprintf ("SierraWireless: gsm_connect: Alrady online!\n");
        return CONNECT_OK;
    }
    else
        wprintf ("is not online!\n");


    for (i = 0; i < 5; i++)   //��ದ��5��
    {
        /*��ȡ����"pppd call gsm"��pid��*/
        pp = popen ("pidof pppd", "r");
        memset (shell_ret_buf, 0, sizeof shell_ret_buf);
        fgets (shell_ret_buf, sizeof shell_ret_buf, pp);
        shell_ret_num = strtol (shell_ret_buf, NULL, 10);
        pclose (pp);

        /*���pppd_pid_num > 0, killall pppd */
        if (shell_ret_num > 0)
        {
            wprintf ("SierraWireless: gsm_connect: killall pppd ... \n");
            system ("killall pppd&");
        }

        MySleep (2, 0);

        /*�ٴλ�ȡ����"pppd"��pid��*/
        pp = popen ("pidof pppd", "r");
        memset (shell_ret_buf, 0, sizeof shell_ret_buf);
        fgets (shell_ret_buf, sizeof shell_ret_buf, pp);
        shell_ret_num = strtol (shell_ret_buf, NULL, 10);
        pclose (pp);

        /*���pppd_pid_num > 0, ɱ������: pppd*/
        if (shell_ret_num > 0)
        {
            char cmd_buf_kill[500];
            wprintf ("SierraWireless: gsm_connect: kill -9  pppd ... \n");
            strlcpy (cmd_buf_kill, "kill -9 ", sizeof (cmd_buf_kill));
            strcat (cmd_buf_kill, shell_ret_buf);
            system (cmd_buf_kill);
        }

        /*���½�������*/
        wprintf ("SierraWireless: gsm_connect: pppd call gsm ....\n");
        system ("pppd call gsm > /var/log/wcx_pppd.log &");

        /*���pppd������Ϣ*/
        end_time = time (NULL) + 10;
        while (time (NULL) < end_time)
        {
            /*��ȡ����"pppd call gsm"�ķ�����Ϣ*/
            //pp = popen ( "grep -c  \"ip-up finished\"   /var/log/wcx_pppd.log", "r" );
            pp = popen ("grep -c  \"local  IP address\"   /var/log/wcx_pppd.log", "r");
            memset (shell_ret_buf, 0, sizeof shell_ret_buf);
            fgets (shell_ret_buf, sizeof shell_ret_buf, pp);
            shell_ret_num = strtol (shell_ret_buf, NULL, 10);
            pclose (pp);

            /*���������Ϣ����"ip-up finished", ��ת��end��. ��������ȴ�"ip-up finished"... ����ڹ涨��ʱ�仹�Ȳ������������...*/
            if (shell_ret_num > 0)
            {
                write_log (&run_log, 1, "gsm_connect()", "have rev \"ip_up finished!\"");
                goto end;
            }
        } /*end while*/
    } /*end for*/

end:
    end_time = time (NULL) + 15;
    while (time (NULL) < end_time)
    {
        if (gethostbyname ("www.baidu.cn") != NULL)
            return CONNECT_OK;
    }

    write_log (&run_log, 1, "gsm_connect()", "connect NG!");
    return CONNECT_NG;
}

int main (int argc, char **argv)
{
    int i;
    pid_t pc, sid;
    bool debug_flg;

    /* ��ȡ��������ģʽ */
    debug_flg = false;
    for (i = 1; i < argc; i++)
    {
        if (strcasecmp (argv[i], "-D") == 0)
        {
            debug_flg = true;
            break;
        }
    }

    /* log init */
    log_init (&run_log, "/var/log/vpn_online/run.log", "dbserver", "v0.1.0", "wang_chen_xi", "Boot log");

    if (!debug_flg)
    {
        CreateDaemon ();
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    //��ʱ�������ػ����̣����¿�ʼ��ʽ�����ػ����̹���
    /////////////////////////////////////////////////////////////////////////////////////////////////

    while (1)
    {
        gsm_connect ();
        MySleep (10, 0);
    }

    exit (0);
}
