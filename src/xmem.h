/* $Id: xmem.h,v 1.3 2004/01/12 21:41:56 sisoft Exp $ */
#ifndef __XMEM_H__
#define __XMEM_H__

extern void *xmalloc(size_t size);
extern void *xcalloc(size_t number, size_t size);
extern void *xrealloc(void *ptr, size_t size);
extern char *xstrdup(char *str);
#define xfree(p) do { if(p) free(p); p = NULL; } while(0)


extern char *restrcpy(char **dst, char *src);
extern char *restrcat(char **dst, char *src);

#endif
