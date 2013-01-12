/**********************************************************
 * tty/tio routines.
 *
 * Most part of this code is taken from mgetty package
 * copyrighted by 1993 Gert Doering.
 *
 * I don't intend to make mgetty competent software based
 * on the mgetty's source but just to use the well
 * working code.
 **********************************************************/

/*
 * $Id: tty.c,v 1.27 2005/08/16 20:23:23 mitry Exp $
 *
 * $Log: tty.c,v $
 * Revision 1.27  2005/08/16 20:23:23  mitry
 * Pre 0.57.1: minor changes
 *
 * Revision 1.26  2005/08/16 10:32:19  mitry
 * Fixed broken calls of loginscript
 *
 * Revision 1.25  2005/06/10 20:57:28  mitry
 * Changed tty_bufblock()
 *
 * Revision 1.24  2005/05/17 19:51:11  mitry
 * Code cleaning
 *
 * Revision 1.23  2005/05/17 18:17:43  mitry
 * Removed system basename() usage.
 * Added qbasename() implementation.
 *
 * Revision 1.22  2005/05/16 20:33:46  mitry
 * Code cleaning
 *
 * Revision 1.21  2005/05/06 20:53:06  mitry
 * Changed misc code
 *
 * Revision 1.20  2005/04/14 14:48:11  mitry
 * Misc changes
 *
 * Revision 1.19  2005/04/08 18:15:05  mitry
 * More proper sighup handling
 *
 * Revision 1.18  2005/04/04 19:43:56  mitry
 * Added timeout arg to BUFFLUSH() - tty_bufflush()
 *
 * Revision 1.17  2005/03/31 20:39:20  mitry
 * Added support for HUP_XXX and removed old code
 *
 * Revision 1.16  2005/03/29 20:41:27  mitry
 * Restore saved tio settings before closing port
 *
 * Revision 1.15  2005/03/28 17:02:53  mitry
 * Pre non-blocking i/o update. Mostly non working.
 *
 * Revision 1.14  2005/02/25 16:41:55  mitry
 * Adding proper tio calls in progress
 * Changed some code
 *
 * Revision 1.13  2005/02/23 21:56:26  mitry
 * Changed tty_carrier() code. To be rewritten
 *
 * Revision 1.12  2005/02/22 16:15:39  mitry
 * Code rewrite
 *
 * Revision 1.11  2005/02/21 18:14:15  mitry
 * One more tty_hangedup rename
 *
 * Revision 1.10  2005/02/21 16:33:42  mitry
 * Changed tty_hangedup to tty_online
 *
 * Revision 1.9  2005/02/18 20:39:34  mitry
 * tty_block() commented out
 * tty_close() now has parameter (may be temporarily) and rewritten
 * Added debug messages
 *
 */

#include "headers.h"
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include "tty.h"


#ifdef POSIX_TERMIOS
static char tio_compilation_type[]="@(#)tty.c compiled with POSIX_TERMIOS";
#endif
#ifdef SYSV_TERMIO
static char tio_compilation_type[]="@(#)tty.c compiled with SYSV_TERMIO";
#endif
#ifdef BSD_SGTTY
static char tio_compilation_type[]="@(#)tty.c compiled with BSD_SGTTY";
#endif


#ifdef BSD
# ifndef IUCLC
# define IUCLC 0
# endif
# ifndef TAB3
#  ifdef NeXT
#   define TAB3 XTABS
#  else
#   define TAB3 OXTABS
#  endif	/* !NeXT */
# endif
#endif


#define DEBUG_SLEEP 0

#define IFGOTHUP \
    if ( tty_gothup || (!is_ip && tty_online && !tty_dcd( tty_fd ))) \
        { tty_status = TTY_HANGUP; return RCDO; }

#define	IFLESSZERO \
    if ( rc < 0 ) {        \
        IFGOTHUP;          \
        else return ERROR; \
    }


char *tty_errs[] = {
    "Ok",                    /* ME_OK        0 */
    "tcget/setattr error",   /* ME_ATTRS     1 */
    "bad speed",             /* ME_SPEED     2 */
    "open error",            /* ME_OPEN      3 */
    "read error",            /* ME_READ      4 */
    "write error",           /* ME_WRITE     5 */
    "timeout",               /* ME_TIMEOUT   6 */
    "close error",           /* ME_CLOSE     7 */
    "can't lock port",       /* ME_CANTLOCK  8 */
    "can't set/get flags",   /* ME_FLAGS     9 */
    "not a tty",             /* ME_NOTATT   10 */
    "no DSR/CTS signals"     /* ME_NODSR    11 */
};


