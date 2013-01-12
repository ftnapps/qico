/**********************************************************
 * work with log file
 **********************************************************/
/*
 * $Id: log.c,v 1.10 2005/08/23 16:50:43 mitry Exp $
 *
 * $Log: log.c,v $
 * Revision 1.10  2005/08/23 16:50:43  mitry
 * Handle NULL-able logfile
 *
 * Revision 1.9  2005/05/17 19:49:06  mitry
 * Fixed strftime() arg broken by previous patch
 *
 * Revision 1.8  2005/05/17 18:17:42  mitry
 * Removed system basename() usage.
 * Added qbasename() implementation.
 *
 * Revision 1.7  2005/05/16 11:17:30  mitry
 * Updated function prototypes. Changed code a bit.
 *
 * Revision 1.6  2005/05/06 20:53:06  mitry
 * Changed misc code
 *
 * Revision 1.5  2005/04/05 09:33:41  mitry
 * Update write_debug_log() prototype
 *
 * Revision 1.4  2005/03/31 19:40:38  mitry
 * Update function prototypes and it's duplication
 *
 */

#include "headers.h"
#define SYSLOG_NAMES
#include <syslog.h>
#ifdef HAVE_SYSLOG_AND_SYS_SYSLOG
#include <sys/syslog.h>
#endif

typedef struct _slncode {
	char *c_name;
	int c_val;
} SLNCODE;

#ifndef HAVE_SYSLOG_FAC_NAMES
static SLNCODE facilitynames[] =
{
	{ "auth",	LOG_AUTH },
        { "cron",	LOG_CRON },
        { "daemon",	LOG_DAEMON },
        { "kern",	LOG_KERN },
        { "lpr",	LOG_LPR },
        { "mail",	LOG_MAIL},
        { "news",	LOG_NEWS },
        { "syslog",	LOG_SYSLOG },
        { "user",	LOG_USER },
        { "uucp",	LOG_UUCP },
        { "local0",	LOG_LOCAL0 },
        { "local1",	LOG_LOCAL1 },
        { "local2",	LOG_LOCAL2 },
        { "local3",	LOG_LOCAL3 },
        { "local4",	LOG_LOCAL4 },
        { "local5",	LOG_LOCAL5 },
        { "local6",	LOG_LOCAL6 },
        { "local7",	LOG_LOCAL7 },
        { NULL,		-1 }
};
#endif

#ifndef HAVE_SYSLOG_PRI_NAMES
static SLNCODE prioritynames[] =
{
	{ "alert",	LOG_ALERT },
	{ "crit",	LOG_CRIT },
	{ "debug",	LOG_DEBUG },
	{ "emerg",	LOG_EMERG },
	{ "err",	LOG_ERR },
	{ "error",	LOG_ERR },	/* DEPRECATED */
	{ "info",	LOG_INFO },
	{ "notice",	LOG_NOTICE },
	{ "panic",	LOG_EMERG },	/* DEPRECATED */
	{ "warn",	LOG_WARNING },	/* DEPRECATED */
	{ "warning",	LOG_WARNING },
	{ NULL,		-1 }
};
#endif

#define CHATLOG_BUF	LARGE_STRING

#define LT_STDERR	0
#define LT_LOGFILE	1
#define LT_SYSLOG	2


static int	log_type = LT_STDERR, mcpos, rcpos;
static int	syslog_priority = LOG_INFO;
static ftnaddr_t *adr;
static char	pktname[MAX_PATH + 5] = {0};
static FILE	*cpkt = NULL;
static FILE	*lemail = NULL;
static char	mchat[CHATLOG_BUF + 5] = {0};
static char	rchat[CHATLOG_BUF + 5] = {0};

void (*log_callback)(const char *) = NULL;

#ifdef NEED_DEBUG
int facilities_levels[256];
#endif


