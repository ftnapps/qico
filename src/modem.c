/**********************************************************
 * Modem routines.
 **********************************************************/
/*
 * $Id: modem.c,v 1.18 2005/08/16 10:32:19 mitry Exp $
 *
 * $Log: modem.c,v $
 * Revision 1.18  2005/08/16 10:32:19  mitry
 * Fixed broken calls of loginscript
 *
 * Revision 1.17  2005/06/03 14:08:48  mitry
 * Added DCD check after hangup and modem_clean_line() before any input waiting
 *
 * Revision 1.16  2005/05/16 23:35:04  mitry
 * Fixed doubled BUSY response
 *
 * Revision 1.15  2005/05/16 20:27:57  mitry
 * Changed code a bit
 *
 * Revision 1.14  2005/05/11 13:22:40  mitry
 * Snapshot
 *
 * Revision 1.13  2005/05/06 20:53:06  mitry
 * Changed misc code
 *
 * Revision 1.12  2005/04/14 15:13:58  mitry
 * I'm tired of stupid errors
 *
 * Revision 1.11  2005/04/14 15:10:26  mitry
 * Fixed uninitialized tty_fd
 *
 * Revision 1.10  2005/04/14 14:48:11  mitry
 * Misc changes
 *
 * Revision 1.9  2005/04/05 09:34:12  mitry
 * New xstrcpy() and xstrcat()
 *
 * Revision 1.8  2005/03/31 20:34:41  mitry
 * Removed code commented out and rewrote some functions
 *
 * Revision 1.7  2005/03/28 20:17:54  mitry
 * Added new replace func strcasestr()
 *
 * Revision 1.6  2005/03/28 16:33:39  mitry
 * Major code rewrite with non-blocking i/o support
 *
 * Revision 1.5  2005/02/25 16:43:53  mitry
 * Added ModemCheckDSR keyword logic
 *
 * Revision 1.4  2005/02/23 21:48:47  mitry
 * Changed debug message
 *
 * Revision 1.3  2005/02/18 20:34:00  mitry
 * Changed alive() to modem_alive()
 * Changed hangup() to modem_hangup()
 * Changed stat_collect() to modem_stat_collect()
 * Changed reset() to modem_reset()
 * Changed mdm_dial() to modem_dial() and added check for modem_reset()
 * return code
 * Changed mdm_done() to modem_done()
 *
 */

#include "headers.h"
#include "qipc.h"
#include "tty.h"
#include "modem.h"

static int dial_rc = MC_OK;
static char *mcs[] = {             /* MC_ defined in tty.h */
    "ok",                          /* MC_OK              0 */
    "fail",                        /* MC_FAIL            1 */
    "error",                       /* MC_ERROR           2 */
    "busy",                        /* MC_BUSY            3 */
    "no dialtone",                 /* MC_NODIAL          4 */
    "ring",                        /* MC_RING            5 */
    "bad",                         /* MC_BAD             6 */
    "cancelled by operator"        /* MC_CANCEL          7 */
};


/*
 * open device, and set up all terminal parameters properly
 */
