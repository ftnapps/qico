/**********************************************************
 * File: qipc.c
 * Created at Sat Aug  7 21:41:57 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: qipc.c,v 1.1 2000/07/18 12:37:20 lev Exp $
 **********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdarg.h>
#ifdef FREE_BSD
 #include <libutil.h>
#endif
#include "ftn.h"
#include "qipc.h"
#include "mailer.h"
#include "qcconst.h"
#include "globals.h"

#ifdef MORDA 

int qc_sock=-1, qc_slen, qc_clen;
struct sockaddr_un qc_serv, qc_clnt;

int qipc_init(int sock)
{
	qc_sock=socket(AF_UNIX, SOCK_DGRAM, 0);
	if(qc_sock<0) return 0;
	bzero(&qc_serv, sizeof(qc_serv));
	qc_serv.sun_family=AF_UNIX;
	strcpy(qc_serv.sun_path, Q_SOCKET);
	bzero(&qc_clnt, sizeof(qc_clnt));
	qc_clnt.sun_family=AF_UNIX;
	strcpy(qc_clnt.sun_path, "/tmp/qico.XXXXXX");
	mktemp(qc_clnt.sun_path);
	unlink(qc_clnt.sun_path);
	qc_clen = sizeof(qc_clnt.sun_family) + strlen(qc_clnt.sun_path) + 1;
	qc_slen = sizeof(qc_serv.sun_family) + strlen(qc_serv.sun_path) + 1;
	if(bind(qc_sock, (struct sockaddr *)&qc_clnt, qc_clen)<0)
		return 0;
  
	log_callback=vlogs;
	return 1;
}

void qipc_done()
{
	log_callback=NULL;
	close(qc_sock);
	unlink(qc_clnt.sun_path);
}

void vlogs(char *str)
{
	qsendpkt(QC_LOGIT, QLNAME, str, strlen(str)+1);
}

void vlog(char *str, ...)
{
	va_list args;
	char lin[MAX_STRING];
	
	va_start(args, str);
	vsnprintf(lin, MAX_STRING-1, str, args);
	va_end(args);
	qsendpkt(QC_LOGIT, QLNAME, lin, strlen(lin)+1);
}

void qsendpkt(char *what, char *line, char *buff, int len)
{
	char buf[MSG_BUFFER];
	int crc;
	
	if(qc_sock<0) return;
	crc=crc16(buff, len);
	sprintf(buf, "%s%04x%04x%5s%-8s",QC_SIGN,len,crc,what,line);	
	memcpy(buf+27, buff, len);
	len=sendto(qc_sock, buf, len+27, 0, (struct sockaddr *)&qc_serv,
			   qc_slen);
}	

void sline(char *str, ...)
{
	va_list args;
	char lin[MAX_STRING];
	
	va_start(args, str);
	vsprintf(lin, str, args);
	va_end(args);
	qsendpkt(QC_SLINE, QLNAME, lin, strlen(lin)+1);
}

void vidle()
{
	qsendpkt(QC_LIDLE, QLNAME, "", 0); 
}

void qlerase()
{
	qsendpkt(QC_ERASE, QLNAME, "", 0); 
}

void qemsisend(ninfo_t *e, int sec, int lst)
{
	pemsi_t pe;
	falist_t *a;
	int l=0;
	strcpy(pe.name, e->name);
	strcpy(pe.sysop, e->sysop);
	strcpy(pe.city, e->place);
	strcpy(pe.flags, e->flags);
	strcpy(pe.phone, e->phone);
	pe.speed=e->speed;
	pe.addrs[0]=0;
	for(a=e->addrs;a;a=a->next) {
		l+=strlen(ftnaddrtoa(&a->addr))+1;
		if(l>sizeof(pe.addrs)) break;
		strcat(pe.addrs, ftnaddrtoa(&a->addr));
		strcat(pe.addrs, " ");
	}
	pe.secure=sec;pe.listed=lst;
	qsendpkt(QC_EMSID, QLNAME, (char *) &pe, sizeof(pe));
}

void qpreset(int snd)
{
	pfile_t pf;
	bzero(&pf, sizeof(pf));
	qsendpkt(snd?QC_SENDD:QC_RECVD, QLNAME, "", 0);
}

void qereset()
{
	pemsi_t pe;
	bzero(&pe, sizeof(pe));
	qsendpkt(QC_EMSID, QLNAME, "", 0);
}

void qqreset()
{
	pemsi_t pq;
	bzero(&pq, sizeof(pq));
	qsendpkt(QC_QUEUE, QLNAME, "", 0);
}
	
void qpqueue(ftnaddr_t *a, int mail, int files, int try, int flags)
{
	pque_t pq;
	strcpy(pq.addr, ftnaddrtoa(a));
	pq.mail=mail;pq.files=files;
	pq.flags=flags;pq.try=try;
	qsendpkt(QC_QUEUE, QLNAME, (char *)&pq, sizeof(pque_t));
}
#else

int qipc_init(int socket) {return 1;}	
void qipc_done() {}	
void vlogs(char *str) {}	
void vlog(char *str, ...) {}	
void sline(char *str, ...) {}	
void vidle() {}	
void qsendpkt(char *what, char *line, char *buff, int len) {}	
void qereset() {}	
void qpreset(int snd) {}
void qemsisend(ninfo_t *e, int sec, int lst) {}
void qlerase() {}
void qqreset() {}
void qpqueue(ftnaddr_t *a, int mail, int files, int try, int flags) {}

#endif

void title(char *str, ...)
{
	va_list args;
	char lin[MAX_STRING];
	
	va_start(args, str);
	vsprintf(lin, str, args);
	va_end(args);
	qsendpkt(QC_TITLE, QLNAME, lin, strlen(lin)+1);
#ifdef FREE_BSD
	setproctitle("%s", lin);
#else
	setproctitle(lin);
#endif
}
