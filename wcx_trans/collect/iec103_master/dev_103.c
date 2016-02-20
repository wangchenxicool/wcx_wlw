/*****************************************************************************
  __   _______   ______  __    ___    ____
 |  | |   ____| /      |/_ |  / _ \  |___ \
 |  | |  |__   |  ,----' | | | | | |   __) |
 |  | |   __|  |  |      | | | | | |  |__ <
 |  | |  |____ |  `----. | | | |_| |  ___) |
 |__| |_______| \______| |_|  \___/  |____/

*****************************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
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
#include "typedef.h"
#include "system.h"
#include "dev_mng.h"
#include "dev_103.h"
#include "inforec.h"
#include "INIFileOP.h"
#include "wcx_log.h"
#include "wcx_utils.h"

#define DEBUG

/* global variable */
static struct iec103_tag_table      m_com[COM_NUM];
static bool                         m_exit_flg;
static struct log_tp                m_error_log;
static struct log_tp                m_boot_log;
static WORD                         m_space_time;

/* conf file */
static bool     m_debug;
static bool     m_display_on;
static bool     m_step;
static bool     m_write_csv;
static char     m_tag_file[100];
static char     m_tag_file_optimized[100]; //优化点表文件
static char     m_csv_file_path[100]; //csv文件存放路径

//////////////////////////////////////////////////////////////////
/* 通讯资源 */
static  BYTE		 m_tx_buf[UART_PACKET_SIZE];
static  BYTE		 m_rx_buf[UART_PACKET_SIZE];

/* 装置数目 */
#define	 DEV_NUM	12
/* 103扫描序号sn */
volatile BYTE m_c103_sn = 0;

/* 103返回信息标识符rii */
volatile BYTE m_c103_rii = 0;

