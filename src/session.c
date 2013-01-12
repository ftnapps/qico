/**********************************************************
 * Session control.
 **********************************************************/
/*
 * $Id: session.c,v 1.25 2005/08/11 19:06:53 mitry Exp $
 *
 * $Log: session.c,v $
 * Revision 1.25  2005/08/11 19:06:53  mitry
 * Added new macro MAILHOURS
 *
 * Revision 1.24  2005/08/10 18:24:47  mitry
 * Skip checking for mail hours on binkp session.
 * Removed commented out code.
 * Changed flexecute() a bit.
 *
 * Revision 1.23  2005/05/16 20:32:42  mitry
 * Changed code a bit
 *
 * Revision 1.22  2005/05/06 20:21:55  mitry
 * Changed nodelist handling
 *
 * Revision 1.21  2005/04/11 23:08:39  mitry
 * Fixed crash on incoming session when cd is lost.
 *
 * Revision 1.20  2005/04/11 22:46:01  mitry
 * Fixed stupid typo
 *
 * Revision 1.19  2005/04/11 18:24:45  mitry
 * Optimized session()
 *
 * Revision 1.18  2005/04/08 18:05:31  mitry
 * Added support for HydraRH1 option
 *
 * Revision 1.17  2005/03/31 20:07:39  mitry
 * Changed tty_gothup value to HUP_SESLIMIT in sessalarm()
 *
 * Revision 1.16  2005/03/28 16:44:34  mitry
 * Changed HAT option handling
 *
 * Revision 1.15  2005/02/23 21:36:05  mitry
 * Changed emsi_send() call
 * tty_online was removed from sig alarm handler
 *
 * Revision 1.14  2005/02/22 16:09:54  mitry
 * Added tty_gothup to sessalarm code
 *
 * Revision 1.13  2005/02/21 16:33:42  mitry
 * Changed tty_hangedup to tty_online
 *
 * Revision 1.12  2005/02/19 21:29:11  mitry
 * Do not init chat if we do not want it.
 *
 * Revision 1.11  2005/02/12 19:03:22  mitry
 * /tmp/qpkt.xxxxxx files now should be removed after session.
 *
 */

#include "headers.h"
#include <fnmatch.h>
#include "ls_zmodem.h"
#include "hydra.h"
#include "janus.h"
#include "binkp.h"
#include "qipc.h"
#include "tty.h"
#include "nodelist.h"


int (*receive_callback)(char *fn);

void addflist(flist_t **fl, char *loc, char *rem, char kill, off_t off, FILE *lo, int fromlo)
{
	flist_t	**t, *q;
	int	type;
	slist_t	*i;

	type = ( off == -1 ) ? IS_FLO : whattype( rem );

	if ( type == IS_PKT && fromlo )
		type = IS_FILE;

	DEBUG(('S',2,"Add file: '%s', sendas: '%s', kill: '%c', fromLO: %s, offset: %ld, type: %ld",
		loc, rem ? rem : "(null)", kill, lo ? "yes" : "no", (long) off, (long) type));

	if ( !bink && MAILHOURS	&& type != IS_PKT && type != IS_FLO )
		return;

	for( i = cfgsl( CFG_AUTOHOLD ); i; i = i->next )
		if ( !xfnmatch( i->str, loc, FNM_PATHNAME ))
			return;

	if ( rnode && rnode->options & O_HAT && type != IS_FLO )
		return;

	switch( type ) {
	case IS_REQ:
		if ( rnode && rnode->options & O_HRQ )
			return;
		if ( rnode && rnode->options & O_NRQ && !cfgi( CFG_IGNORENRQ ))
			return;
		break;
        
	case IS_PKT:
	case IS_FLO:
		/*
		if(rnode&&rnode->options&O_HAT)return;
		*/
		break;

	default:
		if ( rnode && rnode->options & ( O_HXT ) && rem )
			return;
		break;
	}

	for( t = fl; *t && ((*t)->type <= type ); t = &((*t)->next));
	q = (flist_t *) xmalloc( sizeof( flist_t ));
	q->next = *t;
	*t = q;
	q->kill = ( fromlo && kill == '#' && cfgi( CFG_ALWAYSKILLFILES )) ? '^' : kill;

	q->loff = off;
	q->sendas = ( rnode && ( rnode->options & O_FNC ) && rem ) ?
		 xstrdup( fnc( rem )) : xstrdup( rem );
	xfree( rem );
	q->lo = lo;
	q->tosend = loc;
	q->type = type;
	q->suspend = 0;
}

