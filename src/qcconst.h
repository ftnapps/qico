/**********************************************************
 * File: qcconst.h
 * Created at Sun Aug  8 20:57:15 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: qcconst.h,v 1.2 2000/10/07 14:21:42 lev Exp $
 **********************************************************/
#ifndef __QCCONST_H__
#define __QCCONST_H__
#include <time.h>

#define Q_STRING 80
#define Q_PATH   40

typedef struct {
	char *fname;
	int  foff, ftot, toff, ttot, nf, allf, cps, soff, stot;
	time_t start, mtime;
} pfile_t;

typedef struct {
	char name[Q_STRING];
	char city[Q_STRING];
	char sysop[Q_STRING];
	char flags[Q_STRING];
	char phone[Q_STRING];
	char addrs[Q_STRING];
	int  speed;
	int secure;
	int listed;
} pemsi_t;

typedef struct {  
	char addr[24];
	int mail, files, hmail, hfiles, flags, try;
} pque_t;

extern char *QC_SIGN;
extern char *Q_SOCKET;

extern char *QC_LOGIT;
extern char *QC_SLINE;
extern char *QC_RECVD;
extern char *QC_SENDD;
extern char *QC_LIDLE;
extern char *QC_TITLE;
extern char *QC_EMSID;
extern char *QC_QUEUE;
extern char *QC_ERASE;

#define MSG_BUFFER 16384

#define Q_NORM   1
#define Q_IMM    2
#define Q_HOLD   4
#define Q_UNDIAL 8
#define Q_DIAL   16
#define Q_WAITR  32
#define Q_WAITX  64
#define Q_WAITA  128
#define Q_ANYWAIT (Q_WAITR|Q_WAITX|Q_WAITA)


#endif
