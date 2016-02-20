// ******************************************************************
//  自定义变量类型
// ******************************************************************
#ifndef  __TYPEDEF__
#define  __TYPEDEF__
#include  "stdio.h"
#include  "string.h"
#include  "stdlib.h"

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef void  VOID;
//typedef short          bool;

#define BOOL  bool

#define TRUE	1
#define FALSE	0
#define true	1
#define false	0

#define MAKEWORD(low,hi)  ((WORD)((low) + ((hi) << 8)))

#define LOBYTE(wValue)    ((BYTE)(wValue))
#define HIBYTE(wValue)    ((BYTE)((wValue) >> 8))

#define MAKEDWORD(low,hi) ((DWORD)((low) + ((hi) << 16)))

#define LOWORD(dwValue)   ((WORD)(dwValue))
#define HIWORD(dwValue)   ((WORD)((dwValue) >> 16))

typedef DWORD	IPADDR;

/* 位操作 */
#define BITNMASK(n)			(1<<(n))					//位n的屏蔽码
#define CLRBITN(Value,n)	(Value) &= ~(BITNMASK(n))    //清除位n
#define SETBITN(Value,n)	(Value) |=BITNMASK(n)        //设置位n
#define NEGBITN(Value,n)	(Value) ^=BITNMASK(n)        //位n取反
#define TSTBITN(Value,n)	(((Value) & BITNMASK(n)))!=0)//测试位n

#endif