static int boxflist(flist_t **fl, char *path)
{
	DIR		*d;
	struct dirent	*de;
	struct stat	sb;
	char		mn[MAX_PATH], *p;
	int		len, pathlen;

	DEBUG(('S',2,"Add filebox '%s'",path));
	if ( !( d = opendir( path )))
		return 0;
	else {
		pathlen = strlen( path ) + 2;
		while(( de = readdir( d ))) {
			if ( de->d_name[0] == '.' )
				continue;
			len = pathlen + strlen( de->d_name );
			p = xmalloc( len );
			snprintf( p, len, "%s/%s", path, de->d_name );
			if ( !stat( p, &sb )
				&& ( S_ISREG( sb.st_mode ) || S_ISLNK( sb.st_mode ))) {

				xstrcpy( mn, de->d_name, MAX_PATH );
				mapname( mn, cfgs( CFG_MAPOUT ), MAX_PATH );
				addflist( fl, p, xstrdup( mn ), '^', 0, NULL, 0 );
				totalf += sb.st_size;
				totaln++;
			} else
				xfree( p );
		}
		closedir( d );
	}
	return 1;
}

void makeflist(flist_t **fl,ftnaddr_t *fa,int mode)
{
    faslist_t *j;
    char str[MAX_PATH], longboxp[MAX_PATH], *flv="hdfci";

    DEBUG(('S',1,"Make filelist for %s",ftnaddrtoa(fa)));
    asoflist(fl,fa,mode);
    for(j=cfgfasl(CFG_FILEBOX);j;j=j->next)
        if(addr_cmp(fa,&j->addr)) {
            if(!boxflist(fl,j->str))write_log("can't open filebox '%s'",j->str);
            break;
        }

    if(cfgs(CFG_LONGBOXPATH)) {
        xstrcpy( longboxp, ccs, MAX_PATH );
        while(*flv) {
            snprintf(str,MAX_STRING,"%s/%d.%d.%d.%d.%c",longboxp,fa->z,fa->n,fa->f,fa->p,*flv);
            boxflist(fl,str);
            flv++;
        }
        snprintf(str,MAX_STRING,"%s/%d.%d.%d.%d",longboxp,fa->z,fa->n,fa->f,fa->p);
        boxflist(fl,str);
    }
}


static void killtrunc(flist_t *ktfl)
{
	FILE	*f;

	if ( !ktfl->suspend ) {
		switch( ktfl->kill ) {
		case '^':				/* Kill sent file */
			lunlink( ktfl->tosend );
			break;

		case '#':				/* Trunc sent file */
			f = fopen( ktfl->tosend, "w" );
			if ( f )
				fclose( f );
			else
				write_log( "can't truncate %s: %s", ktfl->tosend, strerror( errno ));
			break;
		}
	}
	xfree( ktfl->sendas );
}


void flexecute(flist_t *fl)
{
	char	cmt = '~', str[MAX_STRING], *q;
	int	keep = 0;

	DEBUG(('S',2,"Execute file: '%s', sendas: '%s', kill: '%c' fromLO: %s, offset: %ld, suspend: %ld",
		fl->tosend,
		fl->sendas ? fl->sendas : "(null)",
		fl->kill,
		fl->lo ? "yes" : "no",
		(long) fl->loff, (long) fl->suspend ));

	if ( fl->lo ) {
		if ( fl->loff < 0 ) {			/* FLO file itself */
			fseeko( fl->lo, (off_t) 0, SEEK_SET );
			while( fgets( str, MAX_STRING, fl->lo ))
				if ( *str && *str != '~' && *str != '\n' && *str != '\r' ) {
					keep++;
					break;
				}

			fclose( fl->lo );
			fl->lo = NULL;

			if ( !keep )
				lunlink( fl->tosend );

		} else if ( fl->sendas ) {
			killtrunc( fl );
			if ( !fl->suspend ) {
				fseeko( fl->lo, fl->loff, SEEK_SET );
				fwrite( &cmt, 1, 1, fl->lo );
			}
		}
	} else if( fl->sendas ) {
		killtrunc( fl );		
		if ( cfgi( CFG_RMBOXES ) && !fl->suspend ) {
			q = strrchr( fl->tosend, '/' );
			if ( q && q != fl->tosend ) {
				*q = '\0';
				rmdir( fl->tosend );
				*q = '/';
			}
		}
	}
}


