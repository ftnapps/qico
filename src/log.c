/**********************************************************
 * work with log file
 * $Id: log.c,v 1.1.1.1 2003/07/12 21:26:46 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include <stdarg.h>
#define SYSLOG_NAMES
#include <syslog.h>
#ifdef HAVE_SYSLOG_AND_SYS_SYSLOG
#	include <sys/syslog.h>
#endif

typedef struct _slncode {
	char *c_name;
	int c_val;
} SLNCODE;

#ifndef HAVE_SYSLOG_FAC_NAMES
SLNCODE facilitynames[] =
	{
		{"auth",LOG_AUTH},
	        {"cron",LOG_CRON},
	        {"daemon",LOG_DAEMON},
	        {"kern",LOG_KERN},
	        {"lpr",LOG_LPR},
	        {"mail",LOG_MAIL},
	        {"news",LOG_NEWS},
	        {"syslog",LOG_SYSLOG},
	        {"user",LOG_USER},
	        {"uucp",LOG_UUCP},
	        {"local0",LOG_LOCAL0},
	        {"local1",LOG_LOCAL1},
	        {"local2",LOG_LOCAL2},
	        {"local3",LOG_LOCAL3},
	        {"local4",LOG_LOCAL4},
	        {"local5",LOG_LOCAL5},
	        {"local6",LOG_LOCAL6},
	        {"local7",LOG_LOCAL7},
	        {NULL,-1}
	};
#endif
#ifndef HAVE_SYSLOG_PRI_NAMES
SLNCODE prioritynames[] =
	{
		{"alert",LOG_ALERT,},
		{"crit",LOG_CRIT,},
		{"debug",LOG_DEBUG,},
		{"emerg",LOG_EMERG,},
		{"err",LOG_ERR,},
		{"error",LOG_ERR,},	/* DEPRECATED */
		{"info",LOG_INFO,},                      
		{"notice",LOG_NOTICE,},
		{"panic",LOG_EMERG,},	/* DEPRECATED */
		{"warn",LOG_WARNING,},	/* DEPRECATED */
		{"warning",LOG_WARNING,},
		{NULL,-1,}
	};
#endif

int log_type=0,mcpos,rcpos;
int syslog_priority=LOG_INFO;
char *log_name=NULL;
char *log_tty=NULL;
void (*log_callback)(char *str)=NULL;
ftnaddr_t *adr;
char pktname[MAX_PATH]={0};
FILE *cpkt=NULL;
FILE *lemail=NULL;
char mchat[4096]={0};
char rchat[4096]={0};
extern int runtoss;

#ifdef NEED_DEBUG
int facilities_levels[256];
#endif

int parsefacorprio(char *f,SLNCODE *names)
{
	int i=0;
	while(names[i].c_name) {
		if(!strcasecmp(f,names[i].c_name))return names[i].c_val;
		i++;
	}
	return -1;
}

#ifdef NEED_DEBUG
void parse_log_levels()
{
	char *w,c,*levels=xstrdup(cfgs(CFG_LOGLEVELS));
	memchr(facilities_levels,0,sizeof(facilities_levels));
	for(w=strtok(levels,",;");w;w=strtok(NULL,",;")) {
		c=*w++;
		if(*w)facilities_levels[(unsigned char)c]=atoi(w);
	}
	xfree(levels);
}
#endif


int log_init(char *ln, char *tn)
{
	FILE *log_f;
	char *n,fac[30],*prio;
	int fc,len;
	log_tty=tn?xstrdup(tn):NULL;
	if(*ln!='$') {
		log_f=fopen(ln,"at");
		if(log_f) {
			fclose(log_f);
			log_type=1;
			log_name=xstrdup(ln);
			return 1;
		}
		return 0;
	}
	if(tn) {
		len=strlen(progname)+2+strlen(tn);
		n=malloc(len);
		xstrcpy(n,progname,len);
		xstrcat(n,".",len);
		xstrcat(n,tn,len);
	} else n=progname;

	prio=strchr(ln+1,':');
	if(prio) {
		prio++;
		xstrcpy(fac,ln+1,prio-ln-1);
		if((syslog_priority=parsefacorprio(prio,(SLNCODE*)prioritynames))<0)
			syslog_priority=LOG_INFO;
	} else xstrcpy(fac,ln+1,30);
	if((fc=parsefacorprio(fac,(SLNCODE*)facilitynames))<0)return 0;
	log_type=2;
	log_name=NULL;
	openlog(n,LOG_PID,fc);
	return 1;
}