/* 103帧记数位fbc */
volatile BYTE m_c103_fcb[DEV_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

extern const T_C103_CODE_TABLE g_c103_code[];
extern const WORD	g_c103_code_size;

#ifdef DEBUG
#define wprintf(format, arg...)  \
        printf (format , ##arg)
#else
#define wprintf(format, arg...) {}
#endif

/* 接收中断回调函数，用以判断数据帧的结束 */
static WORD c103_check_packet (const BYTE *p_data, WORD data_len)
{
    if ( (data_len == 1) && ( (p_data[0] == 0x68) || (p_data[0] == 0x10)))
    {
        return UART_PKT_CON;
    }
    if (data_len < 5)
    {
        return UART_PKT_CON;
    }
    if (p_data[0] == 0x10 && data_len == 5 && p_data[4] == 0x16)
    {
        return UART_PKT_FIN;
    }
    else if (p_data[0] == 0x68 && data_len == p_data[1] + 6 && p_data[data_len - 1] == 0x16)
    {
        return UART_PKT_FIN;
    }
    return UART_PKT_CON;
}

/* 校验和 */
BYTE  calc_check_sum (const BYTE *p_data, WORD data_len)
{
    WORD  i;
    BYTE  sum = 0;

    for (i = 0; i < data_len; i++)
    {
        sum += p_data[i];
    }
    return sum;
}

/* 串口管理 */
WORD c103_serial_com (int sn, iec103_tag_tp* p_tag, T_C103_FRAME* p_frame, BOOL brx)
{
    int ret = 0;
    WORD wSize, i;

    p_tag->fcb ^= 0x20;	//帧记数位取反

    switch (p_frame->type)
    {
    case FRAME_T_CON:
        /* 固定帧长报文 */
        m_com[sn].tx_buf[0] = p_frame->type;
        m_com[sn].tx_buf[1] = p_frame->contral | p_tag->fcb;
        m_com[sn].tx_buf[2] = p_frame->address;
        m_com[sn].tx_buf[3] = calc_check_sum (&m_com[sn].tx_buf[1], 2);
        m_com[sn].tx_buf[4] = p_frame->endbyte;
        wSize = 5;
        break;
    case FRAME_T_VOL:
        /* 可变帧长报文 */
        m_com[sn].tx_buf[0] = p_frame->type;
        m_com[sn].tx_buf[1] = p_frame->len;
        m_com[sn].tx_buf[2] = p_frame->len;
        m_com[sn].tx_buf[3] = p_frame->type;
        m_com[sn].tx_buf[4] = p_frame->contral | p_tag->fcb;
        m_com[sn].tx_buf[5] = p_frame->address;
        m_com[sn].tx_buf[4 + p_frame->len] = calc_check_sum (&m_com[sn].tx_buf[4], p_frame->len);
        m_com[sn].tx_buf[5 + p_frame->len] = p_frame->endbyte;
        wSize = p_frame->len + 6;
        break;
    default:
        break;
    }

    /* 出现差错重传三次 */
    for (i = 0; i < 3; i++)
    {
        iec103_send (&m_com[sn].com, m_com[sn].tx_buf, wSize);
        if (!brx)
            break;
        ret = iec103_receive (&m_com[sn].com, m_com[sn].rx_buf, m_com[sn].com_time_out);
        if (ret != 0)
            break;
        MySleep (0, 50);
    }

    /* 接收报文分类 */
    if (ret > 0)
    {
        //NU_Sleep (20);
        p_tag->dev_flag = ONLINE;
        //wprintf ("\033[32;40;1m \nONLINE\n\033[0m");
        switch (m_com[sn].rx_buf[0])
        {
        case FRAME_T_CON:	//固定帧长
            p_frame->type = FRAME_T_CON;
            p_frame->contral = m_com[sn].rx_buf[1];
            p_frame->address = m_com[sn].rx_buf[2];
            p_frame->crc = m_com[sn].rx_buf[3];
            if (p_frame->crc == calc_check_sum (&m_com[sn].rx_buf[1], 2)
                    && p_tag->device_addr == p_frame->address) 	//校验
            {
                memset (&m_com[sn].rx_buf[5], 0, (UART_PACKET_SIZE - 5));
                return ret;
            }
            break;
        case FRAME_T_VOL:	//可变帧长
            p_frame->type = FRAME_T_VOL;
            p_frame->len = m_com[sn].rx_buf[1];
            p_frame->contral = m_com[sn].rx_buf[4];
            p_frame->address = m_com[sn].rx_buf[5];
            p_frame->crc = m_com[sn].rx_buf[p_frame->len + 4];
            if (p_frame->crc == calc_check_sum (&m_com[sn].rx_buf[4], p_frame->len)
                    && p_tag->device_addr == p_frame->address)	//校验
            {
                return ret;
            }
            break;
        }
    }
    else
    {
        wprintf ("\033[31;40;1m \nOFFLINE\n\033[0m");
        p_tag->dev_flag = OFFLINE;
    }

#if 0
    if (brx && !ret && p_frame->contral != 0x40)	//超时无应答发送复位(复位无应答返回)
    {
        p_tag->dev_flag = OFFLINE;	//连接异常
        //c103_device_initialize(p_tag);
    }
#endif

    return 0;
}

/********************************************************/
/*														*/
/*	103 IED侧协议解释器									*/
/*														*/
/********************************************************/
/* request the first data */
BOOL c103_request_first (int sn, iec103_tag_tp* p_tag)
{
    T_DATE		t_date;
    T_EVENT		t_event;
    T_ALARM		t_alarm;
    T_LIST		t_list;
    T_DIC		t_dic;
    T_SOE		t_soe;
    T_DI		t_di, t_di_2;
    T_C103_FRAME t_frame;
    T_C103_ASDU	*p_asdu;
    WORD		di_lo;
    WORD		di_hi;
    BYTE		di_grp;
    BYTE		i;

    printf ("^_^c103_request_first(召唤用户1 级数据)..\n");
    /* make frames */
    t_frame.type = FRAME_T_CON;
    t_frame.contral = 0x5a;
    t_frame.address = p_tag->device_addr;
    t_frame.len = 3;
    t_frame.endbyte = 0x16;

    /* send then receive frames */
    if (!c103_serial_com (sn, p_tag, &t_frame, 1))
    {
        return false;
    }
    //NU_Sleep (50);

    /* analyse frames */
    p_asdu = (T_C103_ASDU *) &m_com[sn].rx_buf[6];
    switch (p_asdu->type)
    {
    case ASDU5:
    {
        if (p_asdu->cot == 4 || p_asdu->cot == 5 || p_asdu->cot == 3)	//复位通信单元
            break;
        return	false;
    }
    case ASDU8:
    {
        if (p_asdu->cot != 9 || p_asdu->inf != 0)	//总查询结束
            break;
        return	false;
    }
    case ASDU2: //动作事件
    {
#if 0
        if (p_asdu->cot != 1)
            return false;
        /* get system date */
        //SYS_Get_Clock (&t_date);
        t_event.dev_id = p_tag->dev_id;
        t_event.e_state = p_asdu->data[0] - 1;
        t_event.e_date.msec = MAKEWORD (p_asdu->data[5], p_asdu->data[6]);
        t_event.e_date.minute = p_asdu->data[7];
        t_event.e_date.hour = p_asdu->data[8];
        t_event.e_date.day = t_date.day;
        t_event.e_date.month = t_date.month;
        t_event.e_date.year = t_date.year;
        t_list.l_type = LIST_T_EVENT;
        //t_event.e_code = C103_Get_Code (p_tag, &t_list, p_asdu->inf);
        /* write db */
        //INF_Record_Event (&t_event);
#endif
    }
    break;
    case ASDU1://自检信息
    {
        if (p_asdu->cot != 1)
            return false;
        else
        {
            /* gets time of day */
            time_t timer = time (NULL);
            /* converts date/time to a structure */
            struct tm *tm_time = localtime (&timer);

            t_alarm.dev_id = p_tag->tag_id;
            t_alarm.e_state = p_asdu->data[0] - 1;
            t_alarm.e_date.msec = MAKEWORD (p_asdu->data[1], p_asdu->data[2]);
            t_alarm.e_date.minute = p_asdu->data[3];
            t_alarm.e_date.hour = p_asdu->data[4];
            t_alarm.e_date.day = tm_time->tm_mday;
            t_alarm.e_date.month = tm_time->tm_mon + 1;
            t_alarm.e_date.year = tm_time->tm_year + 1900;

            /* print */
            wprintf ("analyse:\n");
            wprintf ("%d.%d.%d-%d:%d:%d, state:%d, device_addr:%d, fun:%d, inf:%d\n",
                     t_alarm.e_date.year, t_alarm.e_date.month, t_alarm.e_date.day, t_alarm.e_date.hour, t_alarm.e_date.minute,
                     t_alarm.e_date.msec / 1000, t_alarm.e_state, p_tag->device_addr, p_asdu->fun, p_asdu->inf);
            /* write db */
        }
    }
    break;
    case ASDU10:
    {
        if (p_asdu->cot != 42 || p_asdu->fun != 254 || p_asdu->inf != 241)
            return false;
        return true;
    }
    break;
    case ASDU44: //上送全遥信
    {
        if (p_asdu->cot != 9)
            return FALSE;
#if 0
        for (i = 0; i < p_asdu->fun; i++)
        {
            t_di.di_num = p_asdu->vsq * 16 + (p_asdu->fun - 1) * 48;
            di_grp = (t_di.di_num + 31) / 32;
            for (i = 0; i < di_grp; i++)
            {
                di_lo = MAKEWORD (p_asdu->data[10 * i + 0], p_asdu->data[10 * i + 1]);
                di_hi = MAKEWORD (p_asdu->data[10 * i + 5], p_asdu->data[10 * i + 6]);
            }

        }
#endif
        if (p_asdu->fun != 1)
            return FALSE;
        t_di.di_num = p_asdu->vsq * 16;
        di_grp = (t_di.di_num + 31) / 32;
        for (i = 0; i < di_grp; i++)
        {
            di_lo = MAKEWORD (p_asdu->data[10 * i + 0], p_asdu->data[10 * i + 1]);
            di_hi = MAKEWORD (p_asdu->data[10 * i + 5], p_asdu->data[10 * i + 6]);
            t_di.di_val[i] = MAKEDWORD (di_lo, di_hi);
        }
        //INF_Set_Di (p_tag->dev_id, &t_di);
    }
    break;
    case ASDU40: //变位遥信
    {
#if 0
        if (p_asdu->cot != 1)
            return FALSE;
        if (p_asdu->fun == 2)
        {
            t_dic.e_code = p_asdu->inf - DI_FIRST_INF + 42;
        }
        else if (p_asdu->fun == 1)
        {
            t_dic.e_code = p_asdu->inf - DI_FIRST_INF; //超过106个遥信需要计算FUN
        }
        else
            return FALSE;
        t_dic.dev_id = p_tag->dev_id;
        t_dic.e_state = p_asdu->data[0];
        //INF_Record_Dic (&t_dic);
#endif
    }
    break;
    case ASDU41: //SOE
    {
#if 0
        if (p_asdu->cot != 1)
            return FALSE;
        //SYS_Get_Clock (&t_date);
        t_soe.dev_id = p_tag->dev_id;
        if (p_asdu->fun == 2)
        {
            t_soe.e_code = p_asdu->inf - DI_FIRST_INF + 42;
        }
        else if (p_asdu->fun == 1)
        {
            t_soe.e_code = p_asdu->inf - DI_FIRST_INF;
        }

        t_soe.e_state = p_asdu->data[0];
        t_soe.e_date.msec  = MAKEWORD (p_asdu->data[1], p_asdu->data[2]);
        t_soe.e_date.minute = p_asdu->data[3];
        t_soe.e_date.hour  = p_asdu->data[4];
        t_soe.e_date.day   = t_date.day;
        t_soe.e_date.month = t_date.month;
        t_soe.e_date.year  = t_date.year;
        //INF_Record_Soe (&t_soe);
#endif
    }
    break;
    case ASDU64:
    {
        if (p_asdu->cot != 0x0c)
            return FALSE;
        return true;
    }
    break;
    default:
        return FALSE;
    } /* end switch */

    if ( (t_frame.contral & 0x20) != 0)		// check is have first data?
    {
        return c103_request_first (sn, p_tag);
    }
    if (t_frame.contral == 0x08 || t_frame.contral == 0x09)
    {
        return true;
    }

    return false;
}

/* request the second data */
BOOL c103_request_second (int sn, iec103_tag_tp* p_tag)
{
    T_C103_FRAME	t_frame;
    T_C103_ASDU		*p_asdu;
    T_MEASURE		mea;
    BYTE			i;

    printf ("^_^request the second data(召唤用户2 级数据)..\n");

    /* make frames */
    t_frame.type = FRAME_T_CON;
    t_frame.contral = 0x5b;     // call second data
    t_frame.address = p_tag->device_addr;
    t_frame.len = 3;
    t_frame.endbyte = 0x16;

    /* send then receive frames */
    if (!c103_serial_com (sn, p_tag, &t_frame, 1))
    {
        return false;
    }

    /* analyse frames */
    if (t_frame.type == FRAME_T_CON)
    {
        if ( (t_frame.contral & 0x20) == 0)	// no first data
            return true;
        else                                // has first data
            return c103_request_first (sn, p_tag);
    }
    else // is telemetering
    {
        p_asdu = (T_C103_ASDU *) &m_rx_buf[6];
        if (p_asdu->type != ASDU50 || p_asdu->cot != 0x02)
            return FALSE;
        mea.ms_num = p_asdu->vsq;
        for (i = 0; i < mea.ms_num; i++)
        {
            if (p_asdu->data[2 * i + 1] & 0x80) //负数
            {
                mea.ms_val[i] = (0 - MAKEWORD (p_asdu->data[2 * i] & 0xf8, (p_asdu->data[2 * i + 1])))
                                / 8 & INF_MS_VALUE_MASK | INF_MS_NEGATIVE;
            }
            else
                mea.ms_val[i] = (MAKEWORD (p_asdu->data[2 * i] & 0xf8, (p_asdu->data[2 * i + 1]))) / 8 & INF_MS_VALUE_MASK;
        }
        /* write db */
        //INF_Set_Measure (p_tag->dev_id, &mea);
    }
}

/* 主站ASDU21读一个(组/条目)的属性/值 */
/*	p_ied:	设备标识
	ginh:	条目号(0:全部条目)
	ginl:	组号  0 描述 1 参数，2区号 3 保护定值 6 阮压板 9 模拟量
	kod:	数据类型
*/
static BOOL c103_generic_read (T_IED *p_ied, BYTE ginl, BYTE ginh, BYTE kod)
{
#if 0
    T_C103_FRAME t_frame;
    T_C103_ASDU* p_asdu;

    /* 构建ASDU21通用分类读命令 */
    p_asdu = (T_C103_ASDU *) &m_tx_buf[6];
    p_asdu->type = 21;
    p_asdu->vsq = 0x81;
    p_asdu->cot = 42;
    p_asdu->addr = (BYTE) p_ied->dev_data[0];
    p_asdu->fun = 254;
    p_asdu->inf = 241;
    p_asdu->data[0] = m_c103_rii++;
    p_asdu->data[1] = 1;					//NGD
    p_asdu->data[2] = ginl;
    p_asdu->data[3] = ginh;
    p_asdu->data[4] = kod;

    t_frame.type = FRAME_T_VOL;
    t_frame.contral = 0x53;
    t_frame.address = (BYTE) p_ied->dev_data[0];
    t_frame.len = 13;
    t_frame.endbyte = 0x16;

    if (c103_serial_com (p_ied, &t_frame, 1))
    {
        //	if (t_frame.type ==0x10 && (t_frame.contral &0x20)!=0
        //		&& t_frame.address ==p_ied->dev_data[0])	//注意通用分类读后必须请求一级数据
        if (t_frame.type == 0x10 && t_frame.address == p_ied->dev_data[0])
        {
            if (c103_request_first (p_ied))
                return true;
        }
    }
    else
        return false;
#endif
}

/* 主站ASDU10带确认写一个(组/条目)的值 */
/*	p_ied:	设备标识
	ngd:	通用数据数目
*/
static BOOL c103_generic_ackwrite (T_IED *p_ied, BYTE ngd, BYTE len)
{
#if 0
    T_C103_FRAME t_frame;
    T_C103_ASDU* p_asdu;

    /* 加入ASDU10通用分类数据写报文头 */
    p_asdu = (T_C103_ASDU *) &m_tx_buf[6];
    p_asdu->type = 10;
    p_asdu->vsq = 0x81;
    p_asdu->cot = 40;
    p_asdu->addr = (BYTE) p_ied->dev_data[0];
    p_asdu->fun = 254;
    p_asdu->inf = 249;
    p_asdu->data[0] = m_c103_rii++;
    p_asdu->data[1] = ngd;

    /* 帧格式 */
    t_frame.type = FRAME_T_VOL;
    t_frame.contral = 0x53;
    t_frame.address = (BYTE) p_ied->dev_data[0];
    t_frame.len = len;
    t_frame.endbyte = 0x16;

    if (c103_serial_com (p_ied, &t_frame, 1))
    {
        if (t_frame.type == 0x10 && t_frame.contral == 0x21
                && t_frame.address == p_ied->dev_data[0])		//接收数据识别
        {
            c103_request_first (p_ied);
        }
        if (t_frame.type == 0x68 && t_frame.contral == 0
                && t_frame.address == p_ied->dev_data[0])
        {
            p_asdu = (T_C103_ASDU *) &m_rx_buf[6];
            /* 校验ASDU信息 */
            if ( (p_asdu->cot != 44) || (p_asdu->fun != 254) || (p_asdu->inf != 249))
            {
                return false;
            }
            return true;
        }
    }
    else
        return false;
#endif
}

/* 主站ASDU10带执行写一个(组/条目)的值 */
/*	p_ied:	设备标识
	ngd:	通用数据数目
*/
static BOOL c103_generic_exewrite (T_IED *p_ied, BYTE ngd, BYTE len)
{
#if 0
    T_C103_FRAME t_frame;
    T_C103_ASDU* p_asdu;

    /* 加入ASDU10通用分类数据写报文头 */
    p_asdu = (T_C103_ASDU *) &m_tx_buf[6];
    p_asdu->type = 10;
    p_asdu->vsq = 0x81;
    p_asdu->cot = 40;
    p_asdu->addr = (BYTE) p_ied->dev_data[0];
    p_asdu->fun = 254;
    p_asdu->inf = 250;
    p_asdu->data[0] = m_c103_rii++;
    p_asdu->data[1] = ngd;


    /* 帧格式 */
    t_frame.type = FRAME_T_VOL;
    t_frame.contral = 0x53;
    t_frame.address = (BYTE) p_ied->dev_data[0];
    t_frame.len = len;
    t_frame.endbyte = 0x16;

    if (c103_serial_com (p_ied, &t_frame, 1))
    {
        if (t_frame.type == 0x10 && t_frame.contral == 0x21
                && t_frame.address == p_ied->dev_data[0])
        {
            c103_request_first (p_ied);
        }
        if (t_frame.type == 0x68 && t_frame.contral == 0
                && t_frame.address == p_ied->dev_data[0])
        {
            p_asdu = (T_C103_ASDU *) m_rx_buf[6];
            /* 校验ASDU信息 */
            if ( (p_asdu->cot != 40) || (p_asdu->fun != 254) || (p_asdu->inf != 250))
            {
                return false;
            }
            return true;
        }
    }
    else
        return false;
#endif
}

/********************************************************/
/*														*/
/*	103设备服务接口及内部接口定义						*/
/*														*/
/********************************************************/
/* 保护采样通道值 */
BOOL c103_get_analog (T_IED *p_ied, T_CHANNEL *p_chn)
{
    T_C103_ASDU *p_asdu;
    BYTE		i;
    T_C103_DATA *p_rec;

    // 解析数据
    p_asdu = (T_C103_ASDU *) &m_tx_buf[6];

    if (!c103_generic_read (p_ied, 0x09, 0x00, 0x01))
    {
        return FALSE;
    }

    p_asdu = (T_C103_ASDU *) &m_rx_buf[6];

    p_chn->chn_num = p_asdu->data[1];
    if (p_chn->chn_num > SYS_MAX_CHANNEL)
    {
        p_chn->chn_num = SYS_MAX_CHANNEL;
    }
    p_rec = (T_C103_DATA *) &p_asdu->data[2];

    for (i = 0; i < p_chn->chn_num; i++)
    {
        if ( (p_rec->ginh != 0x02) || (p_rec->kod != 0x01) || (p_rec->num != 1))
        {
            return FALSE;
        }

        if (p_rec->type == 0x40)
        {
            float f_val;
            f_val = MAKEWORD (p_rec->data[0], p_rec->data[1]) / 100.0;
            p_chn->chn_val[i] = f_val;
        }
        else
        {
            p_chn->chn_val[i] = (float) p_rec->data[0];
        }
        p_rec = (T_C103_DATA *) ( (BYTE *) p_rec + 6 + p_rec->num * p_rec->size);
    }
    return TRUE;
}

/* 选择当前操作定值区 */
static BOOL c103_select_zone (T_IED *p_ied, BYTE set_no)
{
    BYTE len;
    T_C103_ASDU *p_asdu;
    T_C103_DATA *p_rec;
    BYTE		ngd = 1;

    p_asdu = (T_C103_ASDU *) &m_tx_buf[6];

    p_rec = (T_C103_DATA *) &p_asdu->data[2];
    p_rec->ginh	= 0;
    p_rec->ginl	= 2;
    p_rec->kod	= 1;
    p_rec->type	= 3;
    p_rec->size	= 1;
    p_rec->num	= 1;
    p_rec->data[0] = set_no;

    len = 17;

    if (!c103_generic_exewrite (p_ied, ngd, len))
    {
        return false;
    }

    p_asdu = (T_C103_ASDU *) &m_rx_buf[6];
    p_rec  = (T_C103_DATA *) &p_asdu->data[2];

    if ( (p_rec->ginh != 0) || (p_rec->ginl != 2) || (p_rec->kod != 1))
    {
        return false;
    }
    return (p_rec->data[0] == set_no);
}

/* 定值类服务 */
BOOL c103_get_setting (T_IED *p_ied, T_SET *p_set)
{
    T_C103_ASDU* p_asdu;
    T_C103_DATA* p_rec;
    BYTE		i, j;
    BYTE		set_num;
    BYTE		flag_con;

    /* 选择当前操作定值区 */
//	if ( !c103_select_zone(p_ied, (BYTE)(p_set->set_no)) )
//	{
//		return FALSE;
//	}

    p_asdu = (T_C103_ASDU *) &m_tx_buf[6];

    if (!c103_generic_read (p_ied, 0x03, 0x00, 0x01))
    {
        return FALSE;
    }

    /* 解析数据 */
    p_set->set_num = 0;
    do
    {
        p_asdu = (T_C103_ASDU *) &m_rx_buf[6];
        set_num = p_asdu->data[1] & 0x3f;
        p_set->set_num += set_num;
        flag_con = p_asdu->data[1] & 0x80;
        if (p_set->set_num > SYS_MAX_SET)
        {
            p_set->set_num = SYS_MAX_SET;
            return false;
        }
        p_rec = (T_C103_DATA *) &p_asdu->data[2];

        for (i = 0; i < set_num; i++)
        {
            if ( (p_rec->ginh != 0x01) && (p_rec->kod != 0x01)  || (p_rec->num != 1))
            {
                return false;
            }

            j = p_rec->ginl;
            if (p_rec->type == 0x07)
            {
                float f_val;		//定值格式转换
                //gen_scan_float (p_rec->data, &f_val);
                p_set->set_val[j].type = SET_T_FLOAT;
                p_set->set_val[j].un_val.f_val = f_val;
            }
            else
            {
                p_set->set_val[j].type = SET_T_UINT;
                p_set->set_val[j].un_val.u_val = p_rec->data[0];
            }
            p_rec = (T_C103_DATA *) ( (BYTE *) p_rec + 6 + p_rec->num * p_rec->size);
        }

        if (flag_con != 0)
        {
            ;//c103_request_first (p_ied);
        }
    }
    while (flag_con != 0);
    return true;
}

BOOL c103_get_zone (T_IED *p_ied, WORD *p_set_no)
{
    T_C103_ASDU *p_asdu;
    T_C103_DATA *p_rec;

    p_asdu = (T_C103_ASDU *) &m_tx_buf[6];

    if (!c103_generic_read (p_ied, 0x02, 0x01, 0x01))
    {
        return false;
    }

    p_asdu = (T_C103_ASDU *) &m_rx_buf[6];
    p_rec  = (T_C103_DATA *) &p_asdu->data[2];

    if ( (p_rec->ginh != 0x00) || (p_rec->ginl != 0x03) || (p_rec->kod != 0x01))
    {
        return false;
    }

    *p_set_no = p_rec->data[0];

    return true;
}

/* 软压板类服务 */
BOOL c103_get_sfc (T_IED *p_ied)
{
    T_C103_ASDU* p_asdu;
    T_C103_DATA* p_rec;
    T_SFC t_sfc;
    BYTE		i;

    p_asdu = (T_C103_ASDU *) &m_tx_buf[6];

    if (!c103_generic_read (p_ied, 0x06, 0x00, 0x01))
    {
        return FALSE;
    }

    p_asdu = (T_C103_ASDU *) &m_rx_buf[6];
    /* 解析数据 */
    t_sfc.sfc_num = p_asdu->data[1];
    if (t_sfc.sfc_num > SYS_MAX_SFC)
    {
        t_sfc.sfc_num = SYS_MAX_SFC;
    }
    p_rec = (T_C103_DATA *) &p_asdu->data[2];

    for (i = 1; i <= t_sfc.sfc_num; i++)
    {
        if ( (p_rec->ginh != 0x03) || (p_rec->kod != 0x01) || (p_rec->num != 1))
        {
            return FALSE;
        }
        if ( (p_rec->data[0] / 0x32) == 0x01)
        {
            //INF_Preset_SFC (&t_sfc, i, INF_S_ON);
        }
        else if ( (p_rec->data[0] / 0x32) == 0x02)
        {
            //INF_Preset_SFC (&t_sfc, i, INF_S_OFF);
        }
        p_rec = (T_C103_DATA *) ( (BYTE *) p_rec + 6 + p_rec->num * p_rec->size);
    }

    //INF_Set_SFC (p_ied->dev_id, &t_sfc);
    return TRUE;
}

/* 遥控类服务[最小遥控点号 =1] */
BOOL c103_check_control (int sn, iec103_tag_tp *p_tag, WORD ctrl_no)
{
    T_C103_ASDU	*p_asdu;
    T_C103_FRAME t_frame;
    BYTE ginl = (ctrl_no + 1) / 2;
    BYTE dpi  = ( (ctrl_no % 2) != 0) ? 0x01 : 0x02;

    p_asdu = (T_C103_ASDU *) &m_com[sn].tx_buf[6];

    p_asdu->type = ASDU64;
    p_asdu->vsq	 = 0x01;
    p_asdu->cot	 = 0x0c;
    p_asdu->addr = p_tag->device_addr;
    p_asdu->fun	 = CTRL_FIRST_FUN;
    p_asdu->inf	 = CTRL_FIRST_INF + ginl;
    p_asdu->data[0] = dpi | 0x80;
    p_asdu->data[1] = m_c103_rii++;

    t_frame.type = FRAME_T_VOL;
    t_frame.contral = 0x53;		//发送/确认
    t_frame.address = p_tag->device_addr;
    t_frame.len = 10;
    t_frame.endbyte = 0x16;

    if (c103_serial_com (sn, p_tag, &t_frame, 1))
    {
        if (t_frame.type == 0x10 && t_frame.address == p_tag->device_addr && (t_frame.contral & 0x20) != 0)
        {
            if (c103_request_first (sn, p_tag))

            {
                p_asdu = (T_C103_ASDU *) &m_com[sn].rx_buf[6];
                if (p_asdu->type == ASDU64 && p_asdu->cot == 0x0c && p_asdu->data[0] == (dpi | 0x80))
                    return TRUE;
                else
                    return FALSE;
            }
            else
                return FALSE;
        }
        else
        {
            if (c103_request_second (sn, p_tag))
            {
                p_asdu = (T_C103_ASDU *) &m_com[sn].rx_buf[6];
                if (p_asdu->type == ASDU64 && p_asdu->cot == 0x0c && p_asdu->data[0] == (dpi | 0x80))
                    return TRUE;
                else
                    return FALSE;
            }
            else
                return FALSE;
        }
    }
    else
        return FALSE;
}

BOOL c103_remote_control (int sn, iec103_tag_tp *p_tag, WORD ctrl_no)
{
    T_C103_FRAME t_frame;
    T_C103_ASDU	*p_asdu;
    BYTE ginl = (ctrl_no + 1) / 2;
    BYTE dpi = ( (ctrl_no % 2) != 0) ? 0x01 : 0x02;

    p_asdu = (T_C103_ASDU *) &m_com[sn].tx_buf[6];

    /* make frames */
    p_asdu->type = ASDU64;
    p_asdu->vsq	 = 0x01;
    p_asdu->cot	 = 0x0c;
    p_asdu->addr = p_tag->device_addr;
    p_asdu->fun	 = CTRL_FIRST_FUN;
    p_asdu->inf	 = CTRL_FIRST_INF + ginl;
    p_asdu->data[0] = dpi;
    p_asdu->data[1] = m_c103_rii++;

    t_frame.type = FRAME_T_VOL;
    t_frame.contral = 0x53;		//发送/确认
    t_frame.address = p_tag->device_addr;
    t_frame.len = 10;
    t_frame.endbyte = 0x16;

    if (c103_serial_com (sn, p_tag, &t_frame, 1))
    {
        if (t_frame.type == 0x10 && t_frame.address == p_tag->device_addr)
        {
            if (c103_request_first (sn, p_tag))
            {
                p_asdu = (T_C103_ASDU *) &m_rx_buf[6];
                if (p_asdu->type == ASDU64 && p_asdu->cot == 0x0c && p_asdu->data[0] == dpi)
                    return TRUE;
                else
                    return FALSE;
            }
            else
                return FALSE;
        }
    }
    else
        return FALSE;
}

/* 信号复归 */
BOOL c103_reset_signal (T_IED *p_ied)
{
#if 0
    T_C103_ASDU* p_asdu;
    T_C103_FRAME t_frame;

    /* 执行信号复归 */
    p_asdu = (T_C103_ASDU *) &m_tx_buf[6];
    p_asdu->type = 20;
    p_asdu->vsq	= 0x81;
    p_asdu->cot	= 20;
    p_asdu->addr = (BYTE) p_ied->dev_data[0];
    p_asdu->fun	= (BYTE) p_ied->dev_type;
    p_asdu->inf	= 19;
    p_asdu->data[0] = 0x01;
    p_asdu->data[1] = m_c103_rii++;

    t_frame.type = FRAME_T_VOL;
    t_frame.contral = 0x53;		//发送/确认
    t_frame.address = (BYTE) p_ied->dev_data[0];
    t_frame.len = 10;
    t_frame.endbyte = 0x16;

    if (c103_serial_com (p_ied, &t_frame, 1))
    {
        if (t_frame.type == 0x10 && (t_frame.contral & 0xF0) == 0x20 && t_frame.address == p_ied->dev_data[0])
            c103_request_first (p_ied);
        p_asdu = (T_C103_ASDU *) &m_rx_buf[6];
        if ( (t_frame.contral & 0x0F) == 0 && p_asdu->cot == 20 && p_asdu->inf == 19)
            return true;
    }
    else
        return false;
#endif
}

/* 时间服务 */
BOOL c103_set_clock (T_IED *p_ied, const T_DATE *p_date)
{
#if 0
    T_C103_ASDU* p_asdu;
    T_C103_FRAME t_frame;

    /* 时间同步报文 */
    p_asdu = (T_C103_ASDU *) &m_tx_buf[6];
    p_asdu->type = 6;
    p_asdu->vsq  = 0x81;
    p_asdu->cot  = 8;
    p_asdu->addr = 0xFF;
    p_asdu->fun  = 255;
    p_asdu->inf  = 0;
    p_asdu->data[0] = LOBYTE (p_date->msec);
    p_asdu->data[1] = HIBYTE (p_date->msec);
    p_asdu->data[2] = p_date->minute;
    p_asdu->data[3] = p_date->hour;
    p_asdu->data[4] = ( (p_date->week << 5) & 0xE0) | (p_date->day & 0x1F);
    p_asdu->data[5] = p_date->month;
    p_asdu->data[6] = p_date->year - 2000;

    t_frame.type = FRAME_T_VOL;
    t_frame.contral = 0x44;		//发送/无回答
    t_frame.address = 0xFF;
    t_frame.len = 15;
    t_frame.endbyte = 0x16;

    m_c103_fcb[p_ied->dev_id] = 0x20;

    c103_serial_com (p_ied, &t_frame, 0);

    //SYS_Set_Clock (p_date);

    return TRUE;
#endif
}

/* 设备初始化 */
static BOOL c103_open_ied (T_IED *p_ied)
{
    return true;
}

/* 设备描述表 */
BOOL c103_get_List (T_IED *p_ied, T_LIST* p_list)
{
    int i;
    for (i = 0; i < g_c103_code_size; i++)
    {
        if (p_ied->user_defined_1 == g_c103_code[i].dev_type)
        {
            switch (p_list->l_type)
            {
            case LIST_T_SET:
            {
                p_list->l_ptr = g_c103_code[i].p_setting;
                p_list->l_size = g_c103_code[i].w_setting;
                return true;
            }
            case LIST_T_EVENT:
            {
                p_list->l_ptr = g_c103_code[i].p_event;
                p_list->l_size = g_c103_code[i].w_even;
                return true;
            }
            case LIST_T_ALARM:
            {
                p_list->l_ptr = g_c103_code[i].p_alarm;
                p_list->l_size = g_c103_code[i].w_alarm;
                return true;
            }
            return false;
            }
        }
    }
    return false;
}

/* 取103条目号 */
static WORD C103_Get_Code (T_IED* p_ied, T_LIST* p_list, WORD code)
{
    if (c103_get_List (p_ied, p_list))
    {
        switch (p_list->l_type)
        {
        case  LIST_T_EVENT:
        {
            int i;
            T_EVENT_ENTRY* p_event = (T_EVENT_ENTRY*) p_list->l_ptr;
            for (i = 0; i < p_list->l_size; i++)
            {
                if (p_event[i].e_key == code)
                    return i + 1;
            }
        }
        case LIST_T_ALARM:
        {
            int i;
            T_ALARM_ENTRY* p_alarm = (T_ALARM_ENTRY*) p_list->l_ptr;
            for (i = 0; i < p_list->l_size; i++)
            {
                if (p_alarm[i].e_key  == code)
                    return i + 1;
            }
        }
        case LIST_T_SET:
        {
            int i;
            T_SET_ENTRY* p_set = (T_SET_ENTRY*) p_list->l_ptr;
            for (i = 0; i < p_list->l_size; i++)
            {
                if (p_set[i].e_key  == code)
                    return i + 1;
            }
        }
        break;
        default:
            break;
        } /* end switch */
    }
    return 0;
}

/* 103设备服务接口 */
BOOL c103_service (T_IED *p_ied, T_MESSAGE *p_msg)
{
    BOOL  ret_code = FALSE;
    /* [初始化部分] */
    switch (p_msg->m_type)
    {
    case MSG_T_INIT:
        /* 初始化协议 */
        break;
    case MSG_T_OPEN:
        /* 初始化设备 */
        return c103_open_ied ( (T_IED *) (p_msg->m_data));
    default:
        break;
    }

    /*	[设备操作接口部分] */

    /* 映射到内部操作接口 */
    switch (p_msg->m_type)
    {
    case MSG_T_LIST:
        if (p_msg->m_flag == MSG_F_READ)
        {
            ret_code = c103_get_List (p_ied, (T_LIST*) p_msg->m_data);
        }
        break;
    case MSG_T_SET:
        if (p_msg->m_flag == MSG_F_READ)
        {
            ret_code = c103_get_setting (p_ied, (T_SET *) p_msg->m_data);
        }
        break;
    case MSG_T_ZONE:
        if (p_msg->m_flag == MSG_F_READ)
        {
            ret_code = c103_get_zone (p_ied, (WORD *) p_msg->m_data);
        }
        break;
    case MSG_T_SFC:
        if (p_msg->m_flag == MSG_F_READ)
        {
            ret_code = c103_get_sfc (p_ied);
        }
        break;
    case MSG_T_CTRL:
        if (p_msg->m_flag == MSG_F_CHECK)
        {
            //ret_code = c103_check_control (p_ied, (WORD) p_msg->m_data);
        }
        if (p_msg->m_flag == MSG_F_WRITE)
        {
            //ret_code = c103_remote_control (p_ied, (WORD) p_msg->m_data);
        }
        break;
    case MSG_T_SIGNAL:
        /* 信号复归 */
        if (p_msg->m_flag == MSG_F_WRITE)
        {
            ret_code = c103_reset_signal (p_ied);
        }
        break;
    case MSG_T_ANALOG:
        if (p_msg->m_flag == MSG_F_READ)
        {
            ret_code = c103_get_analog (p_ied, (T_CHANNEL *) p_msg->m_data);
        }
        break;
    case MSG_T_CLOCK:
        /* 广播对时 */
        if (p_msg->m_flag == MSG_F_WRITE)
        {
            ret_code = c103_set_clock (p_ied, (T_DATE *) p_msg->m_data);
        }
        break;
    default:
        break;
    }

    /* 返回操作结果 */
    return ret_code;
}

/* 复位通信单元 */
BOOL c103_reset_cu (int sn, iec103_tag_tp* p_tag)
{
    T_C103_FRAME t_frame;

    printf ("^_^c103_reset_cu(复位通信单元)..\n");

    /* make frames */
    t_frame.type = FRAME_T_CON;
    t_frame.contral = 0x40;			//复位通信单元
    t_frame.address = p_tag->device_addr;
    t_frame.endbyte = 0x16;

    p_tag->fcb = 0x20;		//使帧计数位常为0

    if (c103_serial_com (sn, p_tag, &t_frame, 1))
    {
        if ( (t_frame.contral & 0x0F) == 0)	//确认响应
        {
            if ( (t_frame.contral & 0x20) != 0)		//判别响应帧
                return c103_request_first (sn, p_tag);
            else
                return c103_request_second (sn, p_tag);	//请求二级用户数据
        }
    }
    else
        return false;
}

/* 复位帧计数位 */
BOOL c103_reset_fcb (int sn, iec103_tag_tp* p_tag)
{
    T_C103_FRAME t_frame;

    /* make frames */
    t_frame.type = FRAME_T_CON;
    t_frame.contral = 0x47;			//复位帧计数位
    t_frame.address = p_tag->device_addr;
    t_frame.endbyte = 0x16;
    p_tag->fcb = 0x20;		//使帧计数位常为0

    if (c103_serial_com (sn, p_tag, &t_frame, 1))
    {
        if ( (t_frame.contral & 0x0F) == 0)	//确认响应
        {
            if ( (t_frame.contral & 0x20) != 0)		//判别响应帧
                return c103_request_first (sn, p_tag);
            else
                return c103_request_second (sn, p_tag);	//请求二级用户数据
        }
    }
    else
        return false;
}

/* 总查询 */
BOOL c103_request_gi (int sn, iec103_tag_tp* p_tag) //asdu07
{
    T_C103_FRAME t_frame;
    T_C103_ASDU* p_asdu;

    printf ("^_^c103_request_gi(总查询)..\n");
    p_asdu = (T_C103_ASDU *) &m_com[sn].tx_buf[6];
    p_asdu->type = 7;
    p_asdu->vsq = 0x81;
    p_asdu->cot = 9;
    p_asdu->addr = p_tag->device_addr;
    p_asdu->fun = C103_FUN_GLB;
    p_asdu->inf = 0;
    //--->changing...
    p_asdu->data[0] = m_c103_sn++;

    t_frame.type = FRAME_T_VOL;
    t_frame.address = p_tag->device_addr;
    t_frame.len = 9;
    t_frame.contral = 0x53;	//发送/确认
    t_frame.endbyte = 0x16;

    if (c103_serial_com (sn, p_tag, &t_frame, 1))
    {
        if ( (t_frame.contral & 0x0F) == 0)	//确认响应
        {
            if ( (t_frame.contral & 0x20) != 0)		//判别响应帧
                return c103_request_first (sn, p_tag);
            else
                return c103_request_second (sn, p_tag);	//请求二级用户数据
        }
    }
    else
        return false;
}

/* 上电初始化 */
BOOL c103_device_initialize (int sn, iec103_tag_tp* p_tag)
{
    /* 复位通信单元 */
    c103_reset_cu (sn, p_tag);

    /* 复位帧计数位 */
#if 0
    if (!c103_reset_fcb (sn, p_tag))
    {
        return false;
    }
#endif

    /* request all */
#if 0
    if (!c103_request_gi (sn, p_tag))
    {
        return false;
    }
    return true;
#endif
}

/* module's task */
void task_c103 (int argc, void *argv)
{
#if 0
    T_IED *p_ied;

    /* init switch */
    //p_ied = DEV_First_IED();
    while (p_ied != 0)
    {
        if (p_ied->dev_if == c103_service)
        {
            c103_device_initialize (p_ied);
        }
        //p_ied = DEV_Next_IED (p_ied->dev_id);
    }
loop:
    //p_ied = DEV_First_IED();
    while (p_ied != 0)
    {
        if (p_ied->dev_if == c103_service)
        {
            if (p_ied->dev_flag == ONLINE)
                c103_request_second (p_ied);	// request second data
            else
                c103_device_initialize (p_ied);
        }
        //p_ied = DEV_Next_IED (p_ied->dev_id);
    }
    /* polling_delay_time */
    goto loop;
#endif
}

static void pthread_quit()
{
    write_log (&m_error_log, -1, "pthread_quit()", "iec103_thread exit!");
    m_exit_flg = true;
    MySleep (2, 0);
    exit (1);
}

static int find_next_device (int sn, int i)
{
    if (i = m_com[sn].tag.size() - 1)
        return 0;
    for (size_t j = i; j++; j < m_com[sn].tag.size() - 2 - i)
    {
        wprintf ("!!!!!!!!!!!!!!!!!!!!!c:%d, o:%d, j:%d\n", m_com[sn].tag[j].device_addr, m_com[sn].tag[j + 1].device_addr, j);
        if (m_com[sn].tag[j].device_addr != m_com[sn].tag[j + 1].device_addr)
        {
            printf ("###################:%d\n", j + 1);
            return j + 1;
        }
    }
    return i;
}

static void *thread_iec103_collect (void * com_no)
{
    int i, j, ret;
    WORD sn = (int) com_no;
    WORD loop_count = 0;

    /* write starting */
    write_log (&m_boot_log, 1, "thread_iec103_collect()", "iec103_collect_thread[%d] startting...", sn);
    /* iec103 com initialization */
    iec103_init_rtu (&m_com[sn].com, m_com[sn].device, 9600, "even", 8, 1);
    if (m_display_on == true)
    {
        iec103_set_debug (&m_com[sn].com, TRUE);
    }
    /* iec103 com connect */
    if (iec103_connect (&m_com[sn].com) == -1)
    {
        write_log (&m_error_log, -1, "thread_iec103_collect()", "iec103_connect<%d> err!", sn);
        goto iec103_exit;
    }
    /* get loop count */
    loop_count = m_com[sn].tag.size();
    write_log (&m_boot_log, 1, "thread_iec103_collect()", "sn:%d,loop:%d", sn, loop_count);
    if (loop_count == 0)
    {
        goto iec103_exit;
    }

    while (true)
    {
collect_again:
        if (m_exit_flg == true)
            goto iec103_exit;
        for (i = 0; i < loop_count; i++)
        {
            if (m_exit_flg == true)
            {
                goto iec103_exit;
            }

            /* mutex */
            ret = pthread_mutex_lock (&m_com[sn].com_mutex);
            if (ret != 0)
            {
                write_log (&m_error_log, -1, "thread_iec103_collect()", "pthread_mutex_lock err!");
            }

            /* find device */
            i = find_next_device (sn, i);

            /* polling... */
            printf ("polling.. [i:%d, deviec_addr:%d]\n", i, m_com[sn].tag[i].device_addr);
            if (m_com[sn].tag[i].dev_flag == ONLINE)
            {
                c103_request_second (sn, &m_com[sn].tag[i]);
            }
            else
            {
                c103_device_initialize (sn, &m_com[sn].tag[i]);
            }

            /* unmutex */
            ret = pthread_mutex_unlock (&m_com[sn].com_mutex);
            if (ret != 0)
            {
                write_log (&m_error_log, -1, "thread_iec103_collect()", "pthread_mutex_lock err!");
            }

            /* polling_delay_time */
            if (m_debug == true)
            {
                MySleep (0, 1000 * m_space_time);
            }
            else
            {
                MySleep (0, 1000 * (long) m_com[sn].com_space_time);
            }

            /* pause */
            if (m_step == true)
            {
                printf ("\n--------------- [ push enter key to next step...]---------------\n");
                getchar();
            }
        } /* end for */
    } /* end while(1) */

iec103_exit:
    wprintf ("thread_iec103_collect:%d exited!\n", sn);
    write_log (&m_boot_log, 1, "thread_iec103_collect()", "iec103_collect_thread[%d] exit!", sn);
    pthread_exit (NULL);
}

static inline int query_att_according_pid (sqlite3 * db, int ID, int TYPE, char *result, int result_size)
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
            write_log (&m_error_log, -1, "query_att_according_pid()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
            ret = -1;
        }
    }
    else
    {
        printf ("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        write_log (&m_error_log, -1, "query_att_according_pid()", "sqlite3_get_table err!");
        ret = -1;
    }
    sqlite3_free_table (dbResult);
    return ret;
}

static inline int get_did_from_pid (sqlite3 * db, int tag_id)
{
    int result;
    char *errmsg = NULL;
    char **dbResult;
    int nRow, nColumn;
    int index;
    char SQL[256];
    int dID;

    sprintf (SQL, "select PID from TNODET where TAGID=%d;", tag_id);

    result = sqlite3_get_table (db, SQL, &dbResult, &nRow, &nColumn, &errmsg);
    if (SQLITE_OK == result)
    {
        if (nRow == 1 && nColumn == 1)
        {
            dID = strtol (dbResult[1], NULL, 10);
        }
        else
        {
            write_log (&m_error_log, -1, "Get_did_From_tag_id()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
            dID = -1;
        }
    }
    else
    {
        write_log (&m_error_log, -1, "Get_did_From_tag_id()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
        dID = -1;
    }
    sqlite3_free_table (dbResult);
    return dID;
}

static inline float get_factor_from_pid (sqlite3 * db, int PID)
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
            write_log (&m_error_log, -1, "get_factor_from_pid()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
            K = -1;
        }
    }
    else
    {
        write_log (&m_error_log, -1, "get_factor_from_pid()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
        K = -1;
    }
    sqlite3_free_table (dbResult);
    return K;
}

static inline int query_according_did (sqlite3 * db, int ID, int TYPE, char *result, int result_size)
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
            write_log (&m_error_log, -1, "query_according_did()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
            ret = -1;
        }
    }
    else
    {
        write_log (&m_error_log, -1, "query_according_did()", "sqlite3_get_table err!");
        ret = -1;
    }
    sqlite3_free_table (dbResult);
    return ret;
}