void flkill(flist_t **l, int rc)
{
	flist_t *l1;

	DEBUG(('S',1,"Kill filelist: list=%.8p, rc=%d",*l,rc));

	while( *l ) {
		l1 = *l;

		DEBUG(('S',2,"Kill file: '%s', sendas: '%s', kill: '%c', type: %d, fromLO: %s, offset: %ld, suspend: %ld",
			l1->tosend,
			l1->sendas ? l1->sendas: "(null)",
			l1->kill, l1->type,
			l1->lo ? "yes" : "no",
			(long) l1->loff, (long) l1->suspend));

		/* Close FLO file if it exists */
		if ( l1->lo && l1->loff < 0 ) {
			fseeko( l1->lo, (off_t) 0, SEEK_END );
			fclose( l1->lo );
		}

                /* Unlink successfully sent REQ file */
		if ( l1->type == IS_REQ && rc && !l1->sendas )
			lunlink( l1->tosend );
		
		xfree( l1->sendas );
		xfree( l1->tosend );
		*l = l1->next;
		xfree( l1 );
    }
}


void simulate_send(ftnaddr_t *fa)
{
	flist_t *fl = NULL, *l = NULL;

	makeflist( &fl, fa, 0 );
	for( l = fl; l; l = l->next )
		flexecute( l );
	flkill( &fl, 1 );
}

int receivecb(char *fn)
{
    char *p = strrchr( fn, '.' );
    if ( !p )
        return 0;
    else
        p++;
    if ( !strcasecmp( p, "pkt" ) && cfgi( CFG_SHOWPKT ))
        return( showpkt( fn ));
    if ( !strcasecmp( p, "req" )) {
        if ( emsi_lo & O_NOFREQS )
            return 1;
        else
            return( freq_recv( fn ));
    }
    return 0;
}

static int wazoosend(int zap)
{
    flist_t *l;
    int rc,ticskip=0;
    unsigned long total=totalf+totalm;
    write_log("wazoo send");
    sline("Init zsend...");
    rc=zmodem_sendinit(zap);
    sendf.cps=1;sendf.allf=totaln;sendf.ttot=totalf;
     if(!rc)for(l=fl;l;l=l->next) {
        if(l->sendas) {
            rc=cfgi(CFG_AUTOTICSKIP)?ticskip:0;ticskip=0;
            if(!rc||!istic(l->tosend))rc=zmodem_sendfile(l->tosend,l->sendas,&total,&totaln);
                else write_log("tic file '%s' auto%sed",l->tosend,rc==ZSKIP?"skipp":"refus");
            if(rc<0)break;
            l->suspend = (rc == ZFERR);
            /*
            if(!rc||rc==ZSKIP) {
            */
                if(l->type==IS_REQ)was_req=1;
                flexecute(l);
            /*}*/
            if(rc==ZSKIP||rc==ZFERR)ticskip=rc;
        } else flexecute(l);
    }

    sline("Done zsend...");
    rc=zmodem_senddone();
    qpreset(1);
    return(rc<0);
}

static int wazoorecv(int zap)
{
    int rc;
    write_log("wazoo receive");
    rc=zmodem_receive(cfgs(CFG_INBOUND),zap);
    qpreset(0);
    return(rc==RCDO||rc==ERROR);
}

static int hydra(int originator, int hmod, int rh1)
{
    flist_t *l;
    int rc = XFER_OK, ticskip = 0;
    int file_count = (int) totaln;

    sline( "Hydra-%dk session", hmod*2 );

    hydra_init( ( is_ip ? (HOPT_XONXOFF|HOPT_TELENET) : 0 ), originator, hmod );

    DEBUG(('S',1,"file_count: %ld", file_count));
    for( l = fl; l && file_count; l = l->next )
        if ( l->type == IS_REQ ) {
            DEBUG(('S',1,"Transmitting request(s)"));
            rc = hydra_send( l->tosend, l->sendas );
            if ( rc == XFER_ABORT)
                break;
            l->suspend = (rc == XFER_SUSPEND);

            /*
            if ( rc == XFER_OK || rc == XFER_SKIP ) {
            */
                flexecute( l );
                file_count--;
            /*} */
            break;
        }

    if ( rc == XFER_ABORT ) {
        hydra_deinit();
        return 1;
    }

    DEBUG(('S',1,"file_count: %ld", file_count));
    if ( rh1 || file_count == 0 ) {
        rc = hydra_send( NULL, NULL );
        file_count -= ( file_count == 0 );
    }

    for( l = fl; l && rc != XFER_ABORT; l = l->next )
        if ( l->sendas ) {
            DEBUG(('S',1,"file_count: %ld", file_count));
            rc = cfgi( CFG_AUTOTICSKIP ) ? ticskip : 0;
            ticskip = 0;

            if ( file_count == 0 && !rh1 ) {
                if ( hydra_send( NULL, NULL ) == XFER_ABORT )
                    break;
                file_count--;
            }

            if ( !rc || !istic( l->tosend ))
                rc = hydra_send( l->tosend, l->sendas );
            else
                write_log( "tic file '%s' auto%sed", l->tosend,
                    ( rc == XFER_SKIP ) ? "skipp" : "suspend" );

            if ( rc == XFER_ABORT )
                break;

            l->suspend = (rc == XFER_SUSPEND);
            /*
            if ( rc == XFER_OK || rc == XFER_SKIP ) {
            */
                flexecute( l );
                file_count--;
            /*}*/

            if ( rc == XFER_SKIP || rc == XFER_SUSPEND )
                ticskip = rc;

        } else
            flexecute( l );

    if ( rc == XFER_ABORT ) {
        hydra_deinit();
        return 1;
    }

    DEBUG(('S',1,"file_count: %ld", file_count));
    if ( file_count == 0 && !rh1 ) {
        if ( hydra_send( NULL, NULL ) == XFER_ABORT ) {
            hydra_deinit();
            return 1;
        }
    }

    DEBUG(('S',1,"file_count: %ld", file_count));
    rc = hydra_send( NULL, NULL );
    hydra_deinit();
    return ( rc == XFER_ABORT );
}

