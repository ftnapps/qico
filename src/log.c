/**********************************************************
 * File: log.c
 * Created at Thu Jul 15 16:14:06 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: log.c,v 1.17 2001/05/22 18:55:40 lev Exp $
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
		{ "auth", LOG_AUTH },
        { "cron", LOG_CRON },
        { "daemon", LOG_DAEMON },
        { "kern", LOG_KERN },
        { "lpr", LOG_LPR },
        { "mail", LOG_MAIL },
        { "news", LOG_NEWS },
        { "syslog", LOG_SYSLOG },
        { "user", LOG_USER },
        { "uucp", LOG_UUCP },
        { "local0", LOG_LOCAL0 },
        { "local1", LOG_LOCAL1 },
        { "local2", LOG_LOCAL2 },
        { "local3", LOG_LOCAL3 },
        { "local4", LOG_LOCAL4 },
        { "local5", LOG_LOCAL5 },
        { "local6", LOG_LOCAL6 },
        { "local7", LOG_LOCAL7 },
        { NULL, -1 }
	};
#endif
#ifndef HAVE_SYSLOG_PRI_NAMES
SLNCODE prioritynames[] =
	{
		{ "alert",      LOG_ALERT,      },
		{ "crit",       LOG_CRIT,       },
		{ "debug",      LOG_DEBUG,      },
		{ "emerg",      LOG_EMERG,      },
		{ "err",        LOG_ERR,        },
		{ "error",      LOG_ERR,        },	/* DEPRECATED */
		{ "info",       LOG_INFO,       },                      
		{ "notice",     LOG_NOTICE,     },
		{ "panic",      LOG_EMERG,      },	/* DEPRECATED */
		{ "warn",       LOG_WARNING,    },	/* DEPRECATED */
		{ "warning",    LOG_WARNING,    },
		{ NULL,         -1,             }
	};
#endif

int log_type=0;
int syslog_priority=LOG_INFO;
char *log_name=NULL;
char *log_tty=NULL;
void  (*log_callback)(char *str)=NULL;

#ifdef NEED_DEBUG
int facilities_levels[256];
#endif

int parsefacorprio(char *f, SLNCODE *names)
{
	int i=0;
	while(names[i].c_name) {
		if(!strcasecmp(f, names[i].c_name)) return names[i].c_val;
		i++;
	}
	return -1;
}

#ifdef NEED_DEBUG
void parse_log_levels()
{
	char *levels;
	char *w;
	char c;
	memchr(facilities_levels,0,sizeof(facilities_levels));
	levels = xstrdup(cfgs(CFG_LOGLEVELS));
	for(w=strtok(levels,",;");w;w=strtok(NULL,",;")) {
		c = *w; w++;
		if(*w) facilities_levels[(unsigned char)c] = atoi(w);
	}
	xfree(levels);
}
#endif


int log_init(char *ln, char *tn)
{
	FILE *log_f;
	char *n;int fc, len;
	char fac[30];
	char *prio;
	log_tty=tn?xstrdup(tn):NULL;
	if(*ln!='$') {
		log_f=fopen(ln, "at");
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
		xstrcpy(fac, ln+1, prio-ln-1);
		if((syslog_priority=parsefacorprio(prio,(SLNCODE*)prioritynames))<0)
			syslog_priority=LOG_INFO;
	} else {
		xstrcpy(fac, ln+1,30);
	}
	if((fc=parsefacorprio(fac,(SLNCODE*)facilitynames))<0) return 0;
	log_type=2;
	log_name=NULL;
	openlog(n, LOG_PID, fc);
	return 1;
}

void vwrite_log(char *fmt, char *prefix, va_list args)
{
	FILE *log_f;
	time_t tt;struct tm *t;
	char str[MAX_STRING*16]={0}, *p = NULL;

	tt=time(NULL);t=localtime(&tt);
	strftime(str, 20, "%d %b %y %H:%M:%S", t);

	snprintf(str+18, MAX_STRING*16 - 18, " %s[%ld]: ", log_tty?log_tty:"", (long)getpid());
	p=str+strlen(str);
	if(prefix && *prefix) {
		xstrcpy(p,prefix,p-(char*)str);
		p=str+strlen(str);
	}
#ifdef HAVE_VSNPRINTF
	vsnprintf(p, MAX_STRING*16-50, fmt, args);
#else
	/* to be replaced with some emulation vsnprintf!!! */
	vsprintf(p, fmt, args);
#endif
	if(log_callback) log_callback(str);
	switch(log_type) {
	case 0:
		fputs(p, stderr);
		fputc('\n', stderr);
		break;
	case 1:
		if(log_name) {
			log_f=fopen(log_name, "at");
			if(log_f) {
				fputs(str, log_f);fputc('\n',log_f);
				fclose(log_f);
			}
		}
		break;
	case 2:
		syslog(syslog_priority, p);
		break;
	}
}

void write_log(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vwrite_log(fmt,NULL,args);
	va_end(args);
}


#ifdef NEED_DEBUG
void write_debug_log(unsigned char facility, int level, char *fmt, ...)
{
	va_list args;
	char prefix[16];

#ifndef __GNUC__
	if (facilities_levels[facility] < level) return;
#endif
	snprintf(prefix,16,"DBG_%c%02d: ",facility,level);
	va_start(args, fmt);
	vwrite_log(fmt,prefix,args);
	va_end(args);
}
#endif

void log_done()
{
	if(log_type==2) closelog();
	xfree(log_name);
	xfree(log_tty);
	log_name=NULL;log_tty=NULL;
	log_type=0;
}
