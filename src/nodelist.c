/**********************************************************
 * work with nodelists
 **********************************************************/
/*
 * $Id: nodelist.c,v 1.20 2005/08/22 19:41:53 mitry Exp $
 *
 * $Log: nodelist.c,v $
 * Revision 1.20  2005/08/22 19:41:53  mitry
 * Fixed detection of stale index
 *
 * Revision 1.19  2005/08/19 13:37:56  mitry
 * Fixed wrong detection of '-' sign
 *
 * Revision 1.18  2005/08/18 19:45:51  mitry
 * Fixed 'unknown subst flag' on valid subst statement
 *
 * Revision 1.17  2005/08/18 18:19:47  mitry
 * Fixed Txy flag handling
 *
 * Revision 1.16  2005/08/18 16:21:25  mitry
 * Added debug messages
 *
 * Revision 1.15  2005/08/17 20:44:10  mitry
 * Modified checktimegaps()
 *
 * Revision 1.14  2005/08/16 15:54:11  mitry
 * Added lost '{'
 *
 * Revision 1.13  2005/08/16 15:17:22  mitry
 * Removed unused ninfo_t.haswtime field
 *
 * Revision 1.12  2005/08/12 15:36:19  mitry
 * Changed gmtoff()
 *
 * Revision 1.11  2005/08/11 19:14:02  mitry
 * Added new macro MAILHOURS
 *
 * Revision 1.10  2005/05/17 18:17:42  mitry
 * Removed system basename() usage.
 * Added qbasename() implementation.
 *
 * Revision 1.9  2005/05/16 20:33:46  mitry
 * Code cleaning
 *
 * Revision 1.8  2005/05/12 09:55:01  mitry
 * Fixed nodelist compiling
 *
 * Revision 1.7  2005/05/11 18:11:34  mitry
 * Changed code with xrealloc()
 *
 * Revision 1.6  2005/05/11 16:35:29  mitry
 * Changed return code of ndl_read_entry()
 *
 * Revision 1.5  2005/05/11 13:22:40  mitry
 * Snapshot
 *
 * Revision 1.4  2005/05/06 20:21:55  mitry
 * Changed nodelist handling
 *
 * Revision 1.3  2005/04/05 09:31:12  mitry
 * New xstrcpy() and xstrcat()
 *
 */

#include "headers.h"
#include "nodelist.h"

extern void vwrite_log(const char *, int);

#define NDL_CHK_LOCK	0
#define NDL_LOCK	1
#define NDL_UNLOCK	2


/* #define NDL_LOG */

static char *ndl_idx = "/qnode.idx";
static char *ndl_sign = "qico nodelist index";
static int msg_con = 0;
static idx_t *nodelist_index = NULL;

char *ndl_errors[] = {
	"No nlpath defined in config",
	"Nodelist index is locked",
	"Can't open nodelist index",
	"Can't read nodelist index header",
	"Unknown nodelist index (no index sign)",
	"No qico index version info",
	"Mismatch nodelist index version",
	"Can't read nodelist index data",
	"Nodelist index is out of date",
	"Nodelist entry not found",
	"Can't open nodelist",
	"Can't query nodelist",
	"Nodelist index needs recompiling",
	NULL
	};


static int comp_addrie(const void *a, const void *b);
static int comp_idxent(const void *a, const void *b);
static int ndl_ext(const char *s);
static int ndl_open(void);
static void ndl_close(void);
static int ndl_get_list_name(int index, char *list);
static int ndl_uptodate_file(int index);
static int ndl_uptodate(void);
static int ndl_loadindex(void);
static int ndl_refresh(void);
static int ndl_lock(const char *path, int lock);
static int ndl_find_entry(const ftnaddr_t *addr);
static int ndl_read_entry(size_t index, char *ndl_str);
static int ndl_query(const ftnaddr_t *addr, ninfo_t **nl);
static void ndl_log(const char *fmt, ...);
static int ndl_compile(void);
static int ndl_listed_one(const ftnaddr_t *addr);
static int chktxy(const char *p);
static int checktxy(const char *flags);
static int parsetime(const char *tstr, int *day, int *hour, int *min);
static subst_t *findsubst(const ftnaddr_t *fa, subst_t *subs);
static void nlfree(ninfo_t *nl);


static int comp_addrie(const void *a, const void *b)
{
	const ftnaddr_t	*aa = (const ftnaddr_t *)a;
	const idxaddr_t	*bb = (const idxaddr_t *)b;
	int		res;

	if (( res = aa->z - bb->z ))
		return res;
	if (( res = aa->n - bb->n ))
		return res;
	if (( res = aa->f - bb->f ))
		return res;
	if (( res = aa->p - bb->p ))
		return res;
	return 0;
}


static int comp_idxent(const void *a, const void *b)
{
	const idxaddr_t	*aa = &(((idxent_t *) a)->addr);
	const idxaddr_t	*bb = &(((idxent_t *) b)->addr);
	int		res;

	if (( res = aa->z - bb->z ))
		return res;
	if (( res = aa->n - bb->n ))
		return res;
	if (( res = aa->f - bb->f ))
		return res;
	if (( res = aa->p - bb->p ))
		return res;
	if (( res = ((idxent_t *) a)->index - ((idxent_t *) b)->index ))
		return res;
	return 0;
}


static int ndl_ext(const char *s)
{
	char *p, *q;

	if ( !s || !*s )
		return -1;
	if ( !( p = strchr( s, ',' )))
		return -1;
	q = strchr( ++p, ',' );
	if ( q )
		*q = 0;
	if ( !*p )
		return -1;
	else
		return atoi( p );
}


/*
 * Opens nodelist index and loads index data. Returns 1 on success.
 */