static inline int query_protocol_according_pid (sqlite3 * db, int pid)
{
    int result;
    char *errmsg = NULL;
    char **dbResult;
    int nRow, nColumn;
    int value;
    char SQL[256];

    sprintf (SQL, "select DISTINCT t_cole.id from TNODET,t_dev_col,t_cole \
            where TNODET.tagid=%d and TNODET.pid=t_dev_col.did and t_dev_col.cid=t_cole.id", pid);
    result = sqlite3_get_table (db, SQL, &dbResult, &nRow, &nColumn, &errmsg);
    if (SQLITE_OK == result)
    {
        if (nRow == 1 && nColumn == 1)
            value = strtol (dbResult[1], NULL, 10);
        else
        {
            printf ("SQL:%s\n", SQL);
            write_log (&m_error_log, -1, "query_protocol_according_pid()", "SQL:%s, nRow:%d, nColumn:%d", SQL, nRow, nColumn);
            value = -1;
        }
    }
    else
    {
        write_log (&m_error_log, -1, "query_protocol_according_pid()", "sqlite3_get_table err!");
        value = -1;
    }
    sqlite3_free_table (dbResult);
    return value;
}

static inline int create_optimized_table (sqlite3 * db)
{
    int result;
    char *errmsg = NULL;
    char SQL[] = "create table if not exists OPTIMIZED_TABLE    \
                (   \
                    tag_id          integer not null primary key, \
                    com             integer not null,   \
                    com_space_time  integer not null,   \
                    com_time_out    integer not null,   \
                    device_addr     integer not null,   \
                    iec103_type     integer not null,   \
                    fun             integer not null,   \
                    inf             integer not null,   \
                    operation       integer not null,   \
                    factor          double not null     \
                )";

    result = sqlite3_exec (db, SQL, NULL, 0, &errmsg);
    if (result != SQLITE_OK)
    {
        write_log (&m_error_log, -1, "create_optimized_table()", "sqlite3_exec err!");
        return -1;
    }
    else
        return result;
}

