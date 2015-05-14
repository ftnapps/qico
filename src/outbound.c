/**********************************************************
 * Outbound management.
 **********************************************************/
/*
 * $Id: outbound.c,v 1.21 2005/08/10 19:45:50 mitry Exp $
 *
 * $Log: outbound.c,v $
 * Revision 1.21  2005/08/10 19:45:50  mitry
 * Added param to qscandir() to return full path with file name
 *
 * Revision 1.20  2005/05/17 18:17:42  mitry
 * Removed system basename() usage.
 * Added qbasename() implementation.
 *
 * Revision 1.19  2005/05/16 20:33:46  mitry
 * Code cleaning
 *
 * Revision 1.18  2005/05/11 20:17:36  mitry
 * Changed int var; to size_t var; for strlen()
 *
 * Revision 1.17  2005/05/11 16:37:41  mitry
 * Set initial values to static variables explicitly
 *
 * Revision 1.16  2005/05/06 20:36:45  mitry
 * Major rewrite again
 *
 * Revision 1.15  2005/04/14 18:04:26  mitry
 * Changed scandir() to qscandir()
 *
 * Revision 1.14  2005/04/05 09:31:12  mitry
 * New xstrcpy() and xstrcat()
 *
 * Revision 1.13  2005/03/28 16:37:26  mitry
 * Moved some code to mappath() function (see tools.c)
 *
 * Revision 1.12  2005/02/16 21:42:47  mitry
 * Check the value returned from scandir (n <= 0) for out_aso_rescan(), out_bso_scan_zone(), out_bso_rescan().
 *
 * Revision 1.11  2005/02/16 11:25:29  mitry
 * Next fix for out_bso_scan_zone()
 *
 * Revision 1.10  2005/02/15 22:12:26  mitry
 * Fix for out_bso_scan_zone()
 *
 * Revision 1.9  2005/02/15 11:17:39  mitry
 * Another fix for out_bso_rescan()
 *
 * Revision 1.8  2005/02/09 12:44:23  mitry
 * Changed free() to xfree()
 * Changed out_bso_rescan() code
 *
 * Revision 1.7  2005/02/08 19:57:28  mitry
 * Added outbound_addr_busy().
 * Wiped out old outbound_rescan code.
 * Cleaned code a bit.
 *
 */

#include "headers.h"

static int bso_defzone = 2;
static char *bso_base = NULL;
static char *aso_base = NULL;
static char *p_domain = NULL;
static char *sts_base = NULL;
static size_t pd_len = 0;
static char out_internal_tmp[MAX_PATH + 5];

static qeach_t _each = NULL;
static int _rslow;


static char *ext_lo[] = {
    ".pnt", ".req",
    ".ilo", ".clo", ".dlo", ".flo", ".hlo",
    NULL
};

static char *ext_ut[] = {
    ".iut", ".cut", ".dut", ".out", ".hut",
    NULL
};


int outbound_init(const char *asopath, const char *bsopath,
	const char *stspath, int def_zone)
{
    int		rc = 0;
    size_t	len = 0;

    outbound_done();

    if ( bsopath && *bsopath ) {
        char *p;
        
        len = strlen( bsopath ) - 1;
        xstrcpy( out_internal_tmp, bsopath, MAX_PATH );
        len = chopc( out_internal_tmp, '/' );

        if ( len ) {
            p = strrchr( out_internal_tmp, '/' );

            if ( p ) {
                p_domain = xstrdup( p + 1 );
                pd_len = strlen( p_domain );

                if ( p == out_internal_tmp )
                    *(p + 1) = '\0';
                else
                    *p = '\0';
                bso_base = xstrdup( out_internal_tmp );
                bso_defzone = def_zone;
                rc |= BSO;
            } else
                len = 0;
        }
    }

    if ( !len ) {
        xfree( bso_base );
        xfree( p_domain );
    }

    len = 0;

    if ( asopath && *asopath ) {
        len = strlen( asopath ) - 1;
        xstrcpy( out_internal_tmp, asopath, MAX_PATH );
        len = chopc( out_internal_tmp, '/' );

        if ( !len )
            aso_base = xstrdup( "/" );
        else
            aso_base = xstrdup( out_internal_tmp );
        rc |= ASO;
    }

    if ( !rc )
        return 0;    

    if ( stspath && *stspath ) {
        xstrcpy( out_internal_tmp, stspath, MAX_PATH );
        len = chopc( out_internal_tmp, '/' );
        if ( !len )
            sts_base = xstrdup( "/" );
        else
            sts_base = xstrdup( stspath );
    } else if ( bsopath )
        sts_base = xstrdup( bsopath );
    else if ( asopath )
        sts_base = xstrdup( asopath );

    return rc;
}


