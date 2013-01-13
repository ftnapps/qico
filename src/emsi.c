/**********************************************************
 * EMSI management.
 **********************************************************/
/*
 * $Id: emsi.c,v 1.24 2005/08/12 15:36:19 mitry Exp $
 *
 * $Log: emsi.c,v $
 * Revision 1.24  2005/08/12 15:36:19  mitry
 * Changed gmtoff()
 *
 * Revision 1.23  2005/06/13 17:51:24  mitry
 * Speed up EMSI send with Argus clones.
 * Added emsi_log()
 *
 * Revision 1.22  2005/06/10 20:54:26  mitry
 * Added check for unexpected EMSI_DAT packet in emsi_send().
 * Stop showing 'intro:' after receiving second EMSI_REQ in emsi_init().
 *
 * Revision 1.21  2005/06/03 15:45:11  mitry
 * Added check for expired timer in emsi_send()
 *
 * Revision 1.20  2005/06/03 14:12:26  mitry
 * Misc changes in EMSI stages
 *
 * Revision 1.19  2005/05/16 20:28:48  mitry
 * Changed EMSI stages
 *
 * Revision 1.18  2005/05/06 20:30:30  mitry
 * Have to send '\r' at least once in emsi_init
 *
 * Revision 1.17  2005/04/08 18:05:31  mitry
 * Added support for HydraRH1 option
 *
 * Revision 1.16  2005/04/05 09:31:12  mitry
 * New xstrcpy() and xstrcat()
 *
 * Revision 1.15  2005/04/04 19:43:55  mitry
 * Added timeout arg to BUFFLUSH() - tty_bufflush()
 *
 * Revision 1.14  2005/03/28 17:02:52  mitry
 * Pre non-blocking i/o update. Mostly non working.
 *
 * Revision 1.13  2005/02/25 16:53:49  mitry
 * Fixed stupid syntax error
 *
 * Revision 1.12  2005/02/25 16:42:59  mitry
 * Modified to support multi-line intro lines
 *
 * Revision 1.11  2005/02/23 21:31:14  mitry
 * Rewrite emsi_send()
 * Modified emsi_init() and emsi_recv()
 * Changed debug messages
 *
 * Revision 1.10  2005/02/21 21:00:09  mitry
 * Fix logic for preventive EMSI_INQ
 *
 * Revision 1.9  2005/02/12 18:48:18  mitry
 * Added some code in emsi_init() to speed up ifcico connect with fckng T-Mail.
 *
 */

#include "headers.h"
#include "qipc.h"
#include "crc.h"
#include "tty.h"

#define EMSI_BUF	8192
#define TMP_LEN		1024
#define EMSI_CAT(p)	xstrcat( emsi_dat, (p), EMSI_BUF )
#define EMSI_BEG	"**EMSI_"
#define EMSI_ARGUS1	"-PZT8AF6-"
#define EMSI_SEQ_LEN	14
#define EMSI_LOG_IN	0
#define EMSI_LOG_OUT	1

#define PUTSTRCR( str )	BUFBLOCK( (void *) str, strlen( (char *) str )); PUTCHAR( '\r' )

static char emsi_dat[EMSI_BUF + 1];


static char *emsireq = "**EMSI_REQA77E";
static char *emsiack = "**EMSI_ACKA490";
static char *emsiinq = "**EMSI_INQC816";
static char *emsinak = "**EMSI_NAKEEC3";
static char *emsihbt = "**EMSI_HBTEAEE";
static char *emsidat = "**EMSI_DAT";

static int chat_need = -1, chatmy = -1;

static char *ccodes[2] = {
	"ZMO,ZAP,DZA,"
#ifdef HYDRA8K16K
	"HY4,HY8,H16,"
#endif
	"HYD,JAN,NCP,NRQ,FNC,EII,XMA,ARC,",
	"RH1,PUA,PUP,NPU,HAT,HXT,HRQ,8N1,"
};

static int iccodes[2][14] = {
	{ P_ZMODEM, P_ZEDZAP, P_DIRZAP,
#ifdef HYDRA8K16K
	P_HYDRA4, P_HYDRA8, P_HYDRA16,
#endif
	P_HYDRA, P_JANUS, P_NCP, O_NRQ, O_FNC, O_EII, O_XMA, P_NONE },
	{ O_RH1, O_PUA, O_PUP, O_NPU, O_HAT, O_HXT, O_HRQ, P_NONE }
};

static struct pcod_t {
	char *code;
	char *err;
} pcod[2];



