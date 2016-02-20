/**************************************************************************************
 _____  _____  _____  __   _____    ___ ___  ___  ___   _____  _____  _____ ______
|_   _||  ___|/  __ \/  | |  _  |  /   ||  \/  | / _ \ /  ___||_   _||  ___|| ___ \
  | |  | |__  | /  \/`| | | |/' | / /| || .  . |/ /_\ \\ `--.   | |  | |__  | |_/ /
  | |  |  __| | |     | | |  /| |/ /_| || |\/| ||  _  | `--. \  | |  |  __| |    /
 _| |_ | |___ | \__/\_| |_\ |_/ /\___  || |  | || | | |/\__/ /  | |  | |___ | |\ \
 \___/ \____/  \____/\___/ \___/     |_/\_|  |_/\_| |_/\____/   \_/  \____/ \_| \_|
            ╭══╮　
      　　╭╯@  @║　
      　　╰⊙═⊙╯。oо○　
**************************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <pthread.h>
#include <sqlite3.h>

/* Libevent. */
#include <event.h>

#include "queue.h"
#include "iec104_master.h"
#include "wcx_utils.h"
#include "wcx_log.h"
#include "INIFileOP.h"

#define DEBUG

#ifdef DEBUG
#define wprintf(format, arg...) \
    if (debug == 1) \
        printf (format , ##arg)
#else
#define wprintf(format, arg...) {}
#endif

#define WCX_DEBUG(fmt, ...)     wcx_debug(__FILE__, __LINE__, fmt, __VA_ARGS__)
static void wcx_debug (const char *file, int line, const char *fmt, ...)
{
    va_list ap;

    //if (DEBUG_FLG & DEBUG_MODE)
    {
        fprintf (stderr, "\033[0;31;40mDEBUG\a\033[0m(%s:%d:%d): ", file, line, getpid());
        va_start (ap, fmt);
        vfprintf (stderr, fmt, ap);
        va_end (ap);
    }
}


//--->global val
static struct IEC104_MASTER *iec104[IEC104_SLAVE_IP_NUMS_MAX];
static int                  IEC104_SLAVE_IP_NUMS; //ip地址数

static struct event         ev_timer_connect; //timer 时间对象
static struct timeval       tv_timer_connect;

static struct log_tp        error_log;
static char                 debug; //debug on/off
static bool                 exit_flg;

//--->WRITE_REAL_VAL_TAG_QUEUE
WRITE_REAL_VAL_TAG   WRITE_REAL_TAGS;

//--->Config Variable
static int      rtu_sn;
static int      C_IC_NA_QOI; //总召限定词
static int      C_CI_NA_QOI; //电度总度召限定词
static char     tag_file[100]; //点表文件
static char     csv_file_path[100]; //csv文件存放路径


/* Treats errors and flush or close connection if necessary */
static void iec104_master_error_treat (int code, const char *string, void *arg)
{
    struct IEC104_MASTER *IEC104 = (struct IEC104_MASTER*) arg;

    wprintf ("\033[31;40;1m \n+master:error_treat: %s (%0X)\n\033[0m", string, -code);
    write_log (&error_log, -1, "error_treat()", "%s:%d", string, -code);

    switch (code)
    {
    case INVALID_DATA:
    case INVALID_CRC:
    case INVALID_EXCEPTION_CODE:
        /*iec104_flush ( mb_param );*/
        break;
    case SELECT_FAILURE:
    case SOCKET_FAILURE:
    case CONNECTION_CLOSED:
        event_del (&IEC104->ev_read);
        event_del (&IEC104->ev_timer_tframe);
        event_del (&IEC104->ev_timer_sframe);
        event_del (&IEC104->ev_timer_WriteDB);
        //event_del (&IEC104->ev_timer_C_IC_NA);
        //event_del (&IEC104->ev_timer_C_CI_NA);
        close (IEC104->fd);
        IEC104->is_connect = FALSE;
        break;
    default:
        /* NOP */
        break;
    } // end switch
}

static void IEC104MASTER_Display (struct IEC104_MASTER *IEC104)
{
    wprintf ("\033[34;40;1m+master:send  to [%s]: \033[0m\n", IEC104->ip_addr);
#ifdef WRITE_RCV_LOG
    char str[512];
    bzero (str, sizeof (str));
    strlcpy (str, "++send: ", sizeof (str));
#endif
    for (size_t i = 0; i < IEC104->SendLen; i++)
    {
#ifdef WRITE_RCV_LOG
        char ch[10];
        sprintf (ch, "%.2X ", IEC104->SendBuf[i]);
        strlcat (str, ch, sizeof (str));
#endif
        wprintf ("%.2X ", IEC104->SendBuf[i]);
        if ( ( ( (i + 1) % MaxListDsp) == 0) && (i != IEC104->SendLen - 1))
            wprintf ("%s", "\n");
    }
    wprintf ("%s", "\n");
#ifdef WRITE_RCV_LOG
    write_to_log (IEC104MASTER_RCV_LOG, str, LOG_APPEND);
#endif

    //--->explan..
    if (IEC104->SendLen == 6)
    {
        if ( (IEC104->SendBuf[2] & 0x03) == 0x01)
        {
            wprintf ("%s", "<RECV_NUM_ACK, 接受序号确认帧> ");
        }
        else if ( (IEC104->SendBuf[2] & 0x03) == 0x03)
        {
            switch (IEC104->SendBuf[2])
            {
            case 0x04|0x03:
                wprintf ("%s", "<STARTDT_ACT>, 链路建立请求帧");
                break;
            case 0x08|0x03:
                wprintf ("%s", "<STARTDT_ACK>, 链路建立确认帧");
                break;
            case 0x10|0x03:
                wprintf ("%s", "<STOPDT_ACT>, 链路停止请求帧");
                break;
            case 0x20|0x03:
                wprintf ("%s", "<STOPDT_ACK>, 链路停止确认帧");
                break;
            case 0x40|0x03:
                wprintf ("%s", "<TESTFP_ACT>, 链路测试请求帧");
                break;
            case 0x80|0x03:
                wprintf ("%s", "<TESTFP_ACK>, 链路测试确认帧");
                break;
            default:
                wprintf ("<default> %d:", IEC104->SendBuf[2]);
                break;
            }
        }
    }
    else
    {
        switch (IEC104->SendBuf[6])
        {
        case M_SP_NA:
            wprintf ("%s", "<M_SP_NA, 单点信息> ");
            break;
        case M_DP_NA:
            wprintf ("%s", "<M_DP_NA, 双点信息>");
            break;
        case M_ST_NA:
            wprintf ("%s", "<M_ST_NA>");
            break;
        case M_BO_NA:
            wprintf ("%s", "<M_BO_NA>");
            break;
        case M_ME_NA:
            wprintf ("%s", "<M_ME_NA, 测量值，规一化值>");
            break;
        case M_ME_NB:
            wprintf ("%s", "<M_ME_NB, 测量值，标度化值>");
            break;
        case M_ME_NC:
            wprintf ("%s", "<M_ME_NC>");
            break;
        case M_IT_NA:
            wprintf ("%s", "<M_IT_NA>");
            break;
        case M_PS_NA:
            wprintf ("%s", "<M_PS_NA>");
            break;
        case M_ME_ND:
            wprintf ("%s", "<M_ME_ND>");
            break;
        case M_SP_TB:
            wprintf ("%s", "<M_SP_TB, 带时标CP56Time2a 的单点信息>");
            break;
        case M_DP_TB:
            wprintf ("%s", "<M_DP_TB, 带时标CP56Time2a 的双点信息>");
            break;
        case M_ST_TB:
            wprintf ("%s", "<M_ST_TB>");
            break;
        case M_BO_TB:
            wprintf ("%s", "<M_BO_TB>");
            break;
        case M_ME_TD:
            wprintf ("%s", "<M_ME_TD, 时标CP56Time2a 的测量值，规一化值>");
            break;
        case M_ME_TE:
            wprintf ("<M_ME_TE, 带时标CP56Time2a 的测量值，标度化值>");
            break;
        case M_ME_TF:
            wprintf ("<M_ME_TF>");
            break;
        case M_IT_TB:
            wprintf ("<M_IT_TB>");
            break;
        case M_EP_TD:
            wprintf ("<M_EP_TD>");
            break;
        case M_EP_TE:
            wprintf ("<M_EP_TE>");
            break;
        case M_EP_TF:
            wprintf ("<M_EP_TF>");
            break;
        case M_EI_NA:
            wprintf ("<M_EI_NA, 初始化结束>");
            break;
        case C_SC_NA:
            wprintf ("<C_SC_NA, 单命令>");
            break;
        case C_SE_NA:
            wprintf ("<C_SE_NA, 点命令，规一化值>");
            break;
        case C_IC_NA:
            wprintf ("<C_IC_NA, 总召唤命令>");
            break;
        case C_CI_NA:
            wprintf ("<C_CI_NA, 电度总召唤命令>");
            break;
        case C_DC_NA:
            wprintf ("<C_DC_NA, 双点遥控>");
            break;
        default:
            wprintf ("<default> %d:", IEC104->SendBuf[6]);
            break;
        }
    }
    wprintf ("\n\n");
}

/**
* Set a socket to non-blocking mode.
*/
static int setnonblock (int fd)
{
    int flags;
    flags = fcntl (fd, F_GETFL);
    if (flags < 0)
        return flags;
    flags |= O_NONBLOCK;
    if (fcntl (fd, F_SETFL, flags) < 0)
        return -1;
    return 0;
}

static int Socket (int i)
{
    int ret;
    int option;

    iec104[i]->fd = socket (PF_INET, SOCK_STREAM, 0);
    if (iec104[i]->fd < 0)
    {
        //--->changing...
        write_log (&error_log, -1, "Socket()", "socket: %s", strerror (errno));
        exit (1);
    }

    /* Set the TCP no delay flag */
    /* SOL_TCP = IPPROTO_TCP */
    option = 1;
    ret = setsockopt (iec104[i]->fd, IPPROTO_TCP, TCP_NODELAY, (const void *) &option, sizeof (int));
    if (ret < 0)
    {
        //--->changing...
        close (iec104[i]->fd);
        write_log (&error_log, -1, "Socket()", "setsockopt: %s", strerror (errno));
        exit (1);
    }

#if (!HAVE_DECL___CYGWIN__)
    /**
     * Cygwin defines IPTOS_LOWDELAY but can't handle that flag so it's
     * necessary to workaround that problem.
     **/
    /* Set the IP low delay option */
    option = IPTOS_LOWDELAY;
    ret = setsockopt (iec104[i]->fd, IPPROTO_IP, IP_TOS, (const void *) &option, sizeof (int));
    if (ret < 0)
    {
        //--->changing...
        close (iec104[i]->fd);
        write_log (&error_log, -1, "Socket()", "setsockopt IP_TOS: %s", strerror (errno));
        exit (1);
    }
#endif
}

int connect_nonb (int sockfd, const struct sockaddr* server_addr, socklen_t salen, int nsec)
{
    int ret;
    int flags, n, error;
    socklen_t len;
    fd_set rset, wset;
    struct timeval tval;

    /* 获取当前socket的属性， 并设置 noblocking 属性 */
    flags = fcntl (sockfd, F_GETFL, 0);
    fcntl (sockfd, F_SETFL, flags | O_NONBLOCK);
    errno = 0;
    if ( (n = connect (sockfd, server_addr, salen)) < 0)
    {
        if (errno != EINPROGRESS)
            return (-1);
    }
    /* 可以做任何其它的操作 */
    if (n == 0)
        goto done; //一般是同一台主机调用，会返回 0
    FD_ZERO (&rset);
    FD_SET (sockfd, &rset);
    wset = rset;  //这里会做 block copy
    tval.tv_sec = nsec;
    tval.tv_usec = 0;
    /* 如果nsec 为0，将使用缺省的超时时间，即其结构指针为 NULL */
    /* 如果tval结构中的时间为0，表示不做任何等待，立刻返回 */
    if ( (n = select (sockfd + 1, &rset, &wset, NULL, &tval)) == 0)
    {
        /* time out */
        /*close (sockfd);*/
        return (-1);
    }
    if (FD_ISSET (sockfd, &rset) || FD_ISSET (sockfd, &wset))
    {
        len = sizeof (error);
        /*如果连接成功，此调用返回 0*/
        if (getsockopt (sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
            return (-1);
    }
    else
    {
        //--->changing...
        write_log (&error_log, -1, "Socket()", "select: %s", strerror (errno));
        exit (1);
        //iec104_master_error_treat (SELECT_FAILURE, "select error!", IEC104);
    }
done:
#if 0
    fcntl (sockfd, F_SETFL, flags); //恢复socket 属性
    if (error)
    {
        close (sockfd);
        errno = error;
        return (-1);
    }
#endif
    ret = getpeername (sockfd, (struct sockaddr*) server_addr, &salen);
    if (ret == -1 && errno == ENOTCONN)
    {
        wprintf ("+master: not connect: %s\n", strerror (errno));
        return -1;
    }
    else
    {
        wprintf ("+master: has connect!\n");
        return (0);
    }
}

static int Connect (int i)
{
    int ret;
    struct hostent *phe;
    struct sockaddr_in server_addr;

    if (1)
    {
        wprintf ("+master: Connecting to %s...\n", iec104[i]->ip_addr);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons (iec104[i]->port);
    if ( (server_addr.sin_addr.s_addr = inet_addr (iec104[i]->ip_addr)) == -1)
    {
        if ( (phe = gethostbyname (iec104[i]->ip_addr)) == NULL)
        {
            WCX_DEBUG ("gethostbyname%s", "\n");
            return 0;
        }
        memcpy ( (char *) &server_addr.sin_addr, phe->h_addr, phe->h_length);
    }
    ret = connect_nonb (iec104[i]->fd, (struct sockaddr *) &server_addr, sizeof (struct sockaddr_in), 2);
    if (ret < 0)
    {
        WCX_DEBUG ("connect_nob%s", "\n");
        close (iec104[i]->fd);
    }
    return ret;
}

static void iec104_master_connect()
{
    int i, ret;

    for (i = 0; i < IEC104_SLAVE_IP_NUMS; i++)
    {
        if (iec104[i]->is_connect == FALSE)
        {
            WCX_DEBUG ("connect to %s\n", iec104[i]->ip_addr);
            write_log (&error_log, -1, "connect to %s..", iec104[i]->ip_addr);
            Socket (i);
            ret = Connect (i);
            if (ret == 0) //connect sucessful!
            {
                wprintf ("+master: connect to %s successful!\n", iec104[i]->ip_addr);
                //--->Init..
                iec104[i]->i = i;
                iec104[i]->SendNum = 0;
                iec104[i]->RecvNum = 0;
                iec104[i]->k = 0;
                iec104[i]->w = 0;
                iec104[i]->is_connect = TRUE;
                iec104[i]->is_idlesse = FALSE;

                //--->set&add read event
                event_set (&iec104[i]->ev_read, iec104[i]->fd, EV_READ | EV_PERSIST, on_read, iec104[i]);
                event_add (&iec104[i]->ev_read, NULL);

                //--->set write event
                event_set (&iec104[i]->ev_write, iec104[i]->fd, EV_WRITE, on_write, iec104[i]);

                //--->set&add tframe event
                //evtimer_set (&iec104[i]->ev_timer_tframe, on_timer_tframe, iec104[i]);
                event_set (&iec104[i]->ev_timer_tframe, -1, EV_PERSIST, on_timer_tframe, iec104[i]);
                iec104[i]->tv_timer_tframe.tv_sec = iec104[i]->TimeOut_t3;
                iec104[i]->tv_timer_tframe.tv_usec = 0;
                event_add (&iec104[i]->ev_timer_tframe, &iec104[i]->tv_timer_tframe);

                //--->set&add WriteDB event
                //evtimer_set (&iec104[i]->ev_timer_WriteDB, on_timer_WriteDB, iec104[i]);
                event_set (&iec104[i]->ev_timer_WriteDB, -1, EV_PERSIST, on_timer_WriteDB, iec104[i]);
                iec104[i]->tv_timer_WriteDB.tv_sec = TIME_OUT_WRITEDB;
                iec104[i]->tv_timer_WriteDB.tv_usec = 0;
                event_add (&iec104[i]->ev_timer_WriteDB, &iec104[i]->tv_timer_WriteDB);

                //--->set 总召唤 event
#if 0
                if (iec104[i]->TimeOut_C_IC_NA >= 0)
                {
                    printf ("set zhong zhao huan!\n");
                    //evtimer_set (&iec104[i]->ev_timer_C_IC_NA, on_timer_C_IC_NA, iec104[i]);
                    event_set (&iec104[i]->ev_timer_C_IC_NA, -1, EV_PERSIST, on_timer_C_IC_NA, iec104[i]);
                }
#endif

                //--->set 电度总召唤 event
#if 0
                if (iec104[i]->TimeOut_C_CI_NA >= 0)
                {
                    event_set (&iec104[i]->ev_timer_C_CI_NA, -1, EV_PERSIST, on_timer_C_CI_NA, iec104[i]);
                }
#endif

                //--->set sframe event
                evtimer_set (&iec104[i]->ev_timer_sframe, on_timer_sframe, iec104[i]);

                //--->Send IEC104连接请求
                //IEC104_Master_sAssociateAct (i);
                IEC104_Master_LOGIN (i);
            }
            else
            {
                close (iec104[i]->fd);
            }
        } /* end if */
    } /* end for */
}

static void iec104_send_msg (int i)
{
    int ret;

    if (iec104[i]->SendLen > 0)
    {
        if (iec104[i]->SendLen > 6) //is I_frame
        {
            iec104[i]->SendBuf[2] = (iec104[i]->SendNum % 256);
            iec104[i]->SendBuf[3] = iec104[i]->SendNum / 256;
            iec104[i]->SendBuf[4] = (iec104[i]->RecvNum % 256);
            iec104[i]->SendBuf[5] = iec104[i]->RecvNum / 256;
        }
        else if (iec104[i]->SendLen == 6)
        {
            if ( (iec104[i]->SendBuf[2] & 0x03) == 0x01)  //is S_frame
            {
                iec104[i]->SendBuf[4] = (iec104[i]->RecvNum % 256);
                iec104[i]->SendBuf[5] = iec104[i]->RecvNum / 256;
            }
        }

        /* Since we now have data that needs to be written back to the
        * client, add a write event.
        */
#if 0
        event_add (&iec104[i]->ev_write, NULL);
#else
        ret = write (iec104[i]->fd, iec104[i]->SendBuf, iec104[i]->SendLen);
        if (ret == -1 || ret != iec104[i]->SendLen)
        {
            WCX_DEBUG ("write failure: %s\n", strerror(errno));
            event_del (&iec104[i]->ev_read);
            event_del (&iec104[i]->ev_timer_tframe);
            event_del (&iec104[i]->ev_timer_sframe);
            event_del (&iec104[i]->ev_timer_WriteDB);
            //event_del (&IEC104->ev_timer_C_IC_NA);
            //event_del (&IEC104->ev_timer_C_CI_NA);
            close (iec104[i]->fd);
            iec104[i]->is_connect = FALSE;
        }

        if (debug == TRUE)
        {
            IEC104MASTER_Display (iec104[i]);
        }

        iec104[i]->SendLen = 0;
#endif
    }
}

/**
* This function will be called by libevent when the client socket is
* ready for reading.
*/
static void on_read (int fd, short ev, void *arg)
{
    struct IEC104_MASTER    *IEC104 = (struct IEC104_MASTER*) arg;
    int                     ret, i, j, k;
    unsigned char           *pRcvData, *ptmp;
    uint8_t                 ASDU_TYP; //类型标识
    uint8_t                 ASDU_VSQ; //可变结构限定词
    uint16_t                ASDU_COT; //传送原因
    uint16_t                ASDU_ADDR; //应用服务数据单元公共地址
    uint16_t                INFO_ADDR; //信息对象地址
    uint8_t                 INFO_SQ; //信息顺序 0：非顺序 1：顺序
    uint16_t                INFO_NUMS; //信息数目
    float                   K; //系数

    T_C104_FRAME    *p_frame;
    T_C104_ASDU     *p_asdu;
    BYTE            *p_data, *p_item;

    p_frame = (T_C104_FRAME*) &IEC104->RecvBuf[0];
    p_asdu = (T_C104_ASDU*) &IEC104->RecvBuf[6];
    p_data = p_asdu->data;
    IEC104->is_idlesse = FALSE;

#if 0
    //-->TimeOut
    if (ev == EV_TIMEOUT)
    {
        ftime (&IEC104->NowTime);
        if ( ( (IEC104->NowTime.time - IEC104->IRecvTime.time > IEC104->TimeOut_t2)
                && (IEC104->SSendFlg == FALSE)) || IEC104->w >= IEC104->Max_w)
        {
            //发送S帧
            //ftime (&IEC104->IRecvTime);
            IEC104->SSendFlg = TRUE;
            IEC104->w = 0;
            IEC104_Master_sSFrame (IEC104->i);
        }
        if (IEC104->NowTime.time - IEC104->RecvTime.time > IEC104->TimeOut_t3)
        {
            //发送测试帧
            IEC104_Master_sTestAct (IEC104->i);
        }
        if (IEC104->NowTime.time - IEC104->C_IC_NA_Time.time > IEC104->TimeOut_C_IC_NA)
        {
            //发送总召唤请求帧
            IEC104_Master_sC_IC_NA (0, rtu_sn, VALUE_TAG_START, 20);
            ftime (&IEC104->C_IC_NA_Time);
        }
        //if (IEC104->NowTime.time - IEC104->C_CI_NA_Time.time > IEC104->TimeOut_C_CI_NA)
        if (IEC104->NowTime.time - IEC104->C_IC_NA_Time.time > IEC104->TimeOut_C_IC_NA + 5)
        {
            //发送电度总召唤请求帧
            IEC104_Master_sC_CI_NA (0, rtu_sn, VALUE_TAG_START, 20);
            ftime (&IEC104->C_IC_NA_Time);
        }
        return;
    }
#endif


    //-->RecvData...
    ret = read (IEC104->fd, (uint8_t*) &IEC104->RecvBuf[0], 2);
    if (ret == 0)
    {
        /* Client disconnected, remove the read event and the
        */
        write_log (&error_log, -1, "on_read()", "0-errno:%s, ret:%d", strerror (errno), ret);
        iec104_master_error_treat (CONNECTION_CLOSED, "on_read: Client disconnected!", IEC104);
        return;
    }
    else if (ret < 0 || ret != 2 || IEC104->RecvBuf[0] != 0x68)
    {
        /* Some other error occurred, close the socket, remove
        * the event and free the client structure.
        */
        wprintf ("+master: Connect failure, disconnecting client: %s", strerror (errno));
        write_log (&error_log, -1, "on_read()", "1-errno:%s, ret:%d", strerror (errno), ret);
        iec104_master_error_treat (SOCKET_FAILURE, "on_read: Connect failure, disconnecting client!", IEC104);
        return;
    }

    ret = read (IEC104->fd, (uint8_t*) &IEC104->RecvBuf[2], IEC104->RecvBuf[1]);
    if (ret == 0)
    {
        /* Client disconnected, remove the read event and the
        */
        write_log (&error_log, -1, "on_read()", "2-errno:%s, ret:%d", strerror (errno), ret);
        iec104_master_error_treat (CONNECTION_CLOSED, "on_read: Client disconnected!", IEC104);
        return;
    }
    else if (ret < 0 || ret != IEC104->RecvBuf[1])
    {
        /* Some other error occurred, close the socket, remove
        * the event and free the client structure.
        */
        wprintf ("+master: Connect failure, disconnecting client: %s", strerror (errno));
        write_log (&error_log, -1, "on_read()", "3-errno:%s, ret:%d", strerror (errno), ret);
        iec104_master_error_treat (SOCKET_FAILURE, "on_read: Connect failure, disconnecting client!", IEC104);
        return;
    }

    IEC104->RecvLen = IEC104->RecvBuf[1] + 2;

    /* print the rcv date */
    if (debug == TRUE)
    {
#ifdef WRITE_RCV_LOG
        char str[512];
        bzero (str, sizeof (str));
        strlcpy (str, "---rcv: ", sizeof (str));
#endif
        wprintf ("\033[32;40;1m+master:rcv from [%s]:\n\033[0m", IEC104->ip_addr);
        for (i = 0; i < IEC104->RecvLen; i++)
        {
#ifdef WRITE_RCV_LOG
            char ch[10];
            sprintf (ch, "%.2X ", IEC104->RecvBuf[i]);
            strlcat (str, ch, sizeof (str));
#endif
            wprintf ("%.2X ", IEC104->RecvBuf[i]);
            if ( (i + 1) % 50 == 0)
                wprintf ("\n");
        }
        wprintf ("\n");
#ifdef WRITE_RCV_LOG
        write_to_log (IEC104MASTER_RCV_LOG, str, LOG_APPEND);
#endif
    }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /* IEC104 Analyse */
    //wprintf ("@@Analyse:\n");
    pRcvData = IEC104->RecvBuf;

    if (IEC104->RecvLen < 6)
    {
        if (IEC104->RecvLen > 0)
        {
            IEC104->RecvLen = 0;
            wprintf ("iec104master: len err!\n");
        }
        return;
    }
    if (IEC104->RecvLen == 6) //U_FORMAT or S_FORMAT
    {
        //switch (pRcvData[2])
        switch (p_frame->c1)
        {
        case 0x01: //S_Format
            wprintf ("\033[38;40;1m$ [S帧]\n\033[0m");
            break;
        case 0x07:
            wprintf ("\033[38;40;1m$ [建立连接请求帧]\n\033[0m");
            IEC104_Master_sAssociateAck (IEC104->i);
            IEC104->SendNum = 0;
            IEC104->RecvNum = 0;
            break;
        case 0x0b:
            wprintf ("\033[38;40;1m$ [建立连接确认帧]\n\033[0m");
            IEC104->SendNum = 0;
            IEC104->RecvNum = 0;
#if 0
            //--->add 总召唤事件
            if (IEC104->TimeOut_C_IC_NA >= 0)
            {
                IEC104->tv_timer_C_IC_NA.tv_sec = IEC104->TimeOut_C_IC_NA;
                IEC104->tv_timer_C_IC_NA.tv_usec = 0;
                event_add (&IEC104->ev_timer_C_IC_NA, &IEC104->tv_timer_C_IC_NA);
            }
            //--->add 电度总召唤事件
            if (IEC104->TimeOut_C_CI_NA >= 0)
            {
                IEC104->tv_timer_C_CI_NA.tv_sec = IEC104->TimeOut_C_CI_NA;
                IEC104->tv_timer_C_CI_NA.tv_usec = 0;
                event_add (&IEC104->ev_timer_C_CI_NA, &IEC104->tv_timer_C_CI_NA);
            }
#endif
            break;
        case 0x43:
            wprintf ("\033[38;40;1m$ [测试帧]\n\033[0m");
            IEC104_Master_sTestAck (IEC104->i);
            break;
        default:
            wprintf ("\033[38;40;1m$ [未知短帧: %d]\n\033[0m", pRcvData[2]);
            break;
        }
    }
    else //I_format
    {
        IEC104->is_send_sframe = TRUE;
        if (IEC104->RecvNum == 0x7fff)
            IEC104->RecvNum = 0;
        else
            IEC104->RecvNum += 2;
        IEC104->w++;

        ASDU_TYP = p_asdu->type; //类型标识
        ASDU_VSQ = p_asdu->vsq; //可变结构限定词
        ASDU_COT = (uint16_t) p_asdu->cot_high << 8 | p_asdu->cot_low; //传送原因
        ASDU_ADDR = p_asdu->addr_asdu_high * 256 + p_asdu->addr_asdu_low; //应用服务数据单元公共地址
        INFO_NUMS = p_asdu->vsq & 0x7F; //信息数目
        INFO_SQ = p_asdu->vsq & 0x80; //顺序, 0: 非顺序 1：顺序

        //if (ASDU_ADDR == IEC104->asdu_addr)
        if (1)
        {
            //switch (ASDU_TYP)
            switch (p_asdu->type)
            {
            case  M_SP_NA:
            {
                wprintf ("\033[38;40;1m$ [M_SP_NA, 不带时标的单点信息]\n\033[0m");
            }
            break;

            case  M_ME_NA:
            {
                wprintf ("\033[38;40;1m$ [M_ME_NA, 测量值，规一化值]\n\033[0m");
            }
            break;

            case  M_ME_NC:
            {
                wprintf ("\033[38;40;1m$ [M_ME_NC]\n\033[0m");
            }
            break;

            case  M_PS_NA:
            {
                wprintf ("\033[38;40;1m$ [M_PS_NA, 具有状态变位检出的成组单点信息]\n\033[0m");
            }
            break;

            case  C_IC_NA:
            {
                if (ASDU_COT == 7)
                {
                    wprintf ("\033[38;40;1m$ [C_IC_NA, 总召唤激活确认]\n\033[0m");
                }
            }
            break;

            case  C_SC_NA:
            {
                wprintf ("\033[38;40;1m$ [C_SC_NA]\n\033[0m");
            }
            break;

            case  C_SE_NA:
            {
                wprintf ("\033[38;40;1m$ [C_SE_NA]\n\033[0m");
            }
            break;

            case  M_EI_NA:
            {
                wprintf ("\033[38;40;1m$ [M_EI_NA]\n\033[0m");
            }
            break;

            case  M_SP_TB:
            {
                wprintf ("\033[38;40;1m$ [M_SP_TB,带时标CP56Time2a的单点信息]\n\033[0m");
            }
            break;

            case  C_DC_NA:
            {
                wprintf ("\033[38;40;1m$ [C_DC_NA,双点遥控命令]\n\033[0m");
            }
            break;

            default:
                wprintf ("\033[38;40;1m$ [ErrType(=%d) not define in this program!]\n\033[0m", pRcvData[6]);
                break;
            }

            /* 发送S帧 */
            if (IEC104->is_send_sframe == TRUE)
            {
                if (IEC104->w >= IEC104->Max_w)
                {
                    //Send sframe immediately
                    IEC104_Master_sSFrame (IEC104->i);
                    IEC104->w = 0;
                }
                else
                {
                    //--->add sframe event
                    IEC104->tv_timer_sframe.tv_sec = IEC104->TimeOut_t2;
                    IEC104->tv_timer_sframe.tv_usec = 0;
                    event_add (&IEC104->ev_timer_sframe, &IEC104->tv_timer_sframe);
                    wprintf ("sframe event have added! t2: %d", IEC104->TimeOut_t2);
                }
            }
            else
                wprintf ("not need send s frame!");
        }
        else
        {
            wprintf ("运用地址(%d) != %d", pRcvData[10] + pRcvData[11] * 256, ASDU_ADDR);
        }
    }
    wprintf ("\n\n");
    IEC104->RecvLen = 0;
}

/**
* This function will be called by libevent when the client socket is
* ready for writing.
*/
static void on_write (int fd, short ev, void *arg)
{
    int ret;
    struct IEC104_MASTER *IEC104 = (struct IEC104_MASTER*) arg;

    IEC104->is_idlesse = FALSE;

    ret = write (fd, IEC104->SendBuf, IEC104->SendLen);
    if (ret == -1)
    {
        if (errno == EINTR || errno == EAGAIN)
        {
            /* The write was interrupted by a signal or we
             * were not able to write any data to it,
             * reschedule and return.
             */
            event_add (&IEC104->ev_write, NULL);
            return;
        }
        else
        {
            /* Some other socket error occurred, exit. */
            iec104_master_error_treat (SOCKET_FAILURE, "on_write: Connect failure, disconnecting client!", IEC104);
        }
    }
    else
    {
        //printf ("on_write: have write %.2X bit!\n", ret);
    }

#if 0
    else if ( (bufferq->offset + len) < bufferq->len)
    {
        Not all the data was written, update the offset and
        reschedule the write event.
        bufferq->offset += len;
        event_add (&client->ev_write, NULL);
        return;
    }
#endif

    if (IEC104->SendLen > 6) //is I_frame
    {
        IEC104->SendNum += 2;
    }

    if (debug == TRUE)
    {
        IEC104MASTER_Display (IEC104);
    }

    IEC104->SendLen = 0;
}

static void on_timer_connect (int fd, short event, void *arg)
{
    //wprintf ("timer wakeup\n");
    if (exit_flg)
    {
        printf ("event_loopbreak..\n");
        event_loopbreak ();
    }
    else
    {
        iec104_master_connect ();
    }
    //event_add (&ev_timer_connect, &tv_timer_connect); // reschedule timer
}

static void on_timer_tframe (int fd, short event, void *arg)
{
    struct IEC104_MASTER  *IEC104 = (struct IEC104_MASTER *)arg;

    if (exit_flg)
    {
        printf ("event_loopbreak..\n");
        event_loopbreak ();
    }
    else
    {
        if (IEC104->is_idlesse == TRUE)
        {
            //发送测试帧
            IEC104_Master_sTestAct (IEC104->i);
        }
        else
            IEC104->is_idlesse = TRUE;
    }
    //event_add (&IEC104->ev_timer_tframe, &IEC104->tv_timer_tframe);
}

static void on_timer_WriteDB (int fd, short event, void *arg)
{
    struct IEC104_MASTER *IEC104 = (struct IEC104_MASTER *) arg;
    //printf ("OnTimeWritDB..\n");
    //WRITE_REAL_TAGS.pop ();
    //event_add (&IEC104->ev_timer_tframe, &IEC104->tv_timer_tframe);
}

static void on_timer_sframe (int fd, short event, void *arg)
{
    struct IEC104_MASTER *IEC104 = (struct IEC104_MASTER*) arg;
    if (exit_flg)
    {
        printf ("event_loopbreak..\n");
        event_loopbreak ();
    }
    else
    {
        //发送S帧
        IEC104_Master_sSFrame (IEC104->i);
    }
    //event_add (&IEC104->ev_timer_sframe, &IEC104->tv_timer_sframe);
}

static void on_timer_C_IC_NA (int fd, short event, void *arg)
{
    struct IEC104_MASTER *IEC104 = (struct IEC104_MASTER*) arg;
    //发送总召唤请求帧
    IEC104_Master_sC_IC_NA (IEC104->i, IEC104->asdu_addr, IEC104->info_addr_analog, C_IC_NA_QOI);
    //event_add (&IEC104->ev_timer_C_IC_NA, &IEC104->tv_timer_C_IC_NA);
}

static void on_timer_C_CI_NA (int fd, short event, void *arg)
{
    struct IEC104_MASTER *IEC104 = (struct IEC104_MASTER*) arg;
    //发送电度总召唤请求帧
    IEC104_Master_sC_CI_NA (IEC104->i, IEC104->asdu_addr, IEC104->info_addr_analog, C_CI_NA_QOI);
    //event_add (&IEC104->ev_timer_C_CI_NA, &IEC104->tv_timer_C_CI_NA);
}

/**
 * @brief    激活传输启动
 *
 * @param   i
 */
static void IEC104_Master_sAssociateAct (int i)
{
    unsigned char buf[] =
    {
        0x68, 0x04, 0x07, 0x00, 0x00, 0x00
    };
    if (iec104[i]->is_connect == FALSE)
    {
        return;
    }
    iec104[i]->SendLen = sizeof (buf);
    memcpy (iec104[i]->SendBuf, buf, sizeof (buf));
    iec104_send_msg (i);
}

/**
 * @brief    激活传输确认
 *
 * @param   i
 */
static void IEC104_Master_sAssociateAck (int i)
{
    unsigned char buf[] =
    {
        0x68, 0x04, 0x0b, 0x00, 0x00, 0x00
    };
    if (iec104[i]->is_connect == FALSE)
    {
        return;
    }
    iec104[i]->SendLen = sizeof (buf);
    memcpy (iec104[i]->SendBuf, buf, sizeof (buf));
    iec104_send_msg (i);
}

/**
 * @brief    测试帧
 *
 * @param   i
 */
static void IEC104_Master_sTestAct (int i)
{
    unsigned char buf[] =
    {
        0x68, 0x04, 0x43, 0x00, 0x00, 0x00
    };
    if (iec104[i]->is_connect == FALSE)
    {
        return;
    }
    iec104[i]->SendLen = sizeof (buf);
    memcpy (iec104[i]->SendBuf, buf, sizeof (buf));
    iec104_send_msg (i);
}

/**
 * @brief U帧:测试确认
 */
static void IEC104_Master_sTestAck (int i)
{
    unsigned char buf[] =
    {
        0x68, 0x04, 0x83, 0x00, 0x00, 0x00
    };
    if (iec104[i]->is_connect == FALSE)
        return;
    iec104[i]->SendLen = sizeof (buf);
    memcpy (iec104[i]->SendBuf, buf, sizeof (buf));
    iec104_send_msg (i);
}

/**
 * @brief    S帧
 *
 * @param   i
 */
static void IEC104_Master_sSFrame (int i)
{
    unsigned char buf[] =
    {
        0x68, 0x04, 0x01, 0x00, 0x00, 0x00
    };
    if (iec104[i]->is_connect == FALSE)
        return;
    iec104[i]->SendLen = sizeof (buf);
    memcpy (iec104[i]->SendBuf, buf, sizeof (buf));
    iec104_send_msg (i);
}

/**
 * @brief    总召唤命令
 *
 * @param   i
 */
void IEC104_Master_sC_IC_NA (int i, int asdu_addr, uint32_t info_addr, int qoi)
{
    if (exit_flg)
    {
        printf ("event_loopbreak..\n");
        event_loopbreak ();
        return;
    }
    //68 0d 00 00 00 00 64 01 06 00 01 00 00 00 14
    unsigned char buf[] =
    {
        0x68, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x64, 0x01, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14,
    };
    //asdu_addr, ASDU地址 低前,高后
    buf[10] = asdu_addr & 0xff;
    buf[11] = asdu_addr >> 8;
    //info_addr, 信息对象地址
    buf[12] = info_addr & 0xff;
    buf[13] = info_addr >> 8;
    buf[14] = info_addr >> 16;
    //qoi, 召唤限定词
    buf[15] = qoi;
    if (iec104[i]->is_connect == FALSE)
    {
        return;
    }
    iec104[i]->SendLen = sizeof (buf);
    memcpy (iec104[i]->SendBuf, buf, sizeof (buf));
    iec104[i]->is_send_sframe = FALSE;
    iec104_send_msg (i);
}

/**
 * @brief    电度总召唤
 *
 * @param   i
 * @param   asdu_addr
 * @param   info_addr
 * @param   qoi
 */
void IEC104_Master_sC_CI_NA (int i, int asdu_addr, uint32_t info_addr, int qoi)
{
    if (exit_flg)
    {
        printf ("event_loopbreak..\n");
        event_loopbreak ();
        return;
    }
    unsigned char buf[] =
    {
        0x68, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x65, 0x01, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x14,
    };
    //asdu_addr, ASDU地址 低前,高后
    buf[10] = asdu_addr & 0xff;
    buf[11] = asdu_addr >> 8;
    //info_addr, 信息对象地址
    buf[12] = info_addr & 0xff;
    buf[13] = info_addr >> 8;
    buf[14] = info_addr >> 16;
    //qoi, 召唤限定词
    buf[15] = qoi;
    if (iec104[i]->is_connect == FALSE)
    {
        return;
    }
    iec104[i]->SendLen = sizeof (buf);
    memcpy (iec104[i]->SendBuf, buf, sizeof (buf));
    iec104[i]->is_send_sframe = FALSE;
    iec104_send_msg (i);
}

/**
 * @brief    时钟同步命令
 *
 * @param   i
 */
void IEC104_Master_sC_CS_NA (int i)
{
    if (exit_flg)
    {
        printf ("event_loopbreak..\n");
        event_loopbreak ();
        return;
    }
    unsigned char buf[] =
    {
        0x68, 0x14,  0x02, 0x00, 0x08, 0x00, 0x67, 0x01, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x1F, 0xE0, 0x0C, 0x0B, 0x6A, 0x09, 0x08
    };
    if (iec104[i]->is_connect == FALSE)
        return;
    iec104[i]->SendLen = sizeof (buf);
    memcpy (iec104[i]->SendBuf, buf, sizeof (buf));
    iec104[i]->is_send_sframe = FALSE;
    iec104_send_msg (i);
}

/**
 * @brief    读命令
 *
 * @param   i
 */
void IEC104_Master_sC_RD_NA (int i)
{
    if (exit_flg)
    {
        printf ("event_loopbreak..\n");
        event_loopbreak ();
        return;
    }
    unsigned char buf[] =
    {
        0x68, 0x0d, 0x36, 0x00, 0x26, 0x00, 0x66, 0x01, 0x05, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00
    };
    if (iec104[i]->is_connect == FALSE)
        return;
    iec104[i]->SendLen = sizeof (buf);
    memcpy (iec104[i]->SendBuf, buf, sizeof (buf));
    iec104[i]->is_send_sframe = FALSE;
    iec104_send_msg (i);
}

/**
 * @brief 发送→双点遥控预置
 */
void IEC104_Master_sC_DC_NA_PreSet (int i, int asdu_addr, uint32_t info_addr, unsigned char dco)
{
    if (exit_flg)
    {
        printf ("event_loopbreak..\n");
        event_loopbreak ();
        return;
    }
    unsigned char buf[] =
    {
        0x68, 0x0e, 0x20, 0x00, 0x06, 0x00, 0x2e, 0x01, 0x06, 0x00, 0x01, 0x00, 0x00, 0x0b, 0x00, 0x82
    };
    //asdu_addr, ASDU地址 低前,高后
    buf[10] = asdu_addr & 0xff;
    buf[11] = asdu_addr >> 8;
    //info_addr, 信息对象地址
    buf[12] = info_addr & 0xff;
    buf[13] = info_addr >> 8;
    buf[14] = info_addr >> 16;
    buf[15] = dco;
    if (iec104[i]->is_connect == FALSE)
    {
        return;
    }
    iec104[i]->SendLen = sizeof (buf);
    memcpy (iec104[i]->SendBuf, buf, sizeof (buf));
    iec104[i]->is_send_sframe = FALSE;
    iec104_send_msg (i);
}

/**
 * @brief 发送→双点遥控执行
 */
void IEC104_Master_sC_DC_NA_Execute (int i, int asdu_addr, uint32_t info_addr, unsigned char co)
{
    if (exit_flg)
    {
        printf ("event_loopbreak..\n");
        event_loopbreak ();
        return;
    }
    unsigned char buf[] =
    {
        0x68, 0x0e, 0x20, 0x00, 0x06, 0x00, 0x2e, 0x01, 0x06, 0x00, 0x01, 0x00, 0x00, 0x0b, 0x00, 0x02
    };
    //asdu_addr, ASDU地址 低前,高后
    buf[10] = asdu_addr & 0xff;
    buf[11] = asdu_addr >> 8;
    //info_addr, 信息对象地址
    buf[12] = info_addr & 0xff;
    buf[13] = info_addr >> 8;
    buf[14] = info_addr >> 16;
    buf[15] = co & 0x7f;
    if (iec104[i]->is_connect == FALSE)
    {
        return;
    }
    iec104[i]->SendLen = sizeof (buf);
    memcpy (iec104[i]->SendBuf, buf, sizeof (buf));
    iec104[i]->is_send_sframe = FALSE;
    iec104_send_msg (i);
}

/**
 * @brief    发送→双点遥控撤消
 */
void IEC104_Master_sC_DC_NA_Term (int i, int asdu_addr, uint32_t info_addr, unsigned char co)
{
    if (exit_flg)
    {
        printf ("event_loopbreak..\n");
        event_loopbreak ();
        return;
    }
    unsigned char buf[] =
    {
        0x68, 0x0e, 0x20, 0x00, 0x06, 0x00, 0x2e, 0x01, 0x08, 0x00, 0x01, 0x00, 0x00, 0x0b, 0x00, 0x02
    };
    //asdu_addr, ASDU地址 低前,高后
    buf[10] = asdu_addr & 0xff;
    buf[11] = asdu_addr >> 8;
    //info_addr, 信息对象地址
    buf[12] = info_addr & 0xff;
    buf[13] = info_addr >> 8;
    buf[14] = info_addr >> 16;
    buf[15] = co;
    if (iec104[i]->is_connect == FALSE)
    {
        return;
    }
    iec104[i]->SendLen = sizeof (buf);
    memcpy (iec104[i]->SendBuf, buf, sizeof (buf));
    iec104[i]->is_send_sframe = FALSE;
    iec104_send_msg (i);
}

void IEC104_LongDataSend (int i, int asdu_addr, uint32_t info_addr, unsigned char co)
{
    if (exit_flg)
    {
        printf ("event_loopbreak..\n");
        event_loopbreak ();
        return;
    }
    unsigned char buf[] =
    {
        0x68, 0x0e, 0x20, 0x00, 0x06, 0x00, 0x2e, 0x01, 0x08, 0x00, 0x01, 0x00, 0x00, 0x0b, 0x00, 0x02
    };
    /* asdu_addr, ASDU地址 低前,高后 */
    buf[10] = asdu_addr & 0xff;
    buf[11] = asdu_addr >> 8;
    /* info_addr, 信息对象地址 */
    buf[12] = info_addr & 0xff;
    buf[13] = info_addr >> 8;
    buf[14] = info_addr >> 16;
    /* 传送原因 */
    buf[15] = co;
    if (iec104[i]->is_connect == FALSE)
    {
        return;
    }
    iec104[i]->SendLen = sizeof (buf);
    memcpy (iec104[i]->SendBuf, buf, sizeof (buf));
    iec104[i]->is_send_sframe = FALSE;
    iec104_send_msg (i);
}

static void IEC104_Master_LOGIN (int i)
{
    if (exit_flg)
    {
        printf ("event_loopbreak..\n");
        event_loopbreak ();
        return;
    }
    unsigned char LogInFrame[MAX_DATA_LEN];
    T_C104_FRAME    *p_frame;
    T_C104_ASDU     *p_asdu;
    T_LOG_IN_INF    *p_log_inf;

    p_frame = (T_C104_FRAME*) &LogInFrame[0];
    p_asdu = (T_C104_ASDU*) &LogInFrame[6];
    p_log_inf = (T_LOG_IN_INF*) (&p_asdu->data[0]);

    p_frame->start = 0x68;
    p_frame->len = sizeof (T_LOG_IN_INF) + 10;
    p_asdu->type = TYPE_LOG_IN;
    strlcpy (p_log_inf->user_name, "0002", sizeof (p_log_inf->user_name));
    strlcpy (p_log_inf->passwd, "123", sizeof (p_log_inf->passwd));
    p_log_inf->sn = rtu_sn;

    if (iec104[i]->is_connect == FALSE)
    {
        return;
    }
    iec104[i]->SendLen = p_frame->len + 2;
    memcpy (iec104[i]->SendBuf, LogInFrame, sizeof (LogInFrame));
    iec104_send_msg (i);
}

int IEC104_Master_Display (void *arg)
{
    int dis = (int) arg;
    if (dis == 1)
        debug = TRUE;
    else
        debug = FALSE;
}

static void load_config_from_file ()
{
    int i, ret;
    char key_value[512];
    //--->get tag_file
    ret = ConfigGetKey (IEC104MASTER_CONF_FILE, "tag_file", "file", key_value);
    if (ret == 0)
        strlcpy (tag_file, key_value, sizeof (tag_file));
    else
    {
        strlcpy (tag_file, TAG_FILE, sizeof (TAG_FILE));
        ConfigSetKey (IEC104MASTER_CONF_FILE, "tag_file", "file", TAG_FILE);
    }
    //--->get csv_file_path
    ret = ConfigGetKey (IEC104MASTER_CONF_FILE, "csv_file_path", "path", key_value);
    if (ret == 0)
        strlcpy (csv_file_path, key_value, sizeof (csv_file_path));
    else
    {
        strlcpy (csv_file_path, CSV_FILE_PATH, sizeof (CSV_FILE_PATH));
        ConfigSetKey (IEC104MASTER_CONF_FILE, "csv_file_path", "path", CSV_FILE_PATH);
    }
    //--->get C_IC_NA config
    ret = ConfigGetKey (IEC104MASTER_CONF_FILE, "C_IC_NA", "qoi", key_value);
    if (ret == 0)
        C_IC_NA_QOI = (int) strtol (key_value, NULL, 10);
    else
    {
        ConfigSetKey (IEC104MASTER_CONF_FILE, "C_IC_NA", "qoi", "20");
        C_IC_NA_QOI = 20;
    }
    //--->get C_CI_NA config
    ret = ConfigGetKey (IEC104MASTER_CONF_FILE, "C_CI_NA", "qoi", key_value);
    if (ret == 0)
        C_CI_NA_QOI = (int) strtol (key_value, NULL, 10);
    else
    {
        ConfigSetKey (IEC104MASTER_CONF_FILE, "C_CI_NA", "qoi", "20");
        C_CI_NA_QOI = 20;
    }


    /* get DB config */

    //--->get rtu_sn
    ret = ConfigGetKey (IEC104MASTER_CONF_FILE, "DBServer", "rtu_sn", key_value);
    if (ret == 0)
        rtu_sn = (int) strtol (key_value, NULL, 10);
    else
    {
        ConfigSetKey (IEC104MASTER_CONF_FILE, "DBServer", "rtu_sn", "1");
        rtu_sn = 1;
    }
    //--->get ip
    ret = ConfigGetKey (IEC104MASTER_CONF_FILE, "DBServer", "ip", key_value);
    if (ret == 0)
    {
        char value[250];
        const char *lps = NULL, *lpe = NULL;
        lpe = lps = key_value;
        IEC104_SLAVE_IP_NUMS = 0;
        while (lps)
        {
            lps = strchr (lps, ',');
            if (lps)
            {
                memset (value, 0, sizeof (value));
                strncpy (value, lpe, lps - lpe);
                printf ("DBServerIP:%s\n", value);

                iec104[IEC104_SLAVE_IP_NUMS] =  new (struct IEC104_MASTER);
                if (iec104[IEC104_SLAVE_IP_NUMS] == NULL)
                {
                    WCX_DEBUG ("malloc%s", "\n");
                    exit (1);
                }
                strlcpy (iec104[IEC104_SLAVE_IP_NUMS]->ip_addr, value, sizeof (iec104[IEC104_SLAVE_IP_NUMS]->ip_addr));
                iec104[IEC104_SLAVE_IP_NUMS]->port = DBSERVER_PORT;
                iec104[IEC104_SLAVE_IP_NUMS]->asdu_addr = rtu_sn;
                iec104[IEC104_SLAVE_IP_NUMS]->Max_k = 12;
                iec104[IEC104_SLAVE_IP_NUMS]->Max_w = 8;
                iec104[IEC104_SLAVE_IP_NUMS]->TimeOut_t0 = 10;
                iec104[IEC104_SLAVE_IP_NUMS]->TimeOut_t1 = 12;
                iec104[IEC104_SLAVE_IP_NUMS]->TimeOut_t2 = 5;
                iec104[IEC104_SLAVE_IP_NUMS]->TimeOut_t3 = 15;
                iec104[IEC104_SLAVE_IP_NUMS]->TimeOut_t4 = 8;
                iec104[IEC104_SLAVE_IP_NUMS]->TimeOut_C_IC_NA = 15;
                iec104[IEC104_SLAVE_IP_NUMS]->TimeOut_C_IC_NA = 200;
                iec104[IEC104_SLAVE_IP_NUMS]->TimeOut_zu = 30;
                iec104[IEC104_SLAVE_IP_NUMS]->is_connect = FALSE;
                iec104[IEC104_SLAVE_IP_NUMS]->tag.clear();

                IEC104_SLAVE_IP_NUMS++;
                lps += 1;
                lpe = lps;
            }
        }
    }
    else
    {
        WCX_DEBUG ("get ip err!%s", "\n");
        exit (1);
    }
}

WRITE_REAL_VAL_TAG::WRITE_REAL_VAL_TAG ()
{
    pos = 0;
    queue.resize (REAL_VAL_TAG_COUNT);
    pthread_mutex_init (&mutex, NULL);
}

WRITE_REAL_VAL_TAG::~WRITE_REAL_VAL_TAG ()
{
}

void MAKE_REAL_TAG (int tag, double value, REAL_VAL_TAG *real_val_tag)
{
    long VALUE = value * 1000;

    real_val_tag->info_l = tag & 0xff;
    real_val_tag->info_m = (tag >> 8) & 0xff;
    real_val_tag->info_h = (tag >> 16) & 0xff;

    real_val_tag->val_0 = VALUE & 0xff;
    real_val_tag->val_1 = (VALUE >> 8) & 0xff;
    real_val_tag->val_2 = (VALUE >> 16) & 0xff;
    real_val_tag->val_3 = (VALUE >> 24) & 0xff;

    time_t timer;
    struct tm *tm_time;

    /* gets time of day */
    timer = time (NULL);
    /* converts date/time to a structure */
    tm_time = localtime (&timer);
    //printf ("@@@@@@@@@@@@@@@@@@@year: %d, month:%d, day:%d\n", tm_time->tm_year + 1900, tm_time->tm_mon, tm_time->tm_mday);

    real_val_tag->sec_l = tm_time->tm_sec & 0xff;
    real_val_tag->sec_h = (tm_time->tm_sec >> 8) & 0xff;
    real_val_tag->min = tm_time->tm_min;
    real_val_tag->hour = tm_time->tm_hour;
    //real_val_tag->week_day = tm_time->tm_wday;
    real_val_tag->week_day = tm_time->tm_mday;
    real_val_tag->month = tm_time->tm_mon + 1;
    real_val_tag->year = tm_time->tm_year;
}

void WRITE_REAL_VAL_TAG::push (REAL_VAL_TAG tag)
{
    if (pos < REAL_VAL_TAG_COUNT)
    {
        //printf ("##################WRITE_REAL_VAL_TAG::push\n");
        int ret = pthread_mutex_lock (&mutex);
        if (ret != 0)
        {
            write_log (&error_log, -1, "write_db()", "%s,%d:pthread_mutex_lock err!", __FILE__, __LINE__);
        }
        queue.at (pos++) = tag;
        ret = pthread_mutex_unlock (&mutex);
        if (ret != 0)
        {
            write_log (&error_log, -1, "write_db()", "%s,%d:pthread_mutex_unlock err!", __FILE__, __LINE__);
        }

        //REAL_VAL_TAG *ptag = &tag;

        //int info_addr;
        //info_addr = ptag->info_l;
        //info_addr |= (ptag->info_m << 8);
        //info_addr |= (ptag->info_h << 16);

        //uint32_t value;
        //value = ptag->val_0;
        //value |= (ptag->val_1 << 8);
        //value |= (ptag->val_2 << 16);
        //value |= (ptag->val_3 << 24);
        //double dvalue = value;
        //dvalue = dvalue / 1000;
        //printf ("$$tag_id:% 5d, value: %.5f\n", info_addr, dvalue);
    }
    else
    {
        pop ();
    }
}

void PUSH_REAL_VAL_TAG (REAL_VAL_TAG tag)
{
    WRITE_REAL_TAGS.push (tag);
}

void WRITE_REAL_VAL_TAG::pop ()
{
    int i, ret;
    int SIZE = sizeof (REAL_VAL_TAG) * REAL_VAL_TAG_COUNT;
    unsigned char Batch_Buf[SIZE];
    REAL_VAL_TAG Write_TAG[REAL_VAL_TAG_COUNT];

    if (pos >= REAL_VAL_TAG_COUNT)
    {
        ret = pthread_mutex_lock (&mutex);
        if (ret != 0)
            write_log (&error_log, -1, "write_db()", "%s,%d:pthread_mutex_lock err!", __FILE__, __LINE__);
        for (i = 0; i < REAL_VAL_TAG_COUNT; i++)
        {
            pos--;
            if (pos < 0)
                pos = 0;
            Write_TAG[i] = queue.at (pos);
        }
        ret = pthread_mutex_unlock (&mutex);
        if (ret != 0)
            write_log (&error_log, -1, "write_db()", "%s,%d:pthread_mutex_unlock err!", __FILE__, __LINE__);

        for (i = 0; i < IEC104_SLAVE_IP_NUMS; i++)
        {
            T_C104_FRAME    *p_frame;
            T_C104_ASDU     *p_asdu;

            p_frame = (T_C104_FRAME*) &iec104[i]->SendBuf[0];
            p_asdu = (T_C104_ASDU*) &iec104[i]->SendBuf[6];

            p_frame->start = C104_APDU_SATRT;
            p_frame->len = 10 + SIZE;
            p_asdu->type = M_ME_TE;
            p_asdu->vsq = REAL_VAL_TAG_COUNT;
            p_asdu->cot_low = 0x03; /* 自发 */
            p_asdu->cot_high = 0x00;
            p_asdu->addr_asdu_low = iec104[i]->asdu_addr & 0xff;
            p_asdu->addr_asdu_high = (iec104[i]->asdu_addr >> 8) & 0xff;
            memcpy (p_asdu->data, Write_TAG, SIZE);
            iec104[i]->SendLen = 12 + SIZE;
            if (iec104[i]->is_connect == FALSE)
                continue;
            iec104_send_msg (i);
            /* print */
            printf ("write_rel_value:\n");
            for (size_t j = 0; j < iec104[i]->SendLen; j++)
            {
                printf ("%.2x ", iec104[i]->SendBuf[j]);
                if ( ( ( (j + 1) % 40) == 0) && (j != iec104[i]->SendLen - 1))
                    printf ("\n");
            }
            printf ("\n");
        }
    }
    else
    {
        MySleep (0, 300 * 1000);
    }
}

int IEC104MASTER_Run (void *arg)
{
    int i, ret;

    log_init (&error_log, IEC104MASTER_LOG, "IEC104_MASTER_TEST", "v0.2.0", "wang_chen_xi", "err log");
#ifdef WRITE_RCV_LOG
    write_to_log (IEC104MASTER_RCV_LOG, "[IEC104MASTER send/rcv log]", LOG_NEW | LOG_TIME);
#endif

    exit_flg = false;
    load_config_from_file ();

    /* connect event set&add.. */
    event_init();
    tv_timer_connect.tv_sec = CONNECT_CHECK_TIME;
    tv_timer_connect.tv_usec = 0;
    event_set (&ev_timer_connect, -1, EV_PERSIST, on_timer_connect, NULL);
    event_add (&ev_timer_connect, &tv_timer_connect);
    event_dispatch();
}

int IEC104MASTER_Exit (void *arg)
{
    wprintf ("IEC104_MASTER will exit...!\n");
    exit_flg = true;
    for (size_t i = 0; i < IEC104_SLAVE_IP_NUMS; i++)
    {
        delete (iec104[i]);
    }
    event_loopbreak ();
    wprintf ("IEC104_MASTER exited!\n");
}