void vwrite_log(char *fmt, char *prefix,int dbg,va_list args)
{
	FILE *log_f;
	time_t tt;struct tm *t;
	char str[MAX_STRING*16]={0},*p=NULL;

	tt=time(NULL);t=localtime(&tt);
	strftime(str,20,"%d %b %y %H:%M:%S",t);

	snprintf(str+18,MAX_STRING*16-18," %s[%ld]: ",log_tty?log_tty:"",(long)getpid());
	p=str+strlen(str);
	if(prefix&&*prefix) {
		xstrcpy(p,prefix,p-(char*)str);
		p=str+strlen(str);
	}
#ifdef HAVE_VSNPRINTF
	vsnprintf(p,MAX_STRING*16-50,fmt,args);
#else
	/* to be replaced with some emulation vsnprintf!!! */
	vsprintf(p,fmt,args);
#endif
	if(log_callback&&dbg)log_callback(str);
	switch(log_type) {
		case 0:
			fputs(p,stderr);
			fputc('\n',stderr);
			break;
		case 1:
			if(log_name) {
				if((log_f=fopen(log_name, "at"))!=NULL) {
					fputs(str,log_f);fputc('\n',log_f);
					fclose(log_f);
				}
			}
			break;
		case 2:
			syslog(syslog_priority,p);
			break;
	}
}

void write_log(char *fmt,...)
{
	va_list args;
	va_start(args,fmt);
	vwrite_log(fmt,NULL,1,args);
	va_end(args);
}


#ifdef NEED_DEBUG
void write_debug_log(unsigned char facility,int level,char *fmt,...)
{
	va_list args;
	char prefix[16];

#ifndef __GNUC__
	if (facilities_levels[facility]<level)return;
#endif
	snprintf(prefix,16,"DBG_%c%02d: ",facility,level);
	va_start(args, fmt);
	vwrite_log(fmt,prefix,0,args);
	va_end(args);
}
#endif

void log_done()
{
	if(log_type==2)closelog();
	xfree(log_name);
	xfree(log_tty);
	log_name=NULL;log_tty=NULL;
	log_type=0;
}

int chatlog_init(char *remname,ftnaddr_t *ra,int side)
{
	FILE *chatlog=NULL;
	time_t tt;struct tm *t;
	char str[MAX_STRING*4]={0};
	write_log("Chat opened%s",side?" by remote side":"");
	adr=&cfgal(CFG_ADDRESS)->addr;
	if(cfgs(CFG_RUNONCHAT)&&side)execnowait("/bin/sh","-c",ccs,ftnaddrtoa(adr));
	if(cfgi(CFG_CHATLOGNETMAIL)) {
		snprintf(pktname,MAX_PATH-1,"%s/tmp/%08lx.pkt",cfgs(CFG_INBOUND),sequencer());
		cpkt=openpktmsg(adr,adr,"qico chat-log poster",strdup(cfgs(CFG_SYSOP)),"log of chat",NULL,pktname,137);
		if(!cpkt)write_log("can't open '%s' for writing",pktname);
	}
	if(cfgs(CFG_CHATTOEMAIL)) {
		snprintf(str,MAX_PATH,"/tmp/qlemail.%04lx",(long)getpid());
		lemail=fopen(str,"wt");
		if(!lemail)write_log("can't crearte temporary email-log file");
	}
	if(cfgs(CFG_CHATLOG))chatlog=fopen(ccs,"at");
	if(ccs&&!chatlog)write_log("can't open chat log %s",ccs);
	tt=time(NULL);t=localtime(&tt);
	snprintf(str,MAX_STRING*2,"[Chat with: %s (%u:%u/%u.%u) open by %s at ",remname,ra->z,ra->n,ra->f,ra->p,side?"remote":"my");
	strftime(str+strlen(str),MAX_STRING*2-1,"%d %b %y %H:%M:%S]\n",t);
	if(chatlog){fwrite(str,strlen(str),1,chatlog);fclose(chatlog);}
	if(lemail)fwrite(str,strlen(str),1,lemail);
	if(cpkt) {
		stodos((unsigned char*)str);strtr(str,'\n','\r');
		fwrite(str,strlen(str),1,cpkt);
		runtoss=1;
	}
	mcpos=rcpos=0;*rchat=0;*mchat=0;
	return(cpkt!=NULL||chatlog!=NULL||lemail!=NULL);
}	

