/**********************************************************
 * functions for all qico related programs
 **********************************************************/
/*
 * $Id: qslib.c,v 1.12 2005/08/12 16:40:51 mitry Exp $
 *
 * $Log: qslib.c,v $
 * Revision 1.12  2005/08/12 16:40:51  mitry
 * Added wktime_str()
 *
 * Revision 1.11  2005/08/12 15:36:19  mitry
 * Changed gmtoff()
 *
 * Revision 1.10  2005/05/16 11:20:13  mitry
 * Updated function prototypes. Changed code a bit.
 *
 * Revision 1.9  2005/05/11 18:08:04  mitry
 * Changed xrealloc() code
 *
 * Revision 1.8  2005/05/06 20:41:07  mitry
 * Changed setproctitle() code
 *
 * Revision 1.7  2005/04/05 09:31:12  mitry
 * New xstrcpy() and xstrcat()
 *
 * Revision 1.6  2005/03/31 19:40:38  mitry
 * Update function prototypes and it's duplication
 *
 * Revision 1.5  2005/03/28 16:42:13  mitry
 * Moved common bin2strhex() and strhex2bin() funcs here
 *
 * Revision 1.4  2005/02/08 20:02:58  mitry
 * Some code cleaning
 *
 */

#include "headers.h"
#include <sys/utsname.h>
#include "cvsdate.h"

#ifdef DEBUG
#  undef DEBUG
#endif

#define DEBUG(p)


char *osname = "Unix";
char version[] = PACKAGE_VERSION;

char *hexdigitslower = "0123456789abcdef";
char *hexdigitsupper = "0123456789ABCDEF";
char *hexdigitsall = "0123456789abcdefABCDEF";

char *engms[13] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "Any"};

char *infostrs[] = {
	"Address",
	"Station",
	"  Place",
	"  Sysop",
	"  Phone",
	"  Flags",
	"  Speed",
	NULL
};



void *_xmalloc(size_t size)
{
	void	*p = malloc( size );

	if ( p )
		return p;

	write_log( "!!! xmalloc(): could not allocate %d bytes of memory", size );
	abort();
}


void *_xcalloc(size_t number, size_t size)
{
	void	*p = calloc( number, size );

	if ( p )
		return p;

	write_log( "!!! xcalloc(): could not allocate %dx%d bytes of memory", number, size );
	abort();
}


void *xrealloc(void *ptr, size_t size)
{
	void *p;
	
	if ( ptr )
		p = realloc( ptr, size );
	else
		p = ptr = malloc( size );

	if ( p )
		return p;

	write_log( "!!! xrealloc(): could not allocate %d bytes of memory", size );
	abort();
}


void strlwr(char *s)
{
	while( s && *s ) {
		*s = tolower( *s );
		s++;
	}
}


void strupr(char *s)
{
	while( s && *s ) {
		*s = toupper( *s );
		s++;
	}
}


void strtr(char *s, char a, char b)
{
	while( s && *s ) {
		if( *s == a )
			*s = b;
		s++;
	}
}


void chop(char *str, int n)
{
	char	*p;
    
	if ( str ) {
		p = strchr( str, 0 );
		while( p && n-- )
			*--p = 0;
	}
}


size_t chopc(char *str, char ch)
{
	size_t	slen = strlen ( str ) - 1;

	while( slen && str[slen] == ch )
		str[slen--] = '\0';

	return slen;
}


/*
 * Skips leading blanks
 */
char *skip_blanks(char *str)
{
	if ( str != NULL )
        	while( XBLANK( str ))
            		str++;
	return str;
}


/*
 * Skips trailing blanks
 */
void skip_blanksr(char *str)
{
	char *r;

	if ( str != NULL && *str ) {
		r = str + strlen( str ) - 1;
		while( r >= str && XBLANK( r )) {
			*r = '\0';
			r--;
		}
	}
}


/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NULL terminates (unless siz == 0).
 * Returns dst or NULL if dst is unallocated.
 */
char *xstrcpy(char *dst, const char *src, size_t siz)
{
	register char		*d = dst;
	register const char	*s = src;
	register size_t		n = siz;

	if ( !src )
        	return dst;
	if ( !dst )
		return NULL;

	/* Copy as many bytes as will fit */
	if ( n != 0 && --n != 0 ) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	if (n == 0) {
		if (siz != 0)
			*d = '\0';
	}

	return dst;
}


/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns dst or NULL if dst is unallocated.
 */