static inline int insert_optimized_table (sqlite3 * db,
        int tag_id,
        int com, int com_space_time, int com_time_out, int device_addr, int iec103_type,
        int fun, int inf, int operation, double factor)
{
    int result;
    char *errmsg = NULL;
    char SQL[512];

    sprintf (SQL, "INSERT INTO OPTIMIZED_TABLE VALUES (%d,%d,%d,%d,%d,%d,%d,%d,%d,%lf)",
             tag_id,
             com, com_space_time, com_time_out, device_addr, iec103_type,
             fun, inf, operation, factor);

    result = sqlite3_exec (db, SQL, NULL, 0, &errmsg);
    if (result != SQLITE_OK)
    {
        write_log (&m_error_log, -1, "insert_optimized_table()", "sqlite3_exec err!");
        return -1;
    }
    else
        return result;
}

static int load_config ()
{
    int ret;
    char key_value[256];
    /* get com node */
    for (size_t i = 0; i < COM_NUM; i++)
    {
        char serial[10];
        memset (m_com[i].device, 0, sizeof (m_com[i].device));
        sprintf (serial, "Serial:%d", i);
        ret = ConfigGetKey (IEC103_CONF_FILE, serial, "file_node", key_value);
        if (ret == 0)
            strncpy (m_com[i].device, key_value, sizeof (m_com[i].device) - 1);
        else
        {
            //ConfigSetKey (IEC103_CONF_FILE, serial, "file_node", "/dev/ttyNULL");
            strncpy (m_com[i].device, "/dev/ttyNULL", sizeof (m_com[i].device) - 1);
            write_log (&m_error_log, -1, "thread_iec103_collect()", "get com node err!");
        }
    }
    /* get tag_file */
    ret = ConfigGetKey (IEC103_CONF_FILE, "tag_file", "file", key_value);
    if (ret == 0)
        strlcpy (m_tag_file, key_value, sizeof (m_tag_file));
    else
    {
        strlcpy (m_tag_file, TAG_FILE, sizeof (TAG_FILE));
        ConfigSetKey (IEC103_CONF_FILE, "tag_file", "file", TAG_FILE);
    }
    /* get optimized_tag filename */
    ret = ConfigGetKey (IEC103_CONF_FILE, "tag_file_optimized", "file", key_value);
    if (ret == 0)
        strlcpy (m_tag_file_optimized, key_value, sizeof (m_tag_file_optimized));
    else
    {
        strlcpy (m_tag_file_optimized, TAG_FILE_OPTIMIZED, sizeof (m_tag_file_optimized));
        ConfigSetKey (IEC103_CONF_FILE, "tag_file_optimized", "file", TAG_FILE_OPTIMIZED);
    }
    /* get m_step */
    ret = ConfigGetKey (IEC103_CONF_FILE, "others", "step_mode", key_value);
    if (ret == 0)
    {
        if (strstr (key_value, "yes"))
            m_step = true;
        else if (strstr (key_value, "no"))
            m_step = false;
    }
    else
    {
        m_step = false;
        ConfigSetKey (IEC103_CONF_FILE, "others", "step_mode", "no");
    }
    /* get m_write_csv */
    ret = ConfigGetKey (IEC103_CONF_FILE, "others", "write_tag_to_csv_file", key_value);
    if (ret == 0)
    {
        if (strstr (key_value, "yes"))
            m_write_csv = true;
        else if (strstr (key_value, "no"))
            m_write_csv = false;
    }
    else
    {
        m_write_csv = false;
        ConfigSetKey (IEC103_CONF_FILE, "others", "write_tag_to_csv_file", "no");
    }
    /* get m_display_on */
    ret = ConfigGetKey (IEC103_CONF_FILE, "others", "display", key_value);
    if (ret == 0)
    {
        if (strstr (key_value, "yes"))
            m_display_on = true;
        else if (strstr (key_value, "no"))
            m_display_on = false;
    }
    else
    {
        m_display_on = false;
        ConfigSetKey (IEC103_CONF_FILE, "others", "display", "no");
    }
    /* get csv_file_path */
    ret = ConfigGetKey (IEC103_CONF_FILE, "others", "csv_file_path", key_value);
    if (ret == 0)
        strlcpy (m_csv_file_path, key_value, sizeof (m_tag_file_optimized));
    else
    {
        strlcpy (m_csv_file_path, "/var/log", sizeof ("/var/log"));
        ConfigSetKey (IEC103_CONF_FILE, "others", "csv_file_path", "/var/log");
    }
}

