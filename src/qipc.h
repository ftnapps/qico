/* $Id: qipc.h,v 1.5 2004/01/15 23:39:41 sisoft Exp $ */
#ifndef __QIPC_H__
#define __QIPC_H__
#include "mailer.h"
#include "qcconst.h"
#include "globals.h"

extern void vlogs(char *str);
extern void vlog(char *str, ...);
extern void sline(char *str, ...);
extern void title(char *str, ...);
extern void qsendpkt(char what, char *line, char *buff, int len);
extern void qpreset(int snd);
extern void qemsisend(ninfo_t *e);
extern void qpqueue(ftnaddr_t *a, int mail, int files, int try, int flags);
extern void qpproto(char type, pfile_t *pf);
extern int qrecvpkt(char *str);
extern void qpmydata();

extern char *log_tty;
#define QLNAME is_ip?ip_id:(log_tty?log_tty:"master")
#define qpfsend()  qpproto(QC_SENDD, &sendf)
#define qpfrecv()  qpproto(QC_RECVD, &recvf)
#define qereset()  qsendpkt(QC_EMSID, QLNAME, "", 0)
#define qqreset()  qsendpkt(QC_QUEUE, QLNAME, "", 0)
#define vidle()    qsendpkt(QC_LIDLE, QLNAME, "", 0)
#define qlerase()  qsendpkt(QC_ERASE, QLNAME, "", 0)
#define qlcerase() qsendpkt(QC_CERASE,QLNAME, "", 0)
#define qchat(x)   qsendpkt(QC_CHAT,QLNAME,(x),strlen((x))+1)

#endif