static int modem_init_device(char *dev)
{
	char	port[MAX_PATH + 5] = "/dev/", *p;
	int	speed = 0;
	pid_t	mypid = getpid();
	TIO	tio;

	DEBUG(('M',2,"modem_init_device: dev %s", dev));

	tty_fd = -1;

	if( *dev != '/' )
		xstrcat( port, dev, sizeof( port ));
	else
		xstrcpy( port, dev, sizeof( port ));

	if (( p = strchr( port, ':' ))) {
		*p++ = '\0';
		speed = atoi( p );
	}

	if ( !speed )
		speed = DEFAULT_SPEED;

	DEBUG(('M',4,"modem_init_device: port %s, speed %d", port, speed));

	if ( tty_lock( port ))
		return ME_CANTLOCK;

    
	tty_fd = open( port, O_RDWR | O_NONBLOCK );
	/*
	fd = open( port, O_RDWR | O_NDELAY | O_NOCTTY );
	*/

	if ( tty_fd == -1 ) {
		write_log( "can't open %s", port );
		return ME_OPEN;
	}

	if ( fd_make_stddev( tty_fd ) != OK ) {
		DEBUG(('M',4,"modem_init_device: can't make stdin/stdout/stderr"));
		close( tty_fd );
		tty_unlock( port );
		return ME_OPEN;
	}

	tty_fd = 0;

	if ( tio_get( tty_fd, &tty_stio ) == ERROR ) {
		close( tty_fd );
		tty_unlock( port );
		return ME_ATTRS;
	}

	memcpy( &tio, &tty_stio, sizeof( TIO ));
	tio_raw_mode( &tio );
	tio_local_mode( &tio, TRUE );
	tio_set_speed( &tio, speed );

#ifdef sun
	tio_set_flow_control( tty_fd, &tio, (DATA_FLOW) & (FLOW_SOFT) );
#else
	tio_set_flow_control( tty_fd, &tio, DATA_FLOW );
#endif

	if ( tio_set( tty_fd, &tio ) == ERROR ) {
		close( tty_fd );
		tty_unlock( port );
		return ME_ATTRS;
	}

	if ( getsid( mypid ) == mypid ) {
#ifdef HAVE_TIOCSCTTY
		/* We are on BSD system, set this terminal as out control terminal */
		ioctl( tty_fd, TIOCSCTTY );
#endif
		tcsetpgrp( tty_fd, mypid );
	}

	if ( cfgi( CFG_MODEMCHECKDSR )) {
		speed = tio_get_rs232_lines( tty_fd );
		if ( speed != -1 && ( speed & (TIO_F_DSR|TIO_F_CTS)) == 0 ) {
			DEBUG(('M',1,"No DSR/CTS signals, assuming modem is switched off"));
			close( tty_fd );
			tty_unlock( port );
			return ME_NODSR;
		}
	}

	signal( SIGHUP, tty_sighup );
	signal( SIGPIPE, tty_sighup );

	tty_gothup = FALSE;
	tty_port = xstrdup( port );

	return OK;
}


/*
 * Loop through all devices in "ports", check "nodial"
 * and return available port or NULL
 */
static char *modem_find_device(char *firstport)
{
	DEBUG(('M',2,"modem_find_device"));

#if 0
	slist_t *ports = cfgs( CFG_PORT );

	if ( firstport && strlen( *firstport ) > 0 )


#endif
	return NULL;
}



/* clean_line()
 *
 * wait for the line to be silent for "waittime" seconds
 * if more than 10000 bytes are read, stop logging them. We don't want to
 * have the log files fill up all of the hard disk.
 */
static int modem_clean_line(int waittime)
{
#if defined(MEIBE) || defined(NEXTSGTTY) || defined(BROKEN_VTIME)

	int	bytes = 0;				/* bytes read */

	/* on some systems, the VMIN/VTIME mechanism is obviously totally
	 * broken. So, use a select()/flush queue loop instead.
	 */
	DEBUG(('M',2,"waiting for line to clear (select/%d sec)", waittime ));

	while( GETCHAR( waittime ) > 0 && bytes++ < 10000 );

#else
	TIO	tio, save_tio;

	DEBUG(('M',2,"waiting for line to clear (VTIME=%d)", waittime * 10));

	/* set terminal timeout to "waittime" tenth of a second */
	tio_get( tty_fd, &tio );
	save_tio = tio;				/*!! FIXME - sgtty?! */
	tio.c_lflag &= ~ICANON;
	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = waittime * 10;
	tio_set( tty_fd, &tio );

	/* read everything that comes from modem until a timeout occurs */
	while( GETCHAR( 1 ) > 0 );

	/* reset terminal settings */
	tio_set( tty_fd, &save_tio );
    
#endif

	return 0;
}