static int emsi_parsecod(char *lcod, char *ccod)
{
	char		*q, *p, *c = NULL;
	register int	o = 0;
	register int	i, j;

	DEBUG(('E',3,"link codes: '%s', compatibility codes: '%s'", lcod, ccod));
    
	chat_need = 0;

	pcod[0].code = ccod;
	pcod[1].code = lcod;
	pcod[0].err = "proto";
	pcod[1].err = "traff";
    
	for( j = 0; j < 2; j++ ) {
		q = pcod[j].code;
		while(( p = strsep( &q, "," ))) {
			if ( ! *p )
				break;

			DEBUG(('E', 4, "emsi %s flag: '%s'", pcod[j].err, p));

			if (( c = strstr( ccodes[ j ], p ))) {
				i = ( c - ccodes[j] ) >> 2;
				o |= iccodes[j][i];
			} else if ( !j && !strcmp( p, "CHT" )) { 
				chat_need = 1;
				continue;
			} else
				DEBUG(('E',3, "unknown emsi %s flag: '%s'", pcod[j].err, p));
		}
	}

	DEBUG(('E',5,"emsi_parsecod returns: 0x%x", o));
	return o;
}


static char *emsi_tok(char **b, char *kc)
{
	char *p = *b, *t;

	if ( *p != kc[0] )
		return NULL;

	t = ++p;

	while( *t && *t != kc[1] ) {
		if ( t[1] && t[1] == kc[1] && t[2] && t[2] == kc[1] )
			t += 2;
		t++;
	}
    
	if ( !*t )
		return NULL;

	if ( *t == kc[1] ) {
		*t++ = 0;
		*b = t;
		return p;
	}

	return NULL;
}


static void emsi_dcds(char *s)
{
	char *d = s, *x = s, t;

	while(( t = *s )) {
		if ( t == '}' || t == ']' )
			t = *++s;
		if( t != '\\' )
			*d = C0( t );
		else {
			*d = hexdcd( s[1], s[2] );
			s += 2;
		}
		s++;
		d++;
	}

	*d = 0;
	recode_to_local( x );
}


static void emsi_log(int inout, const char *dat)
{
	char *p;

	if ( cfgs( CFG_LOGLEVELS ) && ( p = strchr( ccs, 'E' )) && cfgs( CFG_EMSILOG )) {
		FILE *elog;
		elog = fopen( ccs, "at" );
		if ( elog ) {
			fprintf( elog, "%s: %s\n", ( inout == EMSI_LOG_IN ? " in" : "out" ), dat );
			fclose( elog );
		}
	}
}


#define EMSI_TOK( from, to )	\
	to = emsi_tok( from, "{}" );	\
	if ( !to )			\
		return 0;		\
	emsi_dcds( to );

#define EMSI_TOKN( from, to )	\
	to = emsi_tok( from, "{}" );	\
	if ( !to )			\
		return 0;

/* for IDENT */
#define EMSI_TOKI( from, to )	\
	to = emsi_tok( from, "[]" );	\
	emsi_dcds( to );

