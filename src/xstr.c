/**********************************************************
 * safety work with strings
 * $Id: xstr.c,v 1.4 2004/02/09 01:05:33 sisoft Exp $
 **********************************************************/
#include <config.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include "xstr.h"

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
