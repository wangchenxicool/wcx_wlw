/*
 * _  _ ____ ___  ___  _  _ ____    ___ ____ ____ ___
 * |\/| |  | |  \ |__] |  | [__      |  |___ [__   |
 * |  | |__| |__/ |__] |__| ___]     |  |___ ___]  |
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
//#include <time.h>
#include <sys/timeb.h>

#include "modbus.h"
#include "modbus_LD_B10_220.h"
#include "INIFileOP.h"

#define SLAVE         0x01

static int SPACE_TIME = 50;
static int STEP_MODE = 0;
static int COUNTS = 1;

void print_usage (const char *prog)
{
    printf ("\nUsage: < %s  串口号  slave,coil_addr,coil_count,coil_status, >\n\n", prog);
    exit (1);
}

void parse_opts (int argc, char *argv[])
{
    int ret, ch;
    opterr = 0;
    char key_value[128];

    SPACE_TIME = 50;
    STEP_MODE = 0;
    COUNTS = 1;
    while ( (ch = getopt (argc, argv, "s:cn:h")) != EOF)
    {
        switch (ch)
        {
        case 's':
            SPACE_TIME = atoi (optarg);
            break;
        case 'c':
            STEP_MODE = 1;
            break;
        case 'n':
            COUNTS = atoi (optarg);
            break;
        case 'h':
        case '?':
        default:
            print_usage (argv[0]);
        }
    }
}

int main (int argc, char *argv[])
{
    int i, ret;
    uint8_t *tab_coils;
    uint32_t *tab_registers;
    modbus_param_t mb_param;

    struct timeb time_send; //记录发送时间
    struct timeb time_rcv; //记录接收时间
    float time_use; //记录modbus采集使用时间

    if (argc < 3)
    {
        print_usage (argv[0]);
    }

    /* get the file node from config file */
    int serial = (unsigned int) strtol (argv[1], NULL, 10);
    char key_value[256]; /* file node */
    memset (key_value, 0, sizeof (key_value));
    switch (serial)
    {
    case 1:
        ret = ConfigGetKey ("config", "Serial:0", "file_node", key_value);
        break;
    case 2:
        ret = ConfigGetKey ("config", "Serial:1", "file_node", key_value);
        break;
    case 3:
        ret = ConfigGetKey ("config", "Serial:2", "file_node", key_value);
        break;
    case 4:
        ret = ConfigGetKey ("config", "Serial:3", "file_node", key_value);
        break;
    case 5:
        ret = ConfigGetKey ("config", "Serial:4", "file_node", key_value);
        break;
    case 6:
        ret = ConfigGetKey ("config", "Serial:5", "file_node", key_value);
        break;
    case 7:
        ret = ConfigGetKey ("config", "Serial:6", "file_node", key_value);
        break;
    default:
        printf ("serial no error!\n");
        exit (1);
    }
    if (ret != 0)
    {
        printf ("config error!\n");
        exit (1);
    }

    /* RTU parity : none, even, odd */
    modbus_init_rtu (&mb_param, key_value, 9600, "none", 8, 1, SLAVE);
    modbus_set_debug (&mb_param, TRUE);
    if (modbus_connect (&mb_param) == -1)
    {
        perror ("[modbus_connect]");
        exit (1);
    }

    /* Allocate and initialize the different memory spaces */
    tab_coils = (uint8_t *) malloc (512 * sizeof (uint8_t));
    memset (tab_coils, 0, 512 * sizeof (uint8_t));
    tab_registers = (uint32_t *) malloc (512 * sizeof (uint32_t));
    memset (tab_registers, 0, 512 * sizeof (uint32_t));

    //--->填写modbus请求帧
    uint8_t query[512];
    int query_length = 0;
    char value[16];
    const char *lps = NULL, *lpe = NULL;
    lpe = lps = argv[2];
    i = 0;
    while (lps)
    {
        lps = strchr (lps, ',');
        if (lps)
        {
            memset (value, 0, sizeof (value));
            strncpy (value, lpe, lps - lpe);
            query[i++] = (unsigned char) strtol (value, NULL, 10);
            lps += 1;
            lpe = lps;
        }
    }
    query_length = i;

    parse_opts (argc, argv);

    modbus_set_slave (&mb_param, query[0]);

    //---> read_coil_status
    ret = read_coil_status_LD_B10_220 (&mb_param, tab_coils, 2000);
    for (i = 0; i < ret; i++)
    {
        printf ("coil%d: %d\n", i, tab_coils[i]);
    }

    //---> read_holding_registers
    int start_addr = query[2] << 8 | query[3];
    int nb = query[4] << 8 | query[5];
    ret = read_holding_registers_LD_B10_220 (&mb_param,  start_addr, nb, tab_registers, 2000);
    for (i = 0; i < ret; i++)
    {
        printf ("register%d: %d\n", i, tab_registers[i]);
    }

    free (tab_coils);
    free (tab_registers);
    modbus_close (&mb_param);

    return 0;
}

