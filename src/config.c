/**********************************************************
 * Config parser.
 **********************************************************/
/*
 * $Id: config.c,v 1.10 2005/08/23 16:51:29 mitry Exp $
 *
 * $Log: config.c,v $
 * Revision 1.10  2005/08/23 16:51:29  mitry
 * Stop running if can't reread config
 *
 * Revision 1.9  2005/08/11 19:15:14  mitry
 * Changed code a bit
 *
 * Revision 1.8  2005/05/16 11:17:30  mitry
 * Updated function prototypes. Changed code a bit.
 *
 * Revision 1.7  2005/05/11 13:22:40  mitry
 * Snapshot
 *
 * Revision 1.6  2005/03/28 17:02:52  mitry
 * Pre non-blocking i/o update. Mostly non working.
 *
 * Revision 1.5  2005/02/09 11:14:09  mitry
 * Add support to escape next character with backslash (\)
 *
 *
 */

#include "headers.h"
#include "nodelist.h"

static aslist_t		*defs = NULL;
static slist_t		*condlist = NULL, *curcond;

#define IS_COMMENT( t ) ( *t == ';' || *t == '#' || ( *t== '/' && t[1] == '/' ))


static int getstr(char **to, const char *from)
{
	*to = xstrdup( from );
	return 1;
}


static int getpath(char **to, const char *from)
{
	if ( strcspn( from, "*[]?<>|" ) != strlen( from ))
		return 0;
	if ( from[strlen( from ) - 1] == '/' )
		chop( (char *) from, 1 );
	*to = xstrdup( from );
	return 1;
}


static int getlong(int *to, const char *from)
{
	if ( strspn( from, "0123456789 \t" ) != strlen( from ))
		return 0;
	*to = atol( from );
	return 1;
}


static int getoct(int *to, const char *from)
{
	if ( strspn( from, "01234567 \t" ) != strlen( from ))
		return 0;
	*to = strtol( from, (char**) NULL, 8 );
	return 1;
}


static int getaddrl(falist_t **to, const char *from)
{
	FTNADDR_T(ta);

	if ( !parseftnaddr( from, &ta, &DEFADDR, 0 ))
		return 0;
	if ( !DEFADDR.z ) {
		DEFADDR.z = ta.z;
		DEFADDR.n = ta.n;
	}
	falist_add( to, &ta );
	return 1;
}


static int getfasl(faslist_t **to, const char *from)
{
	char		*p;
	FTNADDR_T	(ta);

	if ( !parseftnaddr( from, &ta, &DEFADDR, 0 ))
		return 0;
	if ( !DEFADDR.z ) {
		DEFADDR.z = ta.z;
		DEFADDR.n = ta.n;
	}
	p = strchr( from, ' ' );
	if ( !p )
		return 0;
	p = skip_blanks( p );
	faslist_add( to, p, &ta );
	return 1;
}


static int getyesno(int *to, const char *from)
{
	switch( tolower( *from )) {
	case 'y':
	case 't':
	case '1':
		*to = 1;
		break;

	default:
		*to = 0;
	}
	return 1;
}


static int getstrl(slist_t **to, const char *from)
{
	slist_add( to, from );
	return 1;
}


static int setvalue(cfgitem_t *ci, const char *t, int type)
{
	switch( type ) {
	case C_STR:
		return getstr( &ci->value.v_char, t );
	case C_PATH:
		return getpath( &ci->value.v_char, t );
	case C_STRL:
		return getstrl( &ci->value.v_sl, t );
	case C_ADRSTRL:
		return getfasl( &ci->value.v_fasl, t );
	case C_ADDRL:
		return getaddrl( &ci->value.v_al, t );
	case C_INT:
		return getlong( &ci->value.v_int, t );
	case C_OCT:
		return getoct( &ci->value.v_int, t );
	case C_YESNO:
		return getyesno( &ci->value.v_int, t );
	}
	return 0;
}


