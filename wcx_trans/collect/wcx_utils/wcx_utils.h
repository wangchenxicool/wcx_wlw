#ifndef WCX_UTILS
#define WCX_UTILS

#define LOG_NEW           0x01      // 00000001
#define LOG_APPEND        0x02      // 00000010
#define LOG_TIME          0x04      // 00000100
#define LOG_NOTIME        0x08      // 00001000

//#ifdef __cplusplus
//extern "C" {
//#endif [> __cplusplus <]

int getch();
void MySleep (long int s, long int us);
size_t strlcpy (char *dest, const char *src, size_t len);
size_t strlcat (char *dest, const char *src, size_t len);
unsigned long StrToHex (char *str);
unsigned int CallCrc16 (unsigned char *p, unsigned char Len);
int  GetCsvFileCol (const char *source, const int nCsvFileCol, char *pCsvFileCol);
void write_to_log (const char *path, const char *file_name, const char *msg, char write_to_log_flg);
void write_to_log2 (char *file_name, const char *msg, char flg);

//#ifdef __cplusplus
//}
//#endif

#endif

