#ifndef _MYCONIO_H
#define _MYCONIO_H

#define KEY_UP 65
#define KEY_DOWN 66
#define KEY_ENTER 10

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    int getch();
    int menu (const char **menu, int start_row);
    char ** dirGetInfo (const char *pathname);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