static int ndl_open(void)
{
	char		ndlpath[MAX_PATH + 5];
	int		i;
	FILE		*idx;
	idx_t		*ndl_i;
	idxh_t		idx_header;
	struct stat	sb;

	DEBUG(('N',1,"ndl_open"));

	if ( !cfgs( CFG_NLPATH ))
		return NDL_ENOPATH;			/* No nlpath defined */

	xstrcpy( ndlpath, ccs, MAX_PATH );
	
	if ( ndl_lock( ndlpath, NDL_CHK_LOCK ))
		return -(NDL_ELOCKED);			/* Nodelist index is locked */

	xstrcat( ndlpath, ndl_idx, MAX_PATH );

	idx = fopen( ndlpath, "rb" );
	if ( !idx )
		return -(NDL_EOPENINDEX);		/* Can't open nodelist index */

	if ( fread( &idx_header, sizeof( idxh_t ), 1, idx ) != 1 ) {
		fclose( idx );
		return -(NDL_EREADINDEXH);		/* Can't read index header */
	}

	if ( strcmp( idx_header.sign, ndl_sign )) {
		fclose( idx );
		return -(NDL_ENOISIGN);			/* No index sign */
	}

	if ( !( idx_header.version & 0x8000 )) {
		fclose( idx );
		return -(NDL_ENOVERSION);		/* No index version info */
	}

	if (( idx_header.version & 0x7fff ) != NDL_VER ) {
		fclose( idx );
		return -(NDL_EVERSION);			/* Mismatch index version */
	}

	DEBUG(('N',2,"ndl_open: found version %d(%d), lists %d",
		idx_header.version & 0x7fff, NDL_VER, idx_header.nlists));

	nodelist_index = ndl_i = xcalloc( 1, sizeof( idx_t ));
	ndl_i->nlists = 0;

	if ( idx_header.nlists ) {
		ndl_i->lists = xcalloc( idx_header.nlists, sizeof( idx_list_t ));

		for( i = 0; i < idx_header.nlists; i++ )
		{
			if ( fread( &ndl_i->lists[i].info, sizeof( idx_l_t ), 1, idx ) != 1 ) {
				fclose( idx );
				ndl_close();
				return -(NDL_EREADINDEXH);
			}

			ndl_i->lists[i].name = xmalloc( ndl_i->lists[i].info.nsize );
			ndl_i->nlists++;
			
			if ( fread( ndl_i->lists[i].name, ndl_i->lists[i].info.nsize, 1, idx ) != 1 ) {
				fclose( idx );
				ndl_close();
				return -(NDL_EREADINDEXH);
			}

			if ( stat( ndl_i->lists[i].name, &sb ) != 0
				|| ndl_i->lists[i].info.mtime != sb.st_mtime )
			{
				DEBUG(('N',3,"ndl_open: index needs recompiling: list '%s' out of date", ndl_i->lists[i].name));
				fclose( idx );
				ndl_close();
				return -(NDL_ENEEDCOMPILE);
			}

			DEBUG(('N',2,"ndl_open: list %d: '%s'", i + 1, ndl_i->lists[i].name));
		}
	}

	fclose( idx );
	ndl_i->path = xstrdup( ndlpath );
	ndl_i->offset = idx_header.offset;
	ndl_i->ecount = idx_header.ecount;

	if ( stat( ndlpath, &sb ) == 0 )
		ndl_i->mtime = sb.st_mtime;

	DEBUG(('N',3,"ndl_open: path '%s'",ndl_i->path));

	return ndl_loadindex();
}


/*
 * Closes nodelist index and frees used memory.
 */
static void ndl_close(void)
{
	DEBUG(('N',1,"ndl_close"));

	if ( nodelist_index ) {
		int	n;
		idx_t	*ndl_i = nodelist_index;

		if ( ndl_i->lists ) {
			for( n = 0; n < ndl_i->nlists; n++ ) {
				DEBUG(('N',3,"ndl_close: 0x%.8p",ndl_i->lists[n].name));
				xfree( ndl_i->lists[n].name );
			}

			xfree( ndl_i->lists );
		}

		xfree( ndl_i->data );
		xfree( ndl_i->path );
		xfree( nodelist_index );
	}
}


static int ndl_get_list_name(int lindex, char *list)
{
	if ( !nodelist_index->lists[lindex].name )
		return 0;

	xstrcpy( list, nodelist_index->lists[lindex].name, MAX_PATH );

	DEBUG(('N',2,"ndl_get_list_name: '%s'",list));

	return 1;
}


static int ndl_uptodate_file(int index)
{
	char		list[MAX_PATH + 5];
	int		rc;
	struct stat	sb;

	if ( !ndl_get_list_name( index, list ))
		return -(NDL_EENTRYNOTFOUND);

	rc = (( stat( list, &sb ) == 0 ) && ( sb.st_mtime == nodelist_index->lists[index].info.mtime ));
	DEBUG(('N',1,"ndl_uptodate_file: index %d, rc %d", index + 1, rc));
	return rc;
}


static int ndl_uptodate(void)
{
	int		i;
	struct stat	sb;

	if (( stat( nodelist_index->path, &sb ) != 0 || sb.st_mtime != nodelist_index->mtime )
		&& ndl_refresh() != 1 )
	{
		DEBUG(('N',1,"ndl_uptodate: index needs recompiling"));
		return 0;
	}

	for( i = 0; i < nodelist_index->nlists; i++ ) {
		DEBUG(('N',2,"ndl_uptodate: checking index %d", i + 1));
		if ( ndl_uptodate_file( i ) != 1 ) {
			DEBUG(('N',1,"ndl_uptodate: index %d needs recompiling", i + 1));
			return 0;
		}
	}

	DEBUG(('N',1,"ndl_uptodate: index is up to date"));
	return 1;
}


