/**********************************************************
 * File: xmem.h
 * Created at Tue Feb 13 23:12:00 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: xmem.h,v 1.3 2001/03/20 19:49:57 lev Exp $
 **********************************************************/
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
