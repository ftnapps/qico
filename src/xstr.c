/**********************************************************
 * work with strings
 * $Id: xstr.c,v 1.5 2004/05/24 03:21:36 sisoft Exp $
 **********************************************************/
#include "headers.h"

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