static int emsi_parsedat(char *str, ninfo_t *dat)
{
	char		*p, *t, *s, *lcod, *ccod;
	size_t		l, l1;
	FTNADDR_T	(fa);

	emsi_log( EMSI_LOG_IN, str );
    
	if ( !( str = strstr( str, emsidat ))) {
		write_log( "No **EMSI_DAT signature found!" );
		return 0;
	}

	sscanf( str + 10, "%04X", (unsigned *) &l );
	if ( l != ( l1 = strlen( str ) - 18 )) {
		write_log( "Bad EMSI_DAT length: %u, should be: %u!", l, l1 );
		return 0; /* Bad EMSI length */
	}
	
	DEBUG(('E',5,"EMSI_DAT length (%d) is OK!", l ));

	sscanf( str + strlen( str ) - 4, "%04X", &l);
	if ( l != ( l1 = crc16usd( (UINT8 *) str + 2, strlen( str ) - 6 ))) {
		write_log( "Bad EMSI_DAT CRC: %04X, should be: %04X!", l, l1 );
		return 0; /* Bad EMSI CRC */
	}

	DEBUG(('E',5,"EMSI_DAT CRC (%04X) is OK!", l ));

	if ( strncmp( str + 14, "{EMSI}", 6)) {
		write_log( "No EMSI fingerprint!" );
		return 0; /* No EMSI ident */
	}

	t = str + 20;
	*str = 1;

	EMSI_TOK( &t, p ) /* {AKAs} */

	falist_kill( &dat->addrs );

	while(( s = strsep( &p, " " )))
		if ( parseftnaddr( s, &fa, NULL, 0 ) && !falist_find( dat->addrs, &fa ))
			falist_add( &dat->addrs, &fa );

	EMSI_TOK( &t, p ) /* Password */
	if ( *p )
		restrcpy( &dat->pwd, p );
	else
		xfree( dat->pwd );

	EMSI_TOK( &t, p ) /* Link codes */
	lcod = p;
    
	EMSI_TOK( &t, p ) /* Compatibility codes */
	ccod = p;
    
	EMSI_TOK( &t, p ) /* Mailer code */
    
	EMSI_TOK( &t, p ) /* Mailer name */
	restrcpy( &dat->mailer, p );

	EMSI_TOK( &t, p ) /* Mailer version */
	if ( *p ) {
		restrcat( &dat->mailer, " " );
		restrcat( &dat->mailer, p );
	}

	EMSI_TOK( &t, p ) /* Mailer serial number */
	if ( *p ) {
		restrcat( &dat->mailer, "/" );
		restrcat( &dat->mailer, p );
	}
	DEBUG(('E',4,"remote mailer '%s'", dat->mailer ));

	DEBUG(('E',1,"emsi codes %s/%s", lcod, ccod));
	dat->options |= emsi_parsecod( lcod, ccod );

	if ( chat_need == 1 && chatmy != 0 )
		dat->opt |= MO_CHAT;

	xfree( dat->wtime );

	DEBUG(('E',5,"t: '%s'", t));
	while(( p = emsi_tok( &t, "{}" ))) {
		DEBUG(('E',5,"Parsing p: '%s', t: '%s'", p, t));
		if ( !strcmp( p, "IDENT" )) {
			EMSI_TOKN( &t, p )
			DEBUG(('E',5,"IDENT  p: '%s', t: '%s'", p, t));

			/* System name */
			EMSI_TOKI( &p, s )
			DEBUG(('E',5,"System name  s: '%s', p: '%s'", s, p));
			restrcpy( &dat->name, s );

			/* System location */
			EMSI_TOKI( &p, s )
			DEBUG(('E',5,"System location  s: '%s', p: '%s'", s, p));
			restrcpy( &dat->place, s );

			/* Operator name */
			EMSI_TOKI( &p, s )
			DEBUG(('E',5,"Operator name  s: '%s', p: '%s'", s, p));
			restrcpy( &dat->sysop, s );

			/* Phone */
			EMSI_TOKI( &p, s )
			DEBUG(('E',5,"Phone  s: '%s', p: '%s'", s, p));
			restrcpy( &dat->phone, s );

			/* Baud rate */
			EMSI_TOKI( &p, s )
			DEBUG(('E',5,"Baud rate  s: '%s', p: '%s'", s, p));
			if ( s )
				sscanf( s, "%d", &dat->speed );

			/* Flags */
			EMSI_TOKI( &p, s )
			DEBUG(('E',5,"Flags  s: '%s', p: '%s'", s, p));
			restrcpy( &dat->flags, s );

		} else if( !strcmp( p, "TRAF" )) {
			EMSI_TOK( &t, p )
			DEBUG(('E',5,"TRAF  p: '%s', t: '%s'", p, t));
			sscanf( p, "%x %x", (unsigned*)&dat->netmail, (unsigned*)&dat->files );

		} else if ( !strcmp( p, "OHFR" )) {
			EMSI_TOK( &t, p )
			DEBUG(('E',5,"OHFR  p: '%s', t: '%s'", p, t));
			restrcpy( &dat->wtime, p );

		} else if( !strcmp( p, "MOH#" )) {
			EMSI_TOKN( &t, p )
			DEBUG(('E',5,"MOH#  p: '%s', t: '%s'", p, t));
			EMSI_TOKI( &p, s )
			DEBUG(('E',5,"MOH#  s: '%s', p: '%s'", s, p));
			if ( sscanf( s, "%x", (unsigned*)&dat->holded ) != 1 )
				dat->holded = 0;

		} else if ( !strcmp( p, "TRX#" )) {
			EMSI_TOKN( &t, p )
			DEBUG(('E',5,"TRX#  p: '%s', t: '%s'", p, t));
			EMSI_TOKI( &p, s )
			DEBUG(('E',5,"TRX#  s: '%s', p: '%s'", s, p));
			if ( sscanf( s, "%lx", (unsigned long*)&dat->time) !=1 )
				dat->time = 0;

		} else p = emsi_tok( &t, "{}" );
	}
	return 1;
}