int cfgi(int i)
{
	cfgitem_t *ci, *cn = NULL;

	for( ci = configtab[i].items; ci; ci = ci->next ) {
		if ( ci->condition && flagexp( ci->condition, 0 ) == 1 )
			return cci = ci->value.v_int;
		if ( !ci->condition )
			cn = ci;
	}
	return cci = cn->value.v_int;
}


char *cfgs(int i)
{
	cfgitem_t *ci, *cn = NULL;

	for( ci = configtab[i].items; ci; ci = ci->next ) {
		if ( ci->condition && flagexp( ci->condition, 0 ))
			return ccs = ci->value.v_char;
		if ( !ci->condition )
			cn = ci;
	}
	return ccs = cn->value.v_char;
}


slist_t *cfgsl(int i)
{
	cfgitem_t *ci, *cn = NULL;

	for ( ci = configtab[i].items; ci; ci = ci->next ) {
		if ( ci->condition && flagexp( ci->condition, 0 ))
			return ccsl = ci->value.v_sl;
		if ( !ci->condition )
			cn = ci;
	}
	return ccsl = cn->value.v_sl;
}


faslist_t *cfgfasl(int i)
{
	cfgitem_t *ci, *cn = NULL;

	for ( ci = configtab[i].items; ci; ci = ci->next ) {
		if ( ci->condition && flagexp( ci->condition, 0 ))
			return ccfasl = ci->value.v_fasl;
		if ( !ci->condition )
			cn = ci;
	}
	return ccfasl = cn->value.v_fasl;
}


falist_t *cfgal(int i)
{
	cfgitem_t *ci, *cn = NULL;

	for ( ci = configtab[i].items; ci; ci = ci->next ) {
		if ( ci->condition && flagexp( ci->condition, 0 ))
			return ccal = ci->value.v_al;
		if ( !ci->condition )
			cn = ci;
	}
	return ccal = cn->value.v_al;
}


static int parsekeyword(const char *kw, const char *arg, const char *cfgname, int line)
{
	int		i = 0, rc = 1;
	slist_t		*cc;
	cfgitem_t	*ci;

	while( configtab[i].keyword && strcasecmp( configtab[i].keyword, kw ))
		i++;

	DEBUG(('C',2,"parse: '%s', '%s' [%d] on %s:%d%s%s",
		kw,arg,i,cfgname,line,curcond?" (c) if ":"",curcond?curcond->str:""));

	if ( !configtab[i].keyword ) {
		write_log( "%s:%d: unknown keyword '%s'", cfgname, line, kw );
		return 0;
	}

        if ( curcond && ( configtab[i].flags & 2 )) {
		write_log( "%s:%d: keyword '%s' can't be defined inside if-expression",
			cfgname, line, kw );
		return 0;
	}

        for ( ci = configtab[i].items; ci; ci = ci->next ) {
		slist_t *a = ci->condition, *b = curcond;

		for(; a && b && a->str == b->str; a = a->next, b = b->next );

		if ( !a && !b )
			break;
	}

	if ( !ci ) {
		ci = (cfgitem_t *) xcalloc( 1, sizeof( cfgitem_t ));
		for( cc = curcond; cc; cc = cc->next )
			slist_addl( &ci->condition, cc->str );
		ci->next = configtab[i].items;
		configtab[i].items = ci;
        }

	if ( setvalue( ci, arg, configtab[i]. type )) {
		if ( !( configtab[i].flags & 8 )) {
			if ( !curcond )
				configtab[i].flags |= 8;
			else
				configtab[i].flags |= 4;
		}
	} else {
		xfree( ci );
		write_log( "%s:%d: can't parse '%s %s'", cfgname, line, kw, arg );
		rc=0;
	}
	return rc;
}


