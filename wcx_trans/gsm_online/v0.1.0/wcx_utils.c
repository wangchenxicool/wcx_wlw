#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <dirent.h>
#include <time.h>
#include <iostream>
#include <string>
#include <string.h>
#include "wcx_utils.h"

#define DEBUG

#ifdef DEBUG
#define wprintf(format, arg...)  \
    printf( format , ## arg)
#else
#define wprintf(format, arg...) {}
#endif



/**
 * @brief
 *
 * @param   s
 * @param   us
 */
void MySleep (long int s, long int us)
{
    struct timeval tv;
    tv.tv_sec = s;
    tv.tv_usec = us;
    if (select (0, NULL, NULL, NULL, &tv) < 0)
        perror ("[MySleep: select error]");
}

/**
 * @brief
 *
 * @return
 */
int getch()
{
    char ch;
    struct termios save, ne;

    /* 设定终端 */
    ioctl (0, TCGETS, &save);
    ioctl (0, TCGETS, &ne);
    ne.c_lflag &= ~ (ECHO | ICANON);
    ioctl (0, TCSETS, &ne);

    /* 从终端获取信息 */
    read (0, &ch, 1);

    /* 恢复终端设定 */
    ioctl (0, TCSETS, &save);

    return ch;
}


/*
 * strlcpy - like strcpy/strncpy, doesn't overflow destination buffer,
 * always leaves destination null-terminated (for len > 0).
 */
size_t strlcpy (char *dest, const char *src, size_t len)
{
    size_t ret = strlen (src);

    if (len != 0)
    {
        if (ret < len)
            strcpy (dest, src);
        else
        {
            strncpy (dest, src, len - 1);
            dest[len - 1] = '\0';
        }
    }

    return ret;
}

/*
 * strlcat - like strcat/strncat, doesn't overflow destination buffer,
 * always leaves destination null-terminated (for len > 0).
 */
size_t strlcat (char *dest, const char *src, size_t len)
{
    size_t dlen = strlen (dest);

    return dlen + strlcpy (dest + dlen, src, (len > dlen ? len - dlen : 0));
}


void CreateDaemon()
{
    pid_t child;
    char buffer[512];
    child = fork();
    if (child == 0)
    {
        setsid();
        child = fork();
        if (child == 0)
        {
            chdir ("/tmp");
            umask (0);
        }
        else if (child > 0)
            exit (0);
    }
    else if (child > 0)
        exit (0);
}


void die (const char* p_text)
{
#ifdef DIE_DEBUG
    /*bug ( p_text );*/
#endif
    exit (1);
}


int wcx_recv_peek (const int fd, void* p_buf, unsigned int len)
{
    while (1)
    {
        int retval = recv (fd, p_buf, len, MSG_PEEK);
        int saved_errno = errno;
        /*wcx_check_pending_actions (kVSFSysUtilIO, retval, fd);*/
        if (retval < 0 && saved_errno == EINTR)
            continue;
        return retval;
    }
}


int wcx_write (const int fd, const void* p_buf, const unsigned int size)
{
    while (1)
    {
        int retval = write (fd, p_buf, size);
        int saved_errno = errno;
        /*wcx_check_pending_actions (kVSFSysUtilIO, retval, fd);*/
        if (retval < 0 && saved_errno == EINTR)
            continue;
        return retval;
    }
}


int wcx_write_loop (const int fd, const void* p_buf, unsigned int size)
{
    int retval;
    int num_written = 0;

    while (1)
    {
        retval = wcx_write (fd, (const char*) p_buf + num_written, size);
        if (retval < 0)
        {
            /* Error */
            return retval;
        }
        else if (retval == 0)
        {
            /* Written all we're going to write.. */
            return num_written;
        }
        if ( (unsigned int) retval > size)
        {
            die ("retval too big in wcx_read_loop");
        }
        num_written += retval;
        size -= (unsigned int) retval;
        if (size == 0)
        {
            /* Hit the write target, cool. */
            return num_written;
        }
    }
}


int wcx_read (const int fd, void* p_buf, const unsigned int size)
{
    while (1)
    {
        int retval = read (fd, p_buf, size);
        int saved_errno = errno;
        /*wcx_check_pending_actions (kVSFSysUtilIO, retval, fd);*/
        if (retval < 0 && saved_errno == EINTR)
            continue;
        return retval;
    }
}


int wcx_read_loop (const int fd, void* p_buf, unsigned int size)
{
    int retval;
    int num_read = 0;

    while (1)
    {
        retval = wcx_read (fd, (char*) p_buf + num_read, size);
        if (retval < 0)
        {
            wprintf ("wcx_read_loop: wcx_read return less then 0\n");
            return retval;
        }
        else if (retval == 0)
        {
            /* Read all we're going to read.. */
            return num_read;
        }
        if ( (unsigned int) retval > size)
        {
            die ("retval too big in wcx_read_loop");
        }
        num_read += retval;
        size -= (unsigned int) retval;
        if (size == 0)
        {
            /* Hit the read target, cool. */
            return num_read;
        }
    } //end while
}



/**
 * @brief    computer crc16
 *
 * @param   p: point to unsigned char buff
 * @param   Len: the length of unsigned char data
 *
 * @return : the value of crc16
 */
unsigned int CallCrc16 (unsigned char *p, unsigned char Len)
{
    volatile unsigned char crc16Hi, crc16LO;  //CRC寄存器
    volatile unsigned char CL, CH;
    volatile unsigned char SaveHi, SaveLo;
    unsigned char i, j;

    CL = 1;    //多项式
    CH = 0xA0; //多项式
    crc16Hi = 0xFF;  //寄存器置1
    crc16LO = 0XFF;

    while (Len-- != 0)
    {
        crc16LO = crc16LO ^ *p;
        for (j = 0x80; j != 0; j >>= 1)
        {
            SaveHi = crc16Hi;
            SaveLo = crc16LO;
            crc16Hi = (crc16Hi >> 1);   //高位右移一位
            crc16LO = (crc16LO >> 1);
            if ( (SaveHi & 0x01) == 1)
            {
                crc16LO |= 0x80;
            }
            if ( (SaveLo & 0x01) == 1)
            {
                crc16LO = crc16LO ^ CL;
                crc16Hi = crc16Hi ^ CH;
            }
        } // end for
        p++;
    } // end for
    //return (crc16Hi << 8 | crc16LO);
    return (crc16LO << 8 | crc16Hi);
}


/**
 * @brief    str to HEX
 *
 * @param   str
 *
 * @return
 */
unsigned long StrToHex (char *str)
{
    unsigned long Hex = 0;
    register unsigned int i;

    // 将字符串中的^M和^J字符去掉
    char *ptr = str;
    while (*ptr != '\0')
    {
        if (*ptr == '\x00a' || *ptr == '\x00d')
            *ptr = '\0';
        ptr++;
    }

    for (i = 0; i < strlen (str); i++)
    {
        Hex *= 16;
        if ( (str[i] <= '9') && (str[i] >= '0'))
        {
            Hex += str[i] - '0';
        }
        else if ( (str[i] <= 'F') && (str[i] >= 'A'))
        {
            Hex += str[i] - 'A' + 10;
        }
        else if ( (str[i] <= 'f') && (str[i] >= 'a'))
        {
            Hex += str[i] - 'a' + 10;
        }
        else
        {
            return 0xffff;  //出错时返回0xffff
        }
    }
    return Hex;
}


/**
 * @brief
 *
 * @param   source
 * @param   nCsvFileCol
 * @param   pCsvFileCol
 *
 * @return
 */
int  GetCsvFileCol (const char *source, const int nCsvFileCol, char *pCsvFileCol)
{
    int i;
    char *psource, *p;

    psource = (char *) malloc ( (strlen (source) + 1) * sizeof (char));
    if (psource == NULL)
    {
        perror ("[molloc]");
        return (-1);
    }

    strcpy (psource, source);
    p = strtok (psource, ",");
    for (i = 0; i < nCsvFileCol; i++)
    {
        p = strtok (NULL, ",");
        if (!p)
        {
            break;
        }
    }

    if (p != NULL)
    {
        strcpy (pCsvFileCol, p);
    }
    else
    {
        strcpy (pCsvFileCol, "-1");
    }
    free (psource);

    return (0);
}


/**
 * @brief    write log to file
 *
 * @param   path
 * @param   file_name
 * @param   msg
 * @param   write_to_log_flg
 */
#if 0
void write_to_log (const char *path, const char *file_name, const char *msg)
{
    //system("echo collect.c -- pthread_create Failed! ------- $(date) >> /var/log/wcx_CollectNG.log");
    char cmd[512];
    int size = sizeof (cmd);
    strlcpy (cmd, "echo ", size);
    strlcat (cmd, msg, size);
    strlcat (cmd, " ---- $(date)", size);
    strlcat (cmd, " >> ", size);
    strlcat (cmd, path, size);
    strlcat (cmd, "/", size);
    strlcat (cmd, file_name, size);
    system (cmd);
}
#endif