void chatlog_write(char *text,int side)
{
	FILE *chatlog=NULL;
	long i,n,m;
	char quot[2]={0},*tmp,*cbuf=side?rchat:mchat;
	if(side)*quot='>'; else *quot=' ';
	if(text&&*text) {
		strncpy(cbuf+(side?rcpos:mcpos),text,4095-(side?rcpos:mcpos));
		if(side)rcpos+=strlen(text);
		    else mcpos+=strlen(text);
		while((side?rcpos:mcpos)&&(tmp=strchr(cbuf,'\n'))) {
			i=n=m=0;
			while(i<(tmp-cbuf+1))if(cbuf[i]=='\b'){i++;m+=2;if((--n)<0){n=0;m--;}}
			    else cbuf[n++]=cbuf[i++];
			if(n&&cfgs(CFG_CHATLOG)) {
				chatlog=fopen(ccs,"at");
				fwrite(quot,1,1,chatlog);
				fwrite(cbuf,1,n,chatlog);
				fclose(chatlog);
			}
			if(n&&lemail) {
				fwrite(quot,1,1,lemail);
				fwrite(cbuf,1,n,lemail);
			}
			if(n&&cpkt) {
				for(i=0;i<n;i++) {
					if(cbuf[i]=='\n')cbuf[i]='\r';
					if(cbuf[i]==7)cbuf[i]='*';
					cbuf[i]=todos(cbuf[i]);
				}
				fwrite(quot,1,1,cpkt);
				fwrite(cbuf,1,n,cpkt);
			}
			if(cbuf[n])strncpy(cbuf,cbuf+n,4091);
			if(side)rcpos-=(n+m);
			    else mcpos-=(n+m);
		}
	}
}

void chatlog_done()
{
	FILE *chatlog=NULL;
	time_t tt;struct tm *t;
	char str[MAX_STRING*2]={0};
	write_log("Chat closed");
	if(rcpos)chatlog_write("\n",1);
	if(mcpos)chatlog_write("\n",0);
	if(cfgs(CFG_CHATLOG))chatlog=fopen(ccs,"at");
	tt=time(NULL);t=localtime(&tt);
	strftime(str,MAX_STRING-1,"\n[--- Chat closed at %d %b %y %H:%M:%S ---]\n\n",t);
	if(chatlog){fwrite(str,strlen(str),1,chatlog);fclose(chatlog);}
	if(lemail)fwrite(str,strlen(str),1,lemail);
	if(cpkt) {
		stodos((unsigned char*)str);strtr(str,'\n','\r');
		fwrite(str,strlen(str)-1,1,cpkt);
		closeqpkt(cpkt,adr);
		snprintf(str,MAX_STRING,"%s/%s",cfgs(CFG_INBOUND),basename(pktname));
		if(rename(pktname,str))write_log("can't rename %s to %s: %d",pktname,str,strerror(errno));
		    else chmod(str,cfgi(CFG_DEFPERM));
	}
	if(lemail) {
		fclose(lemail);
		snprintf(str,MAX_STRING*2-1,"mail -s chatlog %s </tmp/qlemail.%04lx",cfgs(CFG_CHATTOEMAIL),(long)getpid());
		execsh(str);
		lunlink(strrchr(str,'<')+1);
	}
}