/* baud rate table */
static struct	speedtab {
#ifdef POSIX_TERMIOS
    speed_t	cbaud;
#else
    unsigned short cbaud;	/* baud rate, e.g. B9600 */
#endif
    int	 nspeed;		/* speed in numeric format */
    char *speed;		/* speed in display format */
} speedtab[] = {
	{ B50,	  50,	 "50"	 },
	{ B75,	  75,	 "75"	 },
	{ B110,	  110,	 "110"	 },
	{ B134,	  134,	 "134"	 },
	{ B150,	  150,	 "150"	 },
	{ B200,	  200,	 "200"	 },
	{ B300,	  300,	 "300"	 },
	{ B600,	  600,	 "600"	 },
#ifdef	B900
	{ B900,	  900,	 "900"	},
#endif
	{ B1200,  1200,	 "1200"	 },
	{ B1800,  1800,	 "1800"	 },
	{ B2400,  2400,	 "2400"	 },
#ifdef	B3600
	{ B3600,  3600,	 "3600"	},
#endif
	{ B4800,  4800,	 "4800"	 },
#ifdef	B7200
	{ B7200,  7200,  "7200"	},
#endif
	{ B9600,  9600,	 "9600"	 },
#ifdef	B14400
	{ B14400, 14400, "14400" },
#endif
#ifdef	B19200
	{ B19200, 19200, "19200" },
#endif	/* B19200 */
#ifdef	B28800
	{ B28800, 28800, "28800" },
#endif
#ifdef	B38400
	{ B38400, 38400, "38400" },
#endif	/* B38400 */
#ifdef	EXTA
	{ EXTA,	  19200, "EXTA"	 },
#endif
#ifdef	EXTB
	{ EXTB,	  38400, "EXTB"	 },
#endif
#ifdef	B57600
	{ B57600, 57600, "57600" },
#endif
#ifdef	B76800
	{ B76800, 76800, "76800" },
#endif
#ifdef	B115200
	{ B115200,115200,"115200"},
#endif
#ifdef B230400
	{ B230400,230400,"230400"},
#endif
#ifdef B460800
	{ B460800,460800,"460800"},
#endif
	{ 0,	  0,	 ""	 }
};


TIO tty_stio;

#if 0
#ifdef HYDRA8K16K
#define RX_BUF_SIZE (16384 + 1024)
#define TX_BUF_SIZE (16384 + 1024)
#else
#define RX_BUF_SIZE (4096 + 1024)
#define TX_BUF_SIZE (4096 + 1024)
#endif
#endif /* 0 */

#define RX_BUF_SIZE (0x8100)
#define TX_BUF_SIZE (0x8100)

static unsigned char tty_rx_buf[RX_BUF_SIZE];
static unsigned char tty_tx_buf[TX_BUF_SIZE];

/* tty_rx_left - number of bytes in receive buffer
 * tty_rx_ptr  - pointer to next byte in receive buffer
 * tty_tx_ptr  - pointer to next byte to send out in transmit buffer
 * tty_tx_free - number of byte left in transmit buffer
 *
 *                             |  <- tty_rx_left ->  |
 * +---------------------------+*********************+--------------------+
 * |read data//  tty_rx_ptr <- |       new data      |                    | RX_BUF_SIZE
 * +---------------------------+*********************+--------------------+
 *
 *                                                   | <- tty_tx_free ->  |
 * +---------------------------+*********************+--------------------+
 * |sent data//  tty_tx_ptr -> |     unsent data     | <-   new data      | TX_BUF_SIZE
 * +---------------------------+*********************+--------------------+
 */
static int tty_rx_ptr = 0;
static int tty_rx_left = 0;
static int tty_tx_ptr = 0;
static int tty_tx_free = TX_BUF_SIZE;

int tty_fd = -1;
int tty_error = 0;
int tty_status = TTY_SUCCESS;


#define FDS(f) ( (f) ? ( *(f) ? "true" : "false" ) : "not set" )

/*
 * Checks out whether or not stdin and/or stdout are ready to receive/send data
 *
 * Parameters:
 *    rd   - if not NULL & TRUE - check read (stdin) fd
 *    wd   - if not NULL & TRUE - check write (stdout) fd
 *    tval - a pointer to struct timeval to wait for event. If tval:
 *           NULL  - wait for at least one fd is ready or forever
 *           {0,0} - do not wait at all, just return fds state
 *           other - wait for specified time
 *
 * Return values: number of fds are ready or -1 on error (errno in tty_error).
 *    rd - true if stdin is ready
 *    wd - true if stdout is ready
 */
int tty_select(boolean *rd, boolean *wd, struct timeval *tval)
{
	fd_set	rfd, wfd;
	int rc;

	DEBUG(('T',2,"tty_select"));

	FD_ZERO( &rfd );
	FD_ZERO( &wfd );
	if ( rd && *rd ) {
		FD_SET( tty_fd, &rfd );
		*rd = FALSE;
	}
	if ( wd && *wd ) {
		FD_SET( tty_fd, &wfd );
		*wd = FALSE;
	}
    
	tty_error = 0;
	rc = select( tty_fd + 1, &rfd, &wfd, NULL, ( tval ? tval : NULL ));

	tty_error = errno;
	tty_status = TTY_SUCCESS;
	if ( rc < 0 ) {
		if ( EWBOEA() ) {
			tty_status = TTY_TIMEOUT;
		} else if ( errno == EINTR ) {
			tty_status = ( tty_online && tty_gothup ) ? TTY_HANGUP : TTY_TIMEOUT;
        } else if ( errno == EPIPE ) {
		tty_gothup = HUP_LINE;
		tty_status = TTY_HANGUP;
        } else
		tty_status = TTY_ERROR;
	} else if ( rc == 0 )
		tty_status = TTY_TIMEOUT;
	else {
		if ( rd && FD_ISSET( tty_fd, &rfd ))
			*rd = TRUE;
		if ( wd && FD_ISSET( tty_fd, &wfd ))
			*wd = TRUE;
	}

	DEBUG(('T',2,"tty_select: fd=%d rc=%i (rd=%s, wd=%s)", tty_fd, rc, FDS( rd ), FDS( wd )));

	return rc;
}


/*
 * Returns whether the rx_buf has data.
 */
int tty_hasinbuf(void)
{
	return ( tty_rx_left > 0 );
}


/*
 * Returns whether the in stream has data.
 */