char *xstrcat(char *dst, const char *src, size_t siz)
{
	register char		*d = dst;
	register const char	*s = src;
	register size_t		n = siz;
	size_t			dlen;

	if ( !src )
		return dst;
	if ( !dst )
		return NULL;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n > 0) {
		while (*s != '\0' && n > 1) {
			*d++ = *s++;
			n--;
		}
		*d = '\0';
	}

	return dst;
}


char *xstrdup(const char *str)
{
	char	*s;
	size_t	len;

	if ( !str )
		return NULL;

	len = strlen( str ) + 1;
	s = xmalloc( len );
	if ( s )
		return xstrcpy( s, str, len );

	write_log( "!!! xstrdup(): could not duplicate string");
	abort();
}


char *restrcpy(char **dst, const char *src)
{
	xfree( *dst );

	if ( !src )
        	return NULL;

	return *dst = xstrdup( src );
}


char *restrcat(char **dst, const char *src)
{
	size_t	len;

	if ( !src )
		return *dst;
	if ( !*dst )
		return *dst = xstrdup( src );

	len = strlen( *dst ) + strlen( src ) + 1;
	*dst = xrealloc( *dst, len );
	return xstrcat( *dst, src, len );
}


void bin2strhex(void *str, const void *binstr, size_t blen)
{
	register unsigned char		*s = str;
	register const unsigned char	*b = binstr;

	while( blen-- ) {
		*s++ = hexdigitslower[(*b >> 4) & 0x0f];
		*s++ = hexdigitslower[(*b++) & 0x0f];
	}
	*s = '\0';
}


int strhex2bin(void *binstr, const void *str)
{
	register unsigned char		*dest = binstr;
	register const unsigned char	*s = str;
	register const char		*p;

	if ( str == NULL )
		return 0;

	while( *s && *(s + 1)) {
		if (( p = strchr( hexdigitsall, *(s++)))) {
			*dest = (byte) ( p - hexdigitsall );
			if (( p = strchr( hexdigitsall, *(s++)))) {
				*dest <<= 4;
				*dest++ |= (byte) ( p - hexdigitsall );
			} else
				return 0;
		} else
			return 0;
	}

	return ( dest - (unsigned char *) binstr );
}


time_t gmtoff(time_t tt)
{
	struct tm	gt;
	time_t		gmt;

	memcpy( &gt, gmtime( &tt ), sizeof( struct tm ));
	gt.tm_isdst = 0;
	gmt = mktime( &gt );
	memcpy( &gt, localtime( &tt ), sizeof( struct tm ));
	gt.tm_isdst = 0;
	return mktime( &gt ) - gmt;
}


char *wktime_str(const char *flags)
{
	char		*p, *oflags, *optr;
	static char	res[80];
	time_t		tm = time( NULL );
	long		tz = gmtoff( tm ) / 3600;

	optr = oflags = xstrdup( flags );
	res[0] = '\0';
	while(( p = strsep( &optr, "," ))) {
		if ( !strcmp( p, "CM" )) {
			xstrcpy( res, "00:00-24:00", 80 );
			break;
		}
		if ( p[0] == 'T' && strlen( p ) == 3 ) {
			snprintf( res, 79, "%02ld:%02d-%02ld:%02d",
				( toupper( p[1] ) - 'A' + tz ) % 24,
				islower((int) p[1] ) ? 30 : 0,
				( toupper( p[2] ) - 'A' + tz ) % 24,
				islower((int) p[2] ) ? 30 : 0 );
			break;
		}
	}
	xfree( oflags );
	return res[0] ? res : NULL;
}



#ifndef HAVE_SETPROCTITLE

#define MAXLINE	2048
/* return number of bytes left in a buffer */
#define SPACELEFT(buf, ptr)	(sizeof buf - ((ptr) - buf))

/*
**  SETPROCTITLE -- set process title for ps
**
**	Parameters:
**		fmt -- a printf style format string.
**		a, b, c -- possible parameters to fmt.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Clobbers argv of our main procedure so ps(1) will
**		display the title.
*/

#define SPT_NONE	0	/* don't use it at all */
#define SPT_REUSEARGV	1	/* cover argv with title information */
#define SPT_BUILTIN	2	/* use libc builtin */
#define SPT_PSTAT	3	/* use pstat(PSTAT_SETCMD, ...) */
#define SPT_PSSTRINGS	4	/* use PS_STRINGS->... */
#define SPT_SYSMIPS	5	/* use sysmips() supported by NEWS-OS 6 */
#define SPT_SCO		6	/* write kernel u. area */
#define SPT_CHANGEARGV	7	/* write our own strings into argv[] */

