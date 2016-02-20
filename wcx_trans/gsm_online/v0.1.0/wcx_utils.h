#ifndef WCX_UTILS
#define WCX_UTILS

size_t strlcpy ( char *dest, const char *src, size_t len );
size_t strlcat ( char *dest, const char *src, size_t len );
unsigned int CallCrc16 ( unsigned char *p, unsigned char Len );
unsigned long StrToHex ( char *str );
int  GetCsvFileCol ( const char *source, const int nCsvFileCol, char *pCsvFileCol );
void MySleep ( long int s, long int us );
int getch();
void write_to_log ( const char *path, const char *file_name, const char *msg);
size_t strlcat (char *dest, const char *src, size_t len);
void CreateDaemon();
void die (const char* p_text);
int wcx_recv_peek (const int fd, void* p_buf, unsigned int len);
int wcx_write (const int fd, const void* p_buf, const unsigned int size);
int wcx_write_loop (const int fd, const void* p_buf, unsigned int size);
int wcx_read (const int fd, void* p_buf, const unsigned int size);
int wcx_read_loop (const int fd, void* p_buf, unsigned int size);

#endif
