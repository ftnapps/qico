/**********************************************************
 * File: log.c
 * Created at Thu Jul 15 16:14:06 1999 by pk // aaz@ruxy.org.ru
 * 
 * $Id: log.c,v 1.9 2001/02/16 11:36:02 aaz Exp $
 **********************************************************/
#include "headers.h"
#include <stdarg.h>
#define SYSLOG_NAMES
#include <syslog.h>

#ifndef HAVE_SYSLOG_NAMES
typedef struct _slncode {
	char	*c_name;
	int	c_val;
} SLNCODE;

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

int log_type=0;
char *log_name=NULL;
char *log_tty=NULL;
void  (*log_callback)(char *str)=NULL;

int parsefacility(char *f)
{
	int i=0;
	while(facilitynames[i].c_name) {
		if(!strcasecmp(f, facilitynames[i].c_name))
			return facilitynames[i].c_val;
		i++;
	}
	return -1;
}

int log_init(char *ln, char *tn)
{
	FILE *log_f;
	char *n;int fc;
	log_tty=tn?strdup(tn):NULL;
	if(*ln!='$') {
		log_f=fopen(ln, "at");
		if(log_f) {
			fclose(log_f);
			log_type=1;
			log_name=strdup(ln);
			return 1;
		}
		return 0;
	}
	if(tn) {
		n=malloc(strlen(progname)+2+strlen(tn));
		strcpy(n,progname);
		strcat(n,".");
		strcat(n,tn);
	} else n=progname;
	if((fc=parsefacility(ln+1))<0) return 0;
	log_type=2;
	log_name=NULL;
	openlog(n, LOG_PID, fc);
	return 1;
}

void write_log(char *fmt, ...)
{
	time_t tt;struct tm *t;
	FILE *log_f;va_list args;
	char str[MAX_STRING*16]={0}, *p;
	
	tt=time(NULL);t=localtime(&tt);
	strftime(str, 20, "%d %b %y %H:%M:%S", t);
	sprintf(str+18, " %s[%d]: ", log_tty?log_tty:"", getpid());
	va_start(args, fmt);
	p=str+strlen(str);
	vsnprintf(p, MAX_STRING-1, fmt, args);
	va_end(args);
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
		syslog(LOG_INFO, p);
		break;
	}
}


void log_done()
{
	if(log_type==2) closelog();
	if(log_name) sfree(log_name);
	if(log_tty) sfree(log_tty);
	log_name=NULL;log_tty=NULL;
	log_type=0;
}