static int ndl_loadindex(void)
{
	FILE	*idx;
	idx_t	*ndl_i = nodelist_index;

	DEBUG(('N',1,"ndl_loadindex: ecount %ld",ndl_i->ecount));

	if ( ndl_i->ecount == 0 )
		return 1;

	xfree( ndl_i->data );

	idx = fopen( ndl_i->path, "rb" );
	if ( !idx )
		return -(NDL_EOPENINDEX);	/* Can't open nodelist index */

	if ( fseeko( idx, ndl_i->offset, SEEK_SET ) == -1 ) {
		fclose( idx );
		return (-NDL_EREADINDEXD);
	}

	ndl_i->data = xcalloc( ndl_i->ecount, sizeof( idxent_t ));

	if ( fread( ndl_i->data, sizeof( idxent_t ), ndl_i->ecount, idx ) != ndl_i->ecount ) {
		DEBUG(('N',1,"ndl_loadindex: fread '%s'",strerror( errno )));
		xfree( ndl_i->data );
		fclose( idx );
		return (-NDL_EREADINDEXD);
	}

	fclose( idx );
	DEBUG(('N',2,"ndl_loadindex: loaded %ld entries",ndl_i->ecount));

	return 1;
}


static int ndl_refresh(void)
{
	DEBUG(('N',2,"ndl_refresh: refreshing index"));
	ndl_close();
	return ndl_open();
}


static int ndl_lock(const char *path, int lock)
{
	char nlock[MAX_PATH + 5];

	snprintf( nlock, MAX_PATH, "%s%s.lock", path, ndl_idx );

	switch( lock ) {
	case NDL_CHK_LOCK:
		return islocked( nlock );

	case NDL_LOCK:
		return lockpid( nlock );

	case NDL_UNLOCK:
		return !lunlink( nlock );
	}

	return 0;
}


/*
 * Makes bsearch on raw index data. Returns index on success or -1
 * if no such entry found.
 */
static int ndl_find_entry(const ftnaddr_t *addr)
{
	register size_t	lim, base = 0, ptr;
	register int	cmp;

	DEBUG(('N',2,"ndl_find_entry: data %.8p",nodelist_index->data));
	if ( !nodelist_index->data )
		return -1;

	for( lim = nodelist_index->ecount; lim != 0; lim >>= 1 ) {
		ptr = base + (lim >> 1);
		cmp = comp_addrie( addr, &nodelist_index->data[ptr].addr );
		if ( cmp == 0 )
			return (int) ptr;
		if ( cmp > 0 ) {	/* addr > entry: move right */
			base = ptr + 1;
			lim--;
		}			/* else move left */
	}
	return (-1);
}


/*
 * Reads nodelist line into ndl_str from index pointed by index :)
 * Returns 1 on success or 0 on failure. In latter case ndl_str
 * contains error message.
 */
static int ndl_read_entry(size_t index, char *ndl_str)
{
	char		list[MAX_PATH + 5];
	FILE		*idx;
	idx_t		*ndl_i = nodelist_index;
	idxent_t	*ie;

	DEBUG(('N',2,"ndl_read_entry: data %.8p, index %d",
		ndl_i->data,index));

	if ( !ndl_i->data )
		return -(NDL_EENTRYNOTFOUND);

	ie = &ndl_i->data[index];

	if ( !ndl_get_list_name( ie->index, list ))
		return -(NDL_EENTRYNOTFOUND);

	/*
	if ( ndl_uptodate_file( ie->index ) != 1 )
		return -(NDL_EOUTOFDATE);
	*/

	idx = fopen( list, "rt" );
	if ( !idx )
		return -(NDL_EOPENLIST);
	
	if ( fseek( idx, ie->offset, SEEK_SET )) {
		fclose( idx );
		return -(NDL_EREADLIST);
	}

	if ( !fgets( ndl_str, MAX_STRING, idx )) {
		fclose( idx );
		return -(NDL_EREADLIST);
	}

	{
		int l = strlen( ndl_str );
		
		while( l-- && ( ndl_str[l] == '\n' || ndl_str[l] == '\r' ))
			 ndl_str[l] = '\0';
	}

	fclose( idx );
	return 1;
}


typedef enum { NDL_STATUS, NDL_ADDR, NDL_SYSNAME, NDL_LOCATION, NDL_SYSOP, NDL_PHONE, NDL_SPEED, NDL_FLAGS } ndl_field_t;

/*
 * Query FTN address addr info from nodelist. Returns 1 on success or -(NDL_EXXX)
 * on failure.
 */
static int ndl_query(const ftnaddr_t *addr, ninfo_t **nl)
{
	int		ptr;
	char		ndl_str[MAX_STRING + 5], *p, *t;
	idx_t		*ndl_i;
	ninfo_t		*nlent;
	ndl_field_t	field = NDL_STATUS;

	DEBUG(('N',1,"ndl_query"));

	if ( nodelist_index == NULL && ( ptr = ndl_open()) < 1 )
		return ptr;

	ndl_i = nodelist_index;

	DEBUG(('N',2,"ndl_query: nodelist_index %.8p",nodelist_index));

	if (( ptr = ndl_find_entry( addr )) == -1 )
		return -(NDL_EENTRYNOTFOUND);

	if (( ptr = ndl_read_entry( (size_t) ptr, ndl_str )) != 1 )
		return ptr;

	t = ndl_str;
	nlent = xcalloc( 1, sizeof( ninfo_t ));

	DEBUG(('N',3,"ndl_query: ndl_str '%s'",ndl_str));
	while((	p = strsep( &t, "," ))) {
		DEBUG(('N',4,"ndl_query: p '%s', t '%s'",p,t));
		switch( field ) {
		case NDL_SYSNAME:
		case NDL_LOCATION:
		case NDL_SYSOP:
			strtr( p, '_', ' ' );
		default:
			break;
		}

		switch( field ) {
		case NDL_STATUS:
			if ( !strcasecmp( p, "pvt" ))		nlent->type = NT_PVT;
			else if ( !strcasecmp( p, "hold" ))	nlent->type = NT_HOLD;
			else if ( !strcasecmp( p, "down" ))	nlent->type = NT_DOWN;
			else if ( !strcasecmp( p, "hub" ))	nlent->type = NT_HUB;
			break;

		case NDL_ADDR:
			break;

		case NDL_SYSNAME:
			nlent->name = xstrdup( p );
			break;

		case NDL_LOCATION:
			nlent->place = xstrdup( p );
			break;

		case NDL_SYSOP:
			nlent->sysop = xstrdup( p );
			break;

		case NDL_PHONE:
			nlent->phone = xstrdup( p );
			break;

		case NDL_SPEED:
			nlent->speed = atoi( p );
			nlent->flags = xstrdup( t );
			break;

		case NDL_FLAGS:
		default:
			if ( nlent->type == NT_PVT ) {
#ifdef WITH_BINKP
				if ( strstr( p, "IBN" ))
					nlent->opt |= MO_BINKP;
#endif
				if ( strstr( p, "IFC" ))
					nlent->opt |= MO_IFC;
			}

			if (( p[0] == 'T' && strlen( p ) == 3 )
				|| ( strcmp( p, "CM" ) == 0 ))
			{
				nlent->wtime = xstrdup( p );
			}
		}
		field++;
	}

	if ( nlent->opt )
		nlent->host = xstrdup( ftnaddrtoia( addr ));

	*nl = nlent;
	falist_add( &nlent->addrs, addr );
	return 1;
}