static int modem_send_str(char *cmd)
{
    int rc = 1;

    if ( !cmd )
        return 1;
    
    DEBUG(('M',1,">> %s",cmd));

    while( *cmd && rc > 0 ) {
        switch( *cmd ) {
            case '|':
                rc = write( tty_fd, "\r", 1 );
                qsleep( 500 );	/* sleep for 0.5 sec */
                break;

            case '~':
                qsleep( 1000 );	/* sleep for 1 sec */
                rc = 1;
                break;

            case '\'':
            case '`':
                qsleep( 200 );	/* sleep for 0.2 sec */
                rc = 1;
                break;

            /*
            case '^':
                rc = tio_toggle_dtr( tty_fd, DTR_HI ) == OK;
                break;
            */

            case 'v':
                rc = 0;
                if ( !strchr( cmd, '^' ))	/* It's just a 'v' command */
                    goto writeit;
                cmd++;
                while( *cmd && *cmd != '^' ) {
                    if      ( *cmd == '~' ) rc += 1000;
                    else if ( *cmd == '\'' ) rc += 200;
                    cmd++;
                }
                if ( *cmd != '^' )		/* Just in case */
                    write_log( "Can't lower dtr without raising it back" );
                else
                    rc = tio_toggle_dtr( tty_fd, rc > 250 ? rc : 250 ) == OK;
                break;

            default:
writeit:
                rc = write( tty_fd, cmd, 1);
                DEBUG(('M',3,">>> %c, rc=%d",C0(*cmd),rc));
        }
        cmd++;
    }
    
    if ( rc > 0 )
        DEBUG(('M',2,"modem_send_str: sent"));
    else
        DEBUG(('M',2,"modem_send_str: error, rc=%d, errno=%d",rc,errno));

    return rc;
}


static int modem_get_str(char *buf, size_t nbytes, int timeout)
{
    time_t tout = timer_set( timeout );
    int rc = ERROR, ch = 0, ptr = 0;

    buf[0] = '\0';
    while( !timer_expired( tout ) ) {
        if ( tty_gothup ) {
            rc = RCDO;
            break;
        }

        rc = ch = GETCHAR( timer_rest( tout ));

        DEBUG(('M',3,"modem_get_str: '%c' %d (timer rest %d)", C0( ch ), ch, timer_rest( tout )));
        /*
        if ( NOTTO( ch ))
            break;
        */

        rc = OK;
        if ( ch == '\r' || ch == '\n' ) {
            if ( ptr )
                break;
            else
                continue;
        }

        if ( ptr == (int) nbytes ) {
            ptr--;
            DEBUG(('M',2,"modem_get_str: line exceeds buffer (%d)", ptr));
            rc = ERROR;
            break;
        }

        if ( ch >= 0 )
            buf[ptr++] = ch;
    }
    buf[ptr] = '\0';

    if ( timer_expired( tout )) {
        DEBUG(('M',2,"modem_get_str: timed out"));
        rc = TIMEOUT;
    }

    return rc;
}