int tty_hasdata(int sec, int usec)
{
	boolean		rd = TRUE;
	struct timeval	tv;

	DEBUG(('T',5,"tty_hasdata"));

	if ( tty_hasinbuf() )
		return 1;

	tv.tv_sec = sec;
	tv.tv_usec = usec;
	return ( tty_select( &rd, NULL, &tv ) > 0 && rd );
}


int tty_hasdata_timed(int *timeout)
{
	int	rc;
	time_t	t = time( NULL );

	rc = tty_hasdata( *timeout, 0 );
	*timeout -= (time( NULL ) - t);

	return rc;
}



/*
 * Attempts to write nbytes of data from the buffer pointed to by buf to stdout.
 *
 * Return values:
 *    Upon successful completion the number of bytes which were written is
 *    returned. Otherwise a value < 0 is returned and the tty_error and tty_status
 *    are set to indicate the error.
 */
int tty_write(const void *buf, size_t nbytes)
{
	int	rc;

#ifdef NEED_DEBUG
	byte	*bbuf = (byte *) buf;
	int	i;
#endif

	DEBUG(('T',5,"tty_write"));

	IFGOTHUP;

	if ( nbytes == 0 )
		return 0;

	rc = write( tty_fd, buf, nbytes );

	tty_error = errno;
	IFGOTHUP;

	if ( rc < 0 ) {
		if ( EWBOEA() ) {
			tty_status = TTY_TIMEOUT;
		} else if ( errno == EINTR ) {
			if ( tty_online && tty_gothup )
				tty_status = TTY_HANGUP;
			else
				tty_status = TTY_TIMEOUT;
		} else if ( errno == EPIPE ) {
			tty_gothup = HUP_LINE;
			tty_status = TTY_HANGUP;
		} else
			tty_status = TTY_ERROR;
	} else if ( rc == 0 ) {
		tty_status = TTY_ERROR;
	} else /* rc > 0 */
		tty_status = TTY_SUCCESS;

	DEBUG(('T',6,"tty_write: rc = %d", rc));

#ifdef NEED_DEBUG
	if ( rc > 0 )
		for( i = 0; i < rc; i++ )
			DEBUG(('T',9,"tty_write: '%c' (%d)", C0(bbuf[i]), bbuf[i]));
#endif

	return ( tty_status == TTY_SUCCESS ? rc : tty_status );
}


/*
 * Attempts to read nbytes of data from the stdin into the buffer pointed to
 * by buf.
 *
 * Return values:
 *    Upon successful completion the number of bytes which were read is
 *    returned. Otherwise a value < 0 is returned and the tty_error and tty_status
 *    are set to indicate the error.
 */
int tty_read(void *buf, size_t nbytes)
{
	int	rc;

#ifdef NEED_DEBUG
	byte	*bbuf = (byte *) buf;
	int	i;
#endif

	DEBUG(('T',5,"tty_read"));

	IFGOTHUP;

	rc = read( tty_fd, buf, nbytes );
	IFGOTHUP;

	tty_error = errno;
	if ( rc < 0 ) {
		if ( EWBOEA() ) {
			tty_status = TTY_TIMEOUT;
		} else if ( errno == EINTR ) {
			if ( tty_online && tty_gothup )
				tty_status = TTY_HANGUP;
			else
				tty_status = TTY_TIMEOUT;
		} else if ( errno == EPIPE ) {
			tty_gothup = HUP_LINE;
			tty_status = TTY_HANGUP;
		} else
			tty_status = TTY_ERROR;
	} else if ( rc == 0 ) {
		tty_status = TTY_ERROR;
	} else /* rc > 0 */
		tty_status = TTY_SUCCESS;

	DEBUG(('T',6,"tty_read: rc = %d", rc));

#ifdef NEED_DEBUG
	if ( rc > 0 )
		for( i = 0; i < rc; i++ )
			DEBUG(('T',9,"tty_read: '%c' (%d)", C0(bbuf[i]), bbuf[i]));
#endif

	return ( tty_status == TTY_SUCCESS ? rc : tty_status );
}


RETSIGTYPE tty_sighup(int sig)
{
	DEBUG(('T',1,"tty_sighup: got SIG%s signal, %d", sigs[sig], tty_gothup));

	getevt();
	DEBUG(('T',2,"tty_sighup: %d", tty_gothup));
	if (( is_ip && sig == SIGPIPE ) || ( !is_ip && !tty_dcd( tty_fd )))
		tty_gothup = HUP_LINE;
	else
		tty_gothup = HUP_OPERATOR;
}


char *baseport(const char *p)
{
	char		*q;
	static char	pn[MAX_PATH + 5];

	q = qbasename( p );
	if ( !q || !*q )
		return NULL;

	xstrcpy( pn, q, MAX_PATH );
	if (( q = strrchr( pn, ':' )))
		*q = 0;
	return pn;
}


int tty_isfree(const char *port, const char *nodial)
{
	int		pid = 0;
	FILE		*f;
	struct stat	s;
	char		lckname[MAX_PATH + 5];

	if ( !port )
		return 0;

	if ( nodial ) {
		snprintf( lckname, MAX_PATH, "%s.%s", nodial, port );
		if ( !stat( lckname, &s ))
			return 0;
	}

	LCK_NAME(lckname, port);
	if (( f = fopen( lckname, "r" ))) {
		fscanf( f, "%d", &pid );
		fclose( f );
		if ( pid && kill( pid, 0 ) && ( errno == ESRCH )) {
			lunlink( lckname );
			return 1;
		}
		return 0;
	}
	return 1;
}


char *tty_findport(slist_t *ports, const char *nodial)
{
	for(; ports; ports = ports->next )
		if( tty_isfree( baseport( ports->str ), nodial ))
			return ports->str;
	return NULL;
}


