/**********************************************************
 * helper stuff for client/server iface.
 * $Id: qipc.c,v 1.14 2004/02/06 21:54:46 sisoft Exp $
 **********************************************************/
#include "headers.h"
#ifdef HAVE_LIBUTIL_H
#include <libutil.h>
#endif
#include "byteop.h"
#include "clserv.h"
#include "qipc.h"

void qsendpkt(char what,char *line,char *buff,int len)
{
	char buf[MSG_BUFFER];
	if(!xsend_cb)return;
	len=(len>=MSG_BUFFER)?MSG_BUFFER:len;
	STORE16(buf,(unsigned short)getpid());
	buf[2]=what;
	if(buf[2]==QC_CERASE)buf[2]=QC_ERASE;
	xstrcpy(buf+3,line,8);
	if(what==QC_CHAT||what==QC_CERASE) {
		if(buf[3]=='i'&&buf[4]=='p') {
			buf[3]='I';buf[4]='P';
		} else {
			buf[3]='C';buf[4]='H';
			buf[5]='T';
		}
	}
	memcpy(buf+4+strlen(line),buff,len);
	if(xsend_cb(ssock,buf,4+strlen(line)+len)<0)DEBUG(('I',1,"can't send_cb (fd=%d): %s",ssock,strerror(errno)));
}

int qrecvpkt(char *str)
{
	int rc;
	if(!xsend_cb)return 0;
	rc=xrecv(ssock,str,MSG_BUFFER-1,0);
	if(rc<0&&errno!=EAGAIN)DEBUG(('I',1,"can't recv (fd=%d): %s",ssock,strerror(errno)));
	if(rc<3||!str[2])return 0;
	str[rc]=0;
	return rc;
}

void vlogs(char *str)
{
	qsendpkt(QC_LOGIT,QLNAME,str,strlen(str)+1);
}

void vlog(char *str,...)
{
	va_list args;
	char lin[MAX_STRING];

	va_start(args, str);
#ifdef HAVE_VSNPRINTF
	vsnprintf(lin,MAX_STRING-1,str,args);
#else
	vsprintf(lin,str,args);
#endif
	va_end(args);
	qsendpkt(QC_LOGIT,QLNAME,lin,strlen(lin)+1);
}

void sline(char *str,...)
{
	va_list args;
	char lin[MAX_STRING];

	va_start(args,str);
#ifdef HAVE_VSNPRINTF
	vsnprintf(lin,MAX_STRING-1,str,args);
#else
	vsprintf(lin,str,args);
#endif
	va_end(args);
	qsendpkt(QC_SLINE,QLNAME,lin,strlen(lin)+1);
}

void qemsisend(ninfo_t *e)
{
	unsigned char buf[MSG_BUFFER],*p=buf;
	falist_t *a;
	STORE16(p,e->speed);INC16(p);
	STORE16(p,e->opt);INC16(p);
	STORE32(p,e->options);INC32(p);
	STORE32(p,e->starttime);INC32(p);
	xstrcpy((char*)p,e->name,MSG_BUFFER-(p-buf));p+=strlen((char*)p)+1;
	xstrcpy((char*)p,e->sysop,MSG_BUFFER-(p-buf));p+=strlen((char*)p)+1;
	xstrcpy((char*)p,e->place,MSG_BUFFER-(p-buf));p+=strlen((char*)p)+1;
	xstrcpy((char*)p,e->flags,MSG_BUFFER-(p-buf));p+=strlen((char*)p)+1;
	xstrcpy((char*)p,e->phone,MSG_BUFFER-(p-buf));p+=strlen((char*)p)+1;
	*p=0;
	for(a=e->addrs;a;a=a->next) {
		if(p+strlen(ftnaddrtoa(&a->addr))+1>buf+MSG_BUFFER)break;
		xstrcat((char*)p,ftnaddrtoa(&a->addr),MSG_BUFFER-(p-buf));
		xstrcat((char*)p," ",MSG_BUFFER-(p-buf));
	}
	p+=strlen((char*)p)+1;
	qsendpkt(QC_EMSID,QLNAME,(char*)buf,p-buf);
}

void qpreset(int snd)
{
	qsendpkt(snd?QC_SENDD:QC_RECVD,QLNAME,"",0);
}

void qpqueue(ftnaddr_t *a,int mail,int files,int try,int flags)
{
	unsigned char buf[MSG_BUFFER],*p=buf;
	char *addr=ftnaddrtoa(a);

	STORE32(p,mail);INC32(p);
	STORE32(p,files);INC32(p);
	STORE32(p,flags);INC32(p);
	STORE16(p,try);INC16(p);

	xstrcpy((char*)p,addr,MSG_BUFFER-(p-buf));p+=strlen(addr)+1;
	qsendpkt(QC_QUEUE,QLNAME,(char*)buf,p-buf);
}

void qpmydata()
{
	char buf[MSG_BUFFER];
	xstrcpy(buf,ftnaddrtoa(&DEFADDR),MSG_BUFFER-1);
	qsendpkt(QC_MYDATA,QLNAME,buf,strlen(buf)+1);
}

void qpproto(char type,pfile_t *pf)
{
	unsigned char buf[MSG_BUFFER],*p=buf;

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
	xstrcpy((char*)p,pf->fname?pf->fname:"",MSG_BUFFER-(p-buf));
	p+=strlen((char*)p)+1;
	qsendpkt(type,QLNAME,(char*)buf,p-buf);
}

void title(char *str,...)
{
	va_list args;
	char lin[MAX_STRING];
	va_start(args,str);
#ifdef HAVE_VSNPRINTF
	vsnprintf(lin,MAX_STRING-1,str,args);
#else
	vsprintf(lin,str,args);
#endif
	va_end(args);
	qsendpkt(QC_TITLE,QLNAME,lin,strlen(lin)+1);
	if(cfgi(CFG_USEPROCTITLE)) {
#ifdef HAVE_LIBUTIL
	setproctitle("%s",lin);
#else
	setproctitle(lin);
#endif
	}
}
