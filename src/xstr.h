/* $Id: xstr.h,v 1.6 2004/05/24 03:21:36 sisoft Exp $ */
#ifndef __XSTR_H__
#define __XSTR_H__

#define SS(str) ((str)?(str):"")

extern void strlwr(char *s);
extern void strupr(char *s);
extern void strtr(char *s, char a, char b);
extern void chop(char *s, int n);

#ifndef HAVE_STRLCPY
extern char *xstrcpy(char *dst, char *src, size_t size);
#else /* Remeber about "comma operation" */
#define xstrcpy(dst,src,size)	(strlcpy((dst),(src),(size)),(dst))
#endif

#ifndef HAVE_STRLCAT
extern char *xstrcat(char *dst, char *src, size_t size);
#else /* Remeber about "comma operation" */
#define xstrcat(dst,src,size)	(strlcat((dst),(src),(size)),(dst))
#endif

#ifndef HAVE_VSNPRINTF
#define vsnprintf(C,S,F,A...)	vsprintf((C),(F),##A)
#endif

#endif