void outbound_done(void)
{
    xfree( bso_base );
    xfree( p_domain );
    xfree( aso_base );
    xfree( sts_base );
    pd_len = 0;
}


static char *get_outbound_name(int type, const ftnaddr_t *fa)
{
    out_internal_tmp[0] = '\0';

    switch( type ) {
    case ASO:
        if ( aso_base )
            snprintf( out_internal_tmp, MAX_PATH, "%s/%d.%d.%d.%d.", aso_base, fa->z, fa->n, fa->f, fa->p) ;
        break;

    case BSO:
        if ( bso_base ) {
            char t[30];

            snprintf( out_internal_tmp, MAX_PATH, "%s/%s", bso_base, p_domain );
            if ( fa->z != bso_defzone ) {
                snprintf( t, 30, ".%03x", fa->z );
                xstrcat( out_internal_tmp, t, MAX_PATH );
            }
            snprintf( t, 30, "/%04x%04x.", fa->n, fa->f );
            xstrcat( out_internal_tmp, t, MAX_PATH );
            if( fa->p ) {
                snprintf( t, 30, "pnt/%08x.", fa->p );
                xstrcat( out_internal_tmp, t, MAX_PATH );
            }
        }
        break;

    default:
        return NULL;
    }

    if ( *out_internal_tmp )
        return out_internal_tmp;

    return NULL;
}


static char *get_pkt_name(int type, const ftnaddr_t *fa, int fl)
{
    char e[] = "eut";
    char *name = get_outbound_name( type, fa );

    if ( name ) {
        switch( fl ) {
            case F_NORM:
            case F_REQ:  *e = 'o'; break;
            case F_DIR:  *e = 'd'; break;
            case F_CRSH: *e = 'c'; break;
            case F_HOLD: *e = 'h'; break;
            case F_IMM:  *e = 'i'; break;
        }
        xstrcat( name, e, MAX_PATH );
    }

    return name;
}


static char *get_flo_name(int type, const ftnaddr_t *fa, int fl)
{
    char e[] = "elo";
    char *name = get_outbound_name( type, fa );

    if ( name ) {
        switch( fl ) {
            case F_NORM:
            case F_REQ:  *e = 'f'; break;
            case F_DIR:  *e = 'd'; break;
            case F_CRSH: *e = 'c'; break;
            case F_HOLD: *e = 'h'; break;
            case F_IMM:  *e = 'i'; break;
        }
        xstrcat( name, e, MAX_PATH );
    }

    return name;
}


static char *get_req_name(int type, const ftnaddr_t *fa)
{
    char *name = get_outbound_name( type, fa );

    if ( name )
        xstrcat( name, "req", MAX_PATH );

    return name;
}


static char *get_busy_name(int type, const ftnaddr_t *fa, char b)
{
    char bn[] = "esy";
    char *name = get_outbound_name( type, fa );

    if ( name ) {
        *bn = b;
        xstrcat( name, bn, MAX_PATH );
    }

    return name;
}


static char *get_status_name(const ftnaddr_t *fa)
{
    if ( sts_base ) {
        snprintf( out_internal_tmp, MAX_PATH, "%s/%d.%d.%d.%d.qst", sts_base, fa->z, fa->n, fa->f, fa->p);
        return out_internal_tmp;
    }
    return NULL;
}


static void out_scan_flo(const char *flo_name, const ftnaddr_t *addr)
{
	size_t len = strlen( flo_name );

	DEBUG(('S',3,"out_scan_flo: flo_name == '%s'", flo_name));
	if ( !strcasecmp( (char *) (flo_name + len - 2), "lo" ))
		(*_each)( flo_name, addr, T_ARCMAIL, outbound_flavor( flo_name[len - 3] ), _rslow );
	else if ( !strcasecmp( (char *) (flo_name + len - 2), "ut" ))
		(*_each)( flo_name, addr, T_NETMAIL, outbound_flavor( flo_name[len - 3] ), _rslow );
	else if ( !strcasecmp( (char *) (flo_name + len - 3), "req" ))
		(*_each)( flo_name, addr, T_REQ, F_REQ, _rslow );
}


/*
 * Returns whether the directory entry is an ASO file.
 */
