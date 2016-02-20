/*================================================================================

  ___   _____    ____   _    ___    _  _                   _
 |_ _| | ____|  / ___| / |  / _ \  | || |            ___  | |   __ _  __   __   ___
  | |  |  _|   | |     | | | | | | | || |_   _____  / __| | |  / _` | \ \ / /  / _ \
  | |  | |___  | |___  | | | |_| | |__   _| |_____| \__ \ | | | (_| |  \ V /  |  __/
 |___| |_____|  \____| |_|  \___/     |_|           |___/ |_|  \__,_|   \_/    \___|

================================================================================*/
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

/* Libevent. */
#include <event.h>

#include "iec104_slave.h"
#include "queue.h"
#include "modbus.h"
#include "wcx_utils.h"
#include "wcx_log.h"
#include "collect.h"
#include "memwatch.h"
#include "INIFileOP.h"

#define DEBUG
#define TEST
//#define WRITE_RCV_LOG

#ifdef DEBUG
#define wprintf(format, arg...)   \
if (debug == TRUE)                \
    printf( format , ##arg)
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


//--->Extern Variable
#if !defined(TEST)
extern struct       CollectTagDataTp OptSerial[SERIES_MAX];
extern uint8_t      kaichu_value;
#endif
//--->Global Variable
static struct event_base    *IEC104SLAVE_event_base;
static struct timeval       tv_read; //read timeout
static struct log_tp        log;
static char                 debug; //debug on/off
static int                  fd_io; //kaichu fd
//static modbus_param_t       mb_param; //modbus used

//Config Variable
static int      PORT; //端口号
static int      MAX_K;
static int      MAX_W;
static int      T0, T1, T2, T3;
static int      RTU_ADDR;


/* Treats errors and flush or close connection if necessary */
static void iec104_error_treat (int code, const char *string, void *arg)
{
    struct IEC104_SLAVE *IEC104 = (struct IEC104_SLAVE *) arg;

    wprintf ("\033[31;40;1m \n-slave: error_treat: %s (%0X)\n\033[0m", string, -code);
    write_log (&log, -1, "error_treat()", "%s:%d", string, -code);

    switch (code)
    {
    case INVALID_DATA:
    case INVALID_EXCEPTION_CODE:
        //flush ( mb_param );
        break;
    case SELECT_FAILURE:
    case SOCKET_FAILURE:
    case CONNECTION_CLOSED:
        //event_del (&IEC104->ev_read);
        MySleep (1, 0);
        break;
    default:
        /* NOP */
        break;
    }
}

static void Display (struct IEC104_SLAVE *IEC104)
{
    wprintf ("\033[34;40;1m -slave:send  to [%s]: \033[0m\n", IEC104->IpAddr_Client);
#ifdef WRITE_RCV_LOG
    char str[512];
    bzero (str, sizeof (str));
    strlcpy (str, "++send: ", sizeof (str));
#endif
    int i;
    for (i = 0; i < IEC104->SendLen; i++)
    {
#ifdef WRITE_RCV_LOG
        char ch[10];
        sprintf (ch, "%.2X ", IEC104->SendBuf[i]);
        strlcat (str, ch, sizeof (str));
#endif
        wprintf ("%.2X ", IEC104->SendBuf[i]);
    }
    wprintf ("\n");
#ifdef WRITE_RCV_LOG
    write_to_log (IEC104SLAVE_RCV_LOG, str, LOG_APPEND);
#endif

    //--->explan..
    if (IEC104->SendLen == 6)
    {
        if ( (IEC104->SendBuf[2] & 0x03) == 0x01)
        {
            wprintf ("<RECV_NUM_ACK, 接受序号确认帧> ");
        }
        else if ( (IEC104->SendBuf[2] & 0x03) == 0x03)
        {
            switch (IEC104->SendBuf[2])
            {
            case 0x04|0x03:
                wprintf ("<STARTDT_ACT>, 链路建立请求帧");
                break;
            case 0x08|0x03:
                wprintf ("<STARTDT_ACK>, 链路建立确认帧");
                break;
            case 0x10|0x03:
                wprintf ("<STOPDT_ACT>, 链路停止请求帧");
                break;
            case 0x20|0x03:
                wprintf ("<STOPDT_ACK>, 链路停止确认帧");
                break;
            case 0x40|0x03:
                wprintf ("<TESTFP_ACT>, 链路测试请求帧");
                break;
            case 0x80|0x03:
                wprintf ("<TESTFP_ACK>, 链路测试确认帧");
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
            wprintf ("<M_SP_NA, 单点信息> ");
            break;
        case M_DP_NA:
            wprintf ("<M_DP_NA, 双点信息>");
            break;
        case M_ST_NA:
            wprintf ("<M_ST_NA>");
            break;
        case M_BO_NA:
            wprintf ("<M_BO_NA>");
            break;
        case M_ME_NA:
            wprintf ("<M_ME_NA, 测量值，规一化值>");
            break;
        case M_ME_NB:
            wprintf ("<M_ME_NB, 测量值，标度化值>");
            break;
        case M_ME_NC:
            wprintf ("<M_ME_NC>");
            break;
        case M_IT_NA:
            wprintf ("<M_IT_NA>");
            break;
        case M_PS_NA:
            wprintf ("<M_PS_NA>");
            break;
        case M_ME_ND:
            wprintf ("<M_ME_ND>");
            break;
        case M_SP_TB:
            wprintf ("<M_SP_TB, 带时标CP56Time2a 的单点信息>");
            break;
        case M_DP_TB:
            wprintf ("<M_DP_TB, 带时标CP56Time2a 的双点信息>");
            break;
        case M_ST_TB:
            wprintf ("<M_ST_TB>");
            break;
        case M_BO_TB:
            wprintf ("<M_BO_TB>");
            break;
        case M_ME_TD:
            wprintf ("<M_ME_TD, 时标CP56Time2a 的测量值，规一化值>");
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
    {
        return flags;
    }
    flags |= O_NONBLOCK;
    if (fcntl (fd, F_SETFL, flags) < 0)
    {
        return -1;
    }
    return 0;
}

static void on_timer (int fd, short event, void *arg)
{
    //wprintf ("timer wakeup\n");
}

/**
 * This function will be called by libevent when there is a connection
 * ready to be accepted.
 */
static void on_accept (int fd, short ev, void *arg)
{
    int ret, option;
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof (client_addr);
    struct IEC104_SLAVE *IEC104SLAVE;

    /* Accept the new connection. */
    client_fd = accept (fd, (struct sockaddr *) &client_addr, &client_len);
    if (client_fd == -1)
    {
        write_log (&log, -1, "on_accept()", "accept:%s", strerror (errno));
        warn ("accept failed");
        return;
    }

    /* Set the client socket to non-blocking mode. */
    if (setnonblock (client_fd) < 0)
    {
        write_log (&log, -1, "on_accept()", "setnonblock:%s", strerror (errno));
        warn ("failed to set client socket non-blocking");
    }

    /* Set the TCP no delay flag */
    /* SOL_TCP = IPPROTO_TCP */
    option = 1;
    ret = setsockopt (client_fd, IPPROTO_TCP, TCP_NODELAY, (const void *) &option, sizeof (int));
    if (ret < 0)
    {
        //--->changing...
        close (client_fd);
        write_log (&log, -1, "on_accept()", "setsockopt:%s", strerror (errno));
        return;
    }
#if (!HAVE_DECL___CYGWIN__)
    /**
     * Cygwin defines IPTOS_LOWDELAY but can't handle that flag so it's
     * necessary to workaround that problem.
     **/
    /* Set the IP low delay option */
    option = IPTOS_LOWDELAY;
    ret = setsockopt (client_fd, IPPROTO_IP, IP_TOS, (const void *) &option, sizeof (int));
    if (ret < 0)
    {
        //--->changing...
        close (client_fd);
        write_log (&log, -1, "on_accept()", "setsockopt:%s", strerror (errno));
        return;
    }
#endif

    /* We've accepted a new client, allocate a client object to
     * maintain the state of this client.
     */
    IEC104SLAVE = (struct IEC104_SLAVE*) calloc (1, sizeof (struct IEC104_SLAVE));
    if (IEC104SLAVE == NULL)
    {
        write_log (&log, -1, "on_accept()", "calloc:%s", strerror (errno));
        err (1, "calloc failed");
    }
    else
    {
        write_log (&log, 1, "on_accept()", "calloc ok!");
    }

    /* Init.. */
    strlcpy (IEC104SLAVE->IpAddr_Client, inet_ntoa (client_addr.sin_addr),
             sizeof (IEC104SLAVE->IpAddr_Client));
    IEC104SLAVE->is_connect = TRUE;
    IEC104SLAVE->is_send_sframe = FALSE;
    IEC104SLAVE->is_idlesse = FALSE;
    IEC104SLAVE->SendNum = 0;
    IEC104SLAVE->RecvNum = 0;
    IEC104SLAVE->k = 0;
    IEC104SLAVE->w = 0;
    IEC104SLAVE->fd = client_fd;

    //--->set sframe event
    //event_base_set (IEC104SLAVE_event_base, &IEC104SLAVE->ev_timer_sframe);
    evtimer_set (&IEC104SLAVE->ev_timer_sframe, on_timer_sframe, IEC104SLAVE);

    //--->set write event
    //event_base_set (IEC104SLAVE_event_base, &IEC104SLAVE->ev_write);
    //event_set (&IEC104SLAVE->ev_write, client_fd, EV_WRITE, on_write, IEC104SLAVE);
    //event_assign (&IEC104SLAVE->ev_write, IEC104SLAVE_event_base, client_fd, EV_WRITE, on_write, IEC104SLAVE);

    //--->set&add tframe event
    //event_base_set (IEC104SLAVE_event_base, &IEC104SLAVE->ev_timer_tframe);
    //event_set (&IEC104SLAVE->ev_timer_tframe, -1, EV_PERSIST, on_timer_tframe, IEC104SLAVE);
    event_assign (&IEC104SLAVE->ev_timer_tframe, IEC104SLAVE_event_base, -1, EV_PERSIST,
                  on_timer_tframe, IEC104SLAVE);
    IEC104SLAVE->tv_timer_tframe.tv_sec = T3;
    IEC104SLAVE->tv_timer_tframe.tv_usec = 0;
    event_add (&IEC104SLAVE->ev_timer_tframe, &IEC104SLAVE->tv_timer_tframe);

    //--->set&add read event
    //event_base_set (IEC104SLAVE_event_base, &IEC104SLAVE->ev_read);
    //event_set (&IEC104SLAVE->ev_read, client_fd, EV_READ | EV_PERSIST, on_read, IEC104SLAVE);
    event_assign (&IEC104SLAVE->ev_read, IEC104SLAVE_event_base, client_fd, EV_READ | EV_PERSIST,
                  on_read, IEC104SLAVE);
    event_add (&IEC104SLAVE->ev_read, NULL);

    wprintf ("-slave: Accepted connection from %s\n", inet_ntoa (client_addr.sin_addr));
}

/**
* This function will be called by libevent when the client socket is
* ready for reading.
*/
static void on_read (int fd, short ev, void *arg)
{
    struct IEC104_SLAVE     *IEC104 = (struct IEC104_SLAVE *) arg;
    int                     ret, i, j;
    int                     ret_modbus; //执行modbus命令返回值
    uint8_t                 ASDU_TYP; //类型标识
    uint8_t                 ASDU_VSQ; //可变结构限定词
    uint16_t                ASDU_COT; //传送原因
    uint16_t                ASDU_ADDR; //应用服务数据单元公共地址
    uint16_t                INFO_ADDR; //信息对象地址
    uint8_t                 INFO_SQ; //信息顺序 0：非顺序 1：顺序
    uint16_t                INFO_NUMS; //信息数目
    int                     id, com, slave, fun, addr, value; //测点ID 串口号 RTU addr
    ST_C104_FRAME    *p_frame;
    ST_C104_ASDU     *p_asdu;
    BYTE            *p_data, *p_item;

    p_frame = (ST_C104_FRAME*) &IEC104->RecvBuf[0];
    p_asdu = (ST_C104_ASDU*) &IEC104->RecvBuf[6];
    p_data = p_asdu->data;
    IEC104->is_idlesse = FALSE;

    /* rcv two byte.. */
    do
    {
        ret = read (fd, (uint8_t*) &IEC104->RecvBuf[0], 2);
    }
    while (ret == -1 && errno == EINTR);
    if (ret == 0)
    {
        /* Client disconnected, remove the read event and the
        * free the client structure.
        */
        wprintf ("-slave: on_read0:Connect failure, disconnecting client: %s, ret:%d\n", strerror (errno), ret);
        write_log (&log, -1, "on_read()", "read:%s, ret:%d", strerror (errno), ret);
        IEC104->is_connect = FALSE;
        event_del (&IEC104->ev_read);
        event_del (&IEC104->ev_timer_tframe);
        close (fd);
        free (IEC104);
        write_log (&log, 1, "on_read()", "free");
        return;
    }
    else if (ret < 0 || ret != 2 || IEC104->RecvBuf[0] != C104_APDU_SATRT)
    {
        /* Some other error occurred, close the socket, remove
        * the event and free the client structure.
        */
        wprintf ("-slave: on_read1:Connect failure, disconnecting client: %s, ret:%d\n", strerror (errno), ret);
        write_log (&log, -1, "on_read()", "read:%s, ret:%d", strerror (errno), ret);
        IEC104->is_connect = FALSE;
        event_del (&IEC104->ev_read);
        event_del (&IEC104->ev_timer_tframe);
        close (fd);
        free (IEC104);
        write_log (&log, 1, "on_read()", "free");
        return;
    }

    /* rcv the next all byte.. */
    do
    {
        ret = read (fd, (uint8_t*) &IEC104->RecvBuf[2], p_frame->len);
    }
    while (ret == -1 && errno == EINTR);
    if (ret == 0)
    {
        /* Client disconnected, remove the read event and the
        * free the client structure.
        */
        wprintf ("-slave: on_read2:Connect failure, disconnecting client: %s, ret:%d\n", strerror (errno), ret);
        write_log (&log, -1, "on_read()", "read:%s, ret:%d", strerror (errno), ret);
        IEC104->is_connect = FALSE;
        event_del (&IEC104->ev_read);
        event_del (&IEC104->ev_timer_tframe);
        close (fd);
        free (IEC104);
        write_log (&log, 1, "on_read()", "free");
        return;
    }
    else if (ret < 0 || ret != IEC104->RecvBuf[1])
    {
        /* Some other error occurred, close the socket, remove
        * the event and free the client structure.
        */
        wprintf ("-slave: on_read3:Connect failure, disconnecting client: %s, ret:%d\n", strerror (errno), ret);
        write_log (&log, -1, "on_read()", "read:%s, ret:%d", strerror (errno), ret);
        IEC104->is_connect = FALSE;
        event_del (&IEC104->ev_read);
        event_del (&IEC104->ev_timer_tframe);
        close (fd);
        free (IEC104);
        write_log (&log, 1, "on_read()", "free");
        return;
    }

    IEC104->RecvLen = IEC104->RecvBuf[1] + 2;

    /* print the rcv frame */

    if (debug == TRUE)
    {
#ifdef WRITE_RCV_LOG
        char str[512];
        bzero (str, sizeof (str));
        strlcpy (str, "---rcv: ", sizeof (str));
#endif
        wprintf ("\033\n[32;40;1m-slave: rcv from [%s]:\n\033[0m", IEC104->IpAddr_Client);
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
        write_to_log (IEC104SLAVE_RCV_LOG, str, LOG_APPEND);
#endif
    }

    /* Analyse the frame */

    if (IEC104->RecvLen < 6)
    {
        if (IEC104->RecvLen > 0)
        {
            IEC104->RecvLen = 0;
            wprintf ("!!!!!!!!!!!!-iec104_slave: len err-!!!!!!!!!!\n");
        }
        return;
    }
    if (IEC104->RecvLen == 6) //U_FORMAT or I_FORMAT
    {
        switch (p_frame->c1)
        {
        case 0x01: //S_Format
            wprintf ("\033[38;40;1m$ [S帧]\n\033[0m");
            break;
        case 0x07:
            wprintf ("\033[38;40;1m$ [建立连接请求帧]\n\033[0m");
            IEC104->SendNum = 0;
            IEC104->RecvNum = 0;
            IEC104SLAVE_AssociateAck (IEC104);
            MySleep (1, 0);
            IEC104SLAVE_M_EI_NA (IEC104);
            break;
        case 0x43:
            wprintf ("\033[38;40;1m$ [测试帧]\n\033[0m");
            IEC104SLAVE_TestAck (IEC104);
        case 0x83:
            break;
        default:
            wprintf ("\033[38;40;1m$ [未知短帧: %d]\n\033[0m", p_asdu->type);
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

        //if (ASDU_ADDR == RTU_ADDR)
        if (1)
        {
            switch (p_asdu->type)
            {
            case  M_SP_NA:
            {
                wprintf ("\033[38;40;1m$ [M_SP_NA, 不带时标的单点信息]\n\033[0m");
                if (INFO_SQ)
                    wprintf ("$ 顺序(如:首地址,数据1,数据2..)\n");
                else
                {
                    wprintf ("$ 非顺序(如:地址1,数据1,地址2,数据2..)\n");
                    for (i = 0; i < INFO_NUMS; i++)
                    {
                        p_item = p_data;
                        wprintf ("解析: 信息对象地址:%d, 码值描述%d\n",
                                 (uint32_t) p_item[2] << 16 | (uint32_t) p_item[1] << 8 | p_item[0], //addr
                                 p_item[3]); //码值描述
                        wprintf (" Data%d:\n", i);
                        for (j = 0; j < 4; j++)
                            wprintf ("[%.2X]", *p_data++);
                        wprintf ("\n");
                    }
                }
                break;
            }

            case  M_ME_NA:
            {
                wprintf ("\033[38;40;1m$ [M_ME_NA, 测量值，规一化值]\n\033[0m");
                if (INFO_SQ)
                {
                    wprintf ("$ 顺序(如:首地址,数据1,数据2..)\n");
                }
                else
                {
                    wprintf ("$ 非顺序(如:地址1,数据1,地址2,数据2..)\n");
                    for (i = 0; i < INFO_NUMS; i++)
                    {
                        p_item = p_data;
                        wprintf ("解析: 信息对象地址:%d, value:%d, 码值描述%d\n",
                                 (uint32_t) p_item[2] << 16 | (uint32_t) p_item[1] << 8 | p_item[0], //addr
                                 (uint16_t) p_item[4] << 8 | p_item[3], //value
                                 p_item[5]); //码值描述
                        wprintf (" Data%d:\n", i);
                        for (j = 0; j < 6; j++)
                            wprintf ("[%.2X]", *p_data++);
                        wprintf ("\n");
                    }
                }
                break;
            }

            case  M_ME_NC:
            {
                wprintf ("\033[38;40;1m$ [M_ME_NC]\n\033[0m");
                break;
            }

            case  M_PS_NA:
            {
                wprintf ("\033[38;40;1m$ [M_PS_NA, 具有状态变位检出的成组单点信息]\n\033[0m");
                if (INFO_SQ)
                {
                    wprintf ("$ 顺序(如:首地址,数据1,数据2..)\n");
                }
                else
                {
                    wprintf ("$ 非顺序(如:地址1,数据1,地址2,数据2..)\n");
                    for (i = 0; i < INFO_NUMS; i++)
                    {
                        p_item = p_data;
                        wprintf ("解析: 信息对象地址:%d, 状态位ST:%d, 变位检出CD%d, 码值描述%d\n",
                                 (uint32_t) p_item[2] << 16 | (uint32_t) p_item[1] << 8 | p_item[0], //信息对象地址
                                 (uint16_t) p_item[4] << 8 | p_item[3], //状态位ST(位 0:开,1:合)
                                 (uint16_t) p_item[6] << 8 | p_item[5], //变位检出CD
                                 p_item[7]); //码值描述－－IV
                        wprintf (" Data%d:\n", i);
                        for (j = 0; j < 8; j++)
                            wprintf ("[%.2X]", *p_data++);
                        wprintf ("\n");
                    }
                }
                break;
            }

            case  C_IC_NA:
            {
                if (ASDU_COT == 7)
                {
                    wprintf ("\033[38;40;1m$ [C_IC_NA, 总召唤激活确认]\n\033[0m");
                    if (INFO_SQ)
                    {
                        wprintf ("$ 顺序(如:首地址,数据1,数据2..)\n");
                    }
                    else
                    {
                        wprintf ("$ 非顺序(如:地址1,数据1,地址2,数据2..)\n");
                        p_item = p_data;
                        wprintf ("解析: 信息对象地址:%d, 召唤限定词QOI:%d\n",
                                 (uint32_t) p_item[2] << 16 | (uint32_t) p_item[1] << 8 | p_item[0], //addr
                                 p_item[3]); //召唤限定词QOI
                        wprintf ("\n");
                    }
                }
                break;
            }

            case  C_SE_NA:
            {
                wprintf ("\033[38;40;1m$ [C_SE_NA, 归一化设点命令]\n\033[0m");
                if (INFO_SQ)
                {
                    wprintf ("$ 顺序(如:首地址,数据1,数据2..)\n");
                }
                else
                {
                    wprintf ("$ 非顺序(如:地址1,数据1,地址2,数据2..)\n");
                    for (i = 0; i < INFO_NUMS; i++)
                    {
                        p_item = p_data;
                        int NVA = 0;
                        int QOS = 0;
                        NVA = (uint16_t) p_item[4] << 8 | p_item[3];
                        QOS = (uint16_t) p_item[5];
                        id = (uint32_t) p_item[2] << 16 | (uint32_t) p_item[1] << 8 | p_item[0]; //addr
                        wprintf ("解析: 信息对象地址:%d, 设定值:%d, 设定命令限定词%d\n",
                                 id, //信息对象地址
                                 NVA, QOS);
                        ////////////////////////////////////////////////////////////////////////////////////////
                        if (ASDU_COT == 0x06)  //is激活帧
                        {
                            if ( (QOS & 0x80) == 0x80) //is设定规一化值 选择
                            {
                                wprintf (">>>>>>收到设定规一化值 选择, 选择确认...\n");
                                IEC104SLAVE_C_SE_NA_OkAck (IEC104);
                            }
                            else //is设定规一化值 执行
                            {
#if !defined(TEST)
                                //--->处理...
                                wprintf (">>>>>收到归一化设点命令执行命令...\n");
                                ret = get_comno_slave_addr (id, &com, &slave, &fun, &addr, &value);
                                if (ret < 0)
                                {
                                    wprintf ("get_comno_slave_addr err!\n");
                                    return;
                                }
                                //else
                                //write_log (&log, 1, "on_read()", "id:%d,  com:%d, slave:%d, addr:%d", id, com, slave, addr);
                                ret = pthread_mutex_lock (&OptSerial[com].mutex);
                                if (ret != 0)
                                    write_log (&log, -1, "on_read()", "pthread_mutex_lock err!");
                                OptSerial[com].mb_param.slave = slave;
                                MySleep (0, 100);
                                for (i = 0; i < 5; i++)
                                {
                                    ret_modbus = preset_single_register (&OptSerial[com].mb_param, addr, NVA, 2000); //执行...
                                    if (ret_modbus > 0)
                                        break;
                                    else
                                        MySleep (0, 500);
                                }
                                ret = pthread_mutex_unlock (&OptSerial[com].mutex);
                                if (ret != 0)
                                    write_log (&log, -1, "on_read()", "pthread_mutex_lock err!");
                                if (ret_modbus > 0)
                                {
                                    printf ("preset_single_register ok!\n");
                                    wprintf ("发送设定规一化值结束帧!\n");
                                    IEC104SLAVE_C_SE_NA_EndAck (IEC104);
                                }
                                else
                                {
                                    printf ("preset_single_register err!\n");
                                    write_log (&log, -1, "on_read()", "preset_single_register err!");
                                }
#else
                                wprintf ("发送设定规一化值结束帧!\n");
                                IEC104SLAVE_C_SE_NA_EndAck (IEC104);
#endif
                            }
                        }
                        else if (ASDU_COT == 0x08)  //is停止激活帧
                        {
                            wprintf ("发送设定规一化值撤销选择帧!\n");
                            IEC104SLAVE_C_SE_NA_NoAck (IEC104);
                        }
                        ////////////////////////////////////////////////////////////////////////////////////////
                        //--->print the date
                        wprintf (" Data%d:\n", i);
                        for (j = 0; j < 6; j++)
                            wprintf ("[%.2X]", *p_item++);
                        wprintf ("\n");
                        p_data += 6;
                    }
                }
                break;
            }

            case  M_EI_NA:
            {
                wprintf ("\033[38;40;1m$ [M_EI_NA]\n\033[0m");
                break;
            }

            case  M_SP_TB:
            {
                wprintf ("\033[38;40;1m$ [M_SP_TB,带时标CP56Time2a的单点信息]\n\033[0m");
                if (INFO_SQ)
                {
                    wprintf ("$ 顺序(如:首地址,数据1,数据2..)\n");
                }
                else
                {
                    wprintf ("$ 非顺序(如:地址1,数据1,地址2,数据2..)\n");
                    for (i = 0; i < INFO_NUMS; i++)
                    {
                        p_item = p_data;
                        wprintf ("解析: 信息对象地址:%d, value:%d, %d年/%d月/%d日/星期%d/%d时/%d分/%d秒\n",
                                 (uint32_t) p_item[2] << 16 | (uint32_t) p_item[1] << 8 | p_item[0], //addr
                                 p_item[3], //value
                                 p_item[10] & 0x7f, //year
                                 p_item[9] & 0xf, //month
                                 p_item[8] & 0x1f, //day
                                 p_item[8] >> 5, //week
                                 p_item[7] & 0x1f, //hour
                                 p_item[6] & 0x3f, //minute
                                 ( (uint16_t) p_item[5] << 8 | p_item[4]) / 1000); //second
                        wprintf (" Data%d:\n", i);
                        for (j = 0; j < 11; j++)
                            wprintf ("[%.2X]", *p_data++);
                        wprintf ("\n");
                    }
                }
                break;
            }

            /* dan_dian_yao_kong */
            case  C_SC_NA:
            {
                wprintf ("\033[38;40;1m$ [C_SC_NA,单点遥控命令]\n\033[0m");
                if (INFO_SQ)
                {
                    wprintf ("$ 顺序(如:首地址,数据1,数据2..)\n");
                }
                else
                {
                    wprintf ("$ 非顺序(如:地址1,数据1,地址2,数据2..)\n");
                    for (i = 0; i < INFO_NUMS; i++)
                    {
                        p_item = p_data;
                        id = (uint32_t) p_item[2] << 16 | (uint32_t) p_item[1] << 8 | p_item[0]; //addr
                        wprintf ("解析: 信息对象地址:%d, value:%d\n", id, p_item[3]);
                        if (ASDU_COT == 0x06)  //is激活帧
                        {
                            if ( (p_item[3] & 0x80) == 0x80) //is遥控预置激活
                            {
                                wprintf (">>>>>>收到遥控预置, 发送遥控返校...\n");
                                IEC104SLAVE_C_DC_NA_PreAck (IEC104); //发送→遥控返校
                            }
                            else //is遥控执行激活
                            {
#if !defined(TEST)
                                int switch_type, delay_time;
                                int ret = get_ort3000_switch_attr (id, &switch_type, &delay_time);
                                if (ret < 0)
                                {
                                    printf ("get_ort3000_switch_attr err!\n");
                                    write_log (&log, -1, "on_read()", "get_ort3000_switch_attr err!");
                                    return;
                                }
                                if (id >= 25 && id <= 28) //is ort3000开出量
                                {
                                    if (switch_type == 0) // 是开关输出
                                    {
                                        if ( (p_item[3] & 0x03) == 0x00) //分
                                        {
                                            switch (id)
                                            {
                                            case 25:
                                                kaichu_value |= 0x01;
                                                break;
                                            case 26:
                                                kaichu_value |= 0x02;
                                                break;
                                            case 27:
                                                kaichu_value |= 0x04;
                                                break;
                                            case 28:
                                                kaichu_value |= 0x08;
                                                break;
                                            default:
                                                return;
                                            }
                                            //write_log (&log, -1, "on_read()", "run open!");
                                        }
                                        else if ( (p_item[3] & 0x01) == 0x01) //合
                                        {
                                            switch (id)
                                            {
                                            case 25:
                                                kaichu_value &= 0x0e; //1110
                                                break;
                                            case 26:
                                                kaichu_value &= 0x0d; //1101
                                                break;
                                            case 27:
                                                kaichu_value &= 0x0b; //1011
                                                break;
                                            case 28:
                                                kaichu_value &= 0x07; //0111
                                                break;
                                            default:
                                                return;
                                            }
                                            //write_log (&log, -1, "on_read()", "run close!");
                                        }
                                        //printf ("@@@@@kaichu:%x\n", kaichu_value);
                                        char buffer[1];
                                        buffer[0] = kaichu_value;
                                        write (fd_io, buffer, 1);
                                        IEC104SLAVE_C_DC_NA_ExeAck (IEC104); //发送→遥控撤消
                                    }
                                    else if (switch_type == 1) // 是点动输出
                                    {
                                        /* 取反*/
                                        switch (id)
                                        {
                                        case 25:
                                            kaichu_value ^= 0x01;
                                            break;
                                        case 26:
                                            kaichu_value ^= 0x02;
                                            break;
                                        case 27:
                                            kaichu_value ^= 0x04;
                                            break;
                                        case 28:
                                            kaichu_value ^= 0x08;
                                            break;
                                        default:
                                            return;
                                        }
                                        char buffer[1];
                                        buffer[0] = kaichu_value;
                                        write (fd_io, buffer, 1);
                                        /* 延时 */
                                        MySleep (0, delay_time * 1000);
                                        /* 取反*/
                                        switch (id)
                                        {
                                        case 25:
                                            kaichu_value ^= 0x01;
                                            break;
                                        case 26:
                                            kaichu_value ^= 0x02;
                                            break;
                                        case 27:
                                            kaichu_value ^= 0x04;
                                            break;
                                        case 28:
                                            kaichu_value ^= 0x08;
                                            break;
                                        default:
                                            return;
                                        }
                                        buffer[0] = kaichu_value;
                                        write (fd_io, buffer, 1);
                                        IEC104SLAVE_C_DC_NA_ExeAck (IEC104); //发送→遥控撤消
                                    }
                                }
                                else // 非ort3000开出量
                                {
                                    wprintf (">>>>>收到遥控执行, 执行命令后发送执行确认...\n");
                                    ret = get_comno_slave_addr (id, &com, &slave, &fun, &addr, &value);
                                    if (ret < 0)
                                    {
                                        wprintf ("get_comno_slave_addr err!\n");
                                        return;
                                    }
                                    //else
                                    //write_log (&log, 1, "on_read()", "id:%d,  com:%d, slave:%d, addr:%d", id, com, slave, addr);
                                    ret = pthread_mutex_lock (&OptSerial[com].mutex);
                                    if (ret != 0)
                                        write_log (&log, -1, "on_read()", "pthread_mutex_lock err!");
                                    OptSerial[com].mb_param.slave = slave;
                                    MySleep (0, 100);
                                    for (i = 0; i < 5; i++) // try 5 times
                                    {
                                        if (switch_type == 0) // 是开关输出
                                        {
                                            if ( (p_item[3] & 0x01) == 0x01)
                                                ret_modbus = force_single_coil (&OptSerial[com].mb_param, addr, 1, 2000); //执行...
                                            else if ( (p_item[3] & 0x03) == 0x00)
                                                ret_modbus = force_single_coil (&OptSerial[com].mb_param, addr, 0, 2000); //执行...
                                            if (ret_modbus > 0)
                                                break;
                                            else
                                                MySleep (0, 100);
                                        }
                                        else if (switch_type == 1) // 是点动输出
                                        {
                                            if ( (p_item[3] & 0x01) == 0x01)
                                            {
                                                /* ON */
                                                for (size_t j = 0; j < 5; j++)
                                                {
                                                    ret_modbus = force_single_coil (&OptSerial[com].mb_param, addr, 1, 2000); //执行...
                                                    if (ret_modbus > 0)
                                                        break;
                                                    else
                                                        MySleep (0, 100);
                                                }
                                                /* 延时 */
                                                MySleep (0, delay_time * 1000);
                                                /* OFF */
                                                for (size_t j = 0; j < 5; j++)
                                                {
                                                    ret_modbus = force_single_coil (&OptSerial[com].mb_param, addr, 0, 2000); //执行...
                                                    if (ret_modbus > 0)
                                                        break;
                                                    else
                                                        MySleep (0, 100);
                                                }
                                            }
                                            else if ( (p_item[3] & 0x03) == 0x00)
                                            {
                                                /* OFF */
                                                for (size_t j = 0; j < 5; j++)
                                                {
                                                    ret_modbus = force_single_coil (&OptSerial[com].mb_param, addr, 0, 2000); //执行...
                                                    if (ret_modbus > 0)
                                                        break;
                                                    else
                                                        MySleep (0, 100);
                                                }
                                                /* 延时 */
                                                MySleep (0, delay_time * 1000);
                                                /* ON */
                                                for (size_t j = 0; j < 5; j++)
                                                {
                                                    ret_modbus = force_single_coil (&OptSerial[com].mb_param, addr, 1, 2000); //执行...
                                                    if (ret_modbus > 0)
                                                        break;
                                                    else
                                                        MySleep (0, 100);
                                                }
                                            }
                                            break;
                                        }
                                    }
                                    ret = pthread_mutex_unlock (&OptSerial[com].mutex);
                                    if (ret != 0)
                                        write_log (&log, -1, "on_read()", "pthread_mutex_lock err!");
                                    if (ret_modbus > 0)
                                    {
                                        wprintf ("单点遥控OK!\n");
                                        IEC104SLAVE_C_DC_NA_ExeAck (IEC104); //发送→遥控撤消
                                    }
                                    else
                                    {
                                        wprintf ("单点遥控NG!\n");
                                        write_log (&log, -1, "on_read()", "ret_modbus < 0");
                                    }
                                }
#else
                                IEC104SLAVE_C_DC_NA_ExeAck (IEC104); //发送→遥控撤消
#endif
                            }
                        }
                        else if (ASDU_COT == 0x08)  //is停止激活帧
                        {
                            wprintf (">>>>>撤销确认帧...\n");
                            IEC104SLAVE_C_DC_NA_TermAck (IEC104);
                        }
                        ////////////////////////////////////////////////////////
                        wprintf (" Data%d:\n", i);
                        for (j = 0; j < 4; j++)
                            wprintf ("[%.2X]", *p_item++);
                        wprintf ("\n");
                        p_data += 4;
                    }
                }
                break;
            }

            /* shuang_dian_yao_kong */
            case  C_DC_NA:
            {
                wprintf ("\033[38;40;1m$ [C_DC_NA,双点遥控命令]\n\033[0m");
                if (INFO_SQ)
                {
                    wprintf ("$ 顺序(如:首地址,数据1,数据2..)\n");
                }
                else
                {
                    wprintf ("$ 非顺序(如:地址1,数据1,地址2,数据2..)\n");
                    for (i = 0; i < INFO_NUMS; i++)
                    {
                        p_item = p_data;
                        id = (uint32_t) p_item[2] << 16 | (uint32_t) p_item[1] << 8 | p_item[0]; //addr
                        wprintf ("解析: 信息对象地址:%d, value:%d\n", id, p_item[3]);
                        if (ASDU_COT == 0x06)  //is激活帧
                        {
                            if ( (p_item[3] & 0x80) == 0x80) //is遥控预置激活
                            {
                                wprintf (">>>>>>收到遥控预置, 发送遥控返校...\n");
                                IEC104SLAVE_C_DC_NA_PreAck (IEC104); //发送→遥控返校
                            }
                            else //is遥控执行激活
                            {
#if !defined(TEST)
                                int switch_type, delay_time;
                                int ret = get_ort3000_switch_attr (id, &switch_type, &delay_time);
                                if (ret < 0)
                                    return;
                                if (id >= 25 && id <= 28) //is ort3000开出量
                                {
                                    if (switch_type == 0) // 是开关输出
                                    {
                                        if ( (p_item[3] & 0x03) == 0x01) //分
                                        {
                                            switch (id)
                                            {
                                            case 25:
                                                kaichu_value |= 0x01;
                                                break;
                                            case 26:
                                                kaichu_value |= 0x02;
                                                break;
                                            case 27:
                                                kaichu_value |= 0x04;
                                                break;
                                            case 28:
                                                kaichu_value |= 0x08;
                                                break;
                                            default:
                                                return;
                                            }
                                            //write_log (&log, -1, "on_read()", "run open!");
                                        }
                                        else if ( (p_item[3] & 0x03) == 0x02) //合
                                        {
                                            switch (id)
                                            {
                                            case 25:
                                                kaichu_value &= 0x0e; //1110
                                                break;
                                            case 26:
                                                kaichu_value &= 0x0d; //1101
                                                break;
                                            case 27:
                                                kaichu_value &= 0x0b; //1011
                                                break;
                                            case 28:
                                                kaichu_value &= 0x07; //0111
                                                break;
                                            default:
                                                return;
                                            }
                                            //write_log (&log, -1, "on_read()", "run close!");
                                        }
                                        char buffer[1];
                                        buffer[0] = kaichu_value;
                                        write (fd_io, buffer, 1);
                                        IEC104SLAVE_C_DC_NA_ExeAck (IEC104); //发送→遥控撤消
                                    }
                                    else if (switch_type == 1) // 是点动输出
                                    {
                                        /* 取反*/
                                        switch (id)
                                        {
                                        case 25:
                                            kaichu_value ^= 0x01;
                                            break;
                                        case 26:
                                            kaichu_value ^= 0x02;
                                            break;
                                        case 27:
                                            kaichu_value ^= 0x04;
                                            break;
                                        case 28:
                                            kaichu_value ^= 0x08;
                                            break;
                                        default:
                                            return;
                                        }
                                        char buffer[1];
                                        buffer[0] = kaichu_value;
                                        write (fd_io, buffer, 1);
                                        /* 延时 */
                                        MySleep (0, delay_time * 1000);
                                        /* 取反*/
                                        switch (id)
                                        {
                                        case 25:
                                            kaichu_value ^= 0x01;
                                            break;
                                        case 26:
                                            kaichu_value ^= 0x02;
                                            break;
                                        case 27:
                                            kaichu_value ^= 0x04;
                                            break;
                                        case 28:
                                            kaichu_value ^= 0x08;
                                            break;
                                        default:
                                            return;
                                        }
                                        //write_log (&log, -1, "on_read()", "run close!");
                                        buffer[0] = kaichu_value;
                                        write (fd_io, buffer, 1);
                                        IEC104SLAVE_C_DC_NA_ExeAck (IEC104); //发送→遥控撤消
                                    }
                                }
                                else // 非ort3000开出量
                                {
                                    wprintf (">>>>>收到遥控执行, 执行命令后发送执行确认...\n");
                                    ret = get_comno_slave_addr (id, &com, &slave, &fun, &addr, &value);
                                    if (ret < 0)
                                    {
                                        wprintf ("get_comno_slave_addr err!\n");
                                        return;
                                    }
                                    //else
                                    //write_log (&log, 1, "on_read()", "id:%d,  com:%d, slave:%d, addr:%d", id, com, slave, addr);
                                    printf ("!!!!!!!!com:%d,slave:%d, addr:%d\n", com, slave, addr);
                                    ret = pthread_mutex_lock (&OptSerial[com].mutex);
                                    if (ret != 0)
                                        write_log (&log, -1, "on_read()", "pthread_mutex_lock err!");
                                    OptSerial[com].mb_param.slave = slave;
                                    MySleep (0, 100);
                                    for (i = 0; i < 5; i++)
                                    {
                                        if (switch_type == 0) // 是开关输出
                                        {
                                            if ( (p_item[3] & 0x03) == 0x02)
                                                ret_modbus = force_single_coil (&OptSerial[com].mb_param, addr, 1, 2000); //执行...
                                            else if ( (p_item[3] & 0x03) == 0x01)
                                                ret_modbus = force_single_coil (&OptSerial[com].mb_param, addr, 0, 2000); //执行...
                                            if (ret_modbus > 0)
                                                break;
                                            else
                                                MySleep (0, 100);
                                        }
                                        else if (switch_type == 1) // 是点动输出
                                        {
                                            if ( (p_item[3] & 0x03) == 0x02)
                                            {
                                                /* ON */
                                                for (size_t j = 0; j < 5; j++)
                                                {
                                                    ret_modbus = force_single_coil (&OptSerial[com].mb_param, addr, 1, 2000); //执行...
                                                    if (ret_modbus > 0)
                                                        break;
                                                    else
                                                        MySleep (0, 100);
                                                }
                                                /* 延时 */
                                                MySleep (0, delay_time * 1000);
                                                /* OFF */
                                                for (size_t j = 0; j < 5; j++)
                                                {
                                                    ret_modbus = force_single_coil (&OptSerial[com].mb_param, addr, 0, 2000); //执行...
                                                    if (ret_modbus > 0)
                                                        break;
                                                    else
                                                        MySleep (0, 100);
                                                }
                                            }
                                            else if ( (p_item[3] & 0x03) == 0x01)
                                            {
                                                /* OFF */
                                                for (size_t j = 0; j < 5; j++)
                                                {
                                                    ret_modbus = force_single_coil (&OptSerial[com].mb_param, addr, 0, 2000); //执行...
                                                    if (ret_modbus > 0)
                                                        break;
                                                    else
                                                        MySleep (0, 100);
                                                }
                                                /* 延时 */
                                                MySleep (0, delay_time * 1000);
                                                /* ON */
                                                for (size_t j = 0; j < 5; j++)
                                                {
                                                    ret_modbus = force_single_coil (&OptSerial[com].mb_param, addr, 1, 2000); //执行...
                                                    if (ret_modbus > 0)
                                                        break;
                                                    else
                                                        MySleep (0, 100);
                                                }
                                            }
                                            break;
                                        }
                                    }
                                    ret = pthread_mutex_unlock (&OptSerial[com].mutex);
                                    if (ret != 0)
                                        write_log (&log, -1, "on_read()", "pthread_mutex_lock err!");
                                    if (ret_modbus > 0)
                                    {
                                        wprintf ("遥控OK!\n");
                                        IEC104SLAVE_C_DC_NA_ExeAck (IEC104); //发送→执行确认
                                    }
                                    else
                                    {
                                        wprintf ("遥控NG!\n");
                                        write_log (&log, -1, "on_read()", "ret_modbus < 0");
                                    }
                                }
#else
                                IEC104SLAVE_C_DC_NA_ExeAck (IEC104); //发送→执行确认
#endif
                            }
                        }
                        else if (ASDU_COT == 0x08)  //is停止激活帧
                        {
                            wprintf (">>>>>撤销确认帧...\n");
                            IEC104SLAVE_C_DC_NA_TermAck (IEC104);
                        }
                        ////////////////////////////////////////////////////////
                        p_data += 4;
                        wprintf ("\n");
                    }
                }
                break;
            }

            default:
            {
                wprintf ("Err---TypeId(=%d) not define in this program!\n", p_data[6]);
            }
            break;
            } /* end switch */

            //--->发送S帧
            if (IEC104->is_send_sframe == TRUE)
            {
                if (IEC104->w >= MAX_W)
                {
                    //Send sframe immediately
                    IEC104SLAVE_Sframe (IEC104);
                    IEC104->w = 0;
                }
                else
                {
                    //--->add sframe event
                    IEC104->tv_timer_sframe.tv_sec = T2;
                    IEC104->tv_timer_sframe.tv_usec = 0;
                    event_add (&IEC104->ev_timer_sframe, &IEC104->tv_timer_sframe);
                }
            }
        } /* end if ASDU_ADDR == RTU_ADDR */
        else
            wprintf ("运用地址(%d) != %d\n", ASDU_ADDR, RTU_ADDR);
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
    struct IEC104_SLAVE *IEC104 = (struct IEC104_SLAVE *) arg;
    int ret;

    ST_C104_FRAME *p_frame = (ST_C104_FRAME*) &IEC104->SendBuf[0];
    ST_C104_ASDU *p_asdu = (ST_C104_ASDU*) &IEC104->SendBuf[6];

    /* make frame */
    if (IEC104->SendLen > 0)
    {
        if (IEC104->SendLen > 6) //is I_frame
        {
            IEC104->SendBuf[2] = (IEC104->SendNum % 256);
            IEC104->SendBuf[3] = IEC104->SendNum / 256;
            IEC104->SendBuf[4] = (IEC104->RecvNum % 256);
            IEC104->SendBuf[5] = IEC104->RecvNum / 256;
            IEC104->SendNum += 2;
        }
        else if (IEC104->SendLen == 6)
        {
            if ( (IEC104->SendBuf[2] & 0x03) == 0x01)  //is S_frame
            {
                wprintf ("/*********************** S_frame ****************************\n");
                wprintf ("RecvNum:%d\n\n", IEC104->RecvNum);
                IEC104->SendBuf[4] = (IEC104->RecvNum % 256);
                IEC104->SendBuf[5] = IEC104->RecvNum / 256;
            }
        }
    }

    /* send the frame */
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
            //err (1, "write");
            wprintf ("write err: %s\n", strerror (errno));
            write_log (&log, -1, "on_write()", "write:%s", strerror (errno));
            IEC104->is_connect = FALSE;
            event_del (&IEC104->ev_read);
            event_del (&IEC104->ev_timer_tframe);
            close (fd);
            free (IEC104);
            write_log (&log, 1, "on_write()", "free");
        }
    }
    else if (ret < IEC104->SendLen)
    {
        printf ("write err!\n");
        //event_add (&client->ev_write, NULL);
        //return;
    }

    IEC104->SendLen = 0;

    /* display the send frame */
    if (debug == TRUE)
    {
        Display (IEC104);
    }
}

static void on_write (void *arg)
{
    struct IEC104_SLAVE *IEC104 = (struct IEC104_SLAVE *) arg;
    int fd, ret;

    ST_C104_FRAME *p_frame = (ST_C104_FRAME*) &IEC104->SendBuf[0];
    ST_C104_ASDU *p_asdu = (ST_C104_ASDU*) &IEC104->SendBuf[6];
    fd = IEC104->fd;

    /* make frame */
    if (IEC104->SendLen > 0)
    {
        if (IEC104->SendLen > 6) //is I_frame
        {
            p_frame->c1 = IEC104->SendNum % 256;
            p_frame->c2 = IEC104->SendNum / 256;
            p_frame->c3 = IEC104->RecvNum % 256;
            p_frame->c4 = IEC104->RecvNum / 256;
            IEC104->SendNum += 2;
        }
        else if (IEC104->SendLen == 6)
        {
            if ( (p_frame->c1 & 0x03) == 0x01)  //is S_frame
            {
                wprintf ("/*********************** S_frame ****************************\n");
                wprintf ("RecvNum:%d\n\n", IEC104->RecvNum);
                p_frame->c3 = IEC104->RecvNum % 256;
                p_frame->c4 = IEC104->RecvNum / 256;
            }
        }
    }

    /* send the frame */
    ret = send (fd, IEC104->SendBuf, IEC104->SendLen, 0);
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
            //err (1, "write");
            wprintf ("write err: %s\n", strerror (errno));
            write_log (&log, -1, "on_write()", "write:%s", strerror (errno));
            IEC104->is_connect = FALSE;
            event_del (&IEC104->ev_read);
            event_del (&IEC104->ev_timer_tframe);
            close (fd);
            free (IEC104);
            write_log (&log, 1, "on_write()", "free");
        }
    }
    else if (ret < IEC104->SendLen)
    {
        printf ("write err!\n");
        //event_add (&client->ev_write, NULL);
        //return;
    }

    /* display the send frame */
    if (debug == TRUE)
    {
        Display (IEC104);
    }

    IEC104->SendLen = 0;
}

static void on_timer_tframe (int fd, short event, void *arg)
{
    struct IEC104_SLAVE  *IEC104 = (struct IEC104_SLAVE *) arg;
    if (IEC104->is_idlesse == TRUE)
    {
        /* send the test frame */
        IEC104SLAVE_TestAct (IEC104);
    }
    else
        IEC104->is_idlesse = TRUE;
    //event_add (&IEC104->ev_timer_tframe, &IEC104->tv_timer_tframe);
}

static void on_timer_sframe (int fd, short event, void *arg)
{
    struct IEC104_SLAVE  *IEC104 = (struct IEC104_SLAVE *) arg;
    //发送S帧
    IEC104SLAVE_Sframe (IEC104);
    //event_add (&IEC104->ev_timer_sframe, &IEC104->tv_timer_sframe);
}

static void load_config_from_file (void)
{
    int ret;
    char key_value[64];
    ret = ConfigGetKey (CONF_FILE, "iec104_slave", "port", key_value);
    if (ret == 0)
        PORT = (int) strtol (key_value, NULL, 10);
    else
    {
        ConfigSetKey (CONF_FILE, "iec104_slave", "port", "2404");
        PORT = 2404;
    }
    ret = ConfigGetKey (CONF_FILE, "iec104_slave", "max_k", key_value);
    if (ret == 0)
        MAX_K = (int) strtol (key_value, NULL, 10);
    else
    {
        ConfigSetKey (CONF_FILE, "iec104_slave", "max_k", "12");
        MAX_K = 12;
    }
    ret = ConfigGetKey (CONF_FILE, "iec104_slave", "max_w", key_value);
    if (ret == 0)
        MAX_W = (int) strtol (key_value, NULL, 10);
    else
    {
        ConfigSetKey (CONF_FILE, "iec104_slave", "max_w", "8");
        MAX_W = 8;
    }
    ret = ConfigGetKey (CONF_FILE, "iec104_slave", "t0", key_value);
    if (ret == 0)
        T0 = (int) strtol (key_value, NULL, 10);
    else
    {
        ConfigSetKey (CONF_FILE, "iec104_slave", "t0", "10");
        T0 = 10;
    }
    ret = ConfigGetKey (CONF_FILE, "iec104_slave", "t1", key_value);
    if (ret == 0)
        T1 = (int) strtol (key_value, NULL, 10);
    else
    {
        ConfigSetKey (CONF_FILE, "iec104_slave", "t1", "12");
        T1 = 12;
    }
    ret = ConfigGetKey (CONF_FILE, "iec104_slave", "t2", key_value);
    if (ret == 0)
        T2 = (int) strtol (key_value, NULL, 10);
    else
    {
        ConfigSetKey (CONF_FILE, "iec104_slave", "t2", "5");
        T2 = 5;
    }
    ret = ConfigGetKey (CONF_FILE, "iec104_slave", "t3", key_value);
    if (ret == 0)
        T3 = (int) strtol (key_value, NULL, 10);
    else
    {
        ConfigSetKey (CONF_FILE, "iec104_slave", "t3", "15");
        T3 = 15;
    }
    ret = ConfigGetKey (CONF_FILE, "iec104_slave", "asdu_addr", key_value);
    if (ret == 0)
        RTU_ADDR = (int) strtol (key_value, NULL, 10);
    else
    {
        ConfigSetKey (CONF_FILE, "iec104_slave", "asdu_addr", "1");
        RTU_ADDR = 1;
    }
    wprintf ("port:%d, max_k:%d, max_w:%d, t0:%d, t1:%d, t2:%d, t3:%d, asdu_addr:%d\n",
             PORT, MAX_K, MAX_W, T0, T1, T2, T3, RTU_ADDR);
}

/**
 * @brief    激活传输确认
 *
 * @param   i
 */
static void IEC104SLAVE_AssociateAck (struct IEC104_SLAVE *IEC104)
{
    unsigned char buf[] =
    {
        C104_APDU_SATRT, 0x04, 0x0b, 0x00, 0x00, 0x00
    };
    if (IEC104->is_connect == FALSE)
    {
        return;
    }
    IEC104->SendLen = sizeof (buf);
    memcpy (IEC104->SendBuf, buf, sizeof (buf));
    //event_add (&IEC104->ev_write, NULL);
    on_write (IEC104);
}

/**
 * @brief   初始化结束
 *
 * @param   IEC104
 */
static void IEC104SLAVE_M_EI_NA (struct IEC104_SLAVE *IEC104)
{
    unsigned char buf[] =
    {
        C104_APDU_SATRT, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x46, 0x01, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    if (IEC104->is_connect == FALSE)
    {
        return;
    }
    IEC104->SendLen = sizeof (buf);
    memcpy (IEC104->SendBuf, buf, sizeof (buf));
    //event_add (&IEC104->ev_write, NULL);
    IEC104->is_send_sframe = FALSE;
    on_write (IEC104);
}

/**
 * @brief    测试帧
 *
 * @param   i
 */
static void IEC104SLAVE_TestAct (struct IEC104_SLAVE *IEC104)
{
    unsigned char buf[] =
    {
        C104_APDU_SATRT, 0x04, 0x43, 0x00, 0x00, 0x00
    };
    if (IEC104->is_connect == FALSE)
    {
        return;
    }
    IEC104->SendLen = sizeof (buf);
    memcpy (IEC104->SendBuf, buf, sizeof (buf));
    //event_add (&IEC104->ev_write, NULL);
    on_write (IEC104);
}

/**
 * @brief U帧:测试确认
 */
static void IEC104SLAVE_TestAck (struct IEC104_SLAVE *IEC104)
{
    unsigned char buf[] =
    {
        C104_APDU_SATRT, 0x04, 0x83, 0x00, 0x00, 0x00
    };
    if (IEC104->is_connect == FALSE)
    {
        return;
    }
    IEC104->SendLen = sizeof (buf);
    memcpy (IEC104->SendBuf, buf, sizeof (buf));
    //event_add (&IEC104->ev_write, NULL);
    on_write (IEC104);
}

/**
 * @brief    S帧
 *
 * @param   i
 */
static void IEC104SLAVE_Sframe (struct IEC104_SLAVE *IEC104)
{
    unsigned char buf[] =
    {
        C104_APDU_SATRT, 0x04, 0x01, 0x00, 0x00, 0x00
    };
    if (IEC104->is_connect == FALSE)
    {
        return;
    }
    IEC104->SendLen = sizeof (buf);
    memcpy (IEC104->SendBuf, buf, sizeof (buf));
    //event_add (&IEC104->ev_write, NULL);
    on_write (IEC104);
}

/**
 * @brief 遥控预置确认
 */
void IEC104SLAVE_C_DC_NA_PreAck (struct IEC104_SLAVE *IEC104)
{
    if (IEC104->is_connect == FALSE)
    {
        return;
    }
    IEC104->SendLen = IEC104->RecvLen;
    memcpy (IEC104->SendBuf, IEC104->RecvBuf, IEC104->SendLen);
    IEC104->SendBuf[8] = 0x07;
    //event_add (&IEC104->ev_write, NULL);
    IEC104->is_send_sframe = FALSE;
    on_write (IEC104);
}

/**
 * @brief 遥控执行确认
 */
void IEC104SLAVE_C_DC_NA_ExeAck (struct IEC104_SLAVE *IEC104)
{
    if (IEC104->is_connect == FALSE)
    {
        return;
    }
    IEC104->SendLen = IEC104->RecvLen;
    memcpy (IEC104->SendBuf, IEC104->RecvBuf, IEC104->SendLen);
    IEC104->SendBuf[8] = 0x07;
    //event_add (&IEC104->ev_write, NULL);
    IEC104->is_send_sframe = FALSE;
    on_write (IEC104);
}

/**
 * @brief    遥控撤销确认
 *
 * @param   i
 * @param   co
 */
void IEC104SLAVE_C_DC_NA_TermAck (struct IEC104_SLAVE *IEC104)
{
    if (IEC104->is_connect == FALSE)
    {
        return;
    }
    IEC104->SendLen = IEC104->RecvLen;
    memcpy (IEC104->SendBuf, IEC104->RecvBuf, IEC104->SendLen);
    IEC104->SendBuf[8] = 0x09;
    //event_add (&IEC104->ev_write, NULL);
    IEC104->is_send_sframe = FALSE;
    on_write (IEC104);
}

void IEC104SLAVE_C_SE_NA_OkAck (struct IEC104_SLAVE *IEC104)
{
    if (IEC104->is_connect == FALSE)
    {
        return;
    }
    IEC104->SendLen = IEC104->RecvLen;
    memcpy (IEC104->SendBuf, IEC104->RecvBuf, IEC104->SendLen);
    IEC104->SendBuf[8] = 0x07;
    //event_add (&IEC104->ev_write, NULL);
    IEC104->is_send_sframe = FALSE;
    on_write (IEC104);
}

/**
 * @brief    一化设点命令 激活结束帧
 *
 * @param   IEC104
 */
void IEC104SLAVE_C_SE_NA_EndAck (struct IEC104_SLAVE *IEC104)
{
    if (IEC104->is_connect == FALSE)
    {
        return;
    }
    IEC104->SendLen = IEC104->RecvLen;
    memcpy (IEC104->SendBuf, IEC104->RecvBuf, IEC104->SendLen);
    IEC104->SendBuf[8] = 0x0A;
    //event_add (&IEC104->ev_write, NULL);
    IEC104->is_send_sframe = FALSE;
    on_write (IEC104);
}

void IEC104SLAVE_C_SE_NA_NoAck (struct IEC104_SLAVE *IEC104)
{
    if (IEC104->is_connect == FALSE)
    {
        return;
    }
    IEC104->SendLen = IEC104->RecvLen;
    memcpy (IEC104->SendBuf, IEC104->RecvBuf, IEC104->SendLen);
    IEC104->SendBuf[8] = 0x09;
    //event_add (&IEC104->ev_write, NULL);
    IEC104->is_send_sframe = FALSE;
    on_write (IEC104);
}

int IEC104SLAVE_Display (void *arg)
{
    int dis = (int) arg;
    if (dis == 1)
    {
        debug = TRUE;
    }
    else
    {
        debug = FALSE;
    }
}

int IEC104SLAVE_Exit (void *arg)
{
    wprintf ("IEC104_SLAVE will exit...!\n");
    //event_loopbreak ();
    event_base_loopbreak (IEC104SLAVE_event_base);
    wprintf ("IEC104_SLAVE will exited!\n");
}

int IEC104SLAVE_Run (void *arg)
{
    int ret, listen_fd;
    struct sockaddr_in listen_addr;

    /* The socket accept event. */
    struct event        ev_accept;
    struct event        ev_timer; //timer 时间对象
    struct timeval      tv_timer;
    
    printf ("IEC104 Slave run..[v0.1.1]\n");

    /* load config */
    load_config_from_file ();

    debug = TRUE;
    //debug = FALSE;
    log_init (&log, "/var/log/IEC104_SLAVE.log", "IEC104_SLAVE.....", "v0.1.0", "wang_chen_xi", "IEC104SLAVELOG");
#ifdef WRITE_RCV_LOG
    write_to_log (IEC104SLAVE_RCV_LOG, "[IEC104SLAVE send/rcv log]", LOG_NEW | LOG_TIME);
#endif

#ifndef TEST
    /* open kaichu fd  */
    fd_io = open ("/dev/ort3000c_io0", O_RDWR);
    if (fd_io < 0)
    {
        wprintf ("/dev/ort3000c_io0 can not open!\n");
        write_log (&log, -1, "IEC104SLAVE_Run()", "/dev/ort3000c_io open err!");
    }
#endif

#if 0
    /* modbus initialization */
    modbus_init_rtu (&mb_param, "/dev/ttySAC2", 9600, "none", 8, 1, 1);
    if (debug)
    {
        modbus_set_debug (&mb_param, TRUE);
    }
    if (modbus_connect (&mb_param) == -1)
    {
        write_log (&log, -1, "IEC104SLAVE_Run()", "modbus_init_rtu err!");
        return 1;
    }
    mb_param.slave = 21;
#endif

    /* Create our listening socket. */
    listen_fd = socket (AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        write_log (&log, -1, "IEC104SLAVE_Run()", "socket:%s", strerror (errno));
        err (1, "listen failed");
    }
    /* set reuse_addr */
    int reuseaddr_on = 1;
    if (setsockopt (listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, sizeof (reuseaddr_on)) == -1)
    {
        write_log (&log, -1, "IEC104SLAVE_Run()", "setsockopt:%s", strerror (errno));
        err (1, "setsockopt failed");
    }
    /* bind addr */
    memset (&listen_addr, 0, sizeof (listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons (PORT);
    if (bind (listen_fd, (struct sockaddr *) &listen_addr, sizeof (listen_addr)) < 0)
    {
        write_log (&log, -1, "IEC104SLAVE_Run()", "bind:%s", strerror (errno));
        err (1, "bind failed");
    }
    /* set server socker */
    if (listen (listen_fd, 5) < 0)
    {
        write_log (&log, -1, "IEC104SLAVE_Run()", "listen:%s", strerror (errno));
        err (1, "listen failed");
    }
    /* Set the socket to non-blocking */
    if (setnonblock (listen_fd) < 0)
    {
        write_log (&log, -1, "IEC104SLAVE_Run()", "setnonblock:%s", strerror (errno));
        err (1, "failed to set server socket to non-blocking");
    }

    /* Initialize libevet. */
    //IEC104SLAVE_event_base = event_init();
    //IEC104SLAVE_event_base = event_base_new_with_config (NULL);
    IEC104SLAVE_event_base = event_base_new ();
    if (IEC104SLAVE_event_base == NULL)
    {
        write_log (&log, -1, "IEC104SLAVE_Run()", "event_base_new_with_config");
        err (1, "event_base_new_with_config");
    }

    //ret = event_base_set (IEC104SLAVE_event_base, &ev_accept);
    //if (ret < 0)
    //{
    //write_log (&log, -1, "IEC104SLAVE_Run()", "event_base_set");
    //err (1, "event_base_set");
    //}

    /* set and add time event */
    tv_timer.tv_sec = 5;
    tv_timer.tv_usec = 0;
    //event_set (&ev_timer, -1, EV_PERSIST, on_timer, NULL);
    event_assign (&ev_timer, IEC104SLAVE_event_base, -1, EV_PERSIST, on_timer, NULL);
    event_add (&ev_timer, &tv_timer);

    /* set and add listening socket event */
    //event_set (&ev_accept, listen_fd, EV_READ | EV_PERSIST, on_accept, NULL);
    event_assign (&ev_accept, IEC104SLAVE_event_base, listen_fd, EV_READ | EV_PERSIST, on_accept, NULL);
    event_add (&ev_accept, NULL);

    /* start the libevent event loop.. */
    //event_dispatch();
    event_base_loop (IEC104SLAVE_event_base, 0);

    close (fd_io);
    return 0;
}
