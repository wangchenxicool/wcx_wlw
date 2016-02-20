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
#include "modbus_RPC3FxC.h"
//#include "logger.h"

#define UNKNOWN_ERROR_MSG "Not defined in modbus specification"

#define DEBUG

#ifdef DEBUG
#define wprintf(format, arg...)  \
    printf( format , ##arg)
#else
#define wprintf(format, arg...)
#endif

static const int TAB_HEADER_LENGTH[2] =
{
    HEADER_LENGTH_RTU,
    HEADER_LENGTH_TCP
};

static const int TAB_CHECKSUM_LENGTH[2] =
{
    CHECKSUM_LENGTH_RTU,
    CHECKSUM_LENGTH_TCP
};

static const char *TAB_ERROR_MSG[] =
{
    /* 0x00 */ UNKNOWN_ERROR_MSG,
    /* 0x01 */ "Illegal function code",
    /* 0x02 */ "Illegal data address",
    /* 0x03 */ "Illegal data value",
    /* 0x04 */ "Slave device or server failure",
    /* 0x05 */ "Acknowledge",
    /* 0x06 */ "Slave device or server busy",
    /* 0x07 */ "Negative acknowledge",
    /* 0x08 */ "Memory parity error",
    /* 0x09 */ UNKNOWN_ERROR_MSG,
    /* 0x0A */ "Gateway path unavailable",
    /* 0x0B */ "Target device failed to respond"
};

static const uint8_t NB_TAB_ERROR_MSG = 12;

static unsigned int compute_response_length_RPC3FxC (modbus_param_t *mb_param, uint8_t *query)
{
    int length;
    int offset;

    offset = TAB_HEADER_LENGTH[mb_param->type_com];

    if (query[offset] == FC_READ_COIL_STATUS  || query[offset] == FC_READ_INPUT_STATUS)
    {
        /* Header + 1 * nb values */
        length = 2 + (query[offset + 3] << 8 | query[offset + 4]);
        return length + offset + TAB_CHECKSUM_LENGTH[mb_param->type_com];
    }
    else
    {
        return -1;
    }
}

static int modbus_receive (modbus_param_t *mb_param, uint8_t *query, uint8_t *response, int select_time)
{
    int ret;
    int response_length_computed;
    int offset = TAB_HEADER_LENGTH[mb_param->type_com];

    response_length_computed = compute_response_length_RPC3FxC (mb_param, query);
    ret = receive_msg (mb_param, response_length_computed, response, select_time);
    if (ret >= 0)
    {
        /* GOOD RESPONSE */
        int query_nb_value;
        int response_nb_value;

        /* The number of values is returned if it's corresponding
        * to the query */
        switch (response[offset])
        {
        case FC_READ_COIL_STATUS:
        case FC_READ_INPUT_STATUS:
            query_nb_value = (query[offset + 3] << 8) + query[offset + 4];
            response_nb_value = response[offset + 1];
            break;
        default:
            /* 1 Write functions & others */
            query_nb_value = response_nb_value = 1;
        }

        if (query_nb_value == response_nb_value)
        {
            ret = response_nb_value;
        }
        else
        {
            char *s_error = (char*) malloc (64 * sizeof (char));
            sprintf (s_error, "Quantity not corresponding to the query (%d != %d)", response_nb_value, query_nb_value);
            ret = INVALID_DATA;
            error_treat (mb_param, ret, s_error);
            free (s_error);
        }
    }
    else if (ret == MB_EXCEPTION)
    {
        /* CRC must be checked here (not done in receive_msg) */
        if (mb_param->type_com == RTU)
        {
            ret = check_crc16 (mb_param, response, EXCEPTION_RESPONSE_LENGTH_RTU);
            if (ret < 0)
                return ret;
        }

        /* Check for exception response.
        0x80 + function is stored in the exception
        response. */
        if (0x80 + query[offset] == response[offset])
        {
            int exception_code = response[offset + 1];
            // FIXME check test
            if (exception_code < NB_TAB_ERROR_MSG)
            {
                error_treat (mb_param, -exception_code, TAB_ERROR_MSG[response[offset + 1]]);
                /* RETURN THE EXCEPTION CODE */
                /* Modbus error code is negative */
                return -exception_code;
            }
            else
            {
                /* The chances are low to hit this
                case but it can avoid a vicious
                segfault */
                char *s_error = (char*) malloc (64 * sizeof (char));
                sprintf (s_error,
                         "Invalid exception code %d",
                         response[offset + 1]);
                error_treat (mb_param, INVALID_EXCEPTION_CODE, s_error);
                free (s_error);
                return INVALID_EXCEPTION_CODE;
            }
        }
    }
    else if (ret == SELECT_TIMEOUT)
    {
        error_treat (mb_param, ret, "modbus_receive");
    }

    return ret;
}

/* Reads the data from a modbus slave and put that data into an array */
static int read_registers (modbus_param_t *mb_param, int function, int start_addr,
                           int nb, uint8_t *data_dest, int select_time)
{
    int ret;
    int query_length;
    uint8_t query[MIN_QUERY_LENGTH];
    uint8_t response[MAX_MESSAGE_LENGTH * 4];

    if (nb > MAX_REGISTERS)
    {
        fprintf (stderr, "ERROR Too many holding registers requested (%d > %d)\n", nb, MAX_REGISTERS);
        return INVALID_DATA;
    }

    query_length = build_query_basis (mb_param, function, start_addr, nb, query);

    ret = modbus_send (mb_param, query, query_length);
    if (ret > 0)
    {
        int i;
        int offset;

        ret = modbus_receive (mb_param, query, response, select_time);

        offset = TAB_HEADER_LENGTH[mb_param->type_com];

        /* If ret is negative, the loop is jumped ! */
        for (i = 0; i < ret; i++)
        {
            data_dest[i] = (uint8_t) (response[offset + 2 + i]);
        }
    }

    return ret;
}

/* Turns ON or OFF a single coil in the slave device */
int force_single_coil_RPC3FxC (modbus_param_t *mb_param, int coil_addr, int state, int select_time)
{
    int status;

    status = force_single_coil (mb_param, coil_addr, state, select_time);

    return status;
}

int read_coil_status_RPC3FxC (modbus_param_t *mb_param, int start_addr,
                              int nb, uint8_t *data_dest, int select_time)
{
    int status;

    if (nb > MAX_REGISTERS)
    {
        fprintf (stderr,
                 "ERROR Too many holding registers requested (%d > %d)\n",
                 nb, MAX_REGISTERS);
        return INVALID_DATA;
    }
    status = read_registers (mb_param, FC_READ_COIL_STATUS, start_addr, nb, data_dest, select_time);
    return status;
}

