/**********************************************************
 * for exchange with qcc
 * $Id: qcconst.h,v 1.1.1.1 2003/07/12 21:27:11 sisoft Exp $
 **********************************************************/
#ifndef __QCCONST_H__
#define __QCCONST_H__
#include <time.h>

#define Q_STRING 80
#define Q_PATH   40

typedef struct {
	char *fname;
	int  foff,ftot,toff,ttot,nf,allf,cps,soff,stot,sts;
	time_t start,mtime;
} pfile_t;

#include "opts.h"

#define MSG_BUFFER 2048

#define Q_CHARS		"NHDCIRUdrxw"
#define Q_COLORS	{7,3,1,2,2,7,4,7,6,6,6}
#define Q_NORM		0x0000001
#define Q_HOLD		0x0000002
#define Q_DIR		0x0000004
#define Q_CRASH		0x0000008
#define Q_IMM		0x0000010
#define Q_REQ		0x0000020
#define Q_UNDIAL 	0x0000040
#define Q_DIAL		0x0000080
#define Q_WAITR		0x0000100
#define Q_WAITX		0x0000200
#define Q_WAITA		0x0000400
#define Q_ANYWAIT	(Q_WAITR|Q_WAITX|Q_WAITA)
#define Q_MAXBIT	11

#define QC_MSGQ   'C'
#define QR_MSGQ   'R'

#define QR_POLL   'A'
#define QR_REQ    'B'
#define QR_SEND   'D'
#define QR_STS    'E'
#define QR_CONF   'F'
#define QR_QUIT   'G'
#define QR_INFO   'H'
#define QR_SCAN   'I'
#define QR_KILL   'J'
#define QR_QUEUE  'K'
#define QR_SKIP   'L'
#define QR_REFUSE 'M'
#define QR_HANGUP 'N'
#define QR_RESTMR 'O'
#define QR_CHAT   'P'

#define QC_LOGIT  'a'
#define QC_SLINE  'b'
#define QC_RECVD  'c'
#define QC_SENDD  'd'
#define QC_LIDLE  'e'
#define QC_TITLE  'f'
#define QC_EMSID  'g'
#define QC_QUEUE  'h'
#define QC_ERASE  'i'
#define QC_QUIT   'j'
#define QC_CHAT   'k'
#define QC_CERASE 'l'
#define QC_MYDATA 'm'

#endif
