// ******************************************************************
//  �Զ����������
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

/* λ���� */
#define BITNMASK(n)			(1<<(n))					//λn��������
#define CLRBITN(Value,n)	(Value) &= ~(BITNMASK(n))    //���λn
#define SETBITN(Value,n)	(Value) |=BITNMASK(n)        //����λn
#define NEGBITN(Value,n)	(Value) ^=BITNMASK(n)        //λnȡ��
#define TSTBITN(Value,n)	(((Value) & BITNMASK(n)))!=0)//����λn

#endif
