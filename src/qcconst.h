/**********************************************************
 * File: qcconst.h
 * Created at Sun Aug  8 20:57:15 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: qcconst.h,v 1.7 2000/11/16 18:50:27 lev Exp $
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

#define Q_CHARS		"NHDCIUdrxw"
#define Q_COLORS	{7,3,1,2,2,4,7,6,6,6}
#define Q_NORM		0x0000001
#define Q_HOLD		0x0000002
#define Q_DIR		0x0000004
#define Q_CRASH		0x0000008
#define Q_IMM		0x0000010
#define Q_UNDIAL 	0x0000020
#define Q_DIAL		0x0000040
#define Q_WAITR		0x0000080
#define Q_WAITX		0x0000100
#define Q_WAITA		0x0000200
#define Q_MAXBIT	10
#define Q_ANYWAIT	(Q_WAITR|Q_WAITX|Q_WAITA)

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
