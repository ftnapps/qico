/**********************************************************
 * File: qipc.c
 * Created at Sat Aug  7 21:41:57 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: qipc.c,v 1.3 2000/10/12 20:32:42 lev Exp $
 **********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdarg.h>
#include <errno.h>
#include "ftn.h"
#include "qipc.h"
#include "mailer.h"
#include "qcconst.h"
#include "globals.h"
#ifdef HAVE_LIBUTIL_H
#include <libutil.h>
#endif

#ifdef QCC

int qipc_msg;
key_t qipc_key;

int qipc_init()
{
  	log_callback=vlogs;
	if((qipc_key=ftok(QIPC_KEY,QC_MSGQ))<0) return 0;
	qipc_msg=msgget(qipc_key, 0666);
	return 1;
}

void qipc_done()
{
	log_callback=NULL;
/* 	msgctl(qipc_msg, IPC_RMID, 0); */
}

void qsendpkt(char what, char *line, char *buff, int len)
{
	int rc;
	char buf[MSG_BUFFER];
	if(qipc_msg<0) {
		qipc_msg=msgget(qipc_key, 0666);
		if(qipc_msg<0) return;
		else {
			if(rnode) {
				if(rnode->starttime)
					qemsisend(rnode);
				else qsendqueue();
			} else
				qsendqueue();
		}
	}
	len=(len>=MSG_BUFFER)?MSG_BUFFER:len;
	*((int *)buf)=1;
	*((int *)buf+1)=len;
	*((int *)buf+2)=getpid();
	buf[12]=what;
	strncpy(buf+13,line,8);
	memcpy(buf+13+strlen(line)+1, buff, len);
	rc=msgsnd(qipc_msg, buf, 13+strlen(line)+1+len, IPC_NOWAIT);
	if(rc<0 && errno==EIDRM) {
		qipc_msg=-1;
	}		
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

void sline(char *str, ...)
{
	va_list args;
	char lin[MAX_STRING];
	
	va_start(args, str);
	vsprintf(lin, str, args);
	va_end(args);
	qsendpkt(QC_SLINE, QLNAME, lin, strlen(lin)+1);
}

void qemsisend(ninfo_t *e)
{
	char buf[MSG_BUFFER], *p=buf;
	falist_t *a;
	*((int *)p++)=e->speed;
	*((int *)p++)=e->options;
	*((int *)p++)=e->starttime;
	strncpy(p, e->name, buf+MSG_BUFFER-1-p);p+=strlen(p)+1;
	strncpy(p, e->sysop, buf+MSG_BUFFER-1-p);p+=strlen(p)+1;
	strncpy(p, e->place, buf+MSG_BUFFER-1-p);p+=strlen(p)+1;
	strncpy(p, e->flags, buf+MSG_BUFFER-1-p);p+=strlen(p)+1;
	strncpy(p, e->phone, buf+MSG_BUFFER-1-p);p+=strlen(p)+1;
	*p=0;
	for(a=e->addrs;a;a=a->next) {
		if(p+strlen(ftnaddrtoa(&a->addr))+1>buf+MSG_BUFFER) break;
		strcat(p, ftnaddrtoa(&a->addr));
		strcat(p, " ");
	}
	p+=strlen(p)+1;
	qsendpkt(QC_EMSID, QLNAME, (char *) buf, p-buf);
}

void qpreset(int snd)
{
	pfile_t pf;
	bzero(&pf, sizeof(pf));
	qsendpkt(snd?QC_SENDD:QC_RECVD, QLNAME, "", 0);
}

void qpqueue(ftnaddr_t *a, int mail, int files, int try, int flags)
{
	pque_t pq;
	strcpy(pq.addr, ftnaddrtoa(a));
	pq.mail=mail;pq.files=files;
	pq.flags=flags;pq.try=try;
	qsendpkt(QC_QUEUE, QLNAME, (char *)&pq, sizeof(pque_t));
}

void qpproto(char type, pfile_t *pf)
{
	char buf[MSG_BUFFER], *p=buf;
	memcpy(p, pf, sizeof(pfile_t));
	p+=sizeof(pfile_t);
	strncpy(p, pf->fname, MSG_BUFFER-sizeof(pfile_t));
	p+=strlen(p)+1;
	qsendpkt(type, QLNAME, buf, p-buf);
}

#else

int qipc_init(int socket) {return 1;}	
void qipc_done() {}	
void vlogs(char *str) {}	
void vlog(char *str, ...) {}	
void sline(char *str, ...) {}	
void qsendpkt(char *what, char *line, char *buff, int len) {}	
void qpreset(int snd) {}
void qemsisend(ninfo_t *e, int sec, int lst) {}
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
#ifdef HAVE_LIBUTIL
	setproctitle("%s", lin);
#else
	setproctitle(lin);
#endif
}
