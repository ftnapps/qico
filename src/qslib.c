/**********************************************************
 * functions for all qico related programs
 * $Id: qslib.c,v 1.1 2004/06/01 01:14:12 sisoft Exp $
 **********************************************************/
#include "headers.h"
#include <sys/utsname.h>
#include "cvsdate.h"

char *osname="Unix";
char progname[]=PACKAGE_NAME;
char version[]=PACKAGE_VERSION;

void *xmalloc(size_t size)
{
	void *p = malloc(size);
	if(p) return p;
	write_log("!!! xmalloc(): could not allocate %d bytes of memory",size);
	abort();
	return NULL;
}

void *xcalloc(size_t number, size_t size)
{
	void *p = calloc(number,size);
	if(p) return p;
	write_log("!!! xcalloc(): could not allocate %dx%d bytes of memory",number,size);
	abort();
	return NULL;
}

void *xrealloc(void *ptr, size_t size)
{
	void *p = realloc(ptr,size);
	if(p) return p;
	write_log("!!! xrealloc(): could not allocate %d bytes of memory",size);
	abort();
	return NULL;
}

char *xstrdup(char *str)
{
	char *s;
	if(!str) return NULL;
	if(NULL != (s = strdup(str))) return s;
	write_log("!!! xstrdup(): could not duplicate string");
	abort();
	return NULL;
}

char *restrcpy(char **dst, char *src)
{
	xfree(*dst);
	return *dst=xstrdup(SS(src));
}

char *restrcat(char **dst, char *src)
{
	if(!src) return *dst;
	if(!*dst) return *dst=xstrdup(src);
	*dst = xrealloc(*dst, strlen(*dst) + strlen(src) + 1);
	return strcat(*dst,src);
}

void strlwr(char *s)
{
	while(s&&*s){*s=tolower(*s);s++;}
}

void strupr(char *s)
{
	while(s&&*s){*s=toupper(*s);s++;}
}

void strtr(char *s,char a,char b)
{
	while(s&&*s) {
		if(*s==a)*s=b;
		s++;
	}
}

void chop(char *s,int n)
{
	char *p=strchr(s,0);
	while(p&&n--)*--p=0;
}

#ifndef HAVE_STRLCPY
char *xstrcpy(char *dst, char *src, size_t size)
{
	char *d;
	int n;
	if(!src) return dst;
	if(!dst) return NULL;
	if(size) {
		d = dst;
		n = size - 1;
		while(n-- && *src) *d++ = *src++;
		n += 2;
		while(n--) *d++ = 0;
	}
	return dst;
}
#endif

#ifndef HAVE_STRLCAT
char *xstrcat(char *dst, char *src, size_t size)
{
	char *d;
	int n;
	if(!src) return dst;
	if(!dst) return NULL;
	if(size) {
		d = dst;
		n = size - 1;
		while(n-- && *d) d++;
		n++;
		if(n) while(n-- && *src) *d++ = *src++;
		n++;
		while(n--) *d++ = 0;
	}
	return dst;
}
#endif

time_t gmtoff(time_t tt,int mode)
{
	struct tm lt;
#ifndef TM_HAVE_GMTOFF
	struct tm gt;
	time_t offset;
	lt=*localtime(&tt);
	gt=*gmtime(&tt);
	offset=gt.tm_yday-lt.tm_yday;
	if (offset > 1) offset=-24;
	else if (offset < -1) offset=24;
	else offset*=24;
	offset+=gt.tm_hour-lt.tm_hour;
	if(lt.tm_isdst>0&&mode)offset++;
	offset*=60;
	offset+=gt.tm_min-lt.tm_min;
	offset*=60;
	offset+=gt.tm_sec-lt.tm_sec;
	return -offset;
#else
	lt=*localtime(&tt);
	return lt.tm_gmtoff-(lt.tm_isdst>0)*mode;
#endif
}

#ifndef HAVE_SETPROCTITLE
extern char **environ;
static char *cmdstr=NULL;
static char *cmdstrend=NULL;

void setargspace(int argc,char **argv,char **envp)
{
	int i=0;
	cmdstr=argv[0];
	while(envp[i])i++;
	environ=xmalloc(sizeof(char*)*(i+1));
	i=0;
	while(envp[i]) {
		environ[i]=xstrdup(envp[i]);
		i++;
	}
	environ[i]=NULL;
	cmdstrend=argv[0]+strlen(argv[0]);
	for(i=1;i<argc;i++)if(cmdstrend+1==argv[i])cmdstrend=argv[i]+strlen(argv[i]);
	for(i=0;envp[i];i++)if(cmdstrend+1==envp[i])cmdstrend=envp[i]+strlen(envp[i]);
}

void setproctitle(char *str)
{
	char *p;
	if(!cmdstr)return;
	for(p=cmdstr;p<cmdstrend&&*str;p++,str++)*p=*str;
	*p++=0;while(p<cmdstrend)*p++=' ';
}
#endif

void u_vers(char *progn)
{
	struct utsname uts;
	printf("%s v%s [%s]\n",progn,version,cvsdate);
	if(!uname(&uts))
	printf("%s %s (%s), ",uts.sysname,uts.release,uts.machine);
	printf(
#ifdef __GNUC__
		"g"
#endif
		"cc: "
#ifdef __VERSION__
		__VERSION__
#else
		"unknown"
#endif
		"\n");
	exit(0);
}
