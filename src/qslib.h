/* $Id: qslib.h,v 1.1 2004/06/01 01:14:12 sisoft Exp $ */
#ifndef __QSLIB_H__
#define __QSLIB_H__

extern char progname[];
extern char version[];
extern char cvsdate[];
extern char *osname;

#define FETCH32(p) (((byte*)(p))[0]+(((byte*)(p))[1]<<8)+ \
                   (((byte*)(p))[2]<<16)+(((byte*)(p))[3]<<24))
#define STORE32(p,i) ((byte*)(p))[0]=i;((byte*)(p))[1]=i>>8; \
                       ((byte*)(p))[2]=i>>16;((byte*)(p))[3]=i>>24;
#define INC32(p) p=(byte*)(p)+4
#define FETCH16(p) (((byte*)(p))[0]+(((byte*)(p))[1]<<8))
#define STORE16(p,i) ((byte*)(p))[0]=i;((byte*)(p))[1]=i>>8;
#define INC16(p) p=(byte*)(p)+2

#define SIZES(x) (((x)<1024)?(x):((x)/1024))
#define SIZEC(x) (((x)<1024)?'b':'k')

#define xfree(p) do { if(p) free(p); (p) = NULL; } while(0)
#define SS(str) ((str)?(str):"")

extern void *xmalloc(size_t size);
extern void *xcalloc(size_t number, size_t size);
extern void *xrealloc(void *ptr, size_t size);
extern char *xstrdup(char *str);

extern char *restrcpy(char **dst, char *src);
extern char *restrcat(char **dst, char *src);
extern void strlwr(char *s);
extern void strupr(char *s);
extern void strtr(char *s, char a, char b);
extern void chop(char *s, int n);

#ifndef HAVE_STRLCPY
extern char *xstrcpy(char *dst, char *src, size_t size);
#else
#define xstrcpy(dst,src,size)	(strlcpy((dst),(src),(size)),(dst))
#endif

#ifndef HAVE_STRLCAT
extern char *xstrcat(char *dst, char *src, size_t size);
#else
#define xstrcat(dst,src,size)	(strlcat((dst),(src),(size)),(dst))
#endif

extern time_t gmtoff(time_t tt,int mode);

#ifndef HAVE_SETPROCTITLE
extern void setargspace(int argc,char **argv,char **envp);
extern void setproctitle(char *str);
#endif

extern void u_vers(char *progn);

#endif