void log_rinfo(ninfo_t *e)
{
    falist_t *i;
    struct tm *t,mt;
    char s[MAX_STRING];
    int k=0;
    time_t tt=time(NULL);
    if((i=e->addrs)) {
        write_log("address: %s",ftnaddrtoda(&i->addr));
        i=i->next;
    }
    for(*s=0;i;i=i->next) {
        if(k)xstrcat(s," ",MAX_STRING);
        xstrcat(s,ftnaddrtoda(&i->addr),MAX_STRING);
        k++;if(k==2) {
            write_log("    aka: %s",s);
            k=0;*s=0;
        }
    }
    if(k)write_log("    aka: %s",s);
    write_log(" system: %s",e->name);
    write_log("   from: %s",e->place);
    write_log("  sysop: %s",e->sysop);
    write_log("  phone: %s",e->phone);
    write_log("  flags: [%d] %s",e->speed,e->flags);
    write_log(" mailer: %s",e->mailer);
    mt=*localtime(&tt);t=gmtime(&e->time);
    write_log("   time: %02d:%02d:%02d, %s",t->tm_hour,t->tm_min,t->tm_sec,e->wtime?e->wtime:"unknown");
    if(t->tm_mday!=mt.tm_mday||t->tm_mon!=mt.tm_mon||t->tm_year!=mt.tm_year)
        write_log("   date: %02d/%02d/%04d",t->tm_mday,t->tm_mon+1,t->tm_year+1900);
    if(e->holded&&!e->files&&!e->netmail)write_log(" for us: %d%c on hold",
        SIZES(e->holded),SIZEC(e->holded));
    else if(e->files||e->netmail)write_log(" for us: %d%c mail, %d%c files",
        SIZES(e->netmail),SIZEC(e->netmail),SIZES(e->files),SIZEC(e->files));
}

#define LOG_LOCK_NODE \
    log_rinfo( rnode );                         \
                                                \
    for( pp = rnode->addrs; pp; pp = pp->next ) \
        outbound_locknode( &pp->addr, LCK_s );


