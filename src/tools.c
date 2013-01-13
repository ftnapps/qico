/**********************************************************
 * Misc stuff.
 **********************************************************/
/*
 * $Id: tools.c,v 1.12 2005/08/10 19:45:50 mitry Exp $
 *
 * $Log: tools.c,v $
 * Revision 1.12  2005/08/10 19:45:50  mitry
 * Added param to qscandir() to return full path with file name
 *
 * Revision 1.11  2005/05/17 18:17:42  mitry
 * Removed system basename() usage.
 * Added qbasename() implementation.
 *
 * Revision 1.10  2005/05/16 14:40:59  mitry
 * Added check for NULL fname in lunlink()
 *
 * Revision 1.9  2005/05/16 11:20:13  mitry
 * Updated function prototypes. Changed code a bit.
 *
 * Revision 1.8  2005/05/06 20:48:55  mitry
 * Misc code cleanup
 *
 * Revision 1.7  2005/04/14 18:04:26  mitry
 * Changed scandir() to qscandir()
 *
 * Revision 1.6  2005/04/08 18:11:12  mitry
 * Insignificant changes
 *
 * Revision 1.5  2005/03/28 17:02:53  mitry
 * Pre non-blocking i/o update. Mostly non working.
 *
 * Revision 1.4  2005/02/12 19:38:05  mitry
 * Remove superfluous log message.
 *
 * Revision 1.3  2005/02/08 19:45:55  mitry
 * islocked() is rewritten to reflect zero-sized .csy/.bsy files
 *
 */

#include "headers.h"
/*
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
*/
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#include "charset.h"
#include "crc.h"

static unsigned long seq = 0xFFFFFFFFUL;
static char _qfmc[MAX_PATH + 5];

char *sigs[] = {
	"","HUP","INT","QUIT","ILL","TRAP","IOT","BUS","FPE",
	"KILL","USR1","SEGV","USR2","PIPE","ALRM","TERM"
};


static int initcharset(char *name,unsigned char *tab)
{
    FILE *f;
    int rev=0,n;
    unsigned i,c;
    char buf[MAX_STRING];
    if(!name||!strcasecmp(name,"none"))return -1;
    if(!strcasecmp(name,"internal"))return 1;
    if(!strncasecmp(name,"revert",6)) {
        rev=1;name+=6;
        while(*name==' '||*name=='\t')name++;
        if(!*name)return -1;
    }
    if(!(f=fopen(name,"r"))) {
        write_log("can't open recode file '%s': %s",name,strerror(errno));
        return -1;
    }
    memset(tab,0,128);
    while(fgets(buf,MAX_STRING,f)) {
        if(!isdigit(*buf))continue;
        if(*buf=='0'&&buf[1]=='x') {
            if(sscanf(buf,"0x%x 0x%x",&i,&c)!=2)continue;
        } else {
            if(sscanf(buf,"%u %u",&i,&c)!=2)continue;
        }
        if(rev) { n=i;i=c;c=n; }
        if(c>255||i<128||i>255)continue;
        tab[i-128]=c;
    }
    for(n=0;n<128;n++)if(!tab[n])tab[n]=n+128;
    fclose(f);
    return 1;
}


void recode_to_remote(char *s)
{
    static int loaded=0;
    register unsigned char c;
    if(!loaded||!s)loaded=initcharset(cfgs(CFG_REMOTECP),ctab_remote);
    if(loaded==1&&s) {
        while(*s) {
            c=*(unsigned char*)s;
            if(c>=128)*(unsigned char*)s=ctab_remote[c-128];
            s++;
        }
    }
}


void recode_to_local(char *s)
{
    static int loaded=0;
    register unsigned char c;
    if(!loaded||!s)loaded=initcharset(cfgs(CFG_LOCALCP),ctab_local);
    if(loaded==1&&s) {
        while(*s) {
            c=*(unsigned char*)s;
            if(c>=128)*(unsigned char*)s=ctab_local[c-128];
            s++;
        }
    }
}