static int load_taglist ()
{
    sqlite3 * db;
    sqlite3 * db_tag_file_optimized;
    char * errmsg = NULL;
    char **dbResult;
    int nRow, nColumn;
    int result;
    int ret, i, j;
    int index;
    char SQL[512];
    double XISHU;
    char str[128];
    iec103_tag_tp tag;
    int com, com_space_time, com_time_out, device_addr, iec103_type;
    int tag_id, fun, inf, operation, factor;

    /* clear tag */
    for (i = 0; i < COM_NUM; i++)
    {
        m_com[i].tag.clear();
    }

    /* if optimized_table is exist goto read_optimized_table */
    int fd = open (m_tag_file_optimized, O_RDONLY, 0600);
    if (fd != -1)
    {
        close (fd);
        /* open optimized_tag */
        result = sqlite3_open (m_tag_file_optimized, &db_tag_file_optimized);
        if (result != SQLITE_OK)
        {
            printf ("sqlite3_open err!!!!!!!!!!!!!!!!!!!!!!!!!path_db:%s\n", m_tag_file_optimized);
            exit (1);
        }
        goto read_optimized_table;
    }

    /* open sysdb */
    wprintf ("m_tag_file:%s\n", m_tag_file);
    result = sqlite3_open (m_tag_file, &db);
    if (result != SQLITE_OK)
    {
        printf ("sqlite3_open err!!!!!!!!!!!!!!!!!!!!!!!!!path_db:%s\n", m_tag_file);
        exit (1);
    }

    /* open optimized_tag */
    wprintf ("m_tag_file_optimized:%s\n", m_tag_file_optimized);
    result = sqlite3_open (m_tag_file_optimized, &db_tag_file_optimized);
    if (result != SQLITE_OK)
    {
        printf ("sqlite3_open err!!!!!!!!!!!!!!!!!!!!!!!!!path_db:%s\n", m_tag_file_optimized);
        exit (1);
    }

    /* create_optimized_table */
    result = create_optimized_table (db_tag_file_optimized);
    if (result < 0)
    {
        printf ("create_optimized_table err!\n");
        exit (1);
    }

    /* 读取用户配置测点信息到临时表 */
    wprintf ("read user tag to temp table...\n");
    result = sqlite3_get_table (db, "select DISTINCT pid from T_POT_COL", &dbResult, &nRow, &nColumn, &errmsg);
    if (SQLITE_OK == result)
    {
        index = nColumn;
        for (i = 0; i < nRow ; i++)
        {
            for (j = 0 ; j < nColumn; j++)
            {
                tag_id = strtol (dbResult[index++], NULL, 10);
                int ret = query_protocol_according_pid (db, tag_id);
                if (ret < 0)
                {
                    printf ("query_protocol_according_pid err!\n");
                    exit (1);
                }
                else if (ret == 3) // is iec103 protocol
                {
                    /* get did */
                    int did = get_did_from_pid (db, tag_id);
                    if (did < 0)
                    {
                        printf ("get_did_from_pid err!\n");
                        exit (1);
                    }
                    /* get com number */
                    result = query_according_did (db, did, 37, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("query_according_did err!\n");
                        exit (1);
                    }
                    else
                        com = strtol (str, NULL, 10);
                    /* get com_space_time */
                    result = query_according_did (db, did, 39, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("query_according_did err!\n");
                        exit (1);
                    }
                    else
                        com_space_time = strtol (str, NULL, 10);
                    /* get com_time_out */
                    result = query_according_did (db, did, 40, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("query_according_did err!\n");
                        exit (1);
                    }
                    else
                        com_time_out = strtol (str, NULL, 10);
                    /* get device_addr */
                    result = query_according_did (db, did, 38, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("query_according_did err!\n");
                        exit (1);
                    }
                    else
                        device_addr = strtol (str, NULL, 10);
                    /* get iec103_type */
                    result = query_according_did (db, did, 41, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("get ModbusType err! exit!\n");
                        exit (1);
                    }
                    else
                        iec103_type = strtol (str, NULL, 10);
                    /* get fun */
                    result = query_att_according_pid (db, tag_id, 11, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("get Opertor err!\n");
                        exit (1);
                    }
                    else
                        fun = strtol (str, NULL, 10);
                    /* get inf */
                    result = query_att_according_pid (db, tag_id, 12, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("get Opertor err!\n");
                        exit (1);
                    }
                    else
                        inf = strtol (str, NULL, 10);
                    /* get operation */
                    result = query_att_according_pid (db, tag_id, 13, str, sizeof (str));
                    if (result < 0)
                    {
                        printf ("get Opertor err!\n");
                        exit (1);
                    }
                    else
                        operation = strtol (str, NULL, 10);
                    /* get factor */
                    factor = get_factor_from_pid (db, tag_id);
                    if (factor == -1)
                    {
                        printf ("get factor err!\n");
                        exit (1);
                    }
                    /* 插入临时表 */
                    int ret = insert_optimized_table (db_tag_file_optimized,
                                                      tag_id,
                                                      com, com_space_time, com_time_out, device_addr, iec103_type,
                                                      fun, inf, operation, factor);
                    if (ret < 0)
                    {
                        printf ("insert_optimized_table err!\n");
                        exit (1);
                    }
                }
            }
        } /* end for */
    }
    else
    {
        printf ("sqlite3_get_table err!\n");
        write_log (&m_error_log, -1, "load_taglist()", "sqlite3_get_table err!");
        exit (1);
    }
    sqlite3_free_table (dbResult);
    sqlite3_close (db);

read_optimized_table:

    /* 从临时表按一定顺序读出所有信息到内存 */
    wprintf ("read tag from temp table...\n");
    result = sqlite3_get_table (db_tag_file_optimized,
                                "select\
                                tag_id,\
                                com, com_space_time, com_time_out, device_addr, iec103_type,\
                                fun, inf, operation, factor\
                                from OPTIMIZED_TABLE\
                                order by device_addr",
                                &dbResult, &nRow, &nColumn, &errmsg);
// tag_id(0),
// com(1), com_space_time(2), com_time_out(3), device_addr(4), iec103_type(5),
// fun(6), inf(7), operation(8), factor(9),

    if (SQLITE_OK == result)
    {
        index = nColumn;
        wprintf ("查到%d条记录\n", nRow);
        for (i = 0; i < nRow ; i++)
        {
            /* print to screen */
            //wprintf ("第 %d 条记录\n", i + 1);
            for (j = 0 ; j < nColumn; j++)
            {
                wprintf (" %s | %s |", dbResult[j], dbResult [index++]);
            }
            wprintf ("\n");
            int base = nColumn * (i + 1);
            /* get com number */
            com = strtol (dbResult[base + 1], NULL, 10);
            if (com < 1 || com > 7)
            {
                write_log (&m_error_log, -1, "load_taglist()", "com Error:%d", com);
                exit (1);
            }
            /* get tag_id */
            tag.tag_id = (WORD) strtol (dbResult[base + 0], NULL, 10);
            /* get com_space_time */
            m_com[com - 1].com_space_time = (WORD) strtol (dbResult[base + 2], NULL, 10);
            /* get com_time_out */
            m_com[com - 1].com_time_out = (WORD) strtol (dbResult[base + 3], NULL, 10);
            /* get device_addr */
            tag.device_addr = (BYTE) strtol (dbResult[base + 4], NULL, 10);
            /* get iec103_type */
            tag.iec103_type = (BYTE) strtol (dbResult[base + 5], NULL, 10);
            /* get fun */
            tag.fun = (BYTE) strtol (dbResult[base + 6], NULL, 10);
            /* get inf */
            tag.inf = (BYTE) strtol (dbResult[base + 7], NULL, 10);
            /* get operation */
            tag.operation = (BYTE) strtol (dbResult[base + 8], NULL, 10);
            /* get factor */
            tag.factor = atof (dbResult[base + 9]);
            /* push tag to m_com */
            m_com[com - 1].tag.push_back (tag);
        } /* end for nRow */
    }
    else
    {
        write_log (&m_error_log, -1, "load_taglist()", "sqlite3_get_table err!");
        /* delet the optimized_tags */
        char *cmd;
        cmd = new char[128];
        strlcpy (cmd, "rm -rf ", 128);
        strlcat (cmd, m_tag_file_optimized, 128);
        system (cmd);
        delete[] cmd;
        pthread_quit ();
    }
    sqlite3_free_table (dbResult);

    /* write iec103_master_tag.csv */
    if (m_write_csv == true)
    {
        printf ("write iec103_master csv file...\n");
        char msg[512];
        for (i = 0; i < COM_NUM; i++)
        {
            sprintf (msg, "/*-----[com %d]-----*/", i);
            write_to_log (m_csv_file_path, "iec103_master_tag.csv", msg,  LOG_APPEND | LOG_NOTIME);
            sprintf (msg, "tag_nums: \t%d", m_com[i].tag.size());
            write_to_log (m_csv_file_path, "iec103_master_tag.csv", msg,  LOG_APPEND | LOG_NOTIME);
            write_to_log (m_csv_file_path, "iec103_master_tag.csv",
                          "tag_id, com_space_time, com_time_out, device_addr, iec103_type, fun, inf, operation, factor",
                          LOG_APPEND | LOG_NOTIME);
            for (j = 0; j < m_com[i].tag.size(); j++)
            {
                sprintf (msg, "%d, %d, %d, %d, %d, %d, %d, %d, %f",
                         m_com[i].tag[j].tag_id,
                         m_com[i].com_space_time,
                         m_com[i].com_time_out,
                         m_com[i].tag[j].device_addr,
                         m_com[i].tag[j].iec103_type,
                         m_com[i].tag[j].fun,
                         m_com[i].tag[j].inf,
                         m_com[i].tag[j].operation,
                         m_com[i].tag[j].factor);
                write_to_log (m_csv_file_path, "iec103_master_tag.csv", msg,  LOG_APPEND | LOG_NOTIME);
            }
            write_to_log (m_csv_file_path, "iec103_master_tag.csv", " ", LOG_APPEND | LOG_NOTIME);
        } /* end for */
    }

    sqlite3_close (db_tag_file_optimized);
    printf ("write iec103_master csv file finished!\n");
    return 0;
}