static int aso_select(const char *d_name)
{
	size_t	len;
	char	**p;

	DEBUG(('S',3,"aso_select: '%s'",d_name));

	if ( *d_name == '.' )
		return 0;

	if ( strrchr( d_name, '.' ) == NULL )
		return 0;

	if ( sscanf( d_name, "%u.%u.%u.%u", &len, &len, &len, &len ) != 4 )
		return 0;

	len = strlen( d_name ) - 4;
	for( p = ext_lo; *p; p++ )
		if ( !strcasecmp( *p, d_name + len )) {
			DEBUG(('S',3,"aso_select: '%s' OK",d_name));
			return 1;
		}

	for( p = ext_ut; *p; p++ )
		if ( !strcasecmp( *p, d_name + len )) {
			DEBUG(('S',3,"aso_select: '%s' OK",d_name));
			return 1;
		}

	return 0;
}


static int out_aso_rescan(void)
{
	char	**namelist = NULL;
	int	i, n;

	if ( aso_base == NULL )
		return 0;
    	
	DEBUG(('S',4,"out_aso_rescan: aso_base == '%s'", aso_base));

	n = qscandir( aso_base, &namelist, 0, aso_select, NULL );
	DEBUG(('S',4,"out_aso_rescan: n == %d", n));

	if ( n < 0 ) /* Error */
		return 0;

	for( i = 0; i < n; i++ ) {
		char	*f = qbasename( namelist[i] );
		int	az, an, af, ap;

		if ( sscanf( f, "%u.%u.%u.%u", &az, &an, &af, &ap ) == 4 ) {

			FTNADDR_T( a );
			a.z = (short) az;
			a.n = (short) an;
			a.f = (short) af;
			a.p = (short) ap;

			DEBUG(('S',4,"out_aso_rescan: flo_name == '%s'", namelist[i]));
			out_scan_flo( namelist[i], &a );
		}
		xfree( namelist[i] );
	}

	xfree( namelist );
	return ASO;
}


/*
 * Returns whether the directory entry is a BSO file.
 */
static int bso_select(const char *d_name)
{
	size_t	len;
	char	**p;

	DEBUG(('S',3,"bso_select: '%s'",d_name));

	if ( *d_name == '.' )
		return 0;

	len = strlen( d_name );
	if ( len != 12 )
		return 0;

	if ( strspn( d_name, "0123456789abcdefABCDEF" ) != 8 )
		return 0;

	for( p = ext_lo; *p; p++ )
		if ( !strcasecmp( *p, &d_name[len - 4] )) {
			DEBUG(('S',3,"bso_select: '%s' OK",d_name));
			return 1;
		}
	
	for( p = ext_ut; *p; p++ )
		if ( !strcasecmp( *p, &d_name[len - 4] )) {
			DEBUG(('S',3,"bso_select: '%s' OK",d_name));
			return 1;
		}

	return 0;
}


/*
 * Returns whether the directory entry is a BSO zone.
 */
static int bso_root_select(const char *d_name)
{
	size_t len = strlen( d_name );

	DEBUG(('S',3,"bso_root_select: '%s'",d_name));

	if ( *d_name == '.' )
		return 0;

	if ( strncasecmp( p_domain, d_name, pd_len ) != 0 )
		return 0;

	if ( len == pd_len ||
		( len - 4 == pd_len && strspn( d_name + len - 3, "0123456789abcdefABCDEF" ) == 3 ))
	{
		DEBUG(('S',3,"bso_root_select: '%s' OK",d_name));
		return 1;
	}

	return 0;
}


