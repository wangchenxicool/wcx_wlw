#ifndef _WCX_LOG
#define _WCX_LOG

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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct log_tp
{
    char fd[128];
    unsigned int seg;
};


int slprintf (char *buf, int buflen, char *fmt, ...);
int vslprintf (char *buf, int buflen, const char *fmt, va_list args);
void wcx_log (const char *file, const char *fmt, ...);
void log_init (struct log_tp *log, const char *file, const char *soft, const char* version,
               const char *author, const char *logtype);
void write_log (struct log_tp *log, int error_code, const char *fun, const char *fmt, ...);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

