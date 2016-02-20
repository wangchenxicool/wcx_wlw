#ifndef _INIFILEOP
#define _INIFILEOP
#include <stdio.h>

//#ifdef __cplusplus
//extern "C" {
//#endif [> __cplusplus <]

static int SplitKeyValue (char *buf, char **key, char **val);
static char *strtrimr (char *buf);
static char *strtriml (char *buf);
static int FileGetLine (FILE *fp, char *buffer, int maxlen);

int  ConfigGetKey (const void *CFG_file, const void *section, const void *key, void *buf);
int  ConfigGetKeys (void *CFG_file, void *section, char *keys[]);
int  ConfigGetSections (void *CFG_file, char *sections[]);
int  ConfigSetKey (const void *CFG_file, const void *section, const void *key, const void *buf);
int  FileCopy (const void *source_file, const void *dest_file);
int  JoinNameIndexToSection (char **section, char *name, char *index);
int  SplitSectionToNameIndex (char *section, char **name, char **index);
void INIFileTstmain (void);

//#ifdef __cplusplus
//}
//#endif [> __cplusplus <]

#endif

