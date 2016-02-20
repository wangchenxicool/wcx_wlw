/*******************************************************
___  _____   _____  ___ _____  _   _  _   _ 
|  \/  |\ \ / /|  \/  ||  ___|| \ | || | | |
| .  . | \ V / | .  . || |__  |  \| || | | |
| |\/| |  \ /  | |\/| ||  __| | . ` || | | |
| |  | |  | |  | |  | || |___ | |\  || |_| |
\_|  |_/  \_/  \_|  |_/\____/ \_| \_/ \___/ 
                                            
*******************************************************/

#include <stdio.h>
#include <stdlib.h>
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
#include <string.h>
#include <ctype.h>

#include "myconio.h"


/* getch()���� */
int getch()
{
    char ch;
    struct termios save, ne;

    /* �趨�ն� */
    ioctl (0, TCGETS, &save);
    ioctl (0, TCGETS, &ne);
    ne.c_lflag &= ~ (ECHO | ICANON);
    ioctl (0, TCSETS, &ne);

    /* ���ն˻�ȡ��Ϣ */
    read (0, &ch, 1);

    /* �ָ��ն��趨 */
    ioctl (0, TCSETS, &save);

    return ch;
}

/**
 * @brief
 *
 * @param   menu
 * @param   start_row
 *
 * @return
 */
int menu (const char *menu[], int start_row)
{
    register int    i;
    static int      flag = 1;
    int             item_count;
    char            *title;
    int             key_val;
    int             pointer = 0;
    char            item[100];

    if (flag)
    {
        flag = 0;
        pointer = 0;
    }

    /* ����˵������ */
    item_count = 0;
    while (1)
    {
        if (menu[item_count] != NULL)
            item_count++;
        else
            break;
    }

    /* �����Ļ */
    system ("clear");

    /* ���� menu ѭ�� */
    while (1)
    {
        /* ��ʾװ��ͷ */
        printf ("\n\n");
        puts ("\033[0;32;40m �u�������� �q�q �Шr Have A Good Day! ^_^  '��'-'k', '��'-'j'\033[0m");
        puts ("\033[0;32;40m����������t------------------------------------------------\033[0m");

        /* ��ʾ���� */
        for (i = 0; i < start_row; i++)
        {
            printf ("\n");
        }

        title = (char*) malloc (strlen ( (const char*) menu[0]) * sizeof (char) + 40 * sizeof (char));

        if (title == NULL)
        {
            perror ("malloc");
        }

        sprintf (title, "\033[0;31;44m%s\033[0m", menu[0]);
        puts (title);
        free (title);

        /* ��ʾmenu */
        for (i = 1; i < item_count; i++)
        {
            if (i == pointer + 2)
            {
                char buff[200];
                sprintf (buff, "\033[7;30;43m%s\033[0m", menu[i]);
                puts (buff);
            }
            else
            {
                puts (menu[i]);
            }
        }

        /* ��ȡ������Ϣ,������ */
        tcflush (0, TCIFLUSH);   //��մ��ڻ���
        key_val = getch();
        switch (key_val)
        {
        case 'q':
            return -1;
        case 'k':
            if (pointer > 0)
                pointer--;
            else
                pointer = item_count - 4;
            system ("clear");
            break;
        case 'j':
            pointer++;
            pointer %= (item_count - 3);
            system ("clear");
            break;
        case KEY_ENTER:
            if (pointer == item_count - 4)
                pointer = -1;
            return pointer;
        default:
            if (isdigit (key_val))
            {
                if ( (key_val - '0' >= 0) && (key_val - '0' <= item_count - 5))
                {
                    pointer = key_val - '0'; /* �ַ� �� ��ֵ */
                    return pointer;
                }
            }

            /* �����Ļ */
            system ("clear");
            pointer = 0;
            printf ("key_default! The key_val: %d", key_val);
        }
    }
}

/* ��ȡĿ¼��Ϣ���� */
char ** dirGetInfo (const char *pathname)
{
    char **filenames;
    char no[20];
    DIR *dir;
    struct dirent *ent;
    int n = 0;

    filenames = (char **) malloc (sizeof (char*));
    filenames[0] = NULL;

    dir = opendir (pathname);

    if (!dir)
    {
        return filenames;
    }

    while ( (ent = readdir (dir)))
    {
        filenames = (char**) realloc (filenames, sizeof (char*) * (n + 1));
        filenames[n] = (char*) malloc (strlen (ent->d_name) * sizeof (char) + 20 * sizeof (char));
        sprintf (no, "   [%2d] ", n - 2);
        strcpy (filenames[n], no);
        strcat (filenames[n], ent->d_name);
        n++;
    }  /* end while */

    closedir (dir);

    free (filenames[0]);
    filenames[0] = strdup ("           <file list>          ");
    free (filenames[1]);
    filenames[1] = strdup ("--------------------------------");
    filenames = (char **) realloc (filenames, sizeof (char*) * (n + 1));
    filenames[n++] = strdup ("   [ q] �˳�");
    filenames = (char **) realloc (filenames, sizeof (char*) * (n + 1));
    filenames[n++] = strdup ("--------------------------------");
    filenames = (char **) realloc (filenames, sizeof (char*) * (n + 1));
    filenames[n] = NULL;

    return filenames;
}

