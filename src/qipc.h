/**********************************************************
 * File: qipc.h
 * Created at Sat Aug  7 22:05:35 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: qipc.h,v 1.3 2000/10/12 20:32:42 lev Exp $
 **********************************************************/
#ifndef __QIPC_H__
#define __QIPC_H__
#include "mailer.h"
#include "qcconst.h"
#include "globals.h"

int qipc_init();
void qipc_done();
void vlogs(char *str);
void vlog(char *str, ...);
void sline(char *str, ...);
void title(char *str, ...);
void qsendpkt(char what, char *line, char *buff, int len);
void qpreset(int snd);
void qemsisend(ninfo_t *e);
void qpqueue(ftnaddr_t *a, int mail, int files, int try, int flags);
void qpproto(char type, pfile_t *pf);

extern char *log_tty;
#define QLNAME is_ip?ip_id:(log_tty?log_tty:"master")
#ifdef QCC
#define qpfsend() qpproto(QC_SENDD, &sendf)
#define qpfrecv() qpproto(QC_RECVD, &recvf)
#define qereset() qsendpkt(QC_EMSID, QLNAME, "", 0)
#define qqreset() qsendpkt(QC_QUEUE, QLNAME, "", 0)
#define vidle()   qsendpkt(QC_LIDLE, QLNAME, "", 0)
#define qlerase() qsendpkt(QC_ERASE, QLNAME, "", 0); 
#else
#define qereset()
#define qqreset() 
#define qpfsend()
#define qpfrecv()
#define vidle()
#define qlerase()
#endif

#endif