static int emsisession(int originator, ftnaddr_t *calladdr, int speed)
{
    int rc, proto, xproto;
    unsigned long nfiles;
    char *t, *x, pr[3], s[MAX_STRING];
    falist_t *pp = NULL;
    qitem_t *q = NULL;
    
    was_req = 0;
    got_req = 0;
    receive_callback = receivecb;
    totaln = totalf = totalm = emsi_lo = 0;
    
    if ( originator ) {
    /*
        Outbound session
    */
        write_log( "starting outbound EMSI session" );
        q = q_find( calladdr );
        if ( q ) {
            totalm = q->pkts;
            totalf = q_sum( q ) + q->reqs;
        }

        emsi_lo |= O_PUA;
        emsi_lo |= ( is_freq_available() <= FR_NOTAVAILABLE ) ? O_NRQ : emsi_lo;

        if ( MAILHOURS ) {
            emsi_lo |= O_HXT;
            emsi_lo |= ( emsi_lo & O_NRQ ) ? emsi_lo : O_HRQ;
        }

        emsi_makedat( calladdr, totalm, totalf, emsi_lo,
            cfgs( CFG_PROTORDER ), NULL, TRUE );
        rc = emsi_send();

        if ( rc < 0 )
            return S_REDIAL | S_ADDTRY;

        rc = emsi_recv( SM_OUTBOUND, rnode );
        if ( rc < 0)
            return S_REDIAL | S_ADDTRY;

        title( "Outbound session %s", ftnaddrtoa( &rnode->addrs->addr ));

        LOG_LOCK_NODE

        if ( !has_addr( calladdr, rnode->addrs )) {
            write_log( "remote is not %s", ftnaddrtoa( calladdr ));
            return S_FAILURE;
        }

        flkill( &fl, 0 );
        totalf = totalm = 0;
        
        for( pp = rnode->addrs; pp; pp = pp->next)
            makeflist( &fl, &pp->addr, originator );

        if ( rnode->pwd && strlen( rnode->pwd ))
            rnode->options |= O_PWD;

        if ( nodelist_listed( rnode->addrs, cfgi( CFG_NEEDALLLISTED )))
            rnode->options |= O_LST;

    } else {
    /*
        Inbound session
    */
        rc = emsi_recv( SM_INBOUND, rnode );
        if ( rc < 0) {
            write_log( "unable to establish EMSI session" );
            return S_REDIAL | S_ADDTRY;
        }

        write_log( "starting inbound EMSI session" );

        if (( t = getenv( "CALLER_ID" )) && strcasecmp( t, "none" ) && strlen( t ) > 3 )
            title( "Inbound session %s (CID %s)", ftnaddrtoa( &rnode->addrs->addr ), t );
        else
            title( "Inbound session %s", ftnaddrtoa( &rnode->addrs->addr ));

        LOG_LOCK_NODE

        for( pp = cfgal( CFG_ADDRESS ); pp; pp = pp->next )
            if ( has_addr( &pp->addr, rnode->addrs )) {
                write_log( "remote also has %s", ftnaddrtoa( &pp->addr ));
                return S_FAILURE;
            }

        nfiles = rc = 0;
        t = NULL;


        /*
        for( pp = rnode->addrs; pp; pp = pp->next ) {

            t = findpwd( &pp->addr );

            DEBUG(('S',1,"address: %s, rpwd: %s, lpwd: %s", ftnaddrtoa( &pp->addr ),
                ( rnode->pwd ? rnode->pwd : "(NULL)" ), ( t ? t : "(NULL)" )));

            if ( rnode->pwd && 
                strlen( rnode->pwd )) {       /+* Remote has got password for us *+/
                if ( t )
            } else {                  /+* Remote doesn't have password for us *+/
            }
            if ( t == NULL            /+* We don't have password for this address *+/
                || ( rnode->pwd && !strcasecmp( rnode->pwd, t )))
                                      /+* Or we and remote have equal passwords *+/
            {
                makeflist( &fl, &pp->addr, originator );
                if ( t )
                    rnode->options |= O_PWD;
            } else {
                write_log( "password not matched for %s", ftnaddrtoa( &pp->addr ));
                write_log( "  (got '%s' instead of '%s')", rnode->pwd, t );
                rc = 1;
            }
	}
        */

        for( pp = rnode->addrs; pp; pp = pp->next ) {
            t = findpwd( &pp->addr );
            DEBUG(('S',1,"address: %s, rpwd: %s, lpwd: %s", ftnaddrtoa( &pp->addr ),
                ( rnode->pwd ? rnode->pwd : "(NULL)" ), ( t ? t : "(NULL)" )));
            if ( t == NULL            /* We don't have password for this address */
                || ( rnode->pwd && !strcasecmp( rnode->pwd, t )))
                                      /* Or we and remote have equal passwords */
            {
                makeflist( &fl, &pp->addr, originator );
                if ( t )
                    rnode->options |= O_PWD;
            } else {
                write_log( "password not matched for %s", ftnaddrtoa( &pp->addr ));
                write_log( "  (got '%s' instead of '%s')", rnode->pwd, t );
                rc = 1;
            }
	}

	if ( rnode->pwd && *rnode->pwd && !( rnode->options & O_PWD )) {
	    write_log( "remote proposed password for us: '%s'", rnode->pwd );
            flkill( &fl, 0 );
	}

        if ( nodelist_listed( rnode->addrs, cfgi( CFG_NEEDALLLISTED )))
            rnode->options |= O_LST;

        pr[2] = 0; pr[1] = 0; pr[0] = 0;

        if ( rc ) {                          /* Bad password - abort session */
            emsi_lo |= O_BAD;
            rnode->options |= O_BAD;
        } else {

            xproto = is_freq_available();

            if ( xproto == FR_NOTHANDLED || xproto == FR_NOTAVAILABLE )
    /*            emsi_lo |= O_NRQ;

            if ( xproto == FR_NOTAVAILABLE || rc ) */
                emsi_lo |= O_HRQ;

            if ( MAILHOURS )
                emsi_lo |= O_HXT | O_HRQ;

            if ( cfgi( CFG_SENDONLY )) {
                emsi_lo |= O_HAT;
                emsi_lo &= ~( O_HXT | O_HRQ );
            }

            for( t = cfgs( CFG_PROTORDER ); *t; t++) {

#ifdef HYDRA8K16K
                if ( *t == '4' && rnode->options & P_HYDRA4 ) {
                    *pr = '4';
                    emsi_lo |= P_HYDRA4;
                    break;
                }
                if( *t == '8' && rnode->options & P_HYDRA8 ) {
                    *pr = '8';
                    emsi_lo |= P_HYDRA8;
                    break;
                }
                if( *t == '6' && rnode->options & P_HYDRA16 ) {
                    *pr = '6';
                    emsi_lo |= P_HYDRA16;
                    break;
                }
#endif/*HYDRA8K16K*/

                if( *t == 'H' && rnode->options & P_HYDRA ) {
                    *pr = 'H';
                    emsi_lo |= P_HYDRA;
                    break;
                }
                if( *t == 'J' && rnode->options & P_JANUS ) {
                    *pr = 'J';
                    emsi_lo |= P_JANUS;
                    break;
                }
                if( *t == 'D' && rnode->options & P_DIRZAP ) {
                    *pr = 'D';
                    emsi_lo |= P_DIRZAP;
                    break;
                }
                if( *t == 'Z' && rnode->options & P_ZEDZAP ) {
                    *pr = 'Z';
                    emsi_lo |= P_ZEDZAP;
                    break;
                }
                if( *t== '1' && rnode->options & P_ZMODEM ) {
                    *pr = '1';
                    emsi_lo |= P_ZMODEM;
                    break;
                }
            }

            if ( cfgs( CFG_PROTORDER ) && strchr( ccs, 'C' ))
                pr[1] = 'C';
            else
                rnode->opt &= ~MO_CHAT;
        }

        if ( !*pr )
            emsi_lo |= P_NCP;

        emsi_makedat( &rnode->addrs->addr, totalm, totalf, emsi_lo,
            pr, NULL, !(emsi_lo & O_BAD));

        rc = emsi_send();

        if ( rc < 0 ) {
            flkill( &fl, 0 );
            return S_REDIAL | S_ADDTRY;
        }
    }

    if ( cfgs( CFG_RUNONEMSI ) && *ccs ) {
        write_log( "starting %s %s", ccs, 
            rnode->addrs ? ftnaddrtoa( &rnode->addrs->addr ) : 
            ( calladdr ? ftnaddrtoa( calladdr ) : "unknown" ));
        execnowait( ccs, rnode->addrs ? ftnaddrtoa( &rnode->addrs->addr ) :
            ( calladdr ? ftnaddrtoa( calladdr ) : "unknown" ), NULL, NULL );
    }

    write_log( "we have: %d%c mail, %d%c files", SIZES( totalm ), SIZEC( totalm ),
        SIZES( totalf ), SIZEC( totalf ));

    rnode->starttime = time( NULL );

    if ( cfgi( CFG_MAXSESSION ))
        alarm( cci * 60 );

    DEBUG(('S',1,"Maxsession: %d",cci));

    proto = ( originator ? rnode->options : emsi_lo ) & P_MASK;

    switch ( proto ) {
        case P_NCP:
            write_log( "no compatible protocols" );
            flkill( &fl, 0 );
            return S_FAILURE;

        case P_ZMODEM:
            t = "ZModem-1k";
            break;

        case P_ZEDZAP:
            t = "ZedZap";
            break;

        case P_DIRZAP:
            t = "DirZap";
            break;

#ifdef HYDRA8K16K
        case P_HYDRA4:
            t = "Hydra-4k";
            break;

        case P_HYDRA8:
            t = "Hydra-8k";
            break;

        case P_HYDRA16:
            t = "Hydra-16k";
            break;
#endif/*HYDRA8K16K*/

        case P_HYDRA:
            t = "Hydra";
            break;

        case P_JANUS:
            t = "Janus";
            break;

        default:
            t = "Unknown";
    }

    xproto = ( emsi_lo & O_RH1 ) && ( rnode->options & O_RH1 );
    x = ( *t == 'H' && xproto ) ? "x" : "";

    DEBUG(('S',1,"emsopts: %s%s %x %x %x %x", x, t, rnode->options & P_MASK,
        rnode->options, emsi_lo, rnode->opt));

    snprintf( s, MAX_STRING - 1, "%s%s%s%s%s%s%s%s%s%s%s",
        x, t,
        ( rnode->options & O_LST ) ? "/LST" : "",
        ( rnode->options & O_PWD ) ? "/PWD" : "",
        ( rnode->options & O_HXT ) ? "/MO"  : "",
        ( rnode->options & O_HAT ) ? "/HAT" : "",
        ( rnode->options & O_HRQ ) ? "/HRQ" : "",
        ( rnode->options & O_NRQ ) ? "/NRQ" : "",
        ( rnode->options & O_FNC ) ? "/FNC" : "",
        ( rnode->options & O_BAD ) ? "/BAD" : "",
        ( rnode->opt & MO_CHAT ) ? "/CHT" : "" );

    write_log( "options: %s", s);

    chatinit( rnode->opt & MO_CHAT ? proto : -1 );

    IFPerl( rc = perl_on_session( s );
        if( rc != S_OK ) { flkill( &fl, 0 ); return rc; } );

    qemsisend( rnode );
    qpreset( 0 );
    qpreset( 1 );

    switch ( proto ) {
        case P_ZEDZAP:
        case P_DIRZAP:
        case P_ZMODEM:
            recvf.cps = 1;
            recvf.ttot = rnode->netmail + rnode->files;
            xproto = ( proto & P_ZEDZAP ) ? CZ_ZEDZAP : (( proto & P_DIRZAP ) ? CZ_DIRZAP : CZ_ZEDZIP );
            if ( originator ) {
                rc = wazoosend( xproto );
                if ( !rc )
                    rc = wazoorecv( xproto );
                if( got_req && !rc )
                    rc = wazoosend( xproto );
            } else {
                rc = wazoorecv( xproto | 0x0100 );
                if ( rc )
                    return S_REDIAL;
                rc = wazoosend( xproto );
                if ( was_req )
                    rc = wazoorecv( xproto );
            }
            break;

        case P_HYDRA:
#ifdef HYDRA8K16K
        case P_HYDRA4:
        case P_HYDRA8:
        case P_HYDRA16:
#endif/*HYDRA8K16K*/
            sendf.allf = totaln;
            sendf.ttot = totalf + totalm;
            recvf.ttot = rnode->netmail + rnode->files;
            rc = 1;
            switch ( proto ) {
                case P_HYDRA: rc = 1; break;
#ifdef HYDRA8K16K
                case P_HYDRA4: rc = 2; break;
                case P_HYDRA8: rc = 4; break;
                case P_HYDRA16: rc = 8; break;
#endif/*HYDRA8K16K*/
            }
            rc = hydra( originator, rc, xproto );
            break;

        case P_JANUS:
            sendf.allf = totaln;
            sendf.ttot = totalf + totalm;
            recvf.ttot = rnode->netmail + rnode->files;
            rc = janus();
            break;

        default:
            return S_OK;

    }

    flkill( &fl, !rc );
    return rc ? S_REDIAL : S_OK;
}