char *mappath(const char *fname)
{
	static char	newfn[MAX_PATH+5];
	char		*mapfrom, *mapto, *mapout = cfgs( CFG_MAPOUT );
	slist_t		*mappath;

	DEBUG(('S',5,"mappath: '%s'",fname));
	memset( newfn, 0, MAX_PATH );

	for( mappath = cfgsl( CFG_MAPPATH ); mappath; mappath = mappath->next ) {
		for( mapfrom = mappath->str; *mapfrom && *mapfrom != ' '; mapfrom++ );
		
		for( mapto = mapfrom; *mapto == ' '; mapto++ );
		
		if ( !*mapfrom || !*mapto)
			write_log( "bad mapping '%s'", mappath->str );
		else {
			size_t lenf = mapfrom - mappath->str;

			if ( !strncasecmp( mappath->str, fname, lenf )) {
				size_t lent = strlen( mapto );

				memcpy( newfn, mapto, lent );
				memcpy( newfn + lent, fname + lenf,
					MAX_PATH - lent - strlen( fname + lenf ));
				break;
			}
		}
	}

	if ( mapout && strchr( mapout, 'S' ))
		strtr( newfn, '\\', '/' );

	DEBUG(('S',5,"mappath: out '%s'",newfn));

	return newfn;
}


char *mapname(char *fn, char *map, size_t size)
{
	int	t;
	char	*l;

	if ( !map )
		return fn;

	DEBUG(('S',5,"mapname: '%s', fn: '%s'",map,fn));

	if ( strchr( map, 'c' ))
		recode_to_remote( fn );
	if ( strchr( map, 'k' ))
		recode_to_local( fn );
	if ( strchr( map, 'd' )) {
		if (( l = strrchr( fn, '.' ))) {
			strtr( fn, '.', '_' );
			*l = '.';
		}
	}
	t = whattype( fn );

	if ( strchr( map, 'b' ) && t != IS_FILE ) {
		l = strrchr( fn, '.' );
		snprintf( fn, 14, "%08lx%s", crc32s( fn ), l );
	}

	if ( strchr( map, 'u' ))
		strupr( fn );
	if ( strchr( map, 'l' ))
		strlwr( fn );
	if ( strchr( map, 'f' ))
		xstrcpy( fn, fnc( fn ), size );

	switch( t ) {
	case IS_PKT:
		if ( strchr( map, 'p' ))
			strlwr( fn );
		else if ( strchr( map, 'P' ))
			strupr( fn );
		break;

	case IS_ARC:
		if ( strchr( map, 'a' ))
			strlwr( fn );
		else if ( strchr( map, 'A' ))
			strupr( fn );
		break;

	case IS_FILE:
		if ( istic( fn )) {
			if ( strchr( map, 't' ))
				strlwr( fn );
			else if ( strchr( map, 'T' ))
				strupr( fn );
		} else {
			if ( isdos83name( fn )) {
				if ( strchr( map, 'o' ))
					strlwr( fn );
				else if ( strchr( map, 'O' ))
					strupr( fn );
			}
		}
		break;
	}
	
	if ( bink )
		strtr( fn, ' ', '_' );

	DEBUG(('S',5,"mapname: out '%s'",fn));

	return fn;
}


char *qbasename(const char *name)
{
	char *p = strrchr( name, '/' );

	return p ? p + 1 : (char *) name;
}


int hexdcd(char h, char l)
{
	l = tolower( l );
	h = tolower( h );
	if ( l >= 'a' && l <= 'f' )
		l -= ( 'a' - 10 );
        else
        	l -= '0';

	if ( h >= 'a' && h <= 'f' )
		h -= ( 'a' - 10 );
        else
        	h -= '0';
	return ( h * 16 + l );
}


char *qver(int w)
{
	cfgs( CFG_PROGNAME );

	switch( w ) {
	case 0:
		if ( ccs && strncasecmp( ccs, progname, 4 ))
			return ccs;
		return progname;

	case 1:
		if ( ccs && strncasecmp( ccs, progname, 4 ))
			if ( cfgs( CFG_VERSION ))
				return ccs;
		return version;

	default:
		return ( cfgs( CFG_OSNAME ) ? ccs : osname );
	}
}


unsigned long sequencer(void)
{
	if ( seq == 0xFFFFFFFFUL || time( NULL ) > seq )
		seq = time( NULL );
        else
        	seq++;
	return seq;
}


off_t filesize(const char *fname)
{
	struct stat sb;

	if ( stat( fname, &sb ) == 0 )
		return sb.st_size;

	return -1;
}


