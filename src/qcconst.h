/**********************************************************
 * File: qcconst.h
 * Created at Sun Aug  8 20:57:15 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: qcconst.h,v 1.4 2000/10/12 19:13:17 lev Exp $
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
	char addr[24];
	int mail, files, hmail, hfiles, flags, try;
} pque_t;

#include "opts.h"

#define MSG_BUFFER 2048

#define Q_NORM   1
#define Q_IMM    2
#define Q_HOLD   4
#define Q_UNDIAL 8
#define Q_DIAL   16
#define Q_WAITR  32
#define Q_WAITX  64
#define Q_WAITA  128
#define Q_ANYWAIT (Q_WAITR|Q_WAITX|Q_WAITA)

#define QC_MSGQ  'C'
#define QR_MSGQ  'R'

#define QR_POLL  'P'
#define QR_REQ   'R'
#define QR_SEND  'S'
#define QR_STS   'X'
#define QR_CONF  'C'
#define QR_QUIT  'Q'
#define QR_INFO  'I'
#define QR_SCAN  'N'
#define QR_KILL  'K'

#define QC_LOGIT 'a'
#define QC_SLINE 'b'
#define QC_RECVD 'c'
#define QC_SENDD 'd'
#define QC_LIDLE 'e'
#define QC_TITLE 'f'
#define QC_EMSID 'g'
#define QC_QUEUE 'h'
#define QC_ERASE 'i'

#endif
