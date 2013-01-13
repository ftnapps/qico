/*
 * $Id: qslib.h,v 1.14 2005/08/12 16:40:51 mitry Exp $
 *
 * $Log: qslib.h,v $
 * Revision 1.14  2005/08/12 16:40:51  mitry
 * Added wktime_str()
 *
 * Revision 1.13  2005/08/12 15:36:19  mitry
 * Changed gmtoff()
 *
 * Revision 1.12  2005/05/16 11:20:13  mitry
 * Updated function prototypes. Changed code a bit.
 *
 * Revision 1.11  2005/05/11 20:26:25  mitry
 * Cosmetic change
 *
 * Revision 1.10  2005/05/11 18:08:04  mitry
 * Changed xrealloc() code
 *
 * Revision 1.9  2005/05/06 20:41:08  mitry
 * Changed setproctitle() code
 *
 * Revision 1.8  2005/04/05 09:31:12  mitry
 * New xstrcpy() and xstrcat()
 *
 * Revision 1.7  2005/03/31 19:40:38  mitry
 * Update function prototypes and it's duplication
 *
 * Revision 1.6  2005/03/28 17:02:53  mitry
 * Pre non-blocking i/o update. Mostly non working.
 *
 * Revision 1.5  2005/02/09 11:18:44  mitry
 * More proper writing for XBLANK
 *
 * Revision 1.4  2005/02/08 20:02:21  mitry
 * New define for SIZES and SIZEC
 *
 */

#ifndef __QSLIB_H__
#define __QSLIB_H__

extern	char progname[];
extern	char version[];
extern	char cvsdate[];
extern	char *osname;

extern	char *hexdigitslower;
extern	char *hexdigitsupper;
extern	char *hexdigitsall;
extern	char *engms[];
extern	char *infostrs[];

#define FETCH32(p) (((byte*)(p))[0]+(((byte*)(p))[1]<<8)+ \
                   (((byte*)(p))[2]<<16)+(((byte*)(p))[3]<<24))
#define STORE32(p,i) ((byte*)(p))[0]=i;((byte*)(p))[1]=i>>8; \
                       ((byte*)(p))[2]=i>>16;((byte*)(p))[3]=i>>24;
#define INC32(p) p=(byte*)(p)+4
#define FETCH16(p) (((byte*)(p))[0]+(((byte*)(p))[1]<<8))
#define STORE16(p,i) ((byte*)(p))[0]=i;((byte*)(p))[1]=i>>8;
#define INC16(p) p=(byte*)(p)+2

#define BYTES   ((dword) 9999)
#define KBYTES  ((dword) 10238976)
#define MBYTES  ((dword) 10484711424)

#define SIZES(x) ( ((dword)(x)<BYTES) ? (x) : ( ((dword)(x)<KBYTES) ? ((dword)(x)/1024) : ((dword)(x)/1048576) ))
#define SIZEC(x) ( ((dword)(x)<BYTES) ? 'b' : ( ((dword)(x)<KBYTES) ? 'K'               : 'M' ))

/*
#define SIZES(x) (((x)<1024) ? (x) : (((x)<1048576) ? ((x)/1024) : (((x)<1073741824) ? ((x)/1048576) : ((x)/1073741824))))
#define SIZEC(x) (((x)<1024) ? 'b' : (((x)<1048576) ? 'K' : (((x)<1073741824) ? 'M' : 'G' )))
*/

#define xfree(p) \
    do {                                       \
        if ( p )                               \
            free(p);                           \
        (p) = NULL;                            \
    } while(0)

#define SS(str) ((str)?(str):"")

#define XBLANK( str ) ( *(str) && isspace( *(str) ))
#define xmalloc(n) _xmalloc((size_t)(n))
#define xcalloc(n,s) _xcalloc((size_t)(n),(size_t)(s))

void	*_xmalloc(size_t);
void	*_xcalloc(size_t, size_t);
void	*xrealloc(void *, size_t);
char	*xstrdup(const char *);

char	*restrcpy(char **, const char *);
char	*restrcat(char **, const char *);
void	strlwr(char *);
void	strupr(char *);
void	strtr(char *, char, char);
void	chop(char *, int);
size_t	chopc(char *, char);
char	*skip_blanks(char *);
void	skip_blanksr(char *);

char	*xstrcpy(char *, const char *, size_t);
char	*xstrcat(char *, const char *, size_t);

void	bin2strhex(void *, const void *, size_t);
int	strhex2bin(void *, const void *);

time_t	gmtoff(time_t);
char	*wktime_str(const char *);

#ifndef HAVE_SETPROCTITLE

void	initsetproctitle(int, char **, char **);
void	setproctitle(const char *, ...);

#else

#define initsetproctitle(x,y,z)
#define SPT_TYPE SPT_BUILTIN

#endif /* !HAVE_SETPROCTITLE */

void	u_vers(const char *);

#endif
