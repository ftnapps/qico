/* $Id: xstr.h,v 1.1.1.1 2003/07/12 21:27:19 sisoft Exp $ */
#ifndef __XSTR_H__
#define __XSTR_H__

#ifndef HAVE_STRLCPY
char *xstrcpy(char *dst, char *src, size_t size);
#else /* Remeber about "comma operation" */
#	define xstrcpy(dst,src,size)	(strlcpy((dst),(src),(size)),(dst))
#endif

#ifndef HAVE_STRLCAT
char *xstrcat(char *dst, char *src, size_t size);
#else /* Remeber about "comma operation" */
#	define xstrcat(dst,src,size)	(strlcat((dst),(src),(size)),(dst))
#endif

#endif