int tty_lock(const char *port)
{
	int		rc = -1;
	char		lckname[MAX_PATH + 5];
	const char	*p;

	DEBUG(('M',1,"tty_lock"));
	if ( !( p = strrchr( port, '/' )))
		p = port;
	else
		p++;

	LCK_NAME(lckname, p);
	rc = lockpid( lckname );
	return rc ? 0 : -1;
}


void tty_unlock(const char *port)
{
	long		pid;
	char		lckname[MAX_PATH + 5];
	const char	*p;
	FILE		*f;

	DEBUG(('M',1,"tty_unlock"));
	if ( !( p = strrchr( port, '/' )))
		p = port;
	else
		p++;

	LCK_NAME(lckname, p);
	if (( f = fopen( lckname, "r" ))) {
		fscanf( f, "%ld", &pid );
		fclose( f );
	}

	if ( pid == getpid())
		lunlink( lckname );
}


int tty_close(void)
{
	if ( !tty_port )
		return ME_CLOSE;

	DEBUG(('M',2,"tty_close"));

	tio_flush_queue( tty_fd, TIO_Q_BOTH );
	tio_set( tty_fd, &tty_stio );

	(void) close( tty_fd );

	tty_unlock( tty_port );
	xfree( tty_port );
	tty_online = FALSE;
    
	return ME_OK;
}


int tty_local(TIO *tio, int local)
{
	signal( SIGHUP, local ? SIG_IGN : tty_sighup );
	signal( SIGPIPE, local ? SIG_IGN : tty_sighup );

	if ( !isatty( tty_fd ))
		return ME_NOTATT;

	tio_local_mode( tio, local );
	tio_set_flow_control( tty_fd, tio, local ? FLOW_NONE : FLOW_HARD );

	if ( local )
		tty_gothup = FALSE;
	return 1;
}


void tty_bufclear(void)
{
	tty_tx_ptr = 0;
	tty_tx_free = TX_BUF_SIZE;
}


int tty_bufflush(int tsec)
{	
	int		rc = OK, restsize = TX_BUF_SIZE - tty_tx_free - tty_tx_ptr;
	boolean		wd;
	struct timeval	tv;
	timer_t		tm;
    
	tm = timer_set( tsec );
	while( TX_BUF_SIZE != tty_tx_free ) {
		wd = true;
		tv.tv_sec = timer_rest( tm );
		tv.tv_usec = 0;

		if (( rc = tty_select( NULL, &wd, &tv )) > 0 && wd ) {
			rc = tty_write( tty_tx_buf + tty_tx_ptr, restsize );

			if ( rc == restsize ) {
				tty_bufclear();
			} else if ( rc > 0 ) {
				tty_tx_ptr += rc;
				restsize -= rc;
			} else if ( rc < 0 && tty_status != TTY_TIMEOUT )
				return ERROR;
		} else
			return rc;

		if ( timer_expired( tm ))
			return ERROR;
	}
	return rc;
}


int tty_bufblock(const void *data, size_t nbytes)
{
	int rc = OK, txptr = TX_BUF_SIZE - tty_tx_free;
	char *nptr = (char *)data;

	DEBUG(('T',8,"tty_bufblock: tty_tx_ptr=%d, tty_tx_free=%d, need2buf=%d",
		tty_tx_ptr,tty_tx_free,nbytes));

	tty_status = TTY_SUCCESS;

	while ( nbytes ) {
		if ( nbytes > (size_t) tty_tx_free ) {
			do {
				tty_bufflush( 5 );
				if ( tty_status == TTY_SUCCESS ) {
					size_t n = MIN( (size_t) tty_tx_free, nbytes);
					memcpy( tty_tx_buf, nptr, n );
					tty_tx_free -= n;
					nbytes -= n;
					nptr += n;
				}
			} while ( tty_status != TTY_SUCCESS );
		} else {
			memcpy( (void *) (tty_tx_buf + txptr), nptr, nbytes );
			tty_tx_free -= nbytes;
			nbytes = 0;
		}
	}
	return rc;
}


int tty_bufc(char ch)
{
	return tty_bufblock( &ch, 1 );
}


int tty_putc(char ch)
{
	tty_bufblock( &ch, 1 );
	return tty_bufflush( 5 );
}


int tty_getc(int timeout)
{

	DEBUG(('T',8,"tty_getc: tty_rx_ptr=%d, tty_rx_left=%d",tty_rx_ptr,tty_rx_left));
	if ( tty_rx_left == 0 ) {
		int rc = tty_hasdata( timeout, 0 );
		/*
		int rc = ( timeout > 0 ) ? tty_hasdata( timeout, 0 ) : 1;
                */

		IFGOTHUP;

		if ( rc > 0 ) {
			if (( rc = tty_read( tty_rx_buf, RX_BUF_SIZE )) < 0 ) {
				IFGOTHUP;
				return ( EWBOEA()) ? TTY_TIMEOUT : ERROR;
			} else if ( rc == 0 )
				return ERROR;
			tty_rx_ptr = 0;
			tty_rx_left = rc;
		} else
			return ( tty_gothup ? TTY_HANGUP : TTY_TIMEOUT );
	}

	tty_rx_left--;
	return tty_rx_buf[tty_rx_ptr++];
}


int tty_getc_timed(int *timeout)
{
	int	rc;
	time_t	t = time( NULL );

	rc = tty_getc( *timeout );
	*timeout -= (time( NULL ) - t);
	return rc;
}