static int parsefacorprio(char *f, SLNCODE *names)
{
	int i = 0;

	while( names[i].c_name ) {
		if ( !strcasecmp( f, names[i].c_name ))
			return names[i].c_val;
		i++;
	}
	return -1;
}


int log_init(const char *logname, const char *ttyname)
{
	FILE	*log_f;
	char	*n, fac[30], *prio;
	int	fc;
	size_t	len;

	if ( logname == NULL )
		return 0;

	log_tty = ttyname ? xstrdup( ttyname ) : NULL;

	if ( *logname != '$' ) {
		log_f = fopen( logname, "at" );
		if ( log_f ) {
			fclose( log_f );
			log_type = LT_LOGFILE;
			log_name = xstrdup( logname );
			return 1;
		}
		return 0;
	}

	if ( ttyname ) {
		len = strlen( progname ) + 2 + strlen( ttyname );
		n = malloc( len );
		if ( !n ) {
			fprintf( stderr, "can't malloc() %d bytes of memory\n", len );
			abort();
			exit(1);
		}
		xstrcpy( n, progname, len );
		xstrcat( n, ".", len );
		xstrcat( n, ttyname, len );
	} else
		n = xstrdup( progname );

	prio = strchr( logname + 1, ':' );
	if ( prio ) {
		prio++;
		xstrcpy( fac, logname + 1, 30 );
		if (( syslog_priority = parsefacorprio( prio, (SLNCODE *) prioritynames )) < 0 )
			syslog_priority = LOG_INFO;
	} else
		xstrcpy( fac, logname + 1, 30);

	if (( fc = parsefacorprio( fac, (SLNCODE *) facilitynames )) < 0 )
		return 0;

	log_type = LT_SYSLOG;
	log_name = NULL;
	openlog( n, LOG_PID, fc );
	xfree( n );
	return 1;
}


void vwrite_log(const char *str, int dbg)
{
	char		timedstr[LARGE_STRING + 5] = {0};
	char		newstr[LARGE_STRING + 5];
	FILE		*log_f;
	struct tm	t;
	struct timeval	tv;

	xstrcpy( newstr, str, LARGE_STRING );
	IFPerl( perl_on_log( newstr ));
    
	gettimeofday( &tv, NULL );
	memcpy( &t, localtime( &tv.tv_sec ), sizeof(struct tm));

#ifdef NEED_DEBUG
	if ( facilities_levels['G'] >= 1 )
		snprintf( timedstr, LARGE_STRING, "%02d %3s %02d %02d:%02d:%02d.%03u %s[%ld]: %s",
			t.tm_mday, engms[t.tm_mon], t.tm_year%100, t.tm_hour,
			t.tm_min, t.tm_sec, (unsigned) ( tv.tv_usec / 1000 ),
			SS( log_tty ), (long) getpid(),
			newstr );
	else
#endif
	snprintf( timedstr, LARGE_STRING, "%02d %3s %02d %02d:%02d:%02d %s[%ld]: %s",
		t.tm_mday, engms[t.tm_mon], t.tm_year%100, t.tm_hour,
		t.tm_min, t.tm_sec, SS( log_tty ), (long) getpid(),
		newstr );

	if ( log_callback && dbg )
		log_callback( timedstr );

	switch( log_type ) {
	case LT_STDERR:
		fprintf( stderr, "%s: %s\n", progname, timedstr );
		break;

	case LT_LOGFILE:
		if ( log_name ) {
			if (( log_f = fopen( log_name, "a" )) != NULL ) {
				fprintf( log_f, "%s\n", timedstr );
				fclose( log_f );
			}
		}
		break;

	case LT_SYSLOG:
		syslog( syslog_priority, newstr );
		break;
	}
}


void write_log(const char *fmt, ...)
{
	static char	str[LARGE_STRING + 5];
	va_list		args;

	str[0] = '\0';

	va_start( args, fmt );
	vsnprintf( str, LARGE_STRING, fmt, args );
	va_end( args );

	vwrite_log( str, 1 );
}