void emsi_makedat(ftnaddr_t *remaddr, unsigned long mail, unsigned long files,
    int lopt, char *protos, falist_t *adrs, int showpwd)
{
	char		tmp[TMP_LEN + 5], *p, *xstr;
	int		c;
	falist_t	*cs;
	ftnaddr_t	*ba;
	time_t		tm = time( NULL );

	emsi_dat[0] = '\0';

	EMSI_CAT( "**EMSI_DAT0000{EMSI}{" );

	/* Put best AKA first */
	ba = akamatch( remaddr, adrs ? adrs : cfgal( CFG_ADDRESS ));
	EMSI_CAT( ba->d ? ftnaddrtoda( ba ) : ftnaddrtoa( ba ));
    
	/* Fill rest AKAs */
	for( cs = cfgal( CFG_ADDRESS ); cs; cs = cs->next )
		if ( &cs->addr != ba ) {
			EMSI_CAT( " " );
			EMSI_CAT( cs->addr.d ? ftnaddrtoda( &cs->addr ) : ftnaddrtoa( &cs->addr ));
		}

	EMSI_CAT( "}{" );

	if ( showpwd && ( p = findpwd( remaddr )))
		EMSI_CAT( p );

	EMSI_CAT( "}{8N1" );
    
	if ( cfgi( CFG_HYDRARH1 ) && ( strchr( protos, 'H' ) || strchr( protos, 'h' )

#ifdef HYDRA8K16K

		|| strchr( protos, '4' ) || strchr( protos, '8' ) || strchr( protos, '6' )

#endif/*HYDRA8K16K*/

	)) {
		EMSI_CAT( ",RH1" );
		lopt |= O_RH1;
	} else
		lopt &= (~O_RH1);

	emsi_lo = lopt;

	if ( lopt & O_PUA )
		EMSI_CAT( ",PUA" );

	if ( lopt & O_PUP )
		EMSI_CAT( ",PUP" );

	/* Should be presented only by answering system. Calling system should
	   set NRQ
	*/
	if ( lopt & O_HRQ )
		EMSI_CAT( ",HRQ" );

	if ( lopt & O_HXT )
		EMSI_CAT( ",HXT" );

	if ( lopt & O_HAT )
		EMSI_CAT( ",HAT" );

	EMSI_CAT( "}{" );

	p = protos;
	c = chatmy = 0;

	if ( !( lopt & P_NCP )) {
		while( *p ) {
			switch(toupper(*p++)) {
			case '1':
				EMSI_CAT( "ZMO," );
				c = 1;
				break;

			case 'Z':
				EMSI_CAT( "ZAP," );
				c = 1;
				break;

			case 'D':
				EMSI_CAT( "DZA," );
				c = 1;
				break;

			case 'H':
				EMSI_CAT( "HYD," );
				c = 1;
				break;

#ifdef HYDRA8K16K
			case '4':
				EMSI_CAT( "HY4," );
				c = 1;
				break;

			case '8':
				EMSI_CAT( "HY8," );
				c = 1;
				break;

			case '6':
				EMSI_CAT( "H16," );
				c = 1;
				break;

#endif /*HYDRA8K16K*/

			case 'J':
				EMSI_CAT( "JAN," );
				c = 1;
				break;

			case 'C':
				EMSI_CAT( "CHT," );
				chatmy = 1;
				break;
			}
		}

		if ( lopt & O_NRQ )
			EMSI_CAT( "NRQ," );
	}

	if ( !c )
		EMSI_CAT( "NCP," );

	EMSI_CAT( "ARC,XMA}{FE" );

	for( c = 0; c < 3; c++ ) {
		xstr = strip8( qver( c ));
		EMSI_CAT( "}{" );
		EMSI_CAT( xstr );
		xfree( xstr );
	}
	EMSI_CAT( "}{IDENT}{" );

#define STRIP8_CAT(_x_)	\
	EMSI_CAT( "[" );			\
	EMSI_CAT( p = strip8( cfgs( _x_ )) );	\
	EMSI_CAT( "]" );			\
	xfree( p );

	STRIP8_CAT( CFG_STATION );
	STRIP8_CAT( CFG_PLACE );
	STRIP8_CAT( CFG_SYSOP );
	STRIP8_CAT( CFG_PHONE );
	snprintf( tmp, TMP_LEN, "[%d]", cfgi( CFG_SPEED ));
	EMSI_CAT( tmp );
	STRIP8_CAT( CFG_FLAGS );
	EMSI_CAT( "}" );

#undef STRIP8_CAT

	c = gmtoff( tm );
	snprintf( tmp, TMP_LEN,
		"{TRAF}{%lX %lX}{MOH#}{[%lX]}{TRX#}{[%lX]}{TZUTC}{[%+03d%02d]}",
		mail, files, mail + files,
		tm + c, c / 3600, abs( c % 3600 ));
	EMSI_CAT( tmp );

	EMSI_CAT( "{OHFR}{" );
	EMSI_CAT((( p = strip8( cfgs( CFG_WORKTIME ))) ? p : MSG_NEVER ));
	xfree( p );
	EMSI_CAT( " " );
	EMSI_CAT((( p = strip8( cfgs( CFG_EMSIFREQTIME ))) ? p :
		( p = strip8( cfgs( CFG_FREQTIME ))) ? p : MSG_NEVER ));
	xfree( p );
	EMSI_CAT( "}" );

	/* Calculate emsi length */
	snprintf( tmp, TMP_LEN, "%04X", strlen( emsi_dat ) - 14 );
	memcpy( emsi_dat + 10, tmp, 4 );

	/* EMSI crc16 */
	snprintf( tmp, TMP_LEN, "%04X", crc16usds( (UINT8 *) emsi_dat + 2 ));
	EMSI_CAT( tmp );

	emsi_log( EMSI_LOG_OUT, emsi_dat );
}