static int out_bso_scan_zone(char *zonehold)
{
	int		i, j, n;
	char		*p = NULL, **namelist = NULL;
	FTNADDR_T	( a );

	DEBUG(('S',3,"out_bso_scan_zone: %s", zonehold));

	n = qscandir( zonehold, &namelist, 0, bso_select, qalphasort );
	DEBUG(('S',4,"out_bso_scan_zone: n == %d", n));

	if ( n <= 0 ) /* Error */
		return 0;

	p = strrchr( zonehold, '.' );
	if ( p != NULL) {
		sscanf( p, ".%03x", &i );
		a.z = (short) i;
	} else
		a.z = bso_defzone;

	for( i = 0; i < n; i++ ) {
		int	an, af;
		
		p = qbasename( namelist[i] );
		sscanf( p, "%04x%04x", &an, &af );

		a.n = (short) an;
		a.f = (short) af;
		a.p = 0;

		DEBUG(('S',4,"out_bso_scan_zone: i == %d, lo == '%s'", i, namelist[i]));
		DEBUG(('S',4,"out_bso_scan_zone: %d:%d/%d.%d", a.z, a.n, a.f, a.p));

		if ( !strcasecmp( p + 8, ".pnt" )) {
			char	**namelist_pnt = NULL;
			int	points;

			points = qscandir( namelist[i], &namelist_pnt, 0, bso_select, qalphasort );
			DEBUG(('S',4,"out_bso_scan_zone: points %d", points));

			if ( points >= 0 ) {
				if ( points == 0 ) {
					DEBUG(('S',4,"out_bso_scan_zone: remove empty points dir'%s'", namelist[i]));
					rmdir( namelist[i] );
				} else {
					for( j = 0; j < points; j++ ) {
						int	ap;
						
						p = qbasename( namelist_pnt[j] );

						sscanf( p + 4, "%04x", &ap );
						a.p = (short) ap;
						DEBUG(('S',4,"out_bso_scan_zone: point %d:%d/%d.%d", a.z, a.n, a.f, a.p));
						DEBUG(('S',4,"out_bso_scan_zone: flo '%s'", namelist_pnt[j]));
						out_scan_flo( namelist_pnt[j], &a );
						xfree( namelist_pnt[j] );
					}
					xfree( namelist_pnt );
				}
			}
		} else {
			out_scan_flo( namelist[i], &a );
		}

		xfree( namelist[i] );
	}

	xfree( namelist );
	return 1;
}


static int out_bso_rescan(void)
{
	int	i, n;
	char	**namelist = NULL;

	if ( bso_base == NULL )
		return 0;

	DEBUG(('S',4,"out_bso_rescan: bso_base == '%s', p_domain == '%s'", bso_base, p_domain));

	/* Scan the root and find all outbound directory */
	n = qscandir( bso_base, &namelist, 0, bso_root_select, qalphasort );
	DEBUG(('S',4,"out_bso_rescan: n == %d", n));

	if ( n < 0 )
		return 0;

	/* For each directory found do recursive rescan */
	for( i = 0; i < n; i++ ) {
		DEBUG(('S',4,"out_bso_rescan: namelist[%d] == '%s'", i, namelist[i]));
		out_bso_scan_zone( namelist[i] );
		xfree( namelist[i] );
	}
	
	xfree( namelist );
	return BSO;
}


int outbound_rescan(qeach_t each, int rslow)
{
	int rc = 0;

	_each = each;
	_rslow = rslow;

	rc |= out_aso_rescan();
	rc |= out_bso_rescan();

	return rc;
}


static int out_lock(int type, const ftnaddr_t *adr, int l)
{
	char *obn = get_busy_name( type, adr, 'b' );

	if ( !obn )
		return 0;

	mkdirs( obn );

	if ( islocked( obn ))
		return 0;
    
	if ( l == LCK_s )
		return lockpid( obn );

	if ( l == LCK_t )
		return getpid();

	if ( islocked( get_busy_name( type, adr, 'c' )))
		return 0;
    
	return lockpid( out_internal_tmp );
}


/*
 * Returns whether the address is busy. 0 == free, 1 == busy.
 */
int outbound_addr_busy(const ftnaddr_t *addr)
{
	if ( islocked( get_busy_name( ASO, addr, 'b' )))
		return 1;

	if ( islocked( get_busy_name( ASO, addr, 'c' )))
		return 1;

	if ( islocked( get_busy_name( BSO, addr, 'b' )))
		return 1;

	if ( islocked( get_busy_name( BSO, addr, 'c' ) ))
		return 1;

	return 0;
}


int outbound_locknode(const ftnaddr_t *addr, int l)
{
	int rc = 0;

	if ( aso_base && out_lock( ASO, addr, l ))
		rc |= ASO;
	
	if ( bso_base && out_lock( BSO, addr, l ))
		rc |= BSO;
	
	return rc;
}


int outbound_unlocknode(const ftnaddr_t *addr, int l)
{
	if ( l == LCK_t )
		return 1;

	if ( aso_base ) {
		if ( get_busy_name( ASO, addr, 'b' ))
			lunlink( out_internal_tmp );

		if ( get_busy_name( ASO, addr, 'c' ))
			lunlink( out_internal_tmp );
	}

	if ( bso_base ) {
		if ( get_busy_name( BSO, addr, 'b' ))
			lunlink( out_internal_tmp );

		if ( get_busy_name( BSO, addr, 'c' ))
			lunlink( out_internal_tmp );

		if ( addr->z != bso_defzone )
			rmdirs( out_internal_tmp );
		else if ( addr->p ) {
			rmdir( out_internal_tmp );
		}
	}

	return 1;
}