#ifdef NEED_DEBUG
void parse_log_levels(void)
{
	char	*w, *levels = xstrdup( cfgs( CFG_LOGLEVELS )), *lvrun;
	int	c;

	memset( facilities_levels, 0, sizeof( facilities_levels ));
	lvrun = levels;
	while(( w = strsep( &lvrun, ",;" ))) {
		c = *w++;
		if ( *w )
			facilities_levels[c] = atoi( w );
	}
	xfree( levels );
}


void write_debug_log(unsigned char facility, int level, const char *fmt, ...)
{
	static char	str[1024];
	va_list		args;

	str[0] = '\0';

#ifndef __GNUC__
	if ( facilities_levels[facility] < level )
		return;
#endif
	snprintf( str, 16, "DBG_%c%02d: ", facility, level );

	/* make string in buffer */
	va_start(args, (const char *) fmt);
	vsnprintf( str + strlen( str ), sizeof( str ) - strlen( str ), fmt, args );
	va_end( args );

	vwrite_log( str, facilities_levels['G'] >= level );
}
#endif


void log_done(void)
{
	if ( log_type == LT_SYSLOG )
		closelog();
	xfree( log_name );
	xfree( log_tty );
	log_type = LT_STDERR;
}


int chatlog_init(const char *remname, const ftnaddr_t *ra, int side)
{
	char		str[MAX_STRING + 5] = {0};
	FILE		*chatlog = NULL;
	time_t		tt;
	struct tm	t;

	write_log( "Chat opened%s", side ? " by remote side" : "" );

	adr = &cfgal( CFG_ADDRESS )->addr;
	if ( cfgs( CFG_RUNONCHAT ) && side )
		execnowait( "/bin/sh", "-c", ccs, ftnaddrtoa( adr ));

	if ( cfgi( CFG_CHATLOGNETMAIL )) {
		snprintf( pktname, MAX_PATH - 1, "%s/tmp/%08lx.pkt",
			cfgs( CFG_INBOUND ), sequencer());
		cpkt = openpktmsg( adr, adr, "qico chat-log poster",
			xstrdup( cfgs( CFG_SYSOP )), "chat log", NULL, pktname, 137 );
		if ( !cpkt )
			write_log( "can't open '%s' for writing", pktname );
	}

	if ( cfgs( CFG_CHATTOEMAIL )) {
		snprintf( str, MAX_PATH, "/tmp/qlemail.%04lx", (long) getpid());
		lemail = fopen( str, "wt" );
		if ( !lemail )
			write_log( "can't create temporary email-log file" );
	}

	if ( cfgs( CFG_CHATLOG ))
		chatlog = fopen( ccs, "at" );

	if ( ccs && !chatlog )
		write_log( "can't open chat log %s", ccs );

	tt = time( NULL );
	memcpy( &t, localtime( &tt ), sizeof(struct tm));

	snprintf( str, MAX_STRING, "[Chat with: %s (%u:%u/%u.%u) opened by %s at ",
		remname, ra->z, ra->n, ra->f, ra->p, side ? "remote" : "me" );
	strftime( str + strlen( str ), MAX_STRING - strlen( str ),
		"%d %b %Y %H:%M:%S]\n", &t );

	if ( chatlog ) {
		fwrite( str, strlen( str ), 1, chatlog );
		fclose( chatlog );
	}
	
	if ( lemail )
		fwrite( str, strlen( str ), 1, lemail );

	if ( cpkt ) {
		if ( cfgi( CFG_RECODEPKTS ))
			recode_to_remote( str );
		strtr( str, '\n', '\r' );
		fwrite( str, strlen( str ), 1, cpkt );
		runtoss = 1;
	}

	mcpos = rcpos = 0;
	*rchat = '\0';
	*mchat = '\0';

	return ( cpkt != NULL || chatlog != NULL || lemail != NULL );
}