int islocked(const char *pidfn)
{
	FILE *f;
	long pid;

	if (( f = fopen( pidfn, "rt" ))) {
		if ( filesize( pidfn ) == 0 )
			return 1; /* File exists with zero size */

		fseeko( f, (off_t) 0, SEEK_SET );
		fscanf( f, "%ld", &pid );
		fclose( f );
		if ( kill( (pid_t) pid, 0) && ( errno == ESRCH ))
			lunlink( pidfn );
		else
			return ( (int) pid );
	}
	return 0;
}


int lockpid(const char *pidfn)
{
	int	rc;
	char	tmpname[MAX_PATH+5], pidbuf[15];
#ifndef LOCKSTYLE_OPEN
	char	*p;
#endif

	if ( islocked( pidfn ))
		return 0;

	xstrcpy( tmpname, pidfn, MAX_PATH );

#ifndef LOCKSTYLE_OPEN
	p = strrchr( tmpname, '/' );
	if ( !p )
		p = tmpname;

	snprintf( tmpname + ( p - tmpname ), MAX_PATH - ( p - tmpname + 1 ),
		"/QTEMP.%ld", (long) getpid());
#endif

	rc = open( tmpname, O_WRONLY | O_CREAT | O_EXCL, 0644 );
	if ( rc < 0 )
		return 0;

	snprintf( pidbuf, 12, "%10d\n", getpid());
	write( rc, pidbuf, 11);
	close( rc );

#ifndef LOCKSTYLE_OPEN
	rc = link( tmpname, pidfn );
	lunlink( tmpname );
	if ( rc )
		return 0;
#endif
	return 1;
}


static int touch(char *fn)
{
	FILE *f = fopen( fn, "a" );

	if ( f ) {
		fclose( f );
		return 1;
	} else
		write_log( "can't touch '%s': %s", fn, strerror( errno ));
	return 0;
}


int lunlink(const char *fname)
{
	int rc = 0;
	
	if ( fname && *fname )
		unlink( fname );

	if ( rc < 0 && errno != ENOENT )
		write_log("can't delete file %s: %s", fname, strerror( errno ));
	return rc;
}


FILE *mdfopen(char *fn, const char *pr)
{
	FILE		*f;
	struct stat	sb;
	int		nf = ( stat( fn, &sb )) ? 1 : 0;

	if (( f = fopen( fn, pr ))) {
		if ( nf )
			fchmod( fileno( f ), cfgi( CFG_DEFPERM ));
		return f;
	}
	if ( errno == ENOENT ) {
		mkdirs( fn );
		f = fopen( fn, pr );
		if ( f && nf)
			fchmod( fileno( f ), cfgi( CFG_DEFPERM ));
		return f;
	}
	return NULL;
}


int fexist(const char *s)
{
	struct stat sb;
	return ( stat( s, &sb ) == 0 && S_ISREG( sb.st_mode ));
}


int mkdirs(char *name)
{
	int	rc = 0;
	char	*p = name + 1, *q;

	while(( q = strchr( p, '/' )) && ( rc == 0 || errno == EEXIST )) {
		*q = 0;
		rc = mkdir( name, cfgi( CFG_DIRPERM ));
		*q = '/';
		p = q + 1;
	}
	return rc;
}


void rmdirs(char *name)
{
	int	rc = 0;
	char	*q = strrchr( name, '/' ), *t;

	while( q && q != name && rc == 0 ) {
		*q = 0;
		rc = rmdir( name );
		t = strrchr( name,'/' );
		*q = '/';
		q = t;
	}
}


int qalphasort(const void *a, const void *b)
{
	return strcmp( qbasename((const char *) a ), qbasename((const char *) b ));
}


/*
 * Scan the directory dirname calling select to make a list of selected
 * directory entries then sort using qsort and compare routine dcomp.
 * Returns the number of entries and a pointer to a list of pointers to
 * struct dirent (through namelist). Returns -1 if there were any errors.
 *
 * Based on libc scandir code.
 */