int outbound_flavor(char fl)
{
	switch( tolower( fl )) {
	case 'h': return F_HOLD;
	case 'f':
	case 'n':
	case 'o': return F_NORM;
	case 'd': return F_DIR;
	case 'c': return F_CRSH;
	case 'i': return F_IMM;
	case 'r': return F_REQ;
	}
	return F_ERR;
}


int outbound_attach(const ftnaddr_t *addr, int flv, slist_t *files)
{
	FILE *f = NULL;

	if ( bso_base )
		f = mdfopen( get_flo_name( BSO, addr, flv ), "at" );
	if ( !f && aso_base )
		f = mdfopen( get_flo_name( ASO, addr, flv ), "at" );
	if ( f ) {
		slist_t *fl;

		for( fl = files; fl; fl = fl->next )
			fprintf( f, "%s\n", fl->str );
		fclose( f );
		return 1;
	}
	return 0;
}


int outbound_request(const ftnaddr_t *addr, slist_t *files)
{
	FILE *f = NULL;
	
	if ( bso_base )
		f = mdfopen( get_req_name( BSO, addr ), "at" );
	if ( !f && aso_base )
		f = mdfopen( get_req_name( ASO, addr ), "at" );
	if ( f ) {
		slist_t *fl;

		for( fl = files; fl; fl = fl->next )
			fprintf( f, "%s\r\n", fl->str );
		fclose( f );
		return 1;
	}
	return 0;
}


int outbound_poll(const ftnaddr_t *fa, int flavor)
{
	if ( flavor == F_ERR ) {
		char *pf = cfgs( CFG_POLLFLAVOR );
		flavor = outbound_flavor( *pf );
	}
	return outbound_attach( fa, flavor, NULL );
}


int outbound_setstatus(const ftnaddr_t *fa, sts_t *st)
{
	FILE *f;
	char *name = get_status_name( fa );

	if((f = mdfopen( name, "wt" ))) {
		fprintf( f, "%d %d %lu %lu", st->try, st->flags, st->htime, st->utime );
		if ( st->bp.name && st->bp.flags )
			fprintf( f, " %d %d %lu %s", st->bp.flags, st->bp.size, st->bp.time, st->bp.name );
		fclose( f );
		return 1;
	}
	return 0;
}


int outbound_getstatus(const ftnaddr_t *fa, sts_t *st)
{
	int rc;
	FILE *f;
	char *name = get_status_name( fa );
	char tmp[MAX_PATH + 5];

	tmp[0] = '\0';

	if ( name ) {
		if ((f = fopen( name, "rt" ))) {
			rc = fscanf( f, "%d %d %lu %lu %d %d %lu %s",
				&st->try, &st->flags,
				(unsigned long*) &st->htime,
				(unsigned long*) &st->utime,
				&st->bp.flags, (int*) &st->bp.size,
				(unsigned long*) &st->bp.time,
				tmp );
			fclose( f );

			if ( rc < 8 )
				memset( &st->bp, 0, sizeof( st->bp ));
			else if ( *tmp )
				st->bp.name = xstrdup( tmp );

			if ( rc == 4 || rc == 8 )
				return 1;
			write_log( "status file %s corrupted", name );
        		xfree( st->bp.name );
		}
	}
	memset( st, 0, sizeof( sts_t ));
	return 0;
}


static void floflist(flist_t **fl, char *flon)
{
    FILE *f;
    off_t off;
    char *p, str[MAX_PATH + 5], *m, *map = cfgs(CFG_MAPOUT), *fp, *fn;
    struct stat sb;

    DEBUG(('S',2,"Add LO '%s'",flon));
    if(!stat(flon, &sb))if((f=fopen(flon,"r+b"))) {
        off = ftello( f );
        while(fgets(str,MAX_PATH,f)) {
            p=strrchr(str,'\r');
            if(!p)p=strrchr(str,'\n');
            if(p)*p=0;
            if(!*str)continue;
            p=str;
            switch(*p) {
                case '~':
                    break;

                case '^': /* kill */
                case '#': /* trunc */
                case ' ': /* filename starting with directive */
                    p++;

                default:
                    if (( m = mappath( p )) && *m )
                        p = m;
                    fp = xstrdup( p );
                    m = qbasename( fp );
                    if ( map && strchr( map, 'L' ))
                        strlwr( m );
                    else if( map && strchr( map, 'U' ))
                        strupr( m );
                    fn = qbasename( p );
                    mapname( fn, map, MAX_PATH - ( fn - str ) - 1 );
                    addflist( fl, fp, xstrdup( fn ), str[0], off, f, 1 );
                    if( !stat( fp, &sb )) {
                        totalf += sb.st_size;
                        totaln++;
                    }
            }
            off = ftello( f );
        }
        addflist( fl, xstrdup( flon ), NULL, '^', -1, f, 1 );
    }
}