static int modem_chat(char *cmd, slist_t *oks, slist_t *nds, slist_t *ers, slist_t *bys,
        char *ringing, int maxrings, int timeout, char *rest, size_t restlen, int show)
{
    char buf[MAX_STRING + 5];
    int rc, nrings = 0;
    slist_t *cs;
    time_t t1;
    
    DEBUG(('M',1,"modem_chat: cmd=\"%s\" timeout=%d",cmd,timeout));

    if ( rest )
        *rest = '\0';

    rc = modem_send_str( cmd );
    if ( rc != 1 ) {
        if ( rest )
            xstrcpy( rest, "FAILURE", restlen );
        DEBUG(('M',1,"modem_chat: modem_send_str failed, rc=%d",rc));
        return MC_FAIL;
    }

    DEBUG(('M',2,"modem_send_str OK"));

    if ( !oks && !ers && !bys )
        return MC_OK;
    
    rc = OK;
    t1 = timer_set( timeout );

    while( ISTO( rc ) && !timer_expired( t1 ) && ( !maxrings || nrings < maxrings )) {
        rc = modem_get_str( buf, MAX_STRING-1, timer_rest( t1 ));
        DEBUG(('M',3,"modem_chat: modem_get_str rc=%d, buf='%s'",rc, buf));
        
        if ( rc == RCDO ) {
            if ( rest ) {
                if ( tty_gothup ) {
                    xstrcpy( rest, "Cancelled", restlen );
                    return MC_CANCEL;
                } /* else
                    xstrcpy(rest,"HANGUP",restlen); */
                return MC_BUSY;
            }
        }
        
        if ( rc != OK ) {
            if ( rest )
                xstrcpy( rest, "FAILURE", restlen );
            DEBUG(('M',1,"modem_chat: modem_get_str failed, rc=%d",rc));
            return MC_FAIL;
        }
        
        if ( !*buf || strcasestr( cmd, buf ))
            continue;
        
        for( cs = oks; cs; cs = cs->next )
            if ( !strncmp( buf, cs->str, strlen( cs->str ))) {
                if ( rest )
                    xstrcpy( rest, buf, restlen );
                return MC_OK;
            }

        if ( show )
            write_log( buf );

        for( cs = ers; cs; cs = cs->next )
            if ( !strncmp( buf, cs->str, strlen( cs->str ))) {
                /*
                if ( rest )
                    xstrcpy( rest, buf, restlen );
                */
                return MC_ERROR;
            }

        if ( ringing && !strncmp( buf, ringing, strlen( ringing ))) {
            if ( !nrings && strlen( ringing ) == 4 ) {
                /*
                if ( rest )
                    xstrcpy( rest, buf, restlen );
                */
                return MC_RING;
            }
            nrings++;
            continue;
        }

        for( cs = nds; cs; cs = cs->next )
            if ( !strncmp( buf, cs->str, strlen( cs->str ))) {
                /*
                if ( rest )
                    xstrcpy( rest, buf, restlen );
                */
                return MC_NODIAL;
            }

        for( cs = bys; cs; cs = cs->next )
            if ( !strncmp( buf, cs->str, strlen( cs->str ))) {
		/*
                if ( rest )
                    xstrcpy( rest, buf, restlen );
		*/
                return MC_BUSY;
            }
    }

    if ( rest ) {
        if ( nrings && maxrings && nrings >= maxrings)
            snprintf( rest, restlen, "%d RINGINGs", nrings );
        else if( ISTO( rc ))
            xstrcpy( rest, "TIMEOUT", restlen );
        else
            xstrcpy( rest, "FAILURE", restlen );
    }
    return MC_FAIL;
}


static int modem_stat(char *cmd, slist_t *oks, slist_t *ers,int timeout,
        char *stat, size_t stat_len)
{
    char buf[MAX_STRING + 5];
    int rc;
    slist_t *cs;
    time_t t1;

    DEBUG(('M',2,"modem_stat: cmd=\"%s\" timeout=%d",cmd,timeout));

    if ( stat )
        *stat = '\0';

    rc = modem_send_str( cmd );
    if ( rc != 1 ) {
        if ( stat )
            xstrcpy( stat, "FAILURE", stat_len );
        DEBUG(('M',1,"modem_stat: modem_send_str failed, rc=%d",rc));
        return MC_FAIL;
    }

    if ( !oks && !ers )
        return MC_OK;

    rc = OK;
    t1 = timer_set( timeout );

    while( ISTO( rc ) && !timer_expired( t1 )) {
        rc = modem_get_str( buf, MAX_STRING-1, timer_rest( t1 ));
        DEBUG(('M',3,"modem_stat: modem_get_str rc=%d, buf='%s'", rc, buf));

        if ( !*buf || strcasestr( cmd, buf ))
            continue;

        if ( rc != OK ) {
            if ( stat )
                xstrcat( stat, "FAILURE", stat_len );
            DEBUG(('M',1,"modem_stat: modem_get_str failed"));
            return MC_FAIL;
        }

        for( cs = oks; cs; cs = cs->next )
            if ( !strncmp( buf, cs->str, strlen( cs->str ))) {
                if ( stat )
                    xstrcat( stat, buf, stat_len );
                return MC_OK;
            }

        for( cs = ers; cs; cs = cs->next )
            if ( !strncmp( buf, cs->str, strlen( cs->str ))) {
                if ( stat )
                    xstrcat( stat, buf, stat_len );
                return MC_ERROR;
            }

        if ( stat ) {
            xstrcat( stat, buf, stat_len );
            xstrcat( stat, "\n", stat_len );
        }
    }

    if( stat ) {
        if ( ISTO( rc ))
            xstrcat( stat, "TIMEOUT", stat_len );
        else
            xstrcat( stat, "FAILURE", stat_len );
    }
    return MC_FAIL;
}