void tty_purge(void) {
	DEBUG(('M',3,"tty_purge"));

	tty_rx_ptr = tty_rx_left = 0;
	if ( isatty( tty_fd ))
		tio_flush_queue( tty_fd, TIO_Q_IN );
}


void tty_purgeout(void) {
	DEBUG(('M',3,"tty_purgeout"));

	tty_bufclear();
	if ( isatty( tty_fd ))
		tio_flush_queue( tty_fd, TIO_Q_OUT );
}


int tty_send_break(void)
{
#ifdef POSIX_TERMIOS
	if ( tcsendbreak( tty_fd, 0 ) < 0 ) {
		DEBUG(('M',3,"tcsendbreak() failed"));
		return -1;
	}
#endif
#ifdef SYSV_TERMIO
	if ( ioctl( tty_fd, TCSBRK, 0 ) < 0 ) {
		DEBUG(('M',3,"ioctl( TCSBRK ) failed"));
		return -1;
	}
#endif
#ifdef BSD_SGTTY
	if ( ioctl( tty_fd, TIOCSBRK, 0 ) < 0 ) {
		DEBUG(('M',3,"ioctl( TIOCSBRK ) failed"));
		return -1;
	}
	qsleep( 1000 );
	if ( ioctl( tty_fd, TIOCCBRK, 0 ) < 0 ) {
		DEBUG(('M',3,"ioctl( TIOCCBRK ) failed"));
		return -1;
	}
#endif

	return 0;
}


/* Return the amount of remaining bytes to be transmitted */
int tty_hasout(void)
{
	return (tty_tx_free != TX_BUF_SIZE);
}


/*
 * Returns whether CD line of given tty is on or off
 */
int tty_dcd(int fd)
{
	int rs_lines = tio_get_rs232_lines( fd );

	DEBUG(('M',2,"tty_dcd: CD is %s",(rs_lines & TIO_F_DCD ? "On" : "Off")));

	return ( rs_lines & TIO_F_DCD );
}


/*
 * Close all FDs >= a specified value
 */
static void fd_close_all(int startfd)
{
	int fdlimit = sysconf(_SC_OPEN_MAX);

	while( startfd < fdlimit )
		close( startfd++ );
}


/*
 * Make file descriptor stdin/stdout/stderr
 */
int fd_make_stddev(int fd)
{

	fflush( stdin );
	fflush( stdout );
	fflush( stderr );

	if ( fd > 0 ) {
		(void) close( 0 );
		if ( dup( fd ) != 0 ) {
			DEBUG(('T',2,"fd_make_stddev: can't dup(fd=%d) to stdin", fd));
			return ERROR;
		}
		close( fd );
	}

	(void) close( 1 );
	if ( dup( 0 ) != 1 ) {
		DEBUG(('T',2,"fd_make_stddev: can't dup(fd=%d) to stdout", fd));
		return ERROR;
	}

	(void) close( 2 );
	if ( dup( 0 ) != 2 ) {
		DEBUG(('T',2,"fd_make_stddev: can't dup(0) to stderr"));
		return ERROR;
	}

	setbuf( stdin,  (char *) NULL );
	setbuf( stdout, (char *) NULL );
	setbuf( stderr, (char *) NULL );

	clearerr( stdin );
	clearerr( stdout );
	clearerr( stderr );

	return OK;
}


/*
 * Set the O_NONBLOCK flag of desc if value is nonzero,
 * or clear the flag if value is 0.
 * Return 0 on success, or -1 on error with errno set.
 */
int fd_set_nonblock(int fd, int mode)
{
	int misc;

#if defined(FIONBIO)

	/*
	 * Set non-blocking I/O mode on sockets
	 */
	misc = 1;
	if ( ioctl( fd, FIONBIO, &misc, sizeof( misc )) == -1 ) {
		DEBUG(('T',1,"ioctl: can't set %sblocking mode on fd %d: %s",
			(mode ? "non-" : ""), fd, strerror( errno )));
		return -1;
	}
#endif

	misc = fcntl( fd, F_GETFL, 0);
	if ( misc == -1 ) {
		DEBUG(('T',1,"fnctl: can't get flags on fd %d: %s",
			fd, strerror( errno )));
		return -1;
	}

	if ( mode )
		misc |= O_NONBLOCK;
	else
		misc &= ~O_NONBLOCK;

	if (( misc = fcntl( fd, F_SETFL, misc )) == -1 ) {
		DEBUG(('T',1,"fnctl: can't set %sblocking mode on fd %d: %s",
			(mode ? "non-" : ""), fd, strerror( errno )));
	}
	return misc;
}



/*
 * tio part
 */

/* get current tio settings for given filedescriptor */
int tio_get(int fd, TIO *t)
{ 
#ifdef SYSV_TERMIO
    if ( ioctl( fd, TCGETA, t ) < 0 )
    {
        DEBUG(('T',3,"TCGETA failed"));
        return ERROR;
    }
#endif
#ifdef POSIX_TERMIOS
    if ( tcgetattr( fd, t ) < 0 )
    {
        DEBUG(('T',3,"tcgetattr failed"));
        return ERROR;
    }
#endif
#ifdef BSD_SGTTY
    if ( gtty( fd, t ) < 0 )
    {
        DEBUG(('T',3,"gtty failed"));
        return ERROR;
    }
#endif
    DEBUG(('T',4,"tio_get: c_iflag=%08x, c_oflag=%08x, c_cflag=%08x, c_lflag=%08x",
        t->c_iflag, t->c_oflag, t->c_cflag, t->c_lflag));
    return OK;
}


