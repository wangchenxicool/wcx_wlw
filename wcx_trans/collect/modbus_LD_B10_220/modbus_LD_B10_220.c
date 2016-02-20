#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <termios.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>

/* TCP */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#if defined(__FreeBSD__ ) && __FreeBSD__ < 5
#include <netinet/in_systm.h>
#endif
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#if !defined(UINT16_MAX)
#define UINT16_MAX 0xFFFF
#endif

#include "wcx_utils.h"
#include "modbus.h"
#include "modbus_LD_B10_220.h"
//#include "logger.h"

#define UNKNOWN_ERROR_MSG "Not defined in modbus specification"

#define DEBUG

#ifdef DEBUG
#define wprintf(format, arg...)  \
    printf( format , ##arg)
#else
#define wprintf(format, arg...)
#endif


int read_holding_registers_LD_B10_220 (modbus_param_t *mb_param, int start_addr, int nb,
                                       uint32_t *data_dest, int select_time)
{
    read_holding_registers (mb_param, start_addr * 2, nb, data_dest, 2, select_time);
}

int read_coil_status_LD_B10_220 (modbus_param_t *mb_param, uint8_t *data_dest, int select_time)
{
    int ret;
    int query_length;
    uint8_t query[MIN_QUERY_LENGTH];
    uint8_t response[MAX_MESSAGE_LENGTH];

    query[0] = mb_param->slave;
    query[1] = 0x04;
    query[2] = 0x00;
    query[3] = 0x00;
    query[4] = 0x00;
    query[5] = 0x01;
    query_length = 0x06;

    ret = modbus_send (mb_param, query, query_length);
    if (ret > 0)
    {
        int i, temp, bit;
        int pos = 0;

        ret = receive_msg (mb_param, 7, response, select_time);
        if (ret >= 0)
        {
            int query_nb_value;
            int response_nb_value;
            query_nb_value = 1;
            response_nb_value = (response[2] / 2);
            if (query_nb_value == response_nb_value)
            {
                temp = response[4];
                for (bit = 0x01; (bit & 0xff) && (pos < 5);)
                {
                    data_dest[pos++] = (temp & bit) ? TRUE : FALSE;
                    bit = bit << 1;
                }
                ret = 5;
            }
            else
            {
                char *s_error = (char*) malloc (64 * sizeof (char));
                sprintf (s_error, "Quantity not corresponding to the query (%d != %d)", response_nb_value, query_nb_value);
                ret = INVALID_DATA;
                error_treat (mb_param, ret, s_error);
                free (s_error);
                ret = -1;
            }
        }
    }
    return ret;
}