/**
  Reads in at most buflen - 1 characters from stream f and stores them 
  into the buffer pointed to by buf.

  Returns:
      On success - the number of characters read
      0 if line is comment
     -1 if error occured.

*/
static int xfgets(char *buf, size_t buflen, FILE *f)
{
	char	*str, *p, *c;
	int	rc = 0;

	str = xmalloc( buflen + 1 );

	if ( fgets( str, buflen, f ) == NULL ) {
		*buf = '\0';
		xfree( str );
		return -1;  /* error? */
	}

	p = skip_blanks( str );
	if ( *p && IS_COMMENT( p ) ) { /* comment, skip line */
		xfree( str );
		return 0;
	}

	c = p;
	while( *c ) {
		if ( *c == '\\' && !XBLANK( c + 1 ))
			xstrcpy( c, c + 1, strlen( c ));
		else if ( IS_COMMENT( c )) {
			*c = 0;
			break;
		}
		c++;
	}
	strtr( p, '\t', ' ');
	skip_blanksr( p );
	xstrcpy( buf, p, buflen );
	rc = strlen( p );
	xfree( str );

	return rc;
}


static int parseconfig(const char *cfgname)
{
    FILE *f;
    char s[LARGE_STRING + 1] = {0}, *p, *t, *k;
    int line = 0, rc = 1, xrc = 0;
    slist_t *cc;

    if ( !( f = fopen( cfgname, "r" ))) {
        write_log( "can't open config file '%s': %s", cfgname, strerror( errno ));
        return 0;
    }

    if ( verbose_config )
        printf( "***** %s\n", cfgname );
    
    while(( xrc = xfgets( s, LARGE_STRING, f )) >= 0 ) {
        line++;

        if ( xrc == 0 )
            continue;

contl:
        p = s;

        if ( verbose_config )
            printf( "** cfg line: %d: '%s', len = %-4d\n", line, p, strlen( s ));

        t = p + strlen( p ) - 1;

        if ( *t == '\\' && t > p && *( t - 1 ) == ' ' ) {
            do {
                line++;
            } while(( xrc = xfgets( t, LARGE_STRING - ( t - s ), f )) == 0 );
            goto contl;
        }

        if ( *p != '{' ) {
            if ( *p == '$' && isgraph( p[1] ))
                if (( k = strchr( p, '=' )))
                    *k = ' ';
            t = strchr( p, ' ' );
            /*
            if ( !t )
                t = strchr( p, '\n' );
            if ( !t )
                t = strchr( p, '\r' );
            */
            if( !t )
                t = strchr( p, 0 );
            *t++ = 0;
        } else
            t = p + 1;

contd:
        t = skip_blanks( t );

        if (( k = strchr( t, '$' )) && k[1] == '(' ) {
            char *e = (k + 2);
            char *en = NULL;
            aslist_t *as;

            for(; *e && isgraph( *e ) && *e != ')'; e++ );

            if ( *e == ')' ) {
                *e++ = 0;
                as = aslist_find( defs, k + 2 );

                if ( as == NULL && ( en = getenv( k + 2 ))) {
                    aslist_add( &defs, k + 2, en );  /* Add env define */
                    as = aslist_find( defs, k + 2 );
                }

                if ( as ) {
                    if ( verbose_config )
                        printf( "** %s define: '%s'->'%s'\n",
                            ( en ? "env" : "cfg" ), k + 2, as->arg );
                    e = xstrdup( e );
                    xstrcpy( k, as->arg, LARGE_STRING - ( k - s ));
                    if ( *e )
                        xstrcat( k, e, LARGE_STRING - ( k - s ));
                    xfree( e );
                    if ( verbose_config )
                        printf( "** cfg full line: '%s %s'\n", p, t );
                } else {
                    write_log( "%s:%d: use of uninitialized define '$%s'", cfgname, line, k + 2);
                    rc = 0;
                    *k = 0;
                }
                goto contd;
            }
        }

        if ( *p== '$' && isgraph( p[1] ))  /* Found new define */
            aslist_add( &defs, p + 1, t );
        else if ( !strcasecmp( p, "include" )) {
            if(!strncmp(cfgname,t,MAX_STRING)) {
                write_log("%s:%d: <include> including itself -> infinity loop",cfgname,line);
                rc=0;
            } else if(!parseconfig(t)) {
                write_log("%s:%d: was errors parsing included file '%s'",cfgname,line,t);
                rc=0;
            }
        } else if(*p=='{'||!strcasecmp(p,"if")) {
            for(k=t;*k&&(*k!=':'||k[1]!=' ');k++);
            if(*k==':'&&k[1]==' ') {
                *k++=0;
                if(*(k-2)==' ')*(k-2)=0;
                while(*k==' ')k++;
                for(p=k;*p&&*p!=' ';p++);
                *p++=0;
                while(*p==' ')p++;
                if(!*p) {
                    write_log("%s:%d: inline <if>-expression witout arguments",cfgname,line);
                    rc=0;k=NULL;
                }
                if(*(p+strlen(p)-1)=='}')*(p+strlen(p)-1)=0;
            } else k=NULL;
            cc=slist_add(&condlist,t);
            if(flagexp(cc,1)<0) {
                write_log("%s:%d: can't parse expression '%s'",cfgname,line,t);
                rc=0;
            } else slist_addl(&curcond,cc->str);
            if(k&&curcond) {
                if(!parsekeyword(k,p,cfgname,line))rc=0;
                slist_dell(&curcond);
            }
        } else if(!strcasecmp(p,"else")) {
            if(!curcond) {
                write_log("%s:%d: misplaced <else> without <if>",cfgname,line);
                rc=0;
            } else {
                k=slist_dell(&curcond);
                snprintf(s,LARGE_STRING,"not(%s)",k);
                cc=slist_add(&condlist,s);
                slist_addl(&curcond,cc->str);
            }
        } else if(*p=='}'||!strcasecmp(p,"endif")) {
            if(!curcond) {
                write_log("%s:%d: misplaced <endif> without <if>",cfgname,line);
                rc=0;
            } else slist_dell(&curcond);
        } else if(!parsekeyword(p,t,cfgname,line))rc=0;
    }

    fclose( f );
    return rc;
}


