/* $Id: xmem.h,v 1.1.1.1 2003/07/12 21:27:19 sisoft Exp $ */
#ifndef __XMEM_H__
#define __XMEM_H__

void *xmalloc(size_t size);
void *xcalloc(size_t number, size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(char *str);
#define xfree(p) do { if(p) free(p); p = NULL; } while(0)


char *restrcpy(char **dst, char *src);
char *restrcat(char **dst, char *src);

#endif