/* set current tio settings for given filedescriptor */
int tio_set(int fd, TIO *t)
{
#ifdef SYSV_TERMIO
    if ( ioctl( fd, TCSETA, t ) < 0 )
    {
        DEBUG(('T',3,"ioctl TCSETA failed"));
        return ERROR;
    }
#endif
#ifdef POSIX_TERMIOS
    if ( tcsetattr( fd, TCSANOW, t ) < 0 )
    {
        DEBUG(('T',3,"tcsetattr failed"));
        return ERROR;
    }
#endif /* posix_termios */

#ifdef BSD_SGTTY
    if ( stty( fd, t ) < 0 )
    {
        DEBUG(('T',3,"stty failed"));
        return ERROR;
    }
#endif
    DEBUG(('T',4,"tio_set: c_iflag=%08x, c_oflag=%08x, c_cflag=%08x, c_lflag=%08x",
        t->c_iflag, t->c_oflag, t->c_cflag, t->c_lflag));
    return OK;
}


/*
 * Set speed, do not touch the other flags
 * "speed" is given as numeric baud rate, not as Bxxx constant
 */
int tio_set_speed(TIO *t, unsigned int speed)
{
    int i, symspeed = 0;

    for( i = 0; speedtab[i].cbaud != 0; i++ )
    {
	if ( speedtab[i].nspeed == speed ) 
		{ symspeed = speedtab[i].cbaud; break; }
    }

    if ( symspeed == 0 ) {
        errno = EINVAL;
        DEBUG(('T',2,"tss: unknown/unsupported bit rate: %d", speed));
        return ERROR;
    }

    DEBUG(('T',2,"tss: set speed to %d (%03o)", speed, symspeed));
	
#ifdef SYSV_TERMIO
    t->c_cflag = ( t->c_cflag & ~CBAUD) | symspeed;
#endif
#ifdef POSIX_TERMIOS
    cfsetospeed( t, symspeed );
    cfsetispeed( t, symspeed );
#endif
#ifdef BSD_SGTTY
    t->sg_ispeed = t->sg_ospeed = symspeed;
#endif
    return OK;
}

/*
 * Get port speed. Return integer value, not symbolic constant
 */
int tio_get_speed(TIO *t)
{
#ifdef SYSV_TERMIO
    ushort cbaud = t->c_cflag & CBAUD;
#endif
#ifdef POSIX_TERMIOS
    speed_t cbaud = cfgetospeed( t );
#endif
#ifdef BSD_SGTTY
    int cbaud = t->sg_ospeed;
#endif
    struct speedtab *st;

    for( st = speedtab; st->nspeed != 0; st++ ) {
        if ( st->cbaud == cbaud )
	    break;
    }
    return st->nspeed;
}
 

/* set "sane" mode, usable for login, ...
 * unlike the other tio_mode_* functions, this function initializes
 * all flags, and should be called before calling any other function
 */
void tio_local_mode(TIO * t, int local)
{
	if ( local )
#if defined(SYSV_TERMIO) || defined( POSIX_TERMIOS )
		t->c_cflag |= CLOCAL;
	else
		t->c_cflag &= ~CLOCAL;
#else		/* BSD_SGTTY (tested only on NeXT yet, but should work) */
		t->sg_flags &= ~LNOHANG;
	else
		t->sg_flags |= LNOHANG ;
#endif
}


/*
 */

#define CFLAGS_TO_SET (CREAD | HUPCL)
#define CFLAGS_TO_CLEAR (CSTOPB | PARENB | PARODD | CLOCAL)

void tio_raw_mode(TIO * t)
{
#if defined(SYSV_TERMIO) || defined( POSIX_TERMIOS)
#if 0
    t->c_iflag &= ( IXON | IXOFF | IXANY );	/* clear all flags except */
						/* xon / xoff handshake */
    t->c_oflag  = 0;				/* no output processing */
    t->c_lflag  = 0;				/* no signals, no echo */
#endif /* 0 */
    t->c_cc[VMIN]  = 1;				/* disable line buffering */
    t->c_cc[VTIME] = 0;

    /*
    t->c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP
                            | INLCR | IGNCR | ICRNL | IXON );
    */
    t->c_iflag = 0;
    t->c_oflag = 0;
    t->c_lflag = 0;
    t->c_cflag &= ~( CSIZE | CFLAGS_TO_CLEAR );
    t->c_cflag |= ( CS8 | CFLAGS_TO_SET );

#else
    t->sg_flags = RAW;
#endif
}


/*
 * initialize all c_cc fields (for POSIX and SYSV) to proper start
 * values (normally, the serial driver should do this, but there are
 * numerous systems where some of the more esoteric (VDSUSP...) flags
 * are plain wrong (e.g. set to "m" or so)
 *
 * do /not/ initialize VERASE and VINTR, since some systems use
 * ^H / DEL here, others DEL / ^C.
 */