int readconfig(const char *cfgname)
{
	int		rc, i;
	cfgitem_t	*ci;

	curcond = NULL;
	rc = parseconfig( cfgname );
    aslist_kill(&defs);
    if(curcond) {
        write_log("%s: not all <if>-expressions closed",cfgname);
        while(curcond)slist_dell(&curcond);
        rc=0;
    }
    if(!rc)return 0;
    for(i=0;i<CFG_NNN;i++)
        if(!(configtab[i].flags&8)) {
        if(configtab[i].flags&1) {
            write_log("required keyword '%s' not defined",configtab[i].keyword);
            rc=0;
        }
        ci=xcalloc(1,sizeof(cfgitem_t));
        if(configtab[i].def_val)
            setvalue(ci,configtab[i].def_val,configtab[i].type);
        else memset(&ci->value,0,sizeof(ci->value));
        ci->condition=NULL;ci->next=NULL;
        if(configtab[i].flags&12) {
            cfgitem_t *cia=configtab[i].items;
            for(;cia&&cia->next;cia=cia->next);
            if(cia)cia->next=ci;
        }
        if(!configtab[i].items)configtab[i].items=ci;
    }
    if(rc) { /* read tables */
        recode_to_local(NULL);
        recode_to_remote(NULL);
    }
    return rc;
}


void rereadconfig(int client)
{
    outbound_done();
    log_done();
    killsubsts( &psubsts );
    killconfig();

    if ( !readconfig( configname )) {
        if ( client >= 0 )
            sendrpkt( 1, client, "Bad config, terminated");
        write_log( "There were some errors parsing config, exiting" );
        exit( S_FAILURE );
    }

    if( !log_init( cfgs( CFG_MASTERLOG ), NULL )) {
        write_log( "can't open master log '%s', aborting.", ccs );
        exit( S_FAILURE );
    }

    if ( outbound_init( cfgs( CFG_ASOOUTBOUND ), cfgs( CFG_BSOOUTBOUND ),
        cfgs( CFG_QSTOUTBOUND ), cfgal( CFG_ADDRESS )->addr.z ) == 0 ) {

        write_log("No outbounds defined");
        exit( S_FAILURE );
    }

    psubsts = parsesubsts( cfgfasl( CFG_SUBST ));

#ifdef NEED_DEBUG
    parse_log_levels();

    if ( facilities_levels['C'] >= 8 )
        dumpconfig();
#endif

    IFPerl( perl_on_reload( 0 ));
    do_rescan = 1;
}


