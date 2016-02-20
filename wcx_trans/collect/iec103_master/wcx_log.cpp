#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <pwd.h>
#include <stdarg.h>
#include <time.h>
#include <netinet/in.h>
#include "wcx_utils.h"
#include "wcx_log.h"


int slprintf (char *buf, int buflen, char *fmt, ...)
{
    va_list args;
    int n;

#if defined(__STDC__)
    va_start (args, fmt);
#else
    char *buf;
    int buflen;
    char *fmt;
    va_start (args);
    buf = va_arg (args, char *);
    buflen = va_arg (args, int);
    fmt = va_arg (args, char *);
#endif
    n = vslprintf (buf, buflen, fmt, args);

    va_end (args);

    return n;
}



#define OUTCHAR(c)	(buflen > 0? (--buflen, *buf++ = (c)): 0)

int vslprintf (char *buf, int buflen, const char *fmt, va_list args)
{
    int c, i, n;
    int width, prec, fillch;
    int base, len, neg, quoted;
    unsigned long val = 0;
    char *str, *buf0;
    const char *f;
    unsigned char *p;
    char num[32];
    time_t t;
    struct tm *timenow;
    u_int32_t ip;
    static char hexchars[] = "0123456789abcdef";

    buf0 = buf;
    --buflen;

    while (buflen > 0)
    {
        for (f = fmt; *f != '%' && *f != 0; ++f)
            ;

        if (f > fmt)
        {
            len = f - fmt;
            if (len > buflen)
                len = buflen;
            memcpy (buf, fmt, len);
            buf += len;
            buflen -= len;
            fmt = f;
        }

        if (*fmt == 0)
            break;

        c = *++fmt;
        width = 0;
        prec = -1;
        fillch = ' ';

        if (c == '0')
        {
            fillch = '0';
            c = *++fmt;
        }

        if (c == '*')
        {
            width = va_arg (args, int);
            c = *++fmt;
        }
        else
        {
            while (isdigit (c))
            {
                width = width * 10 + c - '0';
                c = *++fmt;
            }
        }

        if (c == '.')
        {
            c = *++fmt;
            if (c == '*')
            {
                prec = va_arg (args, int);
                c = *++fmt;
            }
            else
            {
                prec = 0;

                while (isdigit (c))
                {
                    prec = prec * 10 + c - '0';
                    c = *++fmt;
                }
            }
        }

        str = 0;
        base = 0;
        neg = 0;
        ++fmt;

        switch (c)
        {
        case 'l':
            c = *fmt++;
            switch (c)
            {
            case 'd':
                val = va_arg (args, long);
                if (val < 0)
                {
                    neg = 1;
                    val = -val;
                }
                base = 10;
                break;
            case 'u':
                val = va_arg (args, unsigned long);
                base = 10;
                break;
            default:
                OUTCHAR ('%');
                OUTCHAR ('l');
                --fmt;		/* so %lz outputs %lz etc. */
                continue;
            }
            break;
        case 'd':
            i = va_arg (args, int);
            if (i < 0)
            {
                neg = 1;
                val = -i;
            }
            else
                val = i;
            base = 10;
            break;
        case 'u':
            val = va_arg (args, unsigned int);
            base = 10;
            break;
        case 'o':
            val = va_arg (args, unsigned int);
            base = 8;
            break;
        case 'x':
        case 'X':
            val = va_arg (args, unsigned int);
            base = 16;
            break;
        case 'p':
            val = (unsigned long) va_arg (args, void *);
            base = 16;
            neg = 2;
            break;
        case 's':
            str = va_arg (args, char *);
            break;
        case 'c':
            num[0] = va_arg (args, int);
            num[1] = 0;
            str = num;
            break;
        case 'm':
            str = strerror (errno);
            break;
        case 'I':
            ip = va_arg (args, u_int32_t);
            ip = ntohl (ip);
            slprintf (num, sizeof (num), (char*) "%d.%d.%d.%d", (ip >> 24) & 0xff,
                      (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
            str = num;
            break;
        case 't':
            time (&t);
            str = (char*) ctime (&t);
            str += 4;		/* chop off the day name */
            str[15] = 0;	/* chop off year and newline */
            break;
        case 'T':
            time (&t);
            timenow = (struct tm*) localtime (&t);
            str = (char*) asctime (timenow);
            str[ strlen ( (char*) asctime (timenow)) - 1 ] = 0;
            break;
        case 'v':		/* "visible" string */
        case 'q':		/* quoted string */
            quoted = c == 'q';
            p = va_arg (args, unsigned char *);
            if (fillch == '0' && prec >= 0)
                n = prec;
            else
            {
                n = strlen ( (char *) p);
                if (prec >= 0 && n > prec)
                    n = prec;
            }

            while (n > 0 && buflen > 0)
            {
                c = *p++;
                --n;

                if (!quoted && c >= 0x80)
                {
                    OUTCHAR ('M');
                    OUTCHAR ('-');
                    c -= 0x80;
                }

                if (quoted && (c == '"' || c == '\\'))
                    OUTCHAR ('\\');

                if (c < 0x20 || (0x7f <= c && c < 0xa0))
                {
                    if (quoted)
                    {
                        OUTCHAR ('\\');
                        switch (c)
                        {
                        case '\t':
                            OUTCHAR ('t');
                            break;
                        case '\n':
                            OUTCHAR ('n');
                            break;
                        case '\b':
                            OUTCHAR ('b');
                            break;
                        case '\f':
                            OUTCHAR ('f');
                            break;
                        default:
                            OUTCHAR ('x');
                            OUTCHAR (hexchars[c >> 4]);
                            OUTCHAR (hexchars[c & 0xf]);
                        }
                    }
                    else
                    {
                        if (c == '\t')
                            OUTCHAR (c);
                        else
                        {
                            OUTCHAR ('^');
                            OUTCHAR (c ^ 0x40);
                        }
                    }
                }
                else
                    OUTCHAR (c);
            }

            continue;
        case 'B':
            p = va_arg (args, unsigned char *);
            for (n = prec; n > 0; --n)
            {
                c = *p++;
                if (fillch == ' ')
                    OUTCHAR (' ');
                OUTCHAR (hexchars[ (c >> 4) & 0xf]);
                OUTCHAR (hexchars[c & 0xf]);
            }
            continue;
        default:
            *buf++ = '%';
            if (c != '%')
                --fmt;		/* so %z outputs %z etc. */
            --buflen;
            continue;
        }

        if (base != 0)
        {
            str = num + sizeof (num);
            *--str = 0;

            while (str > num + neg)
            {
                *--str = hexchars[val % base];
                val = val / base;
                if (--prec <= 0 && val == 0)
                    break;
            }

            switch (neg)
            {
            case 1:
                *--str = '-';
                break;
            case 2:
                *--str = 'x';
                *--str = '0';
                break;
            }

            len = num + sizeof (num) - 1 - str;
        }
        else
        {
            len = strlen (str);
            if (prec >= 0 && len > prec)
                len = prec;
        }

        if (width > 0)
        {
            if (width > buflen)
                width = buflen;
            if ( (n = width - len) > 0)
            {
                buflen -= n;

                for (; n > 0; --n)
                    *buf++ = fillch;
            }
        }

        if (len > buflen)
            len = buflen;

        memcpy (buf, str, len);
        buf += len;
        buflen -= len;
    }

    *buf = 0;
    return buf - buf0;
}


static int head_flg = 1;

void wcx_log (const char *file, const char *fmt, ...)
{
    int fd;
    va_list args;
    char buf[1024];

#if defined(__STDC__)
    va_start (args, fmt);
#else
    char *fmt;
    va_start (args);
    fmt = va_arg (args, char *);
#endif

    vslprintf (buf, sizeof (buf), fmt, args);

    va_end (args);

    /*if (head_flg == 0)*/
        /*fprintf (stderr, "%s\n", buf);*/
    /*syslog ( LOG_ERR, "%s", buf );*/

    if ( (fd = open (file, O_CREAT | O_WRONLY | O_APPEND, 0600)) < 0)
    {
        printf ("file:%s\n", file);
        perror ("open");
        goto log_exit;
    }

    if (write (fd, buf, strlen (buf)) == -1)
    {
        perror ("write");
    }

    if (close (fd) < 0)
    {
        perror ("close");
    }

log_exit:
    ;
}


void log_init (struct log_tp *log, const char *file, const char *soft, const char* version,
               const char *author, const char *logtype)
{
    head_flg = 1;
    strlcpy (log->fd, file, sizeof (log->fd));
    log->seg = 1;

    wcx_log (file, "%s\n",
            "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
    wcx_log (file, "%s<%T>\n", "$Log File Created On: ");
    wcx_log (file, "%s<%s>\n", "$Software: ", soft);
    wcx_log (file, "%s<%s>\n", "$Editor: ", author);
    wcx_log (file, "%s<%s>\n", "$Version: ", version);
    wcx_log (file, "%s\n", "$Remark: all time values are in Local Time");
    wcx_log (file, "%s<%s>\n", "$Log Type: ", logtype);
    wcx_log (file, "%s\n", "$Fields:");
    wcx_log (file, "%s%26s%4s%21s%s\n", "$seg  ", "time", "Err", "funtion  ", "comment");

    head_flg = 0;
}


void write_log (struct log_tp *log, int error_code, const char *fun, const char *fmt, ...)
{
    va_list args;
    char buf[512];

#if defined(__STDC__)
    va_start (args, fmt);
#else
    char *fmt;
    va_start (args);
    fmt = va_arg (args, char *);
#endif

    vslprintf (buf, sizeof (buf), fmt, args);

    va_end (args);

    wcx_log (log->fd, "%6d  %T%4d%19s   %s\n", log->seg++, error_code, fun, buf);
}

