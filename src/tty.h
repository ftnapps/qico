/* $Id: tty.h,v 1.16 2005/05/16 20:33:46 mitry Exp $ */
/*
 * $Log: tty.h,v $
 * Revision 1.16  2005/05/16 20:33:46  mitry
 * Code cleaning
 *
 * Revision 1.15  2005/05/06 20:53:06  mitry
 * Changed misc code
 *
 * Revision 1.14  2005/04/08 18:14:24  mitry
 * Insignificant changes of PUTxxx and BUFxxx defines
 *
 * Revision 1.13  2005/04/04 19:43:56  mitry
 * Added timeout arg to BUFFLUSH() - tty_bufflush()
 *
 * Revision 1.12  2005/03/31 19:40:38  mitry
 * Update function prototypes and it's duplication
 *
 * Revision 1.11  2005/03/28 17:02:53  mitry
 * Pre non-blocking i/o update. Mostly non working.
 *
 * Revision 1.10  2005/02/25 16:41:56  mitry
 * Adding proper tio calls in progress
 * Changed some code
 *
 * Revision 1.9  2005/02/22 16:00:48  mitry
 * Changed M_STAT definition
 *
 * Revision 1.8  2005/02/21 16:33:42  mitry
 * Changed tty_hangedup to tty_online
 *
 * Revision 1.7  2005/02/18 20:36:18  mitry
 * Reflect to rename of functions
 *
 */

#ifndef __TTY_H__
#define __TTY_H__


#ifdef NEXTSGTTY
# define BSD_SGTTY
# undef POSIX_TERMIOS
# undef SYSV_TERMIO
#endif

#if !defined( POSIX_TERMIOS ) && !defined( BSD_SGTTY ) && !defined( SYSV_TERMIO)
# if defined(linux) || defined(sunos4) || defined(_AIX) || defined(BSD) || \
     defined(SVR4) || defined(solaris2) || defined(m88k) || defined(M_UNIX) ||\
     defined(__sgi)
#  define POSIX_TERMIOS
# else
#  define SYSV_TERMIO
# endif
#endif

#ifdef SYSV_TERMIO

#undef POSIX_TERMIOS
#undef BSD_SGTTY
#include <termio.h>
typedef struct termio TIO;
#endif

#ifdef POSIX_TERMIOS
#undef BSD_SGTTY
#include <termios.h>
typedef struct termios TIO;
#endif

#ifdef BSD_SGTTY
#include <sgtty.h>
typedef struct sgttyb TIO;
#endif


/* queue selection flags (for tio_flush_queue) */
#define TIO_Q_IN	0x01		/* incoming data queue */
#define TIO_Q_OUT	0x02		/* outgoing data queue */
#define TIO_Q_BOTH	( TIO_Q_IN | TIO_Q_OUT )

/* RS232 line status flags */
/* system flags are used if available, otherwise we define our own */
#ifdef TIOCM_DTR
# define TIO_F_SYSTEM_DEFS
# define TIO_F_DTR TIOCM_DTR
# define TIO_F_DSR TIOCM_DSR
# define TIO_F_RTS TIOCM_RTS
# define TIO_F_CTS TIOCM_CTS
# define TIO_F_DCD TIOCM_CAR
# define TIO_F_RI  TIOCM_RNG
#else
# define TIO_F_DTR 0x001
# define TIO_F_DSR 0x002
# define TIO_F_RTS 0x004
# define TIO_F_CTS 0x008
# define TIO_F_DCD 0x010
# define TIO_F_RI  0x020
#endif


#ifdef CRTSCTS
# define HARDW_HS CRTSCTS
#else
# ifdef CRTSFL
#  define HARDW_HS CRTSFL
# else
#  ifdef RTSFLOW
#   define HARDW_HS RTSFLOW | CTSFLOW
#  else
#   define HARDW_HS 0
#  endif
# endif
#endif

/* hardware handshake flags
 */