void killconfig(void)
{
    int i;
    cfgitem_t *c,*t;
    slist_kill(&condlist);
    for(i=0;i<CFG_NNN;i++) {
        for(c=configtab[i].items;c;c=t) {
            switch(configtab[i].type) {
                case C_PATH:
                case C_STR:
                xfree(c->value.v_char);
                break;
                case C_STRL:
                slist_kill(&c->value.v_sl);
                break;
                case C_ADRSTRL:
                faslist_kill(&c->value.v_fasl);
                break;
                case C_ADDRL:
                falist_kill(&c->value.v_al);
                break;
                case C_OCT:
                case C_INT:
                case C_YESNO:
                c->value.v_int=0;
                break;
            }
            slist_killn(&c->condition);
            t=c->next;
            xfree(c);
        }
        configtab[i].items=NULL;
        configtab[i].flags=0;
    }
}


#ifdef NEED_DEBUG
void dumpconfig(void)
{
	int		i;
	char		buf[LARGE_STRING + 5];
	cfgitem_t	*c;
	slist_t		*sl;
	falist_t	*al;
	faslist_t	*fasl;
	subst_t		*s;
	dialine_t	*l;

	for( i = 0; i < CFG_NNN; i++ ) {
		write_log( "conf: %s. (type=%d, flags=%d)", configtab[i].keyword,
			configtab[i].type, configtab[i].flags );
		for( c = configtab[i].items; c; c = c->next ) {
			xstrcpy( buf, "conf:   ", LARGE_STRING );
			if ( c->condition ) {
				xstrcat( buf, "if (", LARGE_STRING );
				for( sl = c->condition; sl; sl = sl->next ) {
					xstrcat( buf, sl->str, LARGE_STRING );
					if ( sl->next )
						xstrcat( buf, ") and (", LARGE_STRING );
				}
				xstrcat( buf, "): ", LARGE_STRING );
			} else
				xstrcat( buf, "default: ", LARGE_STRING );
			
			switch( configtab[i].type ) {
			case C_PATH:
			case C_STR:
				snprintf( buf + strlen( buf ), LARGE_STRING, "'%s'", c->value.v_char );
				break;
			case C_INT:
				snprintf( buf + strlen( buf ), LARGE_STRING, "%d", c->value.v_int );
				break;
			case C_OCT:
				snprintf( buf + strlen( buf ), LARGE_STRING, "%o", c->value.v_int );
				break;
			case C_YESNO:
				snprintf( buf + strlen( buf ), LARGE_STRING, "%s", c->value.v_int ? "yes" : "no" );
				break;
			case C_STRL:
				for( sl = c->value.v_sl; sl; sl = sl->next )
					snprintf( buf + strlen( buf ), LARGE_STRING, "'%s', ", sl->str );
				xstrcat( buf, "%", LARGE_STRING );
				break;
			case C_ADRSTRL:
				for( fasl = c->value.v_fasl; fasl; fasl = fasl->next )
					snprintf( buf + strlen( buf ), LARGE_STRING, "%s '%s', ",
						fasl->addr.d ? ftnaddrtoda( &fasl->addr ) : ftnaddrtoa( &fasl->addr ),
						fasl->str );
				xstrcat( buf, "%", LARGE_STRING );
				break;
			case C_ADDRL:
				for( al = c->value.v_al; al; al = al->next )
					snprintf( buf + strlen( buf ), LARGE_STRING, "%s, ",
						al->addr.d ? ftnaddrtoda( &al->addr ) : ftnaddrtoa( &al->addr ));
				xstrcat( buf, "%", LARGE_STRING );
				break;
			}
			write_log("%s",buf);
		}
	}

	for( s = psubsts; s; s = s->next ) {
		write_log( "subst for %s [%d]", ftnaddrtoa( &s->addr ), s->nhids );
		for( l = s->hiddens; l; l = l->next )
			write_log( " * %s,%s,%s,%d,%d", l->phone, l->host, l->timegaps,
				l->flags, l->num );
	}

}
#endif
