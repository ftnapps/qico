/**********************************************************
 * safety work with memory
 * $Id: xmem.c,v 1.1.1.1 2003/07/12 21:27:19 sisoft Exp $
 **********************************************************/
#include "headers.h"

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
	if(*dst) free(*dst);
	return *dst=xstrdup(src?src:"");
}

char *restrcat(char **dst, char *src)
{
	if(!src) return *dst;
	if(!*dst) return *dst=xstrdup(src);
	*dst = xrealloc(*dst, strlen(*dst) + strlen(src) + 1);
	return strcat(*dst,src);
}
