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
#include "INIFileOP.h"

#define SLAVE         0x01

static int SPACE_TIME = 50;
static int STEP_MODE = 0;
static int COUNTS = 1;
static int WAIT_TIME = 250;

void print_usage (const char *prog)
{
    printf ("\nUsage: < %s  串口号  Date1,Date2,..., -scnh >\n\n", prog);
    puts ("  -s: modbus 空闲时间\n"
          "  -c: 单步运行\n"
          "  -n: 重复次数\n"
          "  -w: 等待时间\n"
          "  -h: help\n");
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
    while ( (ch = getopt (argc, argv, "w:s:cn:h")) != EOF)
    {
        switch (ch)
        {
        case 's':
            SPACE_TIME = atoi (optarg);
            break;
        case 'w':
            WAIT_TIME = atoi (optarg);
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
    uint8_t *tab_registers;
    modbus_param_t mb_param;

    struct timeb time_send; //记录发送时间
    struct timeb time_rcv; //记录接收时间
    float time_use; //记录modbus采集使用时间

    if (argc < 3)
    {
        print_usage (argv[0]);
    }
    
    /* get collect_demo.bin pid */
    FILE *pp;
    int pid;
    char str[128];
    pp = popen ("pidof collect_demo.bin", "r");
    memset (str, 0, sizeof (str));
    fgets (str, sizeof (str), pp);
    pid = strtol (str, NULL, 10);
    pclose (pp);
    /* if pid > 0, kill it! */
    if (pid > 0)
    {
        //system ("killall -9 watch_dog_test.bin");
        memset (str, 0, sizeof(str));
        sprintf (str, "kill -9 %d", pid);
        system (str);
    }
    /* get collect.bin pid */
    pp = popen ("pidof collect.bin", "r");
    memset (str, 0, sizeof (str));
    fgets (str, sizeof (str), pp);
    pid = strtol (str, NULL, 10);
    pclose (pp);
    /* if pid > 0, kill it! */
    if (pid > 0)
    {
        //system ("killall -9 watch_dog_test.bin");
        memset (str, 0, sizeof(str));
        sprintf (str, "kill -9 %d", pid);
        system (str);
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
    tab_registers = (uint8_t *) malloc (512 * sizeof (uint8_t));
    memset (tab_registers, 0, 512 * sizeof (uint8_t));

    /* 填写modbus请求帧 */
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

    for (i = 0; i < COUNTS; i++)
    {
        printf ("--------------------------------------\n");
        modbus_send (&mb_param, query, query_length);
        rcv_msg (&mb_param, tab_registers, 2000, WAIT_TIME);
        printf ("\n");
        MySleep (0, SPACE_TIME*1000);
        if (STEP_MODE == 1)
        {
            printf ("push enter key to continue...\n");
            getchar ();
        }
    }

    free (tab_registers);
    modbus_close (&mb_param);

    return 0;
}