void tio_default_cc(TIO *t)
{
#ifdef BSD_SGTTY
    t->sg_erase = 0x7f;            /* erase character */
    t->sg_kill  = 0x25;            /* kill character, ^u */

#else /* posix or sysv */
    t->c_cc[VQUIT]  = CQUIT;
    t->c_cc[VKILL]  = CKILL;
    t->c_cc[VEOF]   = CEOF;
#if defined(VEOL) && VEOL < TIONCC
    t->c_cc[VEOL] = CEOL;
#endif
#if defined(VSTART) && VSTART < TIONCC
    t->c_cc[VSTART] = CSTART;
#endif
#if defined(VSTOP) && VSTOP < TIONCC
    t->c_cc[VSTOP] = CSTOP;
#endif
#if defined(VSUSP) && VSUSP < TIONCC
    t->c_cc[VSUSP] = CSUSP;
#endif
#if defined(VSWTCH) && VSWTCH < TIONCC
    t->c_cc[VSWTCH] = CSWTCH;
#endif
    /* the following are for SVR4.2 (and higher) */
#if defined(VDSUSP) && VDSUSP < TIONCC
    t->c_cc[VDSUSP] = CDSUSP;
#endif
#if defined(VREPRINT) && VREPRINT < TIONCC
    t->c_cc[VREPRINT] = CRPRNT;
#endif
#if defined(VDISCARD) && VDISCARD < TIONCC
    t->c_cc[VDISCARD] = CFLUSH;
#endif
#if defined(VWERASE) && VWERASE < TIONCC
    t->c_cc[VWERASE] = CWERASE;
#endif
#if defined(VLNEXT) && VLNEXT < TIONCC
    t->c_cc[VLNEXT] = CLNEXT;
#endif

#endif /* bsd <-> posix + sysv */
}


/* 
 * set flow control according to the <type> parameter. It can be any
 * combination of
 *   FLOW_XON_IN - use Xon/Xoff on incoming data
 *   FLOW_XON_OUT- respect Xon/Xoff on outgoing data
 *   FLOW_HARD   - use RTS/respect CTS line for hardware handshake
 * (not every combination will work on every system)
 *
 * WARNING: for most systems, this function will not touch the tty
 *          settings, only modify the TIO structure.
 */
int tio_set_flow_control(int fd, TIO *t, int type)
{
#ifdef USE_TERMIOX
    struct termiox tix;
#endif

    DEBUG(('T',2,"tio_set_flow_control(%s%s%s%s )",
        type & FLOW_HARD   ? " HARD": "",
        type & FLOW_XON_IN ? " XON_IN": "",
        type & FLOW_XON_OUT? " XON_OUT": "",
        type == FLOW_NONE ? " NONE" : "" ));
    
#if defined( SYSV_TERMIO ) || defined( POSIX_TERMIOS )
    t->c_cflag &= ~HARDW_HS;
    t->c_iflag &= ~( IXON | IXOFF | IXANY );

    if ( type & FLOW_HARD )
        t->c_cflag |= HARDW_HS;
    if ( type & FLOW_XON_IN )
        t->c_iflag |= IXOFF;
    if ( type & FLOW_XON_OUT ) {
        t->c_iflag |= IXON;
        if ( type & FLOW_XON_IXANY )
            t->c_iflag |= IXANY;
    }
#else
# ifdef NEXTSGTTY
    DEBUG(('T',3,"tio_set_flow_control: not yet implemented"));
# else
#  error "not yet implemented"
# endif
#endif

    /* SVR4 came up with a new method of setting h/w flow control */
#ifdef USE_TERMIOX
    DEBUG(('T',3,"tio_set_flow_control: using termiox"));

    if ( ioctl(fd, TCGETX, &tix ) < 0) {
        DEBUG(('T',3,"ioctl TCGETX"));
        return ERROR;
    }
    if ( type & FLOW_HARD )
        tix.x_hflag |= (RTSXOFF | CTSXON);
    else
        tix.x_hflag &= ~(RTSXOFF | CTSXON);
    
    if ( ioctl( fd, TCSETX, &tix ) < 0 ) {
        DEBUG(('T',3,"ioctl TCSETX" ));
	return ERROR;
    }
#endif

    return OK;
}


/*
 * Returns 0 if `speed' was not found in speedtab[] or
 * symbolic speed otherwise.
 */
int tio_trans_speed(unsigned int speed)
{
    struct speedtab *st;

    for( st = speedtab; st->nspeed != 0; st++ ) {
        if ( st->nspeed == speed )
            break;
    }
    return st->cbaud;
}


/*
 * Toggle dtr with delay for msec milliseconds
 */
