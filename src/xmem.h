/* $Id: xmem.h,v 1.4 2004/02/09 01:05:33 sisoft Exp $ */
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
