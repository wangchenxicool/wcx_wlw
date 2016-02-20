#ifndef _IEC103_COM_H_
#define _IEC103_COM_H_

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#if defined(__FreeBSD__ ) && __FreeBSD__ < 5
#include <netinet/in_systm.h>
#endif
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Modbus_Application_Protocol_V1_1b.pdf Chapter 4 Section 1 Page 5:
 *  - RS232 / RS485 ADU = 253 bytes + slave (1 byte) + CRC (2 bytes) = 256 bytes
 *  - TCP MODBUS ADU = 253 bytes + MBAP (7 bytes) = 260 bytes
 */
#define MAX_ADU_LENGTH_TCP        260

/* Time out between trames in microsecond */
#define TIME_OUT_END_OF_TRAME   500000

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef OFF
#define OFF 0
#endif

#ifndef ON
#define ON 1
#endif

/* Protocol exceptions */
#define ILLEGAL_FUNCTION        -0x01
#define ILLEGAL_DATA_ADDRESS    -0x02
#define ILLEGAL_DATA_VALUE      -0x03
#define SLAVE_DEVICE_FAILURE    -0x04
#define SERVER_FAILURE          -0x04
#define ACKNOWLEDGE             -0x05
#define SLAVE_DEVICE_BUSY       -0x06
#define SERVER_BUSY             -0x06
#define NEGATIVE_ACKNOWLEDGE    -0x07
#define MEMORY_PARITY_ERROR     -0x08
#define GATEWAY_PROBLEM_PATH    -0x0A
#define GATEWAY_PROBLEM_TARGET  -0x0B

/* Local */
#define INVALID_DATA            -0x10
#define INVALID_CRC             -0x11
#define INVALID_EXCEPTION_CODE  -0x12

#define SELECT_TIMEOUT          -0x13
#define SELECT_FAILURE          -0x14
#define SOCKET_FAILURE          -0x15
#define CONNECTION_CLOSED       -0x16
#define MB_EXCEPTION            -0x17

/* Internal using */
#define MSG_LENGTH_UNDEFINED -1

typedef enum
{
    RTU = 0, TCP
}
type_com_t;
    
typedef enum 
{ 
    FLUSH_OR_CONNECT_ON_ERROR, NOP_ON_ERROR 
} error_handling_t;

/* This structure is byte-aligned */
typedef struct
{
    /* Descriptor (tty or socket) */
    int fd;
    /* Communication mode: RTU or TCP */
    type_com_t type_com;
    /* Flag debug */
    int debug;
    /* TCP port */
    int port;
    /* Device: "/dev/ttyS0", "/dev/ttyUSB0" or "/dev/tty.USA19*"
       on Mac OS X for KeySpan USB<->Serial adapters this string
       had to be made bigger on OS X as the directory+file name
       was bigger than 19 bytes. Making it 67 bytes for now, but
       OS X does support 256 byte file names. May become a problem
       in the future. */
#ifdef __APPLE_CC__
    char device[64];
#else
    char device[16];
#endif
    /* Bauds: 9600, 19200, 57600, 115200, etc */
    int baud;
    /* Data bit */
    uint8_t data_bit;
    /* Stop bit */
    uint8_t stop_bit;
    /* Parity: "even", "odd", "none" */
    char parity[5];
    /* In error_treat with TCP, do a reconnect or just dump the error */
    uint8_t error_handling;
    /* IP address */
    char ip[16];
    /* Save old termios settings */
    struct termios old_tios;
} iec103_com_t;


/* All functions used for sending or receiving data return:
   - the numbers of values (bits or word) if success (0 or more)
   - less than 0 for exceptions errors
*/

/* Initializes the iec103_com_t structure for RTU.
   - device: "/dev/ttyS0"
   - baud:   9600, 19200, 57600, 115200, etc
   - parity: "even", "odd" or "none"
   - data_bits: 5, 6, 7, 8
   - stop_bits: 1, 2
*/
void iec103_init_rtu (iec103_com_t *mb_param, const char *device,
                      int baud, const char *parity, int data_bit,
                      int stop_bit);

/* Initializes the iec103_com_t structure for TCP.
   - ip: "192.168.0.5"
   - port: 1099

   Set the port to MODBUS_TCP_DEFAULT_PORT to use the default one
   (502). It's convenient to use a port number greater than or equal
   to 1024 because it's not necessary to be root to use this port
   number.
*/
void iec103_init_tcp (iec103_com_t *mb_param, const char *ip_address, int port);

/* Define the slave number.
   The special value MODBUS_BROADCAST_ADDRESS can be used. */
void iec103_set_slave (iec103_com_t *mb_param);

/* Establishes a modbus connexion.
   Returns 0 on success or -1 on failure. */
int iec103_connect (iec103_com_t *mb_param);

/* Closes a modbus connection */
void iec103_close (iec103_com_t *mb_param);

/* Flush the pending request */
static void iec103_flush (iec103_com_t *mb_param);

/* Activates the debug messages */
void iec103_set_debug (iec103_com_t *mb_param, int boolean);

/* Listens for any query from one or many modbus masters in TCP.

   Returns: socket
 */
int iec103_slave_listen_tcp (iec103_com_t *mb_param, int nb_connection);

/* Waits for a connection */
int iec103_slave_accept_tcp (iec103_com_t *mb_param, int *socket);

/* Closes a TCP socket */
void iec103_slave_close_tcp (int socket);

int receive_msg (iec103_com_t *mb_param, int msg_length_computed, uint8_t *msg, int select_time);

/* Sends a query/response over a serial or a TCP communication */
int iec103_send (iec103_com_t *mb_param, uint8_t *query, int query_length);

void error_treat (iec103_com_t *mb_param, int code, const char *string);

int iec103_receive (iec103_com_t *mb_param, uint8_t *response, int select_time);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _MODBUS_H_ */