int qscandir(const char *dirname, char ***namelist, int nodir,
	int (*nselect)(const char *),
	int (*ndcomp)(const void *, const void *))
{
	register struct dirent	*d;
	register size_t		nitems = 0;
	register char		*p, **names = NULL;
	long			arraysz = 0;
	size_t			l, dlen = 1;
	DIR			*dirp;
	char			_dirname[MAX_PATH + 5] = {'\0'};

	if ( dirname == NULL || *dirname == '\0' )
		return(-1);

	if (( dirp = opendir( dirname )) == NULL )
		return(-1);

	if ( !nodir ) {
		xstrcpy( _dirname, dirname, MAX_PATH );
		dlen = strlen( dirname );
		if ( _dirname[dlen++] != '/' ) {
			xstrcat( _dirname, "/", MAX_PATH );
			dlen++;
		}
	}

	while(( d = readdir( dirp )) != NULL ) {
		DEBUG(('S',5,"qscandir: '%s'",d->d_name));
		if ( nselect != NULL && !(*nselect)( d->d_name ))
			continue;	/* just selected names */
		/*
		 * Make a minimum size copy of the data
		 */
		l = strlen( d->d_name ) + dlen;
		p = (char *) xmalloc( l );
		if ( p == NULL )
			goto fail;
		if ( !nodir )
			xstrcpy( p, _dirname, l );
		else
			*p = '\0';

		xstrcat( p, d->d_name, l );
		/*
		 * Check to make sure the array has space left and
		 * realloc the maximum size.
		 */
		if ( nitems >= arraysz ) {
			const int inc = 4;	/* increase by this much */
			char **names2;

			names2 = (char **) xrealloc((char *) names,
				(arraysz + inc) * sizeof(char *));
			if ( names2 == NULL ) {
				xfree( p );
				goto fail;
			}
			names = names2;
			arraysz += inc;
		}
		names[nitems++] = p;
	}
	closedir( dirp );
	if ( nitems && ndcomp != NULL )
		qsort( names, nitems, sizeof(char *), ndcomp );
	*namelist = names;
	return( nitems );

fail:
	closedir( dirp );
	while( nitems > 0 )
		xfree( names[--nitems] );
	xfree( names );
	*namelist = NULL;
	return (-1);
}


static int qfmcselect(const char *d_name)
{
	if ( d_name == NULL || *d_name == '\0' )
		return 0;
	return ( strncasecmp( d_name, _qfmc, sizeof( _qfmc )) == 0 );
}


static int qfmcasesort(const void *a, const void *b)
{
	return strcasecmp( qbasename((const char *) a ), qbasename((const char *) b ));
}


int fmatchcase(const char *fname, char ***namelist)
{
	char	path[MAX_PATH + 5], *p;
	int	gotcha;

	DEBUG(('S',5,"fmatchcase: '%s'",fname));
	if ( fname == NULL || *fname == '\0' )
		return 0;

	xstrcpy( path, fname, MAX_PATH );
	p = qbasename( path );

	if ( p == NULL || *p == '\0' )
		return (-1);

	xstrcpy( _qfmc, p, MAX_PATH );
	if ( p == path )
		xstrcpy( path, "./", MAX_PATH );
	else
		*p = '\0';

	DEBUG(('S',5,"fmatchcase: path '%s'",path));
	DEBUG(('S',5,"fmatchcase: _qfmc '%s'",_qfmc));

	gotcha = qscandir( path, namelist, 0, qfmcselect, qfmcasesort );

	_qfmc[0] = '\0';

	return gotcha;
}


static int dosallowin83(int c)
{
	static char dos_allow[] = "!@#$%^&()~`'-_{}";

	return (( c >= 'a' && c <= 'z') || ( c >='A' && c <= 'Z' )
		|| ( c >= '0' && c <= '9' ) || strchr( dos_allow, c ));
}


char *fnc(char *s)
{
#define LEN83	13
	static char	s8[LEN83+5];
	char		*p, *q;
	size_t		i = 0;

	if ( !s )
		return NULL;
	if ( !( p = strrchr( s, '/' )))
		p = s;
	else
		s = p;

	while( *p && *p != '.' && i < 8 ) {
		if ( dosallowin83( *p ))
			s8[i++] = tolower( *p );
		p++;
	}
	s8[i] = '\0';

	if ( strstr( s, ".tar.gz" ))
		xstrcat( s8, ".tgz", LEN83 );
	else if ( strstr( s, ".tar.bz2" ))
		xstrcat( s8, ".tb2", LEN83 );
	else if ( strstr( s, ".html" ))
		xstrcat( s8, ".htm", LEN83 );
	else if ( strstr( s, ".jpeg" ))
		xstrcat( s8, ".jpg", LEN83 );
	else if ( strstr( s, ".desc" ))
		xstrcat( s8, ".dsc", LEN83 );
	else {
		if (( p = strrchr( s, '.' ))) {
			xstrcat( s8, ".", LEN83);
			q = p + 4;
			i = strlen( s8 );
			while( *p && q > p ) {
				if ( dosallowin83( *p ))
					s8[i++] = tolower( *p );
				p++;
			}
			s8[i] = '\0';
		}
	}
	return s8;
#undef LEN83
}


