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

#include "com.h"

#define DEBUG

#ifdef DEBUG
#define wprintf(format, arg...)  \
    printf( format , ##arg)
#else
#define wprintf(format, arg...)
#endif

/* Treats errors and flush or close connection if necessary */
void error_treat (iec103_com_t *mb_param, int code, const char *string)
{
    /*fprintf ( stderr, "\nerror_treat: ERROR %s (%0X)\n", string, -code );*/
    if (mb_param->debug)
    {
        wprintf ("\033[31;40;1m \nerror_treat: %s (%0X)\n\033[0m", string, -code);
    }
    //write_to_log ( "/var/log", "wcx_modbus.log", string, true );

    if (mb_param->error_handling == FLUSH_OR_CONNECT_ON_ERROR)
    {
        switch (code)
        {
        case INVALID_DATA:
        case INVALID_CRC:
        case INVALID_EXCEPTION_CODE:
            iec103_flush (mb_param);
            break;
        case SELECT_FAILURE:
        case SOCKET_FAILURE:
        case CONNECTION_CLOSED:
            iec103_close (mb_param);
            iec103_connect (mb_param);
            break;
        default:
            /* NOP */
            break;
        }
    }
}

static void iec103_flush (iec103_com_t *mb_param)
{
    if (mb_param->type_com == RTU)
        tcflush (mb_param->fd, TCIOFLUSH);
    else
    {
        int ret;
        do
        {
            /* Extract the garbage from the socket */
            char devnull[MAX_ADU_LENGTH_TCP];
#if (!HAVE_DECL___CYGWIN__)
            ret = recv (mb_param->fd, devnull, MAX_ADU_LENGTH_TCP, MSG_DONTWAIT);
#else
            /* On Cygwin, it's a bit more complicated to not wait */
            fd_set rfds;
            struct timeval tv;

            tv.tv_sec = 0;
            tv.tv_usec = 0;
            FD_ZERO (&rfds);
            FD_SET (mb_param->fd, &rfds);
            ret = select (mb_param->fd + 1, &rfds, NULL, NULL, &tv);
            if (ret > 0)
            {
                ret = recv (mb_param->fd, devnull, MAX_ADU_LENGTH_TCP, 0);
            }
            else if (ret == -1)
            {
                /* error_treat() doesn't call iec103_flush() in
                this case (avoid infinite loop) */
                error_treat (mb_param, SELECT_FAILURE, "Select failure");
            }
#endif
            if (mb_param->debug && ret > 0)
            {
                wprintf ("%d bytes flushed\n", ret);
            }
        }
        while (ret > 0);
    }
}

/* Sends a query/response over a serial or a TCP communication */
int iec103_send (iec103_com_t *mb_param, uint8_t *query, int query_length)
{
    int i;
    int ret;
    uint16_t s_crc;

    if (mb_param->debug)
    {
        wprintf ("\033[34;40;1m \nsend:\033[0m");
        for (i = 0; i < query_length; i++)
            wprintf (" %.2X", query[i]);
        wprintf ("\n");
    }

    if (mb_param->type_com == RTU)
    {
        //tcflush ( mb_param->fd, TCIOFLUSH );
        ret = write (mb_param->fd, query, query_length);
    }
    else
        ret = send (mb_param->fd, query, query_length, MSG_NOSIGNAL);

    /* Return the number of bytes written (0 to n)
    or SOCKET_FAILURE on error */
    if ( (ret == -1) || (ret != query_length))
    {
        ret = SOCKET_FAILURE;
        error_treat (mb_param, ret, "iec103_send: Write socket failure");
    }

    return ret;
}

#define WAIT_DATA()                                                                \
{                                                                                  \
    while ((select_ret = select(mb_param->fd+1, &rfds, NULL, NULL, &tv)) == -1) {  \
            if (errno == EINTR) {                                                  \
                    fprintf(stderr, "A non blocked signal was caught\n");          \
                    /* Necessary after an error */                                 \
                    FD_ZERO(&rfds);                                                \
                    FD_SET(mb_param->fd, &rfds);                                   \
            } else {                                                               \
                    error_treat(mb_param, SELECT_FAILURE, "Select failure");       \
                    return SELECT_FAILURE;                                         \
            }                                                                      \
    }                                                                              \
                                                                                   \
    if (select_ret == 0) {                                                         \
            /* Timeout */                                                          \
            /* Call to error_treat is done later to manage exceptions */           \
            if ( mb_param->debug )                                                 \
                wprintf ( "\n" );                                                  \
            return SELECT_TIMEOUT;                                                 \
    }                                                                              \
}

/* Waits a reply from a modbus slave or a query from a modbus master.
   This function blocks if there is no replies (3 timeouts).

   In
   - msg_length_computed must be set to MSG_LENGTH_UNDEFINED if undefined

   Out
   - msg is an array of uint8_t to receive the message

   On success, return the number of received characters. On error, return
   a negative value.
*/
int receive_msg (iec103_com_t *mb_param, int msg_length_computed, uint8_t *msg, int select_time)
{
    int select_ret;
    int read_ret;
    fd_set rfds;
    struct timeval tv;
    int length_to_read;
    uint8_t *p_msg;
    enum { FUNCTION, BYTE, COMPLETE };
    int i;

    int msg_length = 0;

    //if (mb_param->debug)
    //{
    //if (msg_length_computed == MSG_LENGTH_UNDEFINED)
    //wprintf ("\nWaiting for a message...\n");
    //else
    //wprintf ("\nWaiting for a message (%d bytes)...\n", msg_length_computed);
    //}

    /* Add a file descriptor to the set */
    FD_ZERO (&rfds);
    FD_SET (mb_param->fd, &rfds);

    tv.tv_sec = 0;
    tv.tv_usec = select_time * 1000;

    length_to_read = msg_length_computed;

    select_ret = 0;
    WAIT_DATA();

    p_msg = msg;
    //if (mb_param->debug)
    //{
    //wprintf ("\033[32;40;1m \nrcv:\033[0m");
    //}
    while (select_ret)
    {
        if (mb_param->type_com == RTU)
            read_ret = read (mb_param->fd, p_msg, length_to_read);
        else
            read_ret = recv (mb_param->fd, p_msg, length_to_read, 0);
        if (read_ret == 0)
        {
            return CONNECTION_CLOSED;
        }
        else if (read_ret < 0)
        {
            /* The only negative possible value is -1 */
            error_treat (mb_param, SOCKET_FAILURE, "receive_msg: Read socket failure");
            return SOCKET_FAILURE;
        }

        /* Sums bytes received */
        msg_length += read_ret;

        /* Display the hex code of each character received */
        if (mb_param->debug)
        {
            for (i = 0; i < read_ret; i++)
                wprintf ("%.2X ", p_msg[i]);
        }

        if (msg_length < msg_length_computed)
        {
            /* Message incomplete */
            length_to_read = msg_length_computed - msg_length;
        }
        else
        {
            length_to_read = 0;
        }

        /* Moves the pointer to receive other data */
        p_msg = & (p_msg[read_ret]);

        if (length_to_read > 0)
        {
            /* If no character at the buffer wait
            TIME_OUT_END_OF_TRAME before to generate an error. */
            tv.tv_sec = 0;
            tv.tv_usec = TIME_OUT_END_OF_TRAME;
            WAIT_DATA();
        }
        else
        {
            /* All chars are received */
            select_ret = FALSE;
        }
    } /* end while */

    //if (mb_param->debug)
    //{
    //wprintf ("\n");
    //}

    return msg_length;
}

int iec103_receive (iec103_com_t *mb_param, uint8_t *rcv_buf, int select_time)
{
    int ret;
    int rcv_len = 0;

    if (mb_param->debug)
    {
        //wprintf ("\nWaiting for a message...\n");
        wprintf ("\033[32;40;1m \nrcv: \033[0m");
    }
    select_time = 2000;
    ret = receive_msg (mb_param, 1, rcv_buf,  select_time);
    if (ret == 1)
    {
        if (rcv_buf[0] == 0x10)
        {
            ret = receive_msg (mb_param, 4, (uint8_t*) &rcv_buf[1],  select_time);
            if (ret == 4)
            {
                if (rcv_buf[4] == 0x16)
                    rcv_len = 5;
                else
                {
                    wprintf ("!!!!!!!!!!!!!!iec103_receive: need_read:0x16, ret:%d\n", rcv_buf[4]);
                    rcv_len = -1;
                }
            }
            else
            {
                wprintf ("!!!!!!!!!!!!!!iec103_receive:read_count:4 ret:%d\n", ret);
                rcv_len = -1;
            }
        }
        else if (rcv_buf[0] == 0x68)
        {
            ret = receive_msg (mb_param, 5, (uint8_t*) &rcv_buf[1],  select_time);
            if (ret == 5)
            {
                if (rcv_buf[1] = rcv_buf[2] && rcv_buf[3] == 0x68)
                {
                    ret = receive_msg (mb_param, rcv_buf[2], (uint8_t*) &rcv_buf[6],  select_time);
                    rcv_len = rcv_buf[2] + 6;
                }
                else
                {
                    wprintf ("!!!!!!!!!!!!!!iec103_receive:need_read:0x68 ret:%d, %d, %d\n", rcv_buf[1], rcv_buf[2], rcv_buf[3]);
                    rcv_len = -1;
                }
            }
            else
            {
                wprintf ("!!!!!!!!!!!!!!iec103_receive:read_count:5 ret:%d\n", ret);
                rcv_len = -1;
            }
        }
        else
        {
            wprintf ("!!!!!!!!!!!!!!iec103_receive:rcv_default:%d\n", rcv_buf[0]);
            rcv_len = -1;
        }
    }
    else
    {
        wprintf ("!!!!!!!!!!!!!!iec103_receive:read_count:1 ret:%d\n", ret);
        rcv_len = -1;
    }
    if (mb_param->debug)
    {
        wprintf ("\n");
    }

    return rcv_len;
}

/* Initializes the iec103_com_t structure for RTU
   - device: "/dev/ttyS0"
   - baud:   9600, 19200, 57600, 115200, etc
   - parity: "even", "odd" or "none"
   - data_bits: 5, 6, 7, 8
   - stop_bits: 1, 2
*/
void iec103_init_rtu (iec103_com_t *mb_param, const char *device,
                      int baud, const char *parity, int data_bit,
                      int stop_bit)
{
    memset (mb_param, 0, sizeof (iec103_com_t));
    strcpy (mb_param->device, device);
    mb_param->baud = baud;
    strcpy (mb_param->parity, parity);
    mb_param->debug = FALSE;
    mb_param->data_bit = data_bit;
    mb_param->stop_bit = stop_bit;
    mb_param->type_com = RTU;
    mb_param->error_handling = FLUSH_OR_CONNECT_ON_ERROR;
}

/* Initializes the iec103_com_t structure for TCP.
   - ip : "192.168.0.5"
   - port : 1099

   Set the port to MODBUS_TCP_DEFAULT_PORT to use the default one
   (502). It's convenient to use a port number greater than or equal
   to 1024 because it's not necessary to be root to use this port
   number.
*/
void iec103_init_tcp (iec103_com_t *mb_param, const char *ip, int port)
{
    memset (mb_param, 0, sizeof (iec103_com_t));
    strncpy (mb_param->ip, ip, sizeof (char) * 16);
    mb_param->port = port;
    mb_param->type_com = TCP;
    mb_param->error_handling = FLUSH_OR_CONNECT_ON_ERROR;
}

/* Sets up a serial port for RTU communications */
static int iec103_connect_rtu (iec103_com_t *mb_param)
{
    struct termios tios;
    speed_t speed;

    if (mb_param->debug)
    {
        fprintf (stderr, "Opening %s at %d bauds (%s)\n", mb_param->device, mb_param->baud, mb_param->parity);
    }

    /* The O_NOCTTY flag tells UNIX that this program doesn't want
       to be the "controlling terminal" for that port. If you
       don't specify this then any input (such as keyboard abort
       signals and so forth) will affect your process

       Timeouts are ignored in canonical input mode or when the
       NDELAY option is set on the file via open or fcntl */
    mb_param->fd = open (mb_param->device, O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL);
    if (mb_param->fd < 0)
    {
        fprintf (stderr, "ERROR Can't open the device %s (%s)\n", mb_param->device, strerror (errno));
        return -1;
    }

    /* Save */
    tcgetattr (mb_param->fd, & (mb_param->old_tios));

    memset (&tios, 0, sizeof (struct termios));

    /* C_ISPEED     Input baud (new interface)
       C_OSPEED     Output baud (new interface)
    */
    switch (mb_param->baud)
    {
    case 110:
        speed = B110;
        break;
    case 300:
        speed = B300;
        break;
    case 600:
        speed = B600;
        break;
    case 1200:
        speed = B1200;
        break;
    case 2400:
        speed = B2400;
        break;
    case 4800:
        speed = B4800;
        break;
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    default:
        speed = B9600;
        fprintf (stderr, "WARNING Unknown baud rate %d for %s (B9600 used)\n", mb_param->baud, mb_param->device);
    }

    /* Set the baud rate */
    if ( (cfsetispeed (&tios, speed) < 0) || (cfsetospeed (&tios, speed) < 0))
    {
        perror ("cfsetispeed/cfsetospeed\n");
        return -1;
    }

    /* C_CFLAG      Control options
       CLOCAL       Local line - do not change "owner" of port
       CREAD        Enable receiver
    */
    tios.c_cflag |= (CREAD | CLOCAL);
    /* CSIZE, HUPCL, CRTSCTS (hardware flow control) */

    /* Set data bits (5, 6, 7, 8 bits)
       CSIZE        Bit mask for data bits
    */
    tios.c_cflag &= ~CSIZE;
    switch (mb_param->data_bit)
    {
    case 5:
        tios.c_cflag |= CS5;
        break;
    case 6:
        tios.c_cflag |= CS6;
        break;
    case 7:
        tios.c_cflag |= CS7;
        break;
    case 8:
    default:
        tios.c_cflag |= CS8;
        break;
    }

    /* Stop bit (1 or 2) */
    if (mb_param->stop_bit == 1)
        tios.c_cflag &= ~ CSTOPB;
    else /* 2 */
        tios.c_cflag |= CSTOPB;

    /* PARENB       Enable parity bit
       PARODD       Use odd parity instead of even */
    if (strncmp (mb_param->parity, "none", 4) == 0)
    {
        tios.c_cflag &= ~ PARENB;
    }
    else if (strncmp (mb_param->parity, "even", 4) == 0)
    {
        tios.c_cflag |= PARENB;
        tios.c_cflag &= ~ PARODD;
    }
    else
    {
        /* odd */
        tios.c_cflag |= PARENB;
        tios.c_cflag |= PARODD;
    }

    /* Read the man page of termios if you need more information. */

    /* This field isn't used on POSIX systems
       tios.c_line = 0;
    */

    /* C_LFLAG      Line options

       ISIG Enable SIGINTR, SIGSUSP, SIGDSUSP, and SIGQUIT signals
       ICANON       Enable canonical input (else raw)
       XCASE        Map uppercase \lowercase (obsolete)
       ECHO Enable echoing of input characters
       ECHOE        Echo erase character as BS-SP-BS
       ECHOK        Echo NL after kill character
       ECHONL       Echo NL
       NOFLSH       Disable flushing of input buffers after
       interrupt or quit characters
       IEXTEN       Enable extended functions
       ECHOCTL      Echo control characters as ^char and delete as ~?
       ECHOPRT      Echo erased character as character erased
       ECHOKE       BS-SP-BS entire line on line kill
       FLUSHO       Output being flushed
       PENDIN       Retype pending input at next read or input char
       TOSTOP       Send SIGTTOU for background output

       Canonical input is line-oriented. Input characters are put
       into a buffer which can be edited interactively by the user
       until a CR (carriage return) or LF (line feed) character is
       received.

       Raw input is unprocessed. Input characters are passed
       through exactly as they are received, when they are
       received. Generally you'll deselect the ICANON, ECHO,
       ECHOE, and ISIG options when using raw input
    */

    /* Raw input */
    tios.c_lflag &= ~ (ICANON | ECHO | ECHOE | ISIG);

    /* C_IFLAG      Input options

       Constant     Description
       INPCK        Enable parity check
       IGNPAR       Ignore parity errors
       PARMRK       Mark parity errors
       ISTRIP       Strip parity bits
       IXON Enable software flow control (outgoing)
       IXOFF        Enable software flow control (incoming)
       IXANY        Allow any character to start flow again
       IGNBRK       Ignore break condition
       BRKINT       Send a SIGINT when a break condition is detected
       INLCR        Map NL to CR
       IGNCR        Ignore CR
       ICRNL        Map CR to NL
       IUCLC        Map uppercase to lowercase
       IMAXBEL      Echo BEL on input line too long
    */
    if (strncmp (mb_param->parity, "none", 4) == 0)
    {
        tios.c_iflag &= ~INPCK;
    }
    else
    {
        tios.c_iflag |= INPCK;
    }

    /* Software flow control is disabled */
    tios.c_iflag &= ~ (IXON | IXOFF | IXANY);

    /* C_OFLAG      Output options
       OPOST        Postprocess output (not set = raw output)
       ONLCR        Map NL to CR-NL

       ONCLR ant others needs OPOST to be enabled
    */

    /* Raw ouput */
    tios.c_oflag &= ~ OPOST;

    /* C_CC         Control characters
       VMIN         Minimum number of characters to read
       VTIME        Time to wait for data (tenths of seconds)

       UNIX serial interface drivers provide the ability to
       specify character and packet timeouts. Two elements of the
       c_cc array are used for timeouts: VMIN and VTIME. Timeouts
       are ignored in canonical input mode or when the NDELAY
       option is set on the file via open or fcntl.

       VMIN specifies the minimum number of characters to read. If
       it is set to 0, then the VTIME value specifies the time to
       wait for every character read. Note that this does not mean
       that a read call for N bytes will wait for N characters to
       come in. Rather, the timeout will apply to the first
       character and the read call will return the number of
       characters immediately available (up to the number you
       request).

       If VMIN is non-zero, VTIME specifies the time to wait for
       the first character read. If a character is read within the
       time given, any read will block (wait) until all VMIN
       characters are read. That is, once the first character is
       read, the serial interface driver expects to receive an
       entire packet of characters (VMIN bytes total). If no
       character is read within the time allowed, then the call to
       read returns 0. This method allows you to tell the serial
       driver you need exactly N bytes and any read call will
       return 0 or N bytes. However, the timeout only applies to
       the first character read, so if for some reason the driver
       misses one character inside the N byte packet then the read
       call could block forever waiting for additional input
       characters.

       VTIME specifies the amount of time to wait for incoming
       characters in tenths of seconds. If VTIME is set to 0 (the
       default), reads will block (wait) indefinitely unless the
       NDELAY option is set on the port with open or fcntl.
    */
    /* Unused because we use open with the NDELAY option */
    tios.c_cc[VMIN] = 0;
    tios.c_cc[VTIME] = 0;

    /*处理未接收字符*/
    tcflush (mb_param->fd, TCIFLUSH);

    if (tcsetattr (mb_param->fd, TCSANOW, &tios) < 0)
    {
        perror ("tcsetattr\n");
        return -1;
    }

    return 0;
}

/* Establishes a modbus TCP connection with a modbus slave */
static int iec103_connect_tcp (iec103_com_t *mb_param)
{
    int ret;
    int option;
    struct sockaddr_in addr;

    mb_param->fd = socket (PF_INET, SOCK_STREAM, 0);
    if (mb_param->fd < 0)
    {
        return mb_param->fd;
    }

    /* Set the TCP no delay flag */
    /* SOL_TCP = IPPROTO_TCP */
    option = 1;
    ret = setsockopt (mb_param->fd, IPPROTO_TCP, TCP_NODELAY, (const void *) &option, sizeof (int));
    if (ret < 0)
    {
        perror ("setsockopt TCP_NODELAY");
        close (mb_param->fd);
        return ret;
    }

#if (!HAVE_DECL___CYGWIN__)
    /**
     * Cygwin defines IPTOS_LOWDELAY but can't handle that flag so it's
     * necessary to workaround that problem.
     **/
    /* Set the IP low delay option */
    option = IPTOS_LOWDELAY;
    ret = setsockopt (mb_param->fd, IPPROTO_IP, IP_TOS, (const void *) &option, sizeof (int));
    if (ret < 0)
    {
        perror ("setsockopt IP_TOS");
        close (mb_param->fd);
        return ret;
    }
#endif

    if (mb_param->debug)
    {
        wprintf ("Connecting to %s\n", mb_param->ip);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons (mb_param->port);
    addr.sin_addr.s_addr = inet_addr (mb_param->ip);
    ret = connect (mb_param->fd, (struct sockaddr *) &addr, sizeof (struct sockaddr_in));
    if (ret < 0)
    {
        perror ("connect");
        close (mb_param->fd);
        return ret;
    }

    return 0;
}

/* Establishes a modbus connexion.
   Returns 0 on success or -1 on failure. */
int iec103_connect (iec103_com_t *mb_param)
{
    int ret;

    if (mb_param->type_com == RTU)
        ret = iec103_connect_rtu (mb_param);
    else
        ret = iec103_connect_tcp (mb_param);

    return ret;
}

/* Closes the file descriptor in RTU mode */
static void iec103_close_rtu (iec103_com_t *mb_param)
{
    if (tcsetattr (mb_param->fd, TCSANOW, & (mb_param->old_tios)) < 0)
        perror ("tcsetattr");

    close (mb_param->fd);
}

/* Closes the network connection and socket in TCP mode */
static void iec103_close_tcp (iec103_com_t *mb_param)
{
    shutdown (mb_param->fd, SHUT_RDWR);
    close (mb_param->fd);
}

/* Closes a modbus connection */
void iec103_close (iec103_com_t *mb_param)
{
    if (mb_param->type_com == RTU)
        iec103_close_rtu (mb_param);
    else
        iec103_close_tcp (mb_param);
}

/* Activates the debug messages */
void iec103_set_debug (iec103_com_t *mb_param, int boolean)
{
    mb_param->debug = boolean;

    /* Log to stderr. */
    /*logger_add_fp_handler (stderr);*/
}

/* Listens for any query from one or many modbus masters in TCP */
int iec103_slave_listen_tcp (iec103_com_t *mb_param, int nb_connection)
{
    int new_socket;
    int yes;
    struct sockaddr_in addr;

    new_socket = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (new_socket < 0)
    {
        perror ("socket");
        return -1;
    }

    yes = 1;
    if (setsockopt (new_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &yes, sizeof (yes)) < 0)
    {
        perror ("setsockopt");
        close (new_socket);
        return -1;
    }

    memset (&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    /* If the modbus port is < to 1024, we need the setuid root. */
    addr.sin_port = htons (mb_param->port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind (new_socket, (struct sockaddr *) &addr, sizeof (addr)) < 0)
    {
        perror ("bind");
        close (new_socket);
        return -1;
    }

    if (listen (new_socket, nb_connection) < 0)
    {
        perror ("listen");
        close (new_socket);
        return -1;
    }

    return new_socket;
}

int iec103_slave_accept_tcp (iec103_com_t *mb_param, int *socket)
{
    struct sockaddr_in addr;
    socklen_t addrlen;

    addrlen = sizeof (struct sockaddr_in);
again:
    mb_param->fd = accept (*socket, (struct sockaddr *) &addr, &addrlen);

    if (mb_param->fd < 0)
    {
        if ( (errno == ECONNABORTED) || (errno == EINTR))
        {
            wprintf ("Listen Again ...\n");
            goto again;
        }
        else
        {
            perror ("accept");
            close (*socket);
            *socket = 0;
        }
    }
    else
    {
        wprintf ("The client %s is connected\n", inet_ntoa (addr.sin_addr));
    }

    return mb_param->fd;
}

/* Closes a TCP socket */
void iec103_slave_close_tcp (int socket)
{
    shutdown (socket, SHUT_RDWR);
    close (socket);
}