static void emsi_banner(void)
{
	char		banner[MAX_STRING + 5];
	ftnaddr_t	*addr = &cfgal( CFG_ADDRESS )->addr;

	snprintf( banner, MAX_STRING, "+ Address %s using %s v%s [%s]",
		addr->d ? ftnaddrtoda( addr ) : ftnaddrtoa( addr ),
		progname, version, cvsdate );

	sline( "Sending EMSI_REQ..." );
	DEBUG(('E',1,"EI: Sending EMSI_REQ"));
	BUFFLUSH( 5 );
	PUTSTRCR( emsireq );
	PUTSTRCR( banner );
	sline( "Waiting for EMSI_INQ..." );
	DEBUG(('E',1,"EI: Waiting for EMSI_INQ"));
}


/*
 * Is use `goto' a poor programmer's style? But who cares? :)
 */

#define EMSI_RESEND_TO	5

/*
 *   STEP 1, EMSI INIT
 */
int emsi_init(int originator)
{
	int	ch = TIMEOUT, got = 0;
	int	tries = 0;
	char	*p = emsi_dat;
	time_t	t1, t2;

	if ( originator ) {
		int	gotreq = 0;
		int	showintro = cfgi( CFG_SHOWINTRO );
		int	do_prevent = !cfgi( CFG_STANDARDEMSI );

		t1 = timer_set( cfgi( CFG_HSTIMEOUT ));

		/* if ( !HASDATA( 1 ) ) */
		do { 
			PUTCHAR('\r');
			qsleep( 500 );
		} while ( !HASDATA( 1 ) && !timer_expired( t1 ));

		if ( timer_expired( t1 ))
			return TIMEOUT;

		/* t1 = timer_set( cci ); */
		t2 = timer_set( EMSI_RESEND_TO );

		while( 1 ) {
			ch = GETCHAR( MAX( 1, MIN( timer_rest( t1 ), timer_rest( t2 ))));
			DEBUG(('E',9,"EI: getchar '%c' %d (%d, %d)",
				C0( ch ), ch, timer_rest( t1 ), timer_rest( t2 )));

			if ( NOTTO( ch ) && ( ch != TIMEOUT ))
				return ch;

			if( timer_expired( t1 ))
				return TIMEOUT;

			if ( timer_expired( t2 ) && CARRIER()) {
				if ( do_prevent && tries == 0 ) {
					do_prevent = 0;
					sline( "Sending preventive EMSI_INQ..." );
					DEBUG(('E',1,"EI: Sending preventive EMSI_INQ..."));
					PUTSTRCR( emsiinq );
				} else {
					if ( ++tries > 10 )
						return TIMEOUT;
					sline( "Sending EMSI_INQ (Try %d of %d)...", tries, 10 );
					DEBUG(('E',1,"EI: Sending EMSI_INQ (Try %d of %d)...",tries,10));
					PUTSTR( emsiinq );
					PUTSTRCR( emsiinq );
				}
				t2 = timer_set( EMSI_RESEND_TO );
				continue;
			}

			if ( ch == TIMEOUT )
				continue;

			ch &= 0x7f;

			if ( ch == '\r' || ch == '\n' ) {
				*p = '\0';
				p = emsi_dat;
                
				if ( strstr( emsi_dat, emsireq )) {
					DEBUG(('E',1,"EI: Got EMSI_REQ (%d)",gotreq));
					if ( gotreq++ )
						return OK;

					sline( "Received EMSI_REQ, sending EMSI_INQ..." );
					DEBUG(('E',1,"EI: Sending EMSI_INQ.."));
					PUTSTRCR( emsiinq );
				} else {
					emsi_dat[79] = '\0';
					if ( showintro && *emsi_dat
						&& !strstr( emsi_dat, EMSI_BEG )
						&& !strstr( emsi_dat, EMSI_ARGUS1 ))
						write_log( "intro: %s", emsi_dat );
				}
				memset( emsi_dat, 0, EMSI_BUF );
				continue;
			}

			if ( ch > 31 )
				*p++ = ch;

			if (( p - emsi_dat ) >= EMSI_BUF ) {
				p = emsi_dat;
				memset( emsi_dat, 0, EMSI_BUF );
			}
		}

		if ( !CARRIER())
			return RCDO;

		return OK;
	}

        PURGEALL();
        emsi_banner();

	t1 = timer_set( cfgi( CFG_HSTIMEOUT ));
	t2 = timer_set( EMSI_RESEND_TO );

	while( !timer_expired( t1 )) {
		ch = GETCHAR( MAX( 1, MIN( timer_rest( t1 ), timer_rest( t2 ))));
		DEBUG(('E',9,"EI: getchar '%c' %d (%d, %d)",
			C0( ch ), ch, timer_rest( t1 ), timer_rest( t2 )));

		if ( NOTTO( ch ) && ( ch != TIMEOUT))
			return ch;

		if ( timer_expired( t1 ))
			return TIMEOUT;

		if (( ch == TIMEOUT ) && timer_expired( t2 )) {
			if ( !got ) {
				emsi_banner();
				t2 = timer_set( EMSI_RESEND_TO );
			} else {
				t2 = t1;
			}
			continue;
		}

		ch &= 0x7f;

		if ( !got && ch == '*' )
			got = 1;

		if ( got && ( ch == '\r' || ch == '\n' )) {
			*p = '\0';
			got = 0;
            
                        if ( p == emsi_dat )
                        	continue;

			DEBUG(('E',2,"EI: Got '%s'",emsi_dat));
			p = emsi_dat;
			if ( strstr( emsi_dat, emsiinq )) {
				sline( "Received EMSI_INQ" );
				DEBUG(('E',1,"EI: Got EMSI_INQ"));
				return OK;
			}
		} else {
			if ( got && ch > 31 )
				*p++ = ch;

			if (( p - emsi_dat ) >= EMSI_BUF ) {
				got = 0;
				p = emsi_dat;
			}
		}
	}

	return TIMEOUT;
}


