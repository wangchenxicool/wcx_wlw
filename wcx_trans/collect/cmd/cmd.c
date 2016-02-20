#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "myconio.h"

#define MAXLINE 80
#define SERV_PORT 55555

static const char *menu_main[] =
{
    "     <Collect Cmd Menu>     ",
    "═══════════════",
    "     [01] mutex   com1...",
    "     [02] unmutex com1....",
    "     [03] mutex   com2...",
    "     [04] unmutex com2....",
    "     [05] mutex   com3...",
    "     [06] unmutex com3....",
    "     [07] mutex   com4...",
    "     [08] unmutex com4....",
    "     [09] mutex   com5...",
    "     [10] unmutex com5....",
    "     [11] mutex   com6...",
    "     [12] unmutex com6....",
    "     [13] mutex   com7...",
    "     [14] unmutex com7....",
    "═══════════════",
    NULL,
};

/**
 * @brief    
 *
 * @return  
 */
int main (int argc, char *argv[])
{
    struct sockaddr_in servaddr;
    int sockfd, ret;
    char buf[MAXLINE];
    //char str[INET_ADDRSTRLEN];
    //socklen_t servaddr_len;

    sockfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror ("socket error");
        exit (1);
    }

    bzero (&servaddr, sizeof (servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton (AF_INET, "127.0.0.1", &servaddr.sin_addr);
    servaddr.sin_port = htons (SERV_PORT);

    /* 进入主循环 */
    while (1)
    {
        memset (buf, 0, sizeof (buf));
        /*printf ("【 主菜单 】");*/
        switch (menu (menu_main, 3))
        {
        case -1:
            // 退出
            printf ("\n◆have exit <main menu>\n");
            goto exit;
        case 0:
            printf ("%s", menu_main[2]);
            strcpy (buf, "0lock");
            break;
        case 1:
            printf ("%s", menu_main[3]);
            strcpy (buf, "0free");
            break;
        case 2:
            printf ("%s", menu_main[4]);
            strcpy (buf, "1lock");
            break;
        case 3:
            printf ("%s", menu_main[5]);
            strcpy (buf, "1free");
            break;
        case 4:
            printf ("%s", menu_main[6]);
            strcpy (buf, "2lock");
            break;
        case 5:
            printf ("%s", menu_main[7]);
            strcpy (buf, "2free");
            break;
        case 6:
            printf ("%s", menu_main[8]);
            strcpy (buf, "3lock");
            break;
        case 7:
            printf ("%s", menu_main[9]);
            strcpy (buf, "3free");
            break;
        case 8:
            printf ("%s", menu_main[10]);
            strcpy (buf, "4lock");
            break;
        case 9:
            printf ("%s", menu_main[11]);
            strcpy (buf, "4free");
            break;
        case 10:
            printf ("%s", menu_main[10]);
            strcpy (buf, "5lock");
            break;
        case 11:
            printf ("%s", menu_main[11]);
            strcpy (buf, "5free");
            break;
        case 12:
            printf ("%s", menu_main[10]);
            strcpy (buf, "6lock");
            break;
        case 13:
            printf ("%s", menu_main[11]);
            strcpy (buf, "6free");
            break;
        default:
            printf ("menu-default!");
            continue;
        }
        
        ret = sendto (sockfd, buf, strlen (buf), MSG_DONTWAIT, (struct sockaddr *) &servaddr, sizeof (servaddr));
        if (ret == -1)
        {
            perror ("sendto error");
            exit (1);
        }
#if 0
        ret = recvfrom (sockfd, buf, MAXLINE, MSG_DONTWAIT, NULL, 0);
        if (ret == -1)
        {
            perror ("recvfrom error");
            exit (1);
        }
#endif
    }
   
exit: 
    close (sockfd);
    return 0;
}