static int sys_init ()
{
    load_config ();

    for (size_t i = 0; i < COM_NUM; i++)
    {
        pthread_mutex_init (&m_com[i].com_mutex, NULL);
        pthread_mutex_init (&m_com[i].com_mutex, NULL);
    }

    m_exit_flg = false;

    /* inite log file */
    log_init (&m_boot_log, IEC103_BOOT_FILE, "iec103_boot", "v0.1.0", "wang_chen_xi", "boot log");
    log_init (&m_error_log, IEC103_ERR_FILE, "iec103_err", "v0.1.0", "wang_chen_xi", "err log");

    /* creat csv file */
    wprintf ("create csv file...\n");
    write_to_log (m_csv_file_path, "iec103_master_tag.csv", "[TagListFile] -- - recording tag_file", LOG_NEW | LOG_TIME);
    write_to_log (m_csv_file_path, "iec103_master_tag.csv", " ", LOG_APPEND | LOG_NOTIME);

    return 0;
}

static int iec103_init ()
{
    for (size_t i = 0; i < COM_NUM; i++)
    {
        int tag_num = m_com[i].tag.size ();
        for (size_t j = 0; j < tag_num; j++)
        {
            m_com[i].tag[j].fcb = 0;
            m_com[i].tag[j].dev_flag = OFFLINE;
        }
    }
}

int iec103_master_exit (void *arg)
{
    m_exit_flg = true;
}

int iec103_master_run (void *arg)
{
    sys_init ();
    load_taglist ();
    iec103_init ();

    for (size_t i = 0; i < COM_NUM; i++)
    {
        if (pthread_create (&m_com[i].com_th_id, NULL, thread_iec103_collect, (void *) i) != 0)
        {
            write_log (&m_boot_log, -1, "main()", "thread_iec103_collect[%d] creat error", i);
            pthread_quit ();
        }
        else
            write_log (&m_boot_log, -1, "main()", "thread_iec103_collect[%d] creat OK!", i);
        MySleep (0, 50 * 1000);
    }

exit:
    for (size_t i = 0; i < COM_NUM; i++)
    {
        pthread_join (m_com[i].com_th_id, NULL);
    }
    printf ("iec103 master exited!\n");
    return 0;
}

#if 0
int main ()
{
    iec103_master_run (NULL);
    return 0;
}
#endif