static void ndl_log(const char *fmt, ...)
{
	static char	str[1024];
	va_list		args;

	str[0] = '\0';

	va_start( args, fmt );
	vsnprintf( str, sizeof( str ), fmt, args );
	va_end( args );

	if ( msg_con )
		printf( "%s\n", str );
	else
		vwrite_log( str, 1 );
}


static int ndl_compile(void)
{
	slist_t		*n;
	char		nidx[MAX_PATH + 5], s[MAX_STRING + 5], nlmax[MAX_PATH + 5];
	char		npath[MAX_PATH + 5], *p;
	int		max, gp = 0, rc, k, line = 0;
	size_t		total = 0, nlists = 0, alloc = 0;
	off_t		pos;
	FILE		*idx, *f;
	struct stat	sb;
	idxh_t		idxh;
	idxent_t	ie, *ies = NULL;
	idx_list_t	*lists = NULL;
	FTNADDR_T	(fa);

	fa.z = cfgal( CFG_ADDRESS )->addr.z;
	fa.n = cfgal( CFG_ADDRESS )->addr.n;

	if ( msg_con )
		setbuf( stdout, NULL );

	if ( !cfgs( CFG_NLPATH )) {
		ndl_log( "Error: nlpath is not defined!" );
		return 0;
	}

	xstrcpy( npath, ccs, MAX_PATH );
	chopc( npath, '/' );

	if ( !ndl_lock( npath, NDL_LOCK )) {
		ndl_log( "Nodelist index is locked" );
		return 0;
	}

	snprintf( nidx, MAX_PATH, "%s%s", npath, ndl_idx );
	if ( !( idx = fopen( nidx, "wb" ))) {
		ndl_log( "Can't open nodelist index %s for writing: %s",
			nidx, strerror( errno ));
		ndl_lock( npath, NDL_UNLOCK );
		return 0;
	}

	ndl_log( "Compiling nodelists..." );
	
	for( n = cfgsl( CFG_NODELIST ); n; n = n->next ) {
		char nodelist[MAX_PATH + 5];

		nodelist[0] = '\0';
		xstrcpy( s, n->str, MAX_STRING );
		if (( p = strchr( s, ' ' ))) {
			*p++ = 0;
			fa.z = atoi( p );
		}

		if ( s[0] == '/' ) {
			p = strrchr( s, '/' );
			if ( p == s ) {
				xstrcpy( nidx, "/", MAX_PATH );
				p++;
			} else {
				*p++ = '\0';
				xstrcpy( nidx, s, MAX_PATH );
			}
			xstrcpy( s, p, MAX_PATH );
		} else
			xstrcpy( nidx, npath, MAX_PATH );

#ifdef NDL_LOG
		ndl_log( "nidx: %s", nidx );
		ndl_log( "s: %s", s );
#endif

		if ( !strcmp( s + strlen( s ) - 4, ".999" )) {
			int		num;
			time_t		mtime = 0;
			DIR		*d;
			struct dirent	*de;

			s[strlen( s ) - 4] = '\0';
			if ( !( d = opendir( nidx ))) {
				ndl_log( "Can't open nodelist directory %s: %s",
					nidx, strerror( errno ));
				fclose( idx );
				ndl_lock( npath, NDL_UNLOCK );
				return 0;
			}

			max = -1;

			while(( de = readdir( d ))) {
				if ( de->d_name[0] == '.' )
					continue;

				if ( !strncasecmp( de->d_name, s, strlen( s ))) {
					p = de->d_name + strlen( s );

					if (( *p == '.' ) && strlen( p ) == 4
						&& strspn( p + 1, "0123456789") == 3 )
					{
						char	_n[MAX_PATH + 5];
						time_t	_mtime = 0;

						snprintf( _n, MAX_PATH, "%s/%s", nidx, de->d_name );
						if ( !stat( _n, &sb ))
							_mtime = sb.st_mtime;

						num = atoi( p + 1 );
						if ( !msg_con ) {
							DEBUG(('N',1,"Found: %s",_n));
							DEBUG(('N',2,"mtime(max): %ld(%d), _mtime(num): %ld(%d)",
								mtime, max, _mtime, num ));
						}
						if ( _mtime ) {
							if ( _mtime >= mtime ) {
								xstrcpy( nlmax, de->d_name, MAX_PATH );
								max = num;
								mtime = _mtime;
							}
						} else if ( num > max ) {
							xstrcpy( nlmax, de->d_name, MAX_PATH );
							max = num;
							mtime = _mtime;
						}
					}
				}
			}
			
			closedir( d );

			if ( max < 0 )
				ndl_log( "No lists matching %s/%s.[0-9] found", nidx, s );
			else
				snprintf( nodelist, MAX_PATH, "%s/%s", nidx, nlmax );
		} else
			snprintf( nodelist, MAX_PATH, "%s/%s", nidx, s );

		if ( !nodelist[0] )
			continue;

		if ( stat( nodelist, &sb ) == -1 ) {
			ndl_log( "Can't open %s for reading: %s",
				nodelist, strerror( errno ));
			continue;
		}

		f = fopen( nodelist, "rt" );
		if ( !f )
			ndl_log( "Can't open %s for reading: %s",
				nodelist, strerror( errno ));
		else {
			lists = xrealloc( lists, sizeof( idx_list_t ) * (nlists + 1));
			lists[nlists].info.mtime = sb.st_mtime;
			lists[nlists].info.nsize = strlen( nodelist ) + 1;
			lists[nlists].name = xstrdup( nodelist );

			k = line = 0;
			while( 1 ) {
				pos = ftello( f );
				line++;

				if ( !fgets( s, MAX_STRING, f ))
					break;

				if ( *s == ';' || *s == '\r' || *s == '\n'
						|| *s == 0x1a || !*s )
					continue;

				if ( !strncmp( s, "Boss,", 5 )) {
					rc = parseftnaddr( s + 5, &fa, NULL, 0 );
					if ( rc )
						gp = 1;
					continue;
				}
				
				if ( gp && !strncmp( s, "Point,", 6 )) {
					fa.p = ndl_ext( s );

				} else if ( !strncmp( s, "Zone,", 5 )) {
					gp = 0;
					fa.z = fa.n = ndl_ext( s );
					fa.f = fa.p = 0;
				} else if ( !strncmp( s, "Host,", 5 )
					|| !strncmp( s, "Region,", 7 ))
				{
					gp = 0;
					fa.n = ndl_ext( s );
					fa.f = fa.p = 0;
				} else if ( !gp && strncmp( s, "Down,", 5 )) {
					fa.f = ndl_ext( s );
					fa.p = 0;
				} else if ( !gp )
					continue;

				if ( fa.z < 0 || fa.n < 0 || fa.f < 0 || fa.p < 0 )
				{
					ndl_log( "Can't parse address at line %d of %s",
						line, qbasename( nodelist ));
					fclose( idx );
					xfree( ies );
					ndl_lock( npath, NDL_UNLOCK );
					return 0;
				}

				ie.addr.z = (unsigned short) fa.z;
				ie.addr.n = (unsigned short) fa.n;
				ie.addr.f = (unsigned short) fa.f;
				ie.addr.p = (unsigned short) fa.p;
				ie.offset = pos;
				ie.index = nlists;

				if ( total >= alloc ) {
					alloc += 64;
#ifdef NDL_LOG 
					ndl_log( "alloc: %ld, total: %ld", alloc, total );
#endif
					ies = xrealloc( ies, sizeof( ie ) * alloc );
				}

				ies[total++] = ie;
				k++;
			}
			
			nlists++;
			fclose( f );
			ndl_log( "Compiled %s: %d nodes", qbasename( nodelist ), k );
			if ( nlists > MAX_NODELIST ) {
				ndl_log( "Too many lists - increase MAX_NODELIST in config.h and rebuild" );
				break;
			}
		}
	}

	pos = sizeof( idxh_t );

	fseeko( idx, sizeof( idxh_t ), SEEK_SET );
	for( k = 0; k < nlists; k++ ) {
		if ( fwrite( &lists[k].info, sizeof( idx_l_t ), 1, idx ) != 1
			|| fwrite( lists[k].name, lists[k].info.nsize, 1, idx ) != 1 )
		{
			ndl_log( "Can't write index header: %s", strerror( errno ));
			goto failure;
		}
		xfree( lists[k].name );
	}
	xfree( lists );

	pos = ftello( idx );

	if ( total ) {
		int firsteq = -1, lasteq = -1, deleted = 0;

		qsort( ies, total, sizeof( ie ), comp_idxent );
		
		/* Delete duplicates */
		k = 0;

		while( k < total - 1 ) {
			if ( !memcmp( &ies[k].addr, &ies[k + 1].addr, sizeof( ies->addr ))) {
				if ( firsteq < 0 )
					firsteq = k;
				lasteq = k + 1;
				k++;
			} else if ( firsteq >= 0 ) {
				memcpy( &ies[firsteq + 1], &ies[lasteq + 1],
					sizeof( *ies ) * (total - lasteq - 1));
				firsteq = lasteq = -1;
				deleted++;
				total--;
			} else
				k++;
		}

		ndl_log( "%d duplicate records deleted", deleted );

		fseeko( idx, pos, SEEK_SET );
		if ( fwrite( ies, sizeof( *ies ), total, idx ) != total ) {
			ndl_log( "Can't write index: %s", strerror( errno ));
			goto failure;
		}
		
		xfree( ies );
	}

	xstrcpy( idxh.sign, ndl_sign, sizeof( idxh.sign ));
	idxh.version = NDL_VER | 0x8000;
	idxh.nlists = nlists;
	idxh.ecount = total;
	idxh.offset = pos;

	fseeko( idx, 0, SEEK_SET );
	if ( fwrite( &idxh, sizeof( idxh ), 1, idx ) != 1 ) {
		ndl_log( "Can't write index header: %s", strerror( errno ));
		goto failure;
	}

	ndl_log( "Total: %d lists, %d nodes", nlists, total );

	fclose( idx );
	ndl_lock( npath, NDL_UNLOCK );
	
	return 1;

failure:
	xfree( ies );
	if ( lists ) {
		for( k = 0; k < nlists; k++ )
			xfree( lists[k].name );
		xfree( lists );
	}
	fclose( idx );
	ndl_lock( npath, NDL_UNLOCK );
	return 0;
}


