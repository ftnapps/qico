/**********************************************************
 * for exchange with qcc
 * $Id: qcconst.h,v 1.7 2004/02/13 22:29:01 sisoft Exp $
 **********************************************************/
#ifndef __QCCONST_H__
#define __QCCONST_H__

typedef struct {
	char *fname;
	int foff,ftot,toff,ttot,nf,allf,cps,soff,stot,sts;
	time_t start,mtime;
} pfile_t;

#define MSG_BUFFER 2048

#define MO_IFC		1
#define MO_BINKP	2
#define MO_CHAT		4

#define Q_STRING 80
#define Q_PATH   40

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

#define O_NRQ 1<<9
#define O_HRQ 1<<10
#define O_FNC 1<<11
#define O_XMA 1<<12
#define O_HAT 1<<13
#define O_HXT 1<<14
#define O_NPU 1<<15
#define O_PUP 1<<16
#define O_PUA 1<<17
#define O_PWD 1<<18
#define O_BAD 1<<19
#define O_RH1 1<<20
#define O_LST 1<<21
#define O_INB 1<<22
#define O_TCP 1<<23

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
#define QR_SET    'S'
#define QR_STYPE  'T'

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