#ifndef SPT_TYPE
# define SPT_TYPE	SPT_REUSEARGV
#endif /* ! SPT_TYPE */


#if SPT_TYPE != SPT_NONE && SPT_TYPE != SPT_BUILTIN

# if SPT_TYPE == SPT_PSTAT
#  include <sys/pstat.h>
# endif /* SPT_TYPE == SPT_PSTAT */
# if SPT_TYPE == SPT_PSSTRINGS
#  include <machine/vmparam.h>
#  include <sys/exec.h>
#  ifndef PS_STRINGS	/* hmmmm....  apparently not available after all */
#   undef SPT_TYPE
#   define SPT_TYPE	SPT_REUSEARGV
#  else /* ! PS_STRINGS */
#   ifndef NKPDE			/* FreeBSD 2.0 */
#    define NKPDE 63
typedef unsigned int	*pt_entry_t;
#   endif /* ! NKPDE */
#  endif /* ! PS_STRINGS */
# endif /* SPT_TYPE == SPT_PSSTRINGS */

# if SPT_TYPE == SPT_PSSTRINGS || SPT_TYPE == SPT_CHANGEARGV
#  define SETPROC_STATIC	static
# else /* SPT_TYPE == SPT_PSSTRINGS || SPT_TYPE == SPT_CHANGEARGV */
#  define SETPROC_STATIC
# endif /* SPT_TYPE == SPT_PSSTRINGS || SPT_TYPE == SPT_CHANGEARGV */

# if SPT_TYPE == SPT_SYSMIPS
#  include <sys/sysmips.h>
#  include <sys/sysnews.h>
# endif /* SPT_TYPE == SPT_SYSMIPS */

# if SPT_TYPE == SPT_SCO
#  include <sys/immu.h>
#  include <sys/dir.h>
#  include <sys/user.h>
#  include <sys/fs/s5param.h>
#  if PSARGSZ > MAXLINE
#   define SPT_BUFSIZE	PSARGSZ
#  endif /* PSARGSZ > MAXLINE */
# endif /* SPT_TYPE == SPT_SCO */

# ifndef SPT_PADCHAR
#  define SPT_PADCHAR	'\0'
# endif /* ! SPT_PADCHAR */

#endif /* SPT_TYPE != SPT_NONE && SPT_TYPE != SPT_BUILTIN */

#ifndef SPT_BUFSIZE
# define SPT_BUFSIZE	MAXLINE
#endif /* ! SPT_BUFSIZE */

/*
**  Pointers for setproctitle.
**	This allows "ps" listings to give more useful information.
*/

static char	**Argv = NULL;		/* pointer to argument vector */
static char	*LastArgv = NULL;	/* end of argv */

void
initsetproctitle(argc, argv, envp)
	int argc;
	char **argv;
	char **envp;
{
	register int i;
	extern char **environ;

	/*
	**  Move the environment so setproctitle can use the space at
	**  the top of memory.
	*/

	if (envp != NULL)
	{
		for (i = 0; envp[i] != NULL; i++)
			continue;
		environ = (char **) xmalloc(sizeof (char *) * (i + 1));
		for (i = 0; envp[i] != NULL; i++)
			environ[i] = xstrdup(envp[i]);
		environ[i] = NULL;
	}

	/*
	**  Save start and extent of argv for setproctitle.
	*/

	Argv = argv;

	/*
	**  Determine how much space we can use for setproctitle.
	**  Use all contiguous argv and envp pointers starting at argv[0]
	*/

	for (i = 0; i < argc; i++)
	{
		if (i == 0 || LastArgv + 1 == argv[i])
			LastArgv = argv[i] + strlen(argv[i]);
	}
	for (i = 0; LastArgv != NULL && envp != NULL && envp[i] != NULL; i++)
	{
		if (LastArgv + 1 == envp[i])
			LastArgv = envp[i] + strlen(envp[i]);
	}
}

#if SPT_TYPE != SPT_BUILTIN