void nodelist_done(void)
{
	ndl_close();
}


int nodelist_compile(int console)
{
	int _mc = msg_con, rc;

	msg_con = console;
	rc = ndl_compile();
	msg_con = _mc;
	return rc;
}


int nodelist_query(const ftnaddr_t *addr, ninfo_t **nl)
{
	int rc = 1;

	if ( nodelist_index == NULL )
		rc = ndl_open();

	if ( rc != 1 && rc != -(NDL_ENEEDCOMPILE))
		return rc;

	if ( rc == -(NDL_ENEEDCOMPILE) || ndl_uptodate() != 1 ) {
		ndl_close();
		rc = nodelist_compile( FALSE ) ? 1 : -(NDL_ENEEDCOMPILE);
	}

	if ( rc != 1 )
		return rc;

	return ndl_query( addr, nl );
}


static int ndl_listed_one(const ftnaddr_t *addr)
{
	return ( ndl_find_entry( addr ) >= 0 );
}


int nodelist_listed(falist_t *addrs, int needall)
{
	int		rc = 1;
	falist_t	*i;

	DEBUG(('N',1,"nodelist_listed: needall %d",needall));
	if ( nodelist_index == NULL )
		rc = ndl_open();

	if ( rc != 1 )
		return 0;

	for( i = addrs; i; i = i->next ) {
		rc = ndl_listed_one( &i->addr );
		DEBUG(('N',2,"nodelist_listed: %s, rc %d",ftnaddrtoa(&i->addr),rc));
		if ( needall ) {
			if ( !rc )
				return 0;
		} else {
			if ( rc )
				return 1;
		}
	}

	return needall;
}


