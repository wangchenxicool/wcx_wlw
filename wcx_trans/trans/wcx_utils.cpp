#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <termios.h>
#include <dirent.h>
#include <time.h>
#include <sys/socket.h>
#include <iostream>
#include <string>

#include "wcx_utils.h"

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

void MySleep (long int s, long int us)
{
    struct timeval tv;
    tv.tv_sec = s;
    tv.tv_usec = us;
    if (select (0, NULL, NULL, NULL, &tv) < 0)
    {
        perror ("[MySleep: select error]");
    }
}

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

void write_to_log (const char *path, const char *file_name, const char *msg, char flg)
{
    std::string cmd;

    //system("echo collect.c -- pthread_create Failed! ------- $(date) >> /var/log/wcx_CollectNG.log");
    cmd = "echo ";
    cmd += msg;
    if (flg & LOG_TIME)
        cmd += " ------- $(date)";
    if (flg & LOG_APPEND)
        cmd += " >> ";
    else
        cmd += " > ";
    cmd += path;
    cmd += "/";
    cmd += file_name;
    system (cmd.c_str());
}

void write_to_log2 (char *file_name, const char *msg, char flg)
{
    std::string cmd;

    //system("echo collect.c -- pthread_create Failed! ------- $(date) >> /var/log/wcx_CollectNG.log");
    cmd = "echo ";
    cmd += msg;
    if (flg & LOG_TIME)
        cmd += " ------- $(date)";
    if (flg & LOG_APPEND)
        cmd += " >> ";
    else
        cmd += " > ";
    cmd += file_name;
    system (cmd.c_str());
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
            dest[len - 1] = 0;
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