static int modem_alive(void)
{
	char	*ac;
	int	rc = MC_OK;

	ac = cfgs( CFG_MODEMALIVE );
	if ( ac == NULL )
		return MC_OK;

	DEBUG(('M',1,"modem_alive: checking modem..."));
	rc = modem_chat( ac, cfgsl( CFG_MODEMOK ), cfgsl( CFG_MODEMERROR ),
		cfgsl( CFG_MODEMNODIAL ), cfgsl( CFG_MODEMBUSY ),
		cfgs( CFG_MODEMRINGING ), cfgi( CFG_MAXRINGS ), 5, NULL, 0, 0 );

#ifdef NEED_DEBUG
	if ( rc != MC_OK )
		DEBUG(('M',1,"modem_alive: failed, rc=%d [%s]",rc,mcs[rc]));
#endif

	return rc;
}


int modem_hangup(void)
{
	slist_t *hc;
	int	rc = MC_FAIL, to = timer_set( cfgi( CFG_WAITRESET ));
    
	write_log("hanging up...");
	tty_purge();
	while ( rc != MC_OK && !timer_expired( to )) {
		if ( cfgsl( CFG_MODEMHANGUP ) == NULL ) {
			rc = ( tio_toggle_dtr( tty_fd, 500 ) == OK ? MC_OK : MC_FAIL );
		} else {
			for( hc = ccsl; hc && rc != MC_OK ; hc = hc->next ) {
				rc = ( modem_send_str( hc->str ) == 1 ? MC_OK : MC_FAIL );
				tty_purge();
			}
		}
            
		DEBUG(('M',2,"hangup: rc %d",rc));
		qsleep( 500 );
		modem_clean_line( 2 );
		DEBUG(('M',2,"dial_rc %d",dial_rc));
		if ( rc == MC_OK ) {
			tty_online = FALSE;
			if ( dial_rc == MC_OK )
				rc = modem_alive();
		}
	}

#ifdef NEED_DEBUG
	if ( rc != MC_OK )
		DEBUG(('M',1,"modem_hangup: failed, rc=%d [%s]",rc,mcs[rc]));
#endif
	
	if ( tty_dcd( tty_fd ))
		write_log( "WARNING: DCD line still active, check modem settings (AT&Dx)" );

	tty_purge();
	return rc;
}


int modem_stat_collect(void)
{
	slist_t	*hc;
	int	rc = MC_OK;
	char	stat[2048], *cur_stat, *p;
    
	if ( !cfgsl( CFG_MODEMSTAT ))
		return MC_OK;

	write_log( "collecting statistics..." );

	tty_purge();
	modem_clean_line( 2 );
	for( hc = cfgsl( CFG_MODEMSTAT ); hc; hc = hc->next ) {
		*stat = '\0';
		rc = modem_stat( hc->str, cfgsl( CFG_MODEMOK ), cfgsl( CFG_MODEMERROR ),
			cfgi( CFG_WAITRESET ), stat, sizeof( stat ));
		for( cur_stat = stat; *cur_stat; ) {
			for( p = cur_stat; *p && *p != '\n' && *p != '\r'; p++ );

			if ( *p )
				*(p++) = '\0';
			write_log( "%s", cur_stat );
			cur_stat = p;
		}
	}
	return rc;
}