void phonetrans(char **pph, slist_t *phtr)
{
	int rc=1;slist_t *pht;
	char *s, *t,*p, tmp[MAX_STRING + 5];
	char *ph = *pph;
	int len=0;

	if(!*pph || !*ph) return;
	for(pht=phtr;pht && rc;pht=pht->next) {
		s=xstrdup(pht->str);
		p=strchr(s,' ');
		if(p) {
			*(p++)='\0';
			while(*p==' ') p++;
			t=strchr(p,' ');
			if(t) *t='\0';
		}
		if(strncmp(ph, s, strlen(s))==0) {
			if(p) xstrcpy(tmp, p, MAX_STRING);else *tmp=0;
			xstrcat(tmp, ph+strlen(s), MAX_STRING);
			len=strlen(tmp)+1;
			xfree(ph); ph=xmalloc(len); *pph=ph;
			xstrcpy(ph, tmp, len);xfree(s);
			return;
		}
		if(s[0]=='=') {
			if(p) xstrcpy(tmp, p, MAX_STRING);else *tmp=0;
			xstrcat(tmp, ph, MAX_STRING);
			len=strlen(tmp)+1;
			xfree(ph); ph=xmalloc(len); *pph=ph;
			xstrcpy(ph, tmp, len);xfree(s);
			return;
		}
		xfree(s);
	}
}


static int chktxy(const char *p)
{
	time_t		tim = time( NULL );
	struct tm	*ti = localtime( &tim );
	int		t = ti->tm_hour * 60 + ti->tm_min - gmtoff( tim ) / 60;
	int		t1, t2;

	{
		char *c = (char *)(p + 1);

		while( *c ) {
			char t = tolower( *c++ );
			if ( t < 'a' || t > 'x' )
				return 0;
		}
	}

	if ( t < 0 )
		t += 1440;

	t1 = ( toupper((int) p[1] ) - 'A' ) * 60 + ( islower((int) p[1] ) ? 30 : 0 );
	t2 = ( toupper((int) p[2] ) - 'A' ) * 60 + ( islower((int) p[2] ) ? 29 : -1 );

	return (( t1 <= t2 && t >= t1 && t <= t2 )
		|| ( t1 > t2 && ( t >= t1 || t <= t2 )));
}


static int checktxy(const char *flags)
{
	char	*u, *p, *w;
	int	rc = 0;

	DEBUG(('N',2,"checktxy(%s)",flags));
	w = xstrdup( flags );
	u = w;
	while(( p = strsep( &u, "," )) && !rc ) {
		DEBUG(('N',2,"token: '%s'",p));
		if ( !strcmp( p, "CM" ))
			rc = 1;
		else if ( p[0] == 'T' && strlen( p ) == 3 )
			rc = chktxy( p );
	}

	xfree( w );
	DEBUG(('N',2,"checktxy(%s): %d",flags,rc));
	return rc;
}


static int parsetime(const char *tstr, int *day, int *hour, int *min)
{
	char	buf[MAX_STRING + 5];
	char	*s;
	int	l = 0;

	DEBUG(('N',2,"parsetime: '%s' %d, %d, %d", tstr, *day, *hour, *min));
	if ( !tstr || !*tstr ) {
		*day = *hour = *min = -1;
		return 0;
	}

	while ( *tstr ) {
		if ( strchr( "0123456789.:", *tstr ))
			buf[l++] = *tstr;
		tstr++;
	}
	buf[l] = '\0';

	s = strchr( buf, '.' );
	if ( s ) {
		*day = atoi( s - 1 );
		s++;
	} else
		s = buf;

	*hour = atoi( s );

	s = strchr( s, ':' );
	*min = ( s && isdigit(*(s + 1))) ? atoi( s + 1 ) : 0;

	DEBUG(('N',3,"parsetime: %d, %d, %d", *day, *hour, *min));
	return 1;
}


#define XFR0	{ xfree( rs ); return 0; }
#define XFR1	{ xfree( rs ); return 1; }

