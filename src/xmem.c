/**********************************************************
 * File: xmem.c
 * Created at Tue Feb 13 23:12:00 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: xmem.c,v 1.4 2001/03/20 15:00:16 lev Exp $
 **********************************************************/
#include "headers.h"

void *xmalloc(size_t size)
{
	void *p = malloc(size);
	if (p) return p;
	write_log("!!! xmalloc(): could not allocate %d bytes of memory",size);
	abort();
	return NULL;
}

void *xcalloc(size_t number, size_t size)
{
	void *p = calloc(number,size);
	if (p) return p;
	write_log("!!! xcalloc(): could not allocate %dx%d bytes of memory",number,size);
	abort();
	return NULL;
}

void *xrealloc(void *ptr, size_t size)
{
	void *p = realloc(ptr,size);
	if (p) return p;
	write_log("!!! xrealloc(): could not allocate %d bytes of memory",size);
	abort();
	return NULL;
}


char *xstrdup(char *str)
{
	char *s = strdup(str);
	if (s) return s;
	write_log("!!! xstrdup(): could not duplicate string");
	abort();
	return NULL;
}

#ifndef HAVE_STRLCPY
char *xstrcpy(char *dst, char *src, size_t size)
{
	char *d;
	int n;
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
	if(size) {
		d = dst;
		n = size - 1;
		while(n-- && *d) *d++;
		n++;
		if(n) while(n-- && *src) *d++ = *src++;
		n++;
		while(n--) *d++ = 0;
	}
	return dst;
}
#endif

char *restrcpy(char **dst, char *src)
{
	if (*dst) free(*dst);
	return *dst=xstrdup(src?src:"");
}

char *restrcat(char **dst, char *src)
{
	if (!src) return *dst;
	if (!*dst) return *dst=xstrdup(src?src:"");
	*dst = xrealloc(*dst, strlen(*dst) + strlen(src) + 1);
	return strcat(*dst,src);
}