/*
 *   STEP 2A, RECEIVE EMSI HANDSHAKE
 */
int emsi_recv(int mode, ninfo_t *rememsi)
{
	int	tries, got = 0, ch;
	time_t	t1, t2;
	char	*p = emsi_dat, *rew;

	/* Step 1
	+-+------------------------------------------------------------------+
	:1: Tries=0, T1=20 seconds, T2=60 seconds                            :
	+-+------------------------------------------------------------------+
	*/
	DEBUG(('E',2,"ER: Step 1"));

	tries = 0;
	t1 = timer_set( 20 );
	t2 = timer_set( cfgi( CFG_HSTIMEOUT ));

	do {

step2:
		/* Step 2
		+-+------------------------------------------------------------------+
		:2: Increment Tries                                                  :
		: :                                                                  :
		: : Tries>6?                      Terminate, and report failure.     :
		: +------------------------------------------------------------------+
		: : Are we answering system?      Transmit EMSI_REQ, go to step 3.   :
		: +------------------------------------------------------------------+
		: : Tries>1?                      Transmit EMSI_NAK, go to step 3.   :
		: +------------------------------------------------------------------+
		: : Go to step 4.                                                    :
		+-+------------------------------------------------------------------+
		*/
		DEBUG(('E',2,"ER: Step 2"));

		if ( ++tries > 6 ) {
			sline( "EMSI receive failed - too many tries" );
			DEBUG(('E',1,"EMSI receive failed - too many tries"));
			return TIMEOUT;
		}

		if ( mode == SM_INBOUND ) {
			sline( "Sending EMSI_REQ (%d)...", tries );
			DEBUG(('E',1,"Sending EMSI_REQ (%d)...",tries));
			PUTSTRCR( emsireq );
		} else if ( tries > 1 ) {
			sline( "Sending EMSI_NAK..." );
			DEBUG(('E',1,"Sending EMSI_NAK (%d)...",tries));
			PUTSTRCR( emsinak );
		} else        
			goto step4;

step3:
		/* Step 3
		+-+------------------------------------------------------------------+
		:3: T1=20 seconds                                                    :
		+-+------------------------------------------------------------------+
		*/
		DEBUG(('E',2,"ER: Step 3"));

		t1 = timer_set( 20 );

step4:
		/* Step 4
		+-+------------------------------------------------------------------+
		:4: Wait for EMSI sequence until EMSI_HBT or EMSI_DAT or any of the  :
		: : timers have expired.                                             :
		: :                                                                  :
		: : If T2 has expired, terminate call and report failure.            :
		: +------------------------------------------------------------------+
		: : If T1 has expired, go to step 2.                                 :
		: +------------------------------------------------------------------+
		: : If EMSI_HBT received, go to step 3.                              :
		: +------------------------------------------------------------------+
		: : If EMSI_DAT received, go to step 5.                              :
		: +------------------------------------------------------------------+
		: : Go to step 4.                                                    :
		+-+------------------------------------------------------------------+
		*/
		DEBUG(('E',2,"ER: Step 4"));

		memset( emsi_dat, 0, EMSI_BUF );
		p = emsi_dat;
		got = 0;

		while( 1 ) {
			ch = GETCHAR( MAX( 1, MIN( timer_rest( t1 ), timer_rest( t2 ))));
			DEBUG(('E',9,"getchar '%c' %d (%d, %d)", C0( ch ), ch, timer_rest( t1 ), timer_rest( t2 )));

			if ( NOTTO( ch ) && ( ch != TIMEOUT ))
				return ch;

			if ( timer_expired( t2 )) {
				sline( "Timeout receiving EMSI_DAT" );
				DEBUG(('E',1,"Timeout receiving EMSI_DAT"));
				return TIMEOUT;
			}

			if ( timer_expired( t1 ))
				break; /* goto step2; */

			if ( ch == TIMEOUT )
				continue;

			if ( !got ) {
				if ( ch == '*' )
					got = 1;
				else
					continue;
			}

			if ( ch == '\r' || ch == '\n' ) {
				if ( !strncmp( emsi_dat, emsihbt, EMSI_SEQ_LEN )) {
					sline( "Received EMSI_HBT" );
					DEBUG(('E',1,"Received EMSI_HBT"));
					goto step3;
				}

				if ( !strncmp( emsi_dat, emsidat, 10 )) {

					/* Step 5
					+-+------------------------------------------------------------------+
					:5: Receive EMSI_DAT packet                                          :
					: +------------------------------------------------------------------+
					: : Packet received OK?                Transmit EMSI_ACK twice, and  :
					: :                                    go to step 6.                 :
					: +------------------------------------------------------------------+
					: : Go to step 2.                                                    :
					+-+------------------------------------------------------------------+
					*/
					DEBUG(('E',2,"ER: Step 5"));

					sline( "Received EMSI_DAT" );
					DEBUG(('E',1,"Received EMSI_DAT"));
					ch = emsi_parsedat( emsi_dat, rememsi );
					DEBUG(('E',1,"Parser result: %d", ch));
					if ( ch ) {
						sline( "EMSI_DAT OK. Sending EMSI_ACK...");
						DEBUG(('E',1,"EMSI_DAT OK. Sending EMSI_ACK"));
						PUTSTRCR( emsiack );
						PUTSTRCR( emsiack );

						/* Step 6
						+-+------------------------------------------------------------------+
						:6: Received EMSI_DAT packet OK, exit.                               :
						+-+------------------------------------------------------------------+
						*/
						DEBUG(('E',2,"ER: Step 6"));

						return OK;

					} else {
						DEBUG(('E',1,"Got string '%s' %d", emsi_dat, strlen(emsi_dat)));
						goto step2;
					}
				}
				goto step4;
			} else {
				if (( p - emsi_dat + 1 ) >= EMSI_BUF ) {
					write_log( "Too long EMSI packet" );
					rew = NULL;
					rew = strstr( emsi_dat + 1, EMSI_BEG );
					if ( rew && rew != emsi_dat ) {
						DEBUG(('E',1,"got EMSI_DAT at offset %d", p - rew));
						memcpy( emsi_dat, rew, p - rew );
						p -= rew - emsi_dat;
					} else {
						p = emsi_dat;
					}
					memset( p, 0, EMSI_BUF - (p - emsi_dat));
				}
				
				if ( ch >= 32 && ch <= 255 )
					*p++ = ch;
			}
		}
	} while( !timer_expired( t2 ));

	return TIMEOUT;
}