void chatlog_write(const char *text, int side)
{
	char	quot[2] = {0}, *tmp, *cbuf = side ? rchat : mchat, tm;
	FILE	*chatlog = NULL;

	if ( side )
		*quot = '>';
	else
		*quot = ' ';

	if ( text && *text ) {
		xstrcpy( cbuf + ( side ? rcpos : mcpos ), text,
			CHATLOG_BUF - ( side ? rcpos : mcpos ) - 1 );
		if ( side )
			rcpos += strlen( text );
		else
			mcpos += strlen( text );

		while(( side ? rcpos : mcpos ) && ( tmp = strchr( cbuf, '\n' ))) {
			long i = 0, n = 0, m = 0;

			while( i < ( tmp - cbuf + 1 ))
				if ( cbuf[i] == '\b' ) {
					i++;
					m += 2;
					if ( --n < 0 ) {
						n = 0;
						m--;
					}
				} else
					cbuf[n++] = cbuf[i++];

			if ( n && cfgs( CFG_CHATLOG )) {
				chatlog = fopen(ccs,"at");
				fwrite( quot, 1, 1, chatlog );
				fwrite( cbuf, 1, n, chatlog );
				fclose( chatlog );
			}

			if( n && lemail ) {
				fwrite( quot, 1, 1, lemail );
				fwrite( cbuf, 1, n, lemail );
			}

			if ( n && cpkt ) {
				for( i = 0; i < n; i++ ) {
					if ( cbuf[i] == '\n' )
						cbuf[i] = '\r';
					if ( cbuf[i] == 7 )
						cbuf[i] = '*';
				}

				tm = cbuf[i];
				cbuf[i] = 0;
				if ( cfgi( CFG_RECODEPKTS ))
					recode_to_remote( cbuf );
				cbuf[i] = tm;
				fwrite( quot, 1, 1, cpkt );
				fwrite( cbuf, 1, n, cpkt );
			}
			if ( cbuf[n] )
				xstrcpy( cbuf, cbuf + n, CHATLOG_BUF - 6 );
			if ( side )
				rcpos -= n + m;
			else
				mcpos -= n + m;
		}
	}
}


void chatlog_done(void)
{
	char		str[MAX_STRING + 5] = {0};
	FILE		*chatlog = NULL;
	time_t		tt;
	struct tm	*t;

	if ( rcpos )
		chatlog_write( "\n", 1 );
	if ( mcpos )
		chatlog_write( "\n", 0 );

	if ( cfgs( CFG_CHATLOG ))
		chatlog = fopen( ccs, "at" );

	tt = time( NULL );
	t = localtime( &tt );

	strftime( str, MAX_STRING - 1, "\n[--- Chat closed at %d %b %Y %H:%M:%S ---]\n\n", t );
	if ( chatlog ) {
		fwrite( str, strlen( str ), 1, chatlog );
		fclose( chatlog );
	}

	if ( lemail )
		fwrite( str, strlen( str ), 1, lemail );
	if ( cpkt ) {
		if ( cfgi( CFG_RECODEPKTS ))
			recode_to_remote( str );
		strtr( str, '\n', '\r' );
		fwrite( str, strlen( str ) - 1, 1, cpkt );
		closeqpkt( cpkt, adr );
		snprintf( str, MAX_STRING - 1, "%s/%s", cfgs( CFG_INBOUND ), qbasename( pktname ));
		if ( rename( pktname, str ))
			write_log( "can't rename %s to %s: %s", pktname, str, strerror( errno ));
		else
			chmod( str, cfgi( CFG_DEFPERM ));
	}

	if ( lemail ) {
		fclose( lemail );
		snprintf( str, MAX_STRING-1, "mail -s chatlog %s < /tmp/qlemail.%04lx",
			cfgs( CFG_CHATTOEMAIL ), (long) getpid());
		execsh( str );
		lunlink( strrchr( str, '<' ) + 1 );
	}

	write_log( "Chat closed" );
}