void setproctitle(const char *fmt, ...)
{
# if SPT_TYPE != SPT_NONE
	register int i;
	register char *p;
	SETPROC_STATIC char buf[SPT_BUFSIZE];
	va_list ap;
#  if SPT_TYPE == SPT_PSTAT
	union pstun pst;
#  endif /* SPT_TYPE == SPT_PSTAT */
#  if SPT_TYPE == SPT_SCO
	int j;
	off_t seek_off;
	static int kmem = -1;
	static pid_t kmempid = -1;
	struct user u;
#  endif /* SPT_TYPE == SPT_SCO */

	p = buf;

	/* print `progname': heading for grep */
	snprintf( p, (size_t) SPACELEFT(buf, p), "%s: ", progname );
	p += strlen(p);

	/* print the argument string */
	va_start( ap, fmt );
	(void) vsnprintf(p, SPACELEFT(buf, p), fmt, ap);
	va_end( ap );

	i = (int) strlen(buf);
	if (i < 0)
		return;

#  if SPT_TYPE == SPT_PSTAT
	pst.pst_command = buf;
	pstat(PSTAT_SETCMD, pst, i, 0, 0);
#  endif /* SPT_TYPE == SPT_PSTAT */
#  if SPT_TYPE == SPT_PSSTRINGS
	PS_STRINGS->ps_nargvstr = 1;
	PS_STRINGS->ps_argvstr = buf;
#  endif /* SPT_TYPE == SPT_PSSTRINGS */
#  if SPT_TYPE == SPT_SYSMIPS
	sysmips(SONY_SYSNEWS, NEWS_SETPSARGS, buf);
#  endif /* SPT_TYPE == SPT_SYSMIPS */
#  if SPT_TYPE == SPT_SCO
	if (kmem < 0 || kmempid != CurrentPid)
	{
		if (kmem >= 0)
			(void) close(kmem);
		kmem = open(_PATH_KMEM, O_RDWR, 0);
		if (kmem < 0)
			return;
		if ((j = fcntl(kmem, F_GETFD, 0)) < 0 ||
		    fcntl(kmem, F_SETFD, j | FD_CLOEXEC) < 0)
		{
			(void) close(kmem);
			kmem = -1;
			return;
		}
		kmempid = CurrentPid;
	}
	buf[PSARGSZ - 1] = '\0';
	seek_off = UVUBLK + (off_t) u.u_psargs - (off_t) &u;
	if (lseek(kmem, (off_t) seek_off, SEEK_SET) == seek_off)
		(void) write(kmem, buf, PSARGSZ);
#  endif /* SPT_TYPE == SPT_SCO */
#  if SPT_TYPE == SPT_REUSEARGV
	if (LastArgv == NULL)
		return;

	if (i > LastArgv - Argv[0] - 2)
	{
		i = LastArgv - Argv[0] - 2;
		buf[i] = '\0';
	}
	(void) xstrcpy(Argv[0], buf, i + 1);
	p = &Argv[0][i];
	while (p < LastArgv)
		*p++ = SPT_PADCHAR;
	Argv[1] = NULL;
#  endif /* SPT_TYPE == SPT_REUSEARGV */
#  if SPT_TYPE == SPT_CHANGEARGV
	Argv[0] = buf;
	Argv[1] = 0;
#  endif /* SPT_TYPE == SPT_CHANGEARGV */
# endif /* SPT_TYPE != SPT_NONE */
}

#endif /* SPT_TYPE != SPT_BUILTIN */

#if 0
void setargspace(int argc,char **argv,char **envp)
{
    int i=0;
    cmdstr=argv[0];
    while(envp[i])i++;
    environ=xmalloc(sizeof(char*)*(i+1));
    i=0;
    while(envp[i]) {
        environ[i]=xstrdup(envp[i]);
        i++;
    }
    environ[i]=NULL;
    cmdstrend=argv[0]+strlen(argv[0]);
    for(i=1;i<argc;i++)if(cmdstrend+1==argv[i])cmdstrend=argv[i]+strlen(argv[i]);
    for(i=0;envp[i];i++)if(cmdstrend+1==envp[i])cmdstrend=envp[i]+strlen(envp[i]);
}


void setproctitle(char *str)
{
    char *p;
    if(!cmdstr)return;
    for(p=cmdstr;p<cmdstrend&&*str;p++,str++)*p=*str;
    *p++=0;while(p<cmdstrend)*p++=' ';
}
#endif /* 0 */

#endif


void u_vers(const char *progn)
{
	struct utsname uts;

	printf( "%s v%s [%s]\n", progn, version, cvsdate );
    
	if( !uname( &uts ))
		printf( "%s %s (%s), ", uts.sysname, uts.release, uts.machine );
	printf(
#ifdef __GNUC__
		"g"
#endif
		"cc: "
#ifdef __VERSION__
		__VERSION__
#else
		"unknown"
#endif
		"\n");
	exit( 0 );
}