int asoflist(flist_t **fl, const ftnaddr_t *fa, int mode)
{
	struct stat	sb;
	char		str[MAX_PATH + 5];
	int		fls[] = { F_IMM, F_CRSH, F_DIR, F_NORM, F_HOLD }, i;

#if 0
	unsigned long	_tm = totalm, _tn = totaln, _tf = totalf;

	DEBUG(('S',2,"asoflist exp in: m %ld, n %ld, f %ld",totalm,totaln,totalf));
	{
	char *name = NULL, **flolist = NULL;
	int t;
	
	if ( bso_base && ( name = get_outbound_name( BSO, fa ))) {
		int flo_count = fmatchcase( name, &flolist ), i;

		DEBUG(('S',3,"asoflist: bso name '%s",name));
		DEBUG(('S',3,"asoflist: bso count %d",flo_count));
		for( i = 0; flo_count && i < flo_count; i++ ) {
			t = whattype( flolist[i] );
			DEBUG(('S',4,"asoflist: type %d, flo '%s'",t,flolist[i]));
			switch( t ) {
			case IS_PKT:
				snprintf( str, MAX_PATH, "%08lx.pkt", sequencer());
				totalm += sb.st_size;
				break;

			case IS_REQ:
				snprintf( str, MAX_PATH, "%04x%04x.req", fa->n, fa->f );
				totalf += sb.st_size;
				break;

			case IS_FLO:
				floflist( fl, flolist[i] );
                                /* Fall through */

			default:
				t = IS_ERR;
			}

			if ( t != IS_ERR ) {
				addflist( fl, xstrdup( flolist[i] ), xstrdup( str ), '^', 0, NULL, 0 );
				totaln++;
			}
			xfree( flolist[i] );
		}
		if ( flo_count > 0 )
			xfree( flolist );
	}
        }
	DEBUG(('S',2,"asoflist exp out: m %ld, n %ld, f %ld",totalm,totaln,totalf));
    
        totalm = _tm;
        totaln = _tn;
        totalf = _tf;
#endif

	/* pkt files */
	for ( i = 0; i < 5; i++ ) {
		char *name = NULL;

		if ( bso_base && ( name = get_pkt_name( BSO, fa, fls[i] ))) {
			if ( !stat( name, &sb )) {
				snprintf( str, MAX_PATH, "%08lx.pkt", sequencer() );
				addflist( fl, xstrdup( name ), xstrdup( str ), '^', 0, NULL, 0 );
				totalm += sb.st_size;
				totaln++;
			}
		}

		if ( aso_base && ( name = get_pkt_name( ASO, fa, fls[i] ))) {
			if ( !stat( name, &sb )) {
				snprintf( str, MAX_PATH, "%08lx.pkt", sequencer() );
				addflist( fl, xstrdup( name ), xstrdup( str ), '^', 0, NULL, 0);
				totalm += sb.st_size;
				totaln++;
			}
		}
	}

	/* file requests */
	snprintf( str, MAX_PATH, "%04x%04x.req", fa->n, fa->f );
	{
		char *name = get_req_name( BSO, fa );
    
		if ( bso_base && name && !stat( name, &sb )) {
			addflist( fl, xstrdup( name ), xstrdup( str ), ' ', 0, NULL, 1);
			totalf += sb.st_size;
			totaln++;
		}
        
		name = get_req_name( ASO, fa );

		if ( aso_base && name && !stat( name, &sb )) {
			addflist( fl, xstrdup( name ), xstrdup( str ), ' ', 0, NULL, 1);
			totalf += sb.st_size;
			totaln++;
		}
	}

	/* flow files */
	for( i = 0; i < ( 5 - (( cfgi( CFG_HOLDOUT ) == 1) && mode )); i++) {
		char *name = NULL;

		if ( bso_base && (name = get_flo_name( BSO, fa, fls[i] )))
			floflist( fl, name );
		if ( aso_base && (name = get_flo_name( ASO, fa, fls[i] )))
			floflist( fl, name );
	}
	return 0;
}