/* most part of checktimegaps() was taken from ifcico-tx.sc by Alexey Gretchaninov */
int checktimegaps(const char *ranges)
{
	int	firstDay, firstHour, firstMinute,
		secondDay, secondHour, secondMinute,
		firstMark, secondMark, currentMark,
		Day, Hour, Min;
	char	*rs, *rsrun, *r;
	time_t	tim;
	struct	tm *ti;

	DEBUG(('N',1,"checktimegaps: '%s'",ranges));
	
	if ( !ranges )
		return 0;

	if ( !strlen( ranges ))
		return 0;

	if ( !( rs = xstrdup( ranges )))
		return 0;

	rsrun = rs;

	while(( r = strsep( &rsrun, "," ))) {
		char *p;

		DEBUG(('N',2,"token: '%s'",r));

		while( *r == ' ' )
			r++;

		if ( !*r )
			continue;

		if ( !strcasecmp( r, "cm" ))
			XFR1;

		if ( !strcasecmp( r, "never" ))
			XFR0;

		if ( r[0] == 'T' ) {
			if ( chktxy( r ))
				XFR1
			else
				continue;
		}

		firstDay = secondDay = -1;
		firstHour = secondHour = -1;
		firstMinute = secondMinute = -1;

		if (( p = strchr( r, '-' ))) {
			/* got range [d.]hh[:mm]-[d.]hh[:mm] */
			
			*p = '\0';
			parsetime( r, &firstDay, &firstHour, &firstMinute );
			
			if ( firstDay != -1 )
				secondDay = firstDay;
			
			parsetime( p + 1, &secondDay, &secondHour, &secondMinute );
		} else {
			/* got [d.](Txy|CM|Never) */
			char *s = strchr( r, '.' );

			if ( s ) {
				firstDay = atoi( r );
				s++;
			} else
				s = r;

			secondDay = firstDay;

			if ( checktxy( s )) {
				firstHour = firstMinute = 0;
				secondHour = 23;
				secondMinute = 59;
			}
		}

		if( firstDay < -1 || firstDay > 7 || firstDay == 0 ||
			firstHour <= -1   || firstHour > 23 ||
			firstMinute < -1  || firstMinute > 59 ||
			secondDay < -1    || secondDay > 7 || secondDay == 0 ||
			secondHour < -1   || secondHour > 24 ||
			secondMinute < -1 || secondMinute > 59 ||
			( secondDay != -1 && secondHour == -1 ))
		{
			XFR0;
		}

		tim = time( NULL );
		ti = localtime( &tim );
		Day = ti->tm_wday;
		if ( !Day )
			Day = 7;

		Hour = ti->tm_hour;
		Min = ti->tm_min;
		firstMark = firstHour * 60;
		if ( firstMinute != -1 )
			firstMark += firstMinute;

		secondMark = secondHour * 60;
		if ( secondMinute != -1 )
			secondMark += secondMinute;

		currentMark = Hour * 60 + Min;

		DEBUG(('N',2,"Range: %d.%02d:%02d - %d.%02d:%02d, Now: %d.%02d:%02d",
			firstDay, firstHour, firstMinute,
			secondDay, secondHour, secondMinute,
			Day, Hour, Min));

		if ( secondDay != -1) {
			if ( firstDay < secondDay ) {
				if ( Day >= firstDay && Day <= secondDay) {
					if ( firstMark < secondMark ) {
						if ( currentMark >= firstMark && currentMark < secondMark )
							XFR1;
					} else if ( currentMark >= firstMark || currentMark < secondMark )
						XFR1;
				}
			} else 	if ( firstDay == secondDay ) {
				if ( Day == firstDay ) {
					if ( firstMark <= secondMark ) {
						if ( currentMark >= firstMark && currentMark < secondMark )
							XFR1;
					} else if ( currentMark >= firstMark || currentMark < secondMark )
						XFR1;
				}
			} else if ( Day >= firstDay || Day <= secondDay ) {
				if ( firstMark <= secondMark ) {
					if(currentMark >= firstMark && currentMark < secondMark )
						XFR1;
				} else if ( currentMark >= firstMark || currentMark < secondMark )
					XFR1;
			}
		} else if ( secondHour != -1 ) {
			if ( firstMark <= secondMark ) {
				if ( currentMark >= firstMark && currentMark < secondMark )
					XFR1;
			} else if ( currentMark >= firstMark || currentMark < secondMark )
				XFR1;
		} else if ( firstDay != -1 ) {
			if ( firstMinute != -1 ) {
				if ( Day == firstDay && Hour == firstHour && Min == firstMinute )
					XFR1;
			} else if ( Day == firstDay && Hour == firstHour )
				XFR1;
		} else {
			if( firstMinute != -1 ) {
				if ( Hour == firstHour && Min == firstMinute )
					XFR1;
			} else if( Hour == firstHour )
				XFR1;
		}
	}

	XFR0;
}

#undef XFR0
#undef XFR1


static subst_t *findsubst(const ftnaddr_t *fa, subst_t *subs)
{
	DEBUG(('N',2,"findsubst: %s [%d]", ftnaddrtoa( fa ), subs ? subs->nhids : 0 ));
	while( subs && !addr_cmp( fa, &subs->addr )) {
		subs = subs->next;
	}
	return subs;
}


typedef enum { SUBST_PHONE, SUBST_WTIME, SUBST_FLAGS } subst_field_t;