int isdos83name(char *fn)
{
	int nl = 0, el = 0, ec = 0, uc = 0, lc = 0, f = 1;

	while( *fn ) {
		if( !dosallowin83( *fn ) && ( *fn != '.' )) {
			f = 0;
			break;
		}

		if ( *fn == '.' )
			ec++;
		else {
			if ( !ec )
				nl++;
			else
				el++;
			if ( isalpha((int) *fn )) {
				if ( isupper((int) *fn ))
					uc++;
				else lc++;
			}
		}
		fn++;
	}

	return ( f && ec < 2 && el < 4 && nl < 9 && ( !lc || !uc ));
}


#if defined(HAVE_STATFS) && defined(STATFS_HAVE_F_BAVAIL)
#    define STAT_V_FS statfs
#else
#    if defined(HAVE_STATVFS) && defined(STATVFS_HAVE_F_BAVAIL)
#        define STAT_V_FS statvfs
#    else
#        undef STAT_V_FS
#    endif
#endif


size_t getfreespace(const char *path)
{
#ifdef STAT_V_FS
	struct STAT_V_FS sfs;

	if ( !STAT_V_FS( path, &sfs )) {
		if ( sfs.f_bsize >= 1024 )
			return(( sfs.f_bsize / 1024L ) * sfs.f_bavail );
		else
			return( sfs.f_bavail / ( 1024L / sfs.f_bsize ));
	} else
		write_log("can't statfs '%s': %s", path, strerror( errno ));
#endif
	return (~0UL);
}


void to_dev_null(void)
{
	int fd;

	close( STDIN_FILENO );
	close( STDOUT_FILENO );
	close( STDERR_FILENO );

	fd = open( devnull, O_RDONLY );
	if ( dup2( fd, STDIN_FILENO ) != STDIN_FILENO ) {
		write_log("reopening of stdin is failed: %s", strerror( errno ));
		exit (-1);
	}
	if ( fd != STDIN_FILENO )
		close( fd );

	fd = open( devnull, O_WRONLY | O_APPEND | O_CREAT, 0600 );
	if ( dup2( fd, STDOUT_FILENO ) != STDOUT_FILENO ) {
		write_log("reopening of stdout is failed: %s", strerror( errno ));
		exit (-1);
	}
	if ( fd != STDOUT_FILENO )
		close( fd );

	fd = open( devnull, O_WRONLY | O_APPEND | O_CREAT, 0600 );
	if ( dup2( fd, STDERR_FILENO ) != STDERR_FILENO ) {
		write_log("reopening of stderr is failed: %s", strerror( errno ));
		exit (-1);
	}
	if ( fd != STDERR_FILENO )
		close( fd );
}


int randper(int base, int diff)
{
    return (base - diff + (int)( diff * 2.0 * rand() / ( RAND_MAX + 1.0 )));
}


int execsh(const char *cmd)
{
	int rc;

	write_log( "executing '%s'", cmd );
	rc = system( cmd );

	return rc;
}


int execnowait(const char *cmd, const char *p1, const char *p2, const char *p3)
{
	pid_t	pid;
	int	rc;

	if (( pid = fork()) == 0 ) {
		to_dev_null();
		setsid();
		rc = execl( cmd, cmd, p1, p2, p3, NULL );
		if ( rc < 0 )
			write_log("can't exec %s: %s", cmd, strerror( errno ));
		exit (-1);
	}

	if ( pid < 0 ) {
		write_log("can't fork(): %s", strerror( errno ));
		return -1;
	}

	return 0;
}


void qsleep(int waittime)			/* wait waittime milliseconds */
{
	struct timeval s;

	s.tv_sec = waittime / 1000;
	s.tv_usec = (waittime % 1000) * 1000;
	select( 0, (fd_set *) NULL, (fd_set *) NULL, (fd_set *) NULL, &s );
}
