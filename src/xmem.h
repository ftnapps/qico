/**********************************************************
 * File: xmem.h
 * Created at Tue Feb 13 23:12:00 2001 by lev // lev@serebryakov.spb.ru
 * 
 * $Id: xmem.h,v 1.1 2001/02/15 20:32:35 lev Exp $
 **********************************************************/
#ifndef __XMEM_H__
#define __XMEM_H__

void *xmalloc(size_t size);
void *xcalloc(size_t number, size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(char *str);
#define xfree(p) do { if(p) free(p); p = NULL; } while(0)

#ifndef HAVE_STRLCPY
char *xstrcpy(char *dst, char *src, size_t size);
#else /* Remeber about "comma operator" */
#	define xstrcpy(dst,src,size)	strlcpy((dst),(src),(size)),(dst)
#endif
#ifndef HAVE_STRLCAT
char *xstrcat(char *dst, char *src, size_t size);
#else /* Remeber about "comma operator" */
#	define xstrcat(dst,src,size)	strlcat((dst),(src),(size)),(dst)
#endif

char *restrcpy(char **dst, char *src);
char *restrcat(char **dst, char *src);

#endif
