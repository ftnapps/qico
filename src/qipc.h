/**********************************************************
 * File: qipc.h
 * Created at Sat Aug  7 22:05:35 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: qipc.h,v 1.1 2000/07/18 12:37:20 lev Exp $
 **********************************************************/
#ifndef __QIPC_H__
#define __QIPC_H__
#include "mailer.h"
#include "qcconst.h"

int qipc_init(int socket);
void qipc_done();
void vlogs(char *str);
void vlog(char *str, ...);
void sline(char *str, ...);
void title(char *str, ...);
void vidle();
void qsendpkt(char *what, char *line, char *buff, int len);
void qereset();
void qpreset(int snd);
void qemsisend(ninfo_t *e, int sec, int lst);
void qlerase();
void qqreset();
void qpqueue(ftnaddr_t *a, int mail, int files, int try, int flags);

extern char *log_tty;
#define QLNAME is_ip?ip_id:(log_tty?log_tty:"master")
#ifdef MORDA
#define qpfsend() qsendpkt(QC_SENDD, QLNAME, (char *)&sendf, sizeof(pfile_t))
#define qpfrecv() qsendpkt(QC_RECVD, QLNAME, (char *)&recvf, sizeof(pfile_t))
#else
#define qpfsend()
#define qpfrecv()
#endif

#endif