static int modem_reset(void)
{
	slist_t	*hc;
	int	rc = MC_OK;
    
	if ( !cfgsl( CFG_MODEMRESET ))
		return MC_OK;

	DEBUG(('M',1,"modem_reset"));

	tty_purge();
	modem_clean_line( 2 );
	for( hc = ccsl; hc && rc == MC_OK; hc = hc->next )
		rc = modem_chat( hc->str, cfgsl( CFG_MODEMOK ), cfgsl( CFG_MODEMNODIAL ),
			cfgsl( CFG_MODEMERROR ), cfgsl( CFG_MODEMBUSY ),
			cfgs( CFG_MODEMRINGING ), cfgi( CFG_MAXRINGS ),
			cfgi( CFG_WAITRESET ), NULL, 0, 1 );
	if ( rc != MC_OK )
		write_log( "modem reset failed, rc=%d [%s]", rc, mcs[rc] );
    
	return rc;
}


int modem_dial(char *phone, char *port)
{
	char	s[MAX_STRING + 5], conn[MAX_STRING + 5];
	TIO	tio;

	dial_rc = modem_init_device( port );
	DEBUG(('M',1,"modem_dial: modem_init_device rc=%d",dial_rc));

	if ( dial_rc != 0 ) {
		write_log( "can't open port '%s': %s", port, tty_errs[dial_rc] );
		return MC_BAD;
	}

	dial_rc = modem_reset();

	if ( dial_rc != MC_OK ) {
		/* XXX tty_close( !MODEM_OK ); */
		return dial_rc;
	}

	xstrcpy( s, cfgs( CFG_DIALPREFIX ), MAX_STRING );
	xstrcat( s, phone, MAX_STRING );
	write_log( "dialing %s", s );
	sline( "Dialing %s", s );
	xstrcat( s, cfgs( CFG_DIALSUFFIX ), MAX_STRING );

	vidle();
    
	dial_rc = modem_chat( s, cfgsl( CFG_MODEMCONNECT ), cfgsl( CFG_MODEMNODIAL ),
		cfgsl( CFG_MODEMERROR ), cfgsl( CFG_MODEMBUSY ), cfgs( CFG_MODEMRINGING ),
		cfgi( CFG_MAXRINGS ), cfgi( CFG_WAITCARRIER ), conn, MAX_STRING, 1 );

	/* XXX tty_purge(); */

	if ( dial_rc != MC_OK ) {
		if ( *conn )
			write_log( "%s", conn );

		if ( dial_rc == MC_RING ) {
			sline( "RING found..." );
		} else
			sline( "Call failed - %s", mcs[dial_rc] );
		return dial_rc;
	}

	sline( "Modem said: %s", conn );
	xfree( connstr );
	connstr = xstrdup( conn );
	write_log( "*** %s", conn );

	tio_get( tty_fd, &tio );
	tty_local( &tio, 0 );

	if ( tio_set( tty_fd, &tio ) == ERROR ) {
		DEBUG(('M',2,"modem_dial: tio_set failed: %s", strerror( errno )));
	}
    
	tty_online = TRUE;
	return dial_rc;
}


void modem_done(void)
{
	TIO tio;

	DEBUG(('M',1,"modem_done"));

	signal( SIGHUP, SIG_IGN );
	signal( SIGPIPE, SIG_IGN );
	tty_gothup = FALSE;
    
        memcpy( &tio, &tty_stio, sizeof( TIO ));

        tio_raw_mode( &tio );
	tio_set_flow_control( tty_fd, &tio, FLOW_NONE );
	tio_local_mode( &tio, TRUE );

	if ( tio_set( tty_fd, &tio ) == ERROR ) {
		DEBUG(('M',2,"modem_done: tio_set failed: %s", strerror( errno )));
	}

	modem_hangup();
	if ( dial_rc == MC_OK ) {
		modem_stat_collect();
		modem_reset();
	}
	tty_close();
}