#define FLOW_NONE	0x00
#define FLOW_HARD	0x01		/* rts/cts */
#define FLOW_XON_IN	0x02		/* incoming data, send xon/xoff */
#define FLOW_XON_OUT	0x04		/* send data, honor xon/xoff */
#define FLOW_SOFT	(FLOW_XON_IN | FLOW_XON_OUT)
#define FLOW_BOTH	(FLOW_HARD | FLOW_SOFT )
#define FLOW_XON_IXANY	0x08		/* set IXANY flag together with IXON */

#define DATA_FLOW	FLOW_HARD

#define TTY_SUCCESS	OK
#define TTY_TIMEOUT	TIMEOUT
#define TTY_HANGUP	RCDO
#define TTY_ERROR	ERROR

#define ME_OK		0
#define ME_ATTRS	1
#define ME_SPEED	2
#define ME_OPEN		3
#define ME_READ		4
#define ME_WRITE	5
#define ME_TIMEOUT	6
#define ME_CLOSE	7
#define ME_CANTLOCK	8
#define ME_FLAGS	9
#define ME_NOTATT	10
#define ME_NODSR	11

#define GETCHAR(t)		tty_getc( t )
#define GETCHART(t)		tty_getc_timed( t )

#define BUFBLOCK(b,n)		tty_bufblock((const void *) b, (size_t) n )
#define BUFCHAR(c)		tty_bufc( c )
#define BUFFLUSH(tsec)		tty_bufflush( tsec )

#define PUTCHAR(c)		tty_putc( c )
#define PUTSTR(s)		{ BUFBLOCK( s, strlen( (char*) s )); BUFFLUSH( 5 ); }
#define HASDATA(t)		tty_hasdata( t, 0 )
#define UHASDATA(t)		tty_hasdata( 0, t )
#define HASDATAT(t)		tty_hasdata_timed( t )
#define PURGE()			tty_purge()
#define PURGEOUT()		tty_purgeout()
#define PURGEALL()		{ tty_purge(); tty_purgeout(); }
#define CARRIER()		(!tty_gothup)
#define PUTBLK(bl, size)	tty_write( bl, size )
#define BUFCLEAR()		tty_bufclear()


#define EWBOEA()	(( errno == EWOULDBLOCK || errno == EAGAIN ))

#define LCK_NAME(name, port) \
	snprintf(name, MAX_PATH, "%s/LCK..%s", cfgs(CFG_LOCKDIR), port )

extern char *tty_errs[];
/* extern char canistr[]; */
extern int tty_fd;
extern TIO tty_stio;

RETSIGTYPE	tty_sighup(int sig);

int	tty_select(boolean *, boolean *, struct timeval *);
int	tty_write(const void *, size_t);
int	tty_read(void *, size_t);
int	tty_hasinbuf(void);
int	tty_hasdata(int, int);
int	tty_hasdata_timed(int *);

char	*baseport(const char *p);
int	tty_isfree(const char *, const char *);
char	*tty_findport(slist_t *, const char *);
int	tty_lock(const char *);
void	tty_unlock(const char *);
int	tty_close(void);
int	tty_local(TIO *, int);

void	tty_bufclear(void);
int	tty_bufflush(int);
int	tty_bufblock(const void *, size_t);
int	tty_bufc(char);
int	tty_putc(char);
int	tty_getc(int);
int	tty_getc_timed(int *);
void	tty_purge(void);
void	tty_purgeout(void);
int	tty_send_break(void);
int	tty_hasout(void);
int	tty_dcd(int);

int	fd_make_stddev(int);
int	fd_set_nonblock(int, int);

int	tio_get(int, TIO *);
int	tio_set(int, TIO *);
int	tio_set_speed(TIO *, unsigned int);
int	tio_get_speed(TIO *);
void	tio_local_mode(TIO *, int);
void	tio_raw_mode(TIO * );
void	tio_default_cc(TIO *);
int	tio_trans_speed(unsigned int);
int	tio_toggle_dtr(int, int);
int	tio_flush_queue(int, int);
int	tio_drain_output(int);
int	tio_set_flow_control(int, TIO *, int);
int	tio_get_rs232_lines(int);

#endif
