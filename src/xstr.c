/**********************************************************
 * File: xstr.c
 * Created at Tue Mar 20 22:37:42 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: xstr.c,v 1.3 2002/03/16 15:54:20 lev Exp $
 **********************************************************/
#include <stdlib.h>
#include <config.h>

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
