/**********************************************************
 * File: qipc.c
 * Created at Sat Aug  7 21:41:57 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: qipc.c,v 1.16 2003/03/13 20:30:58 cyrilm Exp $
 **********************************************************/
#include "headers.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdarg.h>
#ifdef HAVE_LIBUTIL_H
#include <libutil.h>
#endif
#include "qipc.h"
#include "byteop.h"
#ifdef QCC

int qipc_msg=-1;
key_t qipc_key=0;

int qipc_init()
{
  	log_callback=vlogs;
	if((qipc_key=ftok(QIPC_KEY,QC_MSGQ))<0) return 0;
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
	if(!qipc_key) qipc_init();
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
	STORE32(buf,1);
	STORE32(buf+4,len);
	STORE32(buf+8,getpid());
	buf[12]=what;
	xstrcpy(buf+13,line,8);
	memcpy(buf+13+strlen(line)+1, buff, len);
/*	write_log("sendpkt %s %d", line, len); */
	rc=msgsnd(qipc_msg, buf, 13+strlen(line)+1+len, IPC_NOWAIT);
	if(rc<0 && (errno==EIDRM||errno==EINVAL)) {
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
	unsigned char buf[MSG_BUFFER], *p = buf;
	falist_t *a;
	STORE16(p,e->speed);INC16(p);
	STORE32(p,e->options);INC32(p);
	STORE32(p,e->starttime);INC32(p);
	xstrcpy((char*)p, e->name , MSG_BUFFER-(p-buf));p+=strlen((char*)p)+1;
	xstrcpy((char*)p, e->sysop, MSG_BUFFER-(p-buf));p+=strlen((char*)p)+1;
	xstrcpy((char*)p, e->place, MSG_BUFFER-(p-buf));p+=strlen((char*)p)+1;
	xstrcpy((char*)p, e->flags, MSG_BUFFER-(p-buf));p+=strlen((char*)p)+1;
	xstrcpy((char*)p, e->phone, MSG_BUFFER-(p-buf));p+=strlen((char*)p)+1;
	*p=0;
	for(a=e->addrs;a;a=a->next) {
		if(p+strlen(ftnaddrtoa(&a->addr))+1>buf+MSG_BUFFER) break;
		xstrcat((char*)p, ftnaddrtoa(&a->addr), MSG_BUFFER-(p-buf));
		xstrcat((char*)p, " ", MSG_BUFFER-(p-buf));
	}
	p+=strlen((char*)p)+1;
	qsendpkt(QC_EMSID, QLNAME, (char *) buf, p-buf);
}

void qpreset(int snd)
{
	qsendpkt(snd?QC_SENDD:QC_RECVD, QLNAME, "", 0);
}

void qpqueue(ftnaddr_t *a, int mail, int files, int try, int flags)
{
	unsigned char buf[MSG_BUFFER], *p=buf;
	char *addr=ftnaddrtoa(a);

	STORE32(p,mail);INC32(p);
	STORE32(p,files);INC32(p);
	STORE32(p,flags);INC32(p);
	STORE16(p,try);INC16(p);

	xstrcpy((char*)p, addr, MSG_BUFFER-(p-buf));p+=strlen(addr)+1;
	qsendpkt(QC_QUEUE, QLNAME, (char*)buf, p-buf);
}

void qpproto(char type, pfile_t *pf)
{
	unsigned char buf[MSG_BUFFER], *p=buf;
	
	STORE32(p,pf->foff);INC32(p);
	STORE32(p,pf->ftot);INC32(p);
	STORE32(p,pf->toff);INC32(p);
	STORE32(p,pf->ttot);INC32(p);
	STORE16(p,pf->nf);INC16(p);
	STORE16(p,pf->allf);INC16(p);
	STORE32(p,pf->cps);INC32(p);
	STORE32(p,pf->soff);INC32(p);
	STORE32(p,pf->stot);INC32(p);
	STORE32(p,pf->start);INC32(p);
	STORE32(p,pf->mtime);INC32(p);
	xstrcpy((char*)p, pf->fname?pf->fname:"", MSG_BUFFER-(p-buf));
	p+=strlen((char*)p)+1;
	qsendpkt(type, QLNAME, (char*)buf, p-buf);
}

#else

int qipc_init(int socket) {return 1;}	
void qipc_done() {}	
void vlogs(char *str) {}	
void vlog(char *str, ...) {}	
void sline(char *str, ...) {}	
void qsendpkt(char what, char *line, char *buff, int len) {}
void qpreset(int snd) {}
void qemsisend(ninfo_t *e) {}
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
	if(cfgi(CFG_USEPROCTITLE)) {
#ifdef HAVE_LIBUTIL
	setproctitle("%s", lin);
#else
	setproctitle(lin);
#endif
	}
}