/*
 *   STEP 2B, TRANSMIT EMSI HANDSHAKE
 */
int emsi_send(void)
{
	time_t t1, t2;
	char inbuf[TMP_LEN + 5], *p = inbuf;
	int tries, ch;
    
	/* Step 1
	+-+------------------------------------------------------------------+
	:1: Tries=0, T1=60 seconds                                           :
	+-+------------------------------------------------------------------+
	*/
	DEBUG(('E',2,"ES: Step 1"));

	tries = 0;
	t1 = timer_set( cfgi( CFG_HSTIMEOUT ));

step2:
	/* Step 2
	+-+------------------------------------------------------------------+
	:2: Transmit EMSI_DAT packet and increment Tries                     :
	: :                                                                  :
	: +------------------------------------------------------------------+
	: : Tries>6?                        Terminate, and report failure.   :
	: +------------------------------------------------------------------+
	: : Go to step 3.                                                    :
	+-+------------------------------------------------------------------+
	*/
	DEBUG(('E',2,"ES: Step 2"));

	memset( inbuf, 0, TMP_LEN );

	if ( ++tries > 6 )
		return TIMEOUT;

	sline( "Sending EMSI_DAT" );
	DEBUG(('E',1,"Sending EMSI_DAT (%d)",tries));
	PUTSTRCR( emsi_dat );

	/* Step 3
	+-+------------------------------------------------------------------+
	:3: T2=20 seconds                                                    :
	+-+------------------------------------------------------------------+
	*/
	DEBUG(('E',2,"ES: Step 3"));
	
	t2 = timer_set( 20 );

	/* Step 4
	+-+------------------------------------------------------------------+
	:4: Wait for EMSI sequence until T1 has expired                      :
	: :                                                                  :
	: : If T1 has expired, terminate call and report failure.            :
	: +------------------------------------------------------------------+
	: : If T2 has expired, go to step 2.                                 :
	: +------------------------------------------------------------------+
	: : If EMSI_REQ received, go to step 4.                              :
	: +------------------------------------------------------------------+
	: : If EMSI_ACK received, go to step 5.                              :
	: +------------------------------------------------------------------+
	: : If any other sequence received, go to step 2.                    :
	+-+------------------------------------------------------------------+
	*/
	DEBUG(('E',2,"ES: Step 4"));
        	
	while( !timer_expired( t1 ) ) {
		ch = GETCHAR( MAX( 1, MIN( timer_rest( t1 ), timer_rest( t2 ))));
		DEBUG(('E',9,"getchar '%c' %d (%d, %d)", C0( ch ), ch, timer_rest( t1 ), timer_rest( t2 )));

		if ( NOTTO( ch ) && ( ch != TIMEOUT ))
			return ch;

		if ( timer_expired( t2 ))
			goto step2;

		if ( timer_expired( t1 ))
			return TIMEOUT;

		if ( ch == TIMEOUT )
			continue;

		ch &= 0x7f;

		if ( ch == '\r' || ch == '\n' ) { /* Got <CR> */
			*p = '\0';
			p = inbuf;

			if ( *p == '\0' )
				continue;

			DEBUG(('E',2,"Got string '%s' (%d)",inbuf,strlen(inbuf)));

			if ( !strncmp( inbuf, emsidat, 10 )) {
				sline("Got EMSI_DAT - now? kidding :) acknoleging");
				DEBUG(('E',1,"Acknoleging unexpected EMSI_DAT (Argus, yeah?)"));
        	                PUTSTRCR( emsiack );
	                        PUTSTRCR( emsiack );
	                        t2 = timer_set( timer_rest( t2 ) >> 2 );
			} else if ( !strncmp( inbuf, emsireq, EMSI_SEQ_LEN )) {
				sline("Got EMSI_REQ - skipping");
				DEBUG(('E',1,"Skipping EMSI_REQ"));
			} else if ( !strncmp( inbuf, emsiack, EMSI_SEQ_LEN )) {
				sline("Got EMSI_ACK");
				DEBUG(('E',1,"Got EMSI_ACK"));
				/* Step 5
				+-+------------------------------------------------------------------+
				:5: Received EMSI_ACK, exit.                                         :
				+-+------------------------------------------------------------------+
				*/
				DEBUG(('E',2,"ES: Step 5"));

				return OK;
			}

			memset( inbuf, 0, TMP_LEN );
			continue;

		}
        
		/* Put new symbol in buffer */
		if ( ch > 31 ) {
			if ( strlen( inbuf ) < TMP_LEN ) {
				*p++ = ch;
			} else {
				p = inbuf;
				write_log( "Too long EMSI packet" );
				memset( inbuf, 0, TMP_LEN );
			}
		}
	} /* goto step4; */

	return TIMEOUT;

}