int tio_toggle_dtr(int fd, int msec)
{
#if defined(TIOCMBIS) && \
    ( defined(sun) || defined(SVR4) || defined(NeXT) || defined(linux) )
    
    int mctl = TIOCM_DTR;

    DEBUG(('T',2,"tio_toggle_dtr: %d msec", msec));
#if !defined( TIOCM_VALUE )
    if ( ioctl( fd, TIOCMBIC, &mctl ) < 0 )
#else
    if ( ioctl( fd, TIOCMBIC, (char *) mctl ) < 0 )
#endif
    {
        DEBUG(('T',2,"tio_toggle_dtr: TIOCMBIC failed"));
        return ERROR;
    }
    
    qsleep( msec );

#if !defined( TIOCM_VALUE)
    if ( ioctl( fd, TIOCMBIS, &mctl ) < 0 )
#else
    if ( ioctl( fd, TIOCMBIS, (char *) mctl ) < 0 )
#endif
    {
        DEBUG(('T',2,"tio_toggle_dtr: TIOCMBIS failed"));
        return ERROR;
    }
    return OK;
#else						/* !TIOCMBI* */

    /* On HP/UX, lowering DTR by setting the port speed to B0 will
     * leave it there. So, do it via HP/UX's special ioctl()'s...
     */
#if defined(_HPUX_SOURCE) || defined(MCGETA)
    unsigned long mflag = 0L;

    DEBUG(('T',2,"tio_toggle_dtr: %d msec", msec));
    if ( ioctl( fd, MCSETAF, &mflag ) < 0 ) {
        DEBUG(('T',2,"tio_toggle_dtr: MCSETAF failed"));
        return ERROR;
    }
    
    qsleep( msec );

    if ( ioctl( fd, MCGETA, &mflag ) < 0 ) {
        DEBUG(('T',2,"tio_toggle_dtr: MCGETA failed"));
        return ERROR;
    }
    mflag = MRTS | MDTR;
    if ( ioctl( fd, MCSETAF, &mflag ) < 0 ) {
        DEBUG(('T',2,"tio_toggle_dtr: MCSETAF failed"));
        return ERROR;
    }
    return OK;

#else /* !MCGETA */
    
    /* The "standard" way of doing things - via speed = B0
     */
    TIO t, save_t;
    int result;

    DEBUG(('T',2,"tio_toggle_dtr: %d msec", msec));
    if ( tio_get( fd, &t ) == ERROR ) {
         DEBUG(('T',2,"tio_toggle_dtr: tio_get failed"));
         return ERROR;
    }

    save_t = t;
    
#ifdef SYSV_TERMIO 
    t.c_cflag = ( t.c_cflag & ~CBAUD ) | B0;		/* speed = 0 */
#endif
#ifdef POSIX_TERMIOS
    cfsetospeed( &t, B0 );
    cfsetispeed( &t, B0 );
#endif
#ifdef BSD_SGTTY
    t.sg_ispeed = t.sg_ospeed = B0
#endif

    tio_set( fd, &t );
    qsleep( msec );
    result = tio_set( fd, &save_t );
    
    DEBUG(('T',2,"tio_toggle_dtr: result %d", result));
    return result;
#endif					/* !MCSETA */
#endif					/* !SVR4 */
}


/*
 * Flush input or output data queue
 *
 * "queue" is one of the TIO_Q* values from tio.h
 */
int tio_flush_queue(int fd, int queue)
{
    int r = OK;
#ifdef POSIX_TERMIOS
    switch( queue ) {
        case TIO_Q_IN:   r = tcflush( fd, TCIFLUSH ); break;
        case TIO_Q_OUT:  r = tcflush( fd, TCOFLUSH ); break;
        case TIO_Q_BOTH: r = tcflush( fd, TCIOFLUSH );break;
        default:
            DEBUG(('T',2,"tio_flush_queue: invalid ``queue'' argument (%d)", queue ));
            return ERROR;
    }
#endif
#ifdef SYSV_TERMIO
    switch ( queue ) {
        case TIO_Q_IN:   r = ioctl( fd, TCFLSH, 0 ); break;
        case TIO_Q_OUT:  r = ioctl( fd, TCFLSH, 1 ); break;
        case TIO_Q_BOTH: r = ioctl( fd, TCFLSH, 2 ); break;
        default:
            DEBUG(('T',2,"tio_flush_queue: invalid ``queue'' argument (%d)", queue ));
            return ERROR;
    }
#endif
#ifdef BSD_SGTTY
    int arg;
    
    switch ( queue ) {
        case TIO_Q_IN:   arg = FREAD; break;
        case TIO_Q_OUT:  arg = FWRITE; break;
        case TIO_Q_BOTH: arg = FREAD | FWRITE; break;
        default:
            DEBUG(('T',2,"tio_flush_queue: invalid ``queue'' argument (%d)", queue ));
            return ERROR;
    }
    r = ioctl( fd, TIOCFLUSH, (char *) &arg );
#endif
    if ( r != 0 )
        DEBUG(('T',2,"tio: cannot flush queue" ));

    return r;
}


/*
 * tio_drain(fd): wait for output queue to drain
 */
int tio_drain_output(int fd)
{
#ifdef POSIX_TERMIOS
    if ( tcdrain( fd ) == ERROR ) {
        DEBUG(('T',2,"tio_drain: tcdrain" ));
        return ERROR;
    }
#else
# ifdef SYSV_TERMIO
    if ( ioctl( fd, TCSBRK, 1 ) == ERROR ) {
        DEBUG(('T',2,"tio_drain: TCSBRK/1" ));
        return ERROR;
    }
# else	/* no way to wait for data to drain with BSD_SGTTY */
    DEBUG(('T',2,"tio_drain: expect spurious failures" ));
# endif
#endif
    return OK;
}


/* tio_get_rs232_lines()
 *
 * get the status of all RS232 status lines
 * (coded by the TIO_F_* flags. On systems that have the BSD TIOCM_*
 * flags, we use them, on others we may have to do some other tricks)
 *
 * "-1" can mean "error" or "not supported on this system" (e.g. SCO).
 */
int tio_get_rs232_lines(int fd)
{
    int flags;
#ifdef TIO_F_SYSTEM_DEFS
    if ( ioctl(fd, TIOCMGET, &flags ) < 0 )
        DEBUG(('M',3,"tio_get_rs232_lines: TIOCMGET failed"));

#else /* !TIO_F_SYSTEM_DEFS */
    flags=-1;
#endif

    if ( flags != -1 )
    {
        DEBUG(('M',3,"tio_get_rs232_lines: status: [%s][%s][%s][%s][%s][%s]",
            ( flags & TIO_F_RTS ) ? "RTS" : "rts",
            ( flags & TIO_F_CTS ) ? "CTS" : "cts",
            ( flags & TIO_F_DSR ) ? "DSR" : "dsr",
            ( flags & TIO_F_DTR ) ? "DTR" : "dtr",
            ( flags & TIO_F_DCD ) ? "DCD" : "dcd",
            ( flags & TIO_F_RI  ) ? "RI" : "ri" ));
    }
    return flags;
}