static RETSIGTYPE sessalarm(int sig)
{
    signal( SIGALRM, SIG_DFL );
    write_log("session limit of %d minutes is over", cfgi( CFG_MAXSESSION ));
    tty_gothup = HUP_SESLIMIT;
}

int session(int originator, int type, ftnaddr_t *calladdr, int speed)
{
    int rc, ok_fail;
    time_t sest;
    FILE *h;
    char s[MAX_STRING], in_out;
    char *remote_id, *in_out_s;
    falist_t *pp;

    runtoss = 0;
    rnode->starttime = 0;
    rnode->realspeed = effbaud = speed;

    if ( !originator )
        rnode->options |= O_INB;

    if ( is_ip )
        rnode->options |= O_TCP;

    memset( &ndefaddr, 0, sizeof( ndefaddr ));

    if ( calladdr )
        addr_cpy( &ndefaddr, calladdr);

    if ( cfgi( CFG_MINSPEED ) && speed < cci ) {
        write_log( "connection speed is too slow (min %d required)", cci );
        return S_REDIAL | S_ADDTRY;
    }

    memset( &sendf, 0, sizeof( sendf ));
    memset( &recvf, 0, sizeof( recvf ));

    signal( SIGALRM, sessalarm );
    signal( SIGHUP, tty_sighup );
    signal( SIGPIPE, tty_sighup );
    signal( SIGTERM, tty_sighup );
    signal( SIGINT, tty_sighup );
    signal( SIGCHLD, SIG_DFL );

    switch ( type ) {
        case SESSION_AUTO:
            write_log( "trying EMSI..." );
            rc = emsi_init( originator );
            if ( rc < 0 ) {
                write_log( "unable to establish EMSI session" );
                signal( SIGALRM, SIG_DFL );
                return S_REDIAL | S_ADDTRY;
            }

        case SESSION_EMSI:
            rc = emsisession( originator, calladdr, speed );
            break;

#ifdef WITH_BINKP
        case SESSION_BINKP:
            rc = binkpsession( originator, calladdr );
            break;
#endif

        default:
            write_log( "unsupported session type! (%d)", type );
            signal( SIGALRM, SIG_DFL );
            return S_REDIAL | S_ADDTRY;
    }

    DEBUG(('S',4,"session.c: after xxxsession, rc == %d", rc));

    signal( SIGALRM, SIG_DFL );

    for( pp = rnode->addrs; pp; pp = pp->next )
        outbound_unlocknode( &pp->addr, LCK_x );

    if (( rnode->options & O_NRQ && !cfgi( CFG_IGNORENRQ ))
        || rnode->options & O_HRQ )
        rc |= S_HOLDR;

    if ( rnode->options & O_HXT )
        rc |= S_HOLDX;
    
    if( rnode->options & O_HAT )
        rc |= S_HOLDA;

    sest = rnode->starttime ? time( NULL ) - rnode->starttime : 0;

    IFPerl(perl_end_session( sest, rc ));

    write_log( "total: %d:%02d:%02d online, %lu%c sent, %lu%c received",
        sest/3600, sest % 3600 / 60, sest % 60,
        (long) SIZES((long) ( sendf.toff - sendf.soff )), SIZEC( sendf.toff - sendf.soff ),
        (long) SIZES((long) ( recvf.toff - recvf.soff )), SIZEC( recvf.toff - recvf.soff ));

    remote_id = xstrdup( rnode->addrs ? ftnaddrtoa( &rnode->addrs->addr ) 
            : ( calladdr ? ftnaddrtoa( calladdr ): "unknown" ));
    ok_fail = (( rc & S_MASK ) == S_OK ) ? 1 : 0;
    in_out = ( rnode->options & O_INB ) ? 'I' : 'O';
    in_out_s = ( rnode->options & O_INB ) ? "I" : "O";

    write_log( "session with %s %s [%s]",
        remote_id,
        ok_fail ? "successful" : "failed",
        M_STAT );

    if ( rnode->starttime && cfgs( CFG_HISTORY )) {
        h = fopen( ccs, "at" );
        if ( !h )
            write_log( "can't open '%s' for writing: %s", ccs, strerror( errno ));
        else {
            fprintf( h, "%s,%ld,%ld,%s,%s%s%c%d,%lu,%lu\n",
                rnode->tty, rnode->starttime, sest,
                remote_id,
                ( rnode->options & O_PWD) ? "P" : "",
                ( rnode->options & O_LST ) ? "L" : "",
                in_out, ok_fail,
                (long) (sendf.toff - sendf.stot),
                (long) (recvf.toff - recvf.stot) );
            fclose( h );
        }
    }

    while( freq_pktcount ) {
        snprintf( s, MAX_STRING, "/tmp/qpkt.%04lx%02x", (long)getpid(), freq_pktcount-- );
        if ( fexist( s ))
            lunlink( s );
    }

    if ( chatlg )
        chatlog_done();

    if ( cfgs( CFG_AFTERSESSION )) {
        write_log( "starting %s %s %c %d", ccs,
            remote_id, in_out, ok_fail );
        execnowait(ccs, remote_id, in_out_s, ok_fail ? "1" : "0" );
    }

    if ( cfgs( CFG_AFTERMAIL ) && ( recvf.toff - recvf.soff != 0 || runtoss )) {
        write_log("starting %s %s %c %d", ccs,
            remote_id, in_out, ok_fail );
        execnowait( ccs, remote_id, in_out_s, ok_fail ? "1" : "0" );
    }

    DEBUG(('S',4,"session.c: about to leave, rc == %d, returns: %d", rc, ( rc & ~S_ADDTRY )));

    xfree( remote_id );

    return ( rc & ~S_ADDTRY );
}
