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
    "     <IEC104 MASTER TEST>     ",
    "═══════════════",
    "     [0] 发送总召唤激活请求命令...",
    "     [1] 时钟同步激活请求命令...",
    "     [2] 双点遥控命令, 合...",
    "     [3] 双点遥控命令, 开...",
    "     [4] ?????????????????...",
    "     [5] ?????????????????...",
    "     [6] ?????????????????...",
    "     [7] ?????????????????...",
    "     [8] 关闭显示...",
    "     [9] 打开显示...",
    "     [q] 退出菜单模式",
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
    char str[INET_ADDRSTRLEN];
    socklen_t servaddr_len;

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
            printf ("【发送总召唤激活请求命令】");
            strcpy (buf, "cmd0");
            break;
        case 1:
            printf ("【时钟同步激活请求命令】");
            strcpy (buf, "cmd1");
            break;
        case 2:
            printf ("【双点遥控命令, 合】");
            strcpy (buf, "cmd2");
            break;
        case 3:
            printf ("【双点遥控命令, 开】");
            strcpy (buf, "cmd3");
            break;
        case 4:
            printf ("【读数据】");
            strcpy (buf, "cmd4");
            break;
        case 5:
            printf ("【从文件读】");
            strcpy (buf, "cmd5");
            break;
        case 6:
            printf ("【通道检查】");
            strcpy (buf, "cmd6");
            break;
        case 7:
            strcpy (buf, "cmd7");
            break;
        case 8:
            strcpy (buf, "cmd8");
            printf ("【关闭回显】");
            break;
        case 9:
            printf ("【打开回显】");
            strcpy (buf, "cmd9");
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