subst_t *parsesubsts(faslist_t *sbs)
{
	subst_t		*subs = NULL;
	dialine_t	*d, *c;

	while( sbs ) {
		char		*pstr, *p, *t;
		subst_t		*q = findsubst( &sbs->addr, subs );
		subst_field_t	field = SUBST_PHONE;

		if ( !q ) {
			q = xmalloc( sizeof( subst_t ));
			q->next = subs;
			subs = q;
			addr_cpy( &q->addr, &sbs->addr );
			q->hiddens = q->current = NULL;
			q->nhids = 0;
		}

		d = xmalloc( sizeof( dialine_t ));

		/* Insert ind _end_ of list */
		c = q->hiddens;
		if ( !c ) {
			q->current = q->hiddens = d;
		} else {
			while( c->next )
				c = c->next;
			c->next = d;
		}

		d->num = ++q->nhids;
		d->phone = d->timegaps = d->host = NULL;
		d->next = NULL;
		d->flags = 0;

		pstr = p = xstrdup( skip_blanks( sbs->str ));
		while(( t = strsep( &p, " " ))) {
			if ( *t == '\0' )
				continue;

			DEBUG(('N',2,"parsesubst: field [%d], token '%s'", (int) field, t ));
			if ( strcmp( t, "-" ) ) {
				switch( field ) {
				case SUBST_PHONE:
					d->phone = xstrdup( t );
					DEBUG(('N',3,"parsesubst: phone/host '%s'", t ));
					break;

				case SUBST_WTIME:
					d->timegaps = xstrdup( t );
					DEBUG(('N',3,"parsesubst: time '%s'", t ));
					break;

				case SUBST_FLAGS:
					if ( !strcasecmp( t, "ifc" ))
						d->flags |= MO_IFC;
#ifdef WITH_BINKP
					else if ( !strcasecmp( t, "binkp" ))
						d->flags |= MO_BINKP;
#endif
					else
						ndl_log( "unknown subst flag: '%s'", t );

					if ( d->flags ) {
						d->host = xstrdup( d->phone ? d->phone : ftnaddrtoia( &sbs->addr ));
						xfree( d->phone );
						DEBUG(('N',3,"parsesubst: host '%s'", d->host ));
					}
					break;
				
				default:
					DEBUG(('N',2,"parsesubst: unknown subst field"));
				}
			}
			field++;
		}
		xfree( pstr );
		sbs = sbs->next;
	}
	return subs;
}


int applysubst(ninfo_t *nl, subst_t *subs)
{
	subst_t		*sb = findsubst( &nl->addrs->addr, subs );
	dialine_t	*d;
	ninfo_t		*from_nl = NULL;

	if ( !sb )
		return 0;

	d = sb->current;
	sb->current = sb->current->next;
	if ( !sb->current )
		sb->current = sb->hiddens;

	if ( !d->phone && !d->host )
		nodelist_query( &nl->addrs->addr, &from_nl );

	if ( d->phone ) {
		xfree( nl->phone );
		nl->phone = xstrdup( d->phone );
	} else if ( from_nl && from_nl->phone ) {
		xfree( nl->phone );
		nl->phone = xstrdup( from_nl->phone );
		phonetrans( &nl->phone, cfgsl( CFG_PHONETR ));
	}

	if ( d->host ) {
		xfree( nl->host );
		nl->host = xstrdup( d->host );
	}

	if ( d->timegaps ) {
		xfree( nl->wtime );
		nl->wtime = xstrdup( d->timegaps );
	}

	nl->hidnum = ( sb->nhids > 1 ) ? d->num : 0;
	nl->opt &= ~(MO_BINKP|MO_IFC);
	nl->opt |= d->flags;
	if ( from_nl )
		nlkill( &from_nl );
	return 1;
}


void killsubsts(subst_t **l)
{
	while( l && *l ) {
		subst_t		*t = (*l)->next;
		dialine_t	*d = (*l)->hiddens;

		while( d ) {
			dialine_t *e = d->next;

			xfree( d->phone );
			xfree( d->timegaps );
			xfree( d->host );
			xfree( d );
			d = e;
		}
		xfree( *l );
		*l = t;
	}
}


int can_dial(ninfo_t *nl, int ct)
{
	DEBUG(('N',2,"can_dial: ct %d",ct));

	if ( !nl )
		return 0;

	if ( !nl->host || !*nl->host ) {
		char	*p;
		int	d = 0;

		if ( !*nl->phone )
			return 0;

		for( p = nl->phone; *p; p++ )
			if ( !strchr( "0123456789*#TtPpRr,.\"Ww@!-", *p ))
				d++;

		if ( d > 0 )
			return 0;
	}

	if ( ct )
		return 1;

	DEBUG(('N',2,"can_dial: wtime '%s'",nl->wtime));
	if ( nl->wtime ) {
		int i = checktimegaps( nl->wtime );
		DEBUG(('N',2,"can_dial: %s by checktimegaps() == %d",i?"OK":"NO",i));
		return i;
	}

	if ( nl->type == NT_HOLD || nl->type == NT_DOWN
		|| ( nl->type == NT_PVT && !nl->host ))
	{
		DEBUG(('N',2,"can_dial: NO by type"));
		return 0;
	}

	if ( checktxy( nl->flags )) {
		DEBUG(('N',2,"can_dial: OK by flags '%s'",nl->flags));
		return 1;
	}

	if ( nl->addrs->addr.p == 0 && MAILHOURS ) {
		DEBUG(('N',2,"can_dial: by MAILHOURS"));
		return 1;
	}

	DEBUG(('N',2,"can_dial: NO"));
	return 0;
}


int find_dialable_subst(ninfo_t *nl, int ct, subst_t *subs)
{
	int start;

	applysubst( nl, subs );
	start = nl->hidnum;
	while( !can_dial( nl, ct ) &&
		nl->hidnum &&
		applysubst( nl, subs ) &&
		nl->hidnum != start );

	return can_dial( nl, ct );
}


static void nlfree(ninfo_t *nl)
{
	if ( !nl )
		return;
	falist_kill( &nl->addrs );
	xfree( nl->name );
	xfree( nl->place );
	xfree( nl->sysop );
	xfree( nl->phone );
	xfree( nl->wtime );
	xfree( nl->flags );
	xfree( nl->pwd );
	xfree( nl->mailer );
	xfree( nl->host );
}


void nlkill(ninfo_t **nl)
{
	if ( !*nl )
		return;
	nlfree( *nl );
	xfree( *nl );
}
