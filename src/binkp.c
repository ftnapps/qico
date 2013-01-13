/******************************************************************
 * Binkp protocol implementation.
 ******************************************************************/
/*
 * $Id: binkp.c,v 1.20 2005/08/22 17:19:41 mitry Exp $
 *
 * $Log: binkp.c,v $
 * Revision 1.20  2005/08/22 17:19:41  mitry
 * Changed names of functions to more proper form
 *
 * Revision 1.19  2005/08/22 12:51:46  mitry
 * Fixed syntax error
 *
 * Revision 1.18  2005/08/21 03:51:56  mitry
 * Fixed stupid error
 *
 * Revision 1.17  2005/08/21 01:11:21  mitry
 * Added more explanations for session errors (I hope)
 *
 * Revision 1.16  2005/08/12 15:36:19  mitry
 * Changed gmtoff()
 *
 * Revision 1.15  2005/08/10 16:15:54  mitry
 * Changed time parsing logic on binkp session
 *
 * Revision 1.14  2005/08/03 11:34:11  mitry
 * Changed timeout handling
 *
 * Revision 1.13  2005/05/31 17:22:43  mitry
 * Added support for incoming FREQ on binkp/1.0
 *
 * Revision 1.12  2005/05/24 18:45:41  mitry
 * Do not use crypt if got M_OK non-secure
 *
 * Revision 1.11  2005/05/17 18:17:42  mitry
 * Removed system basename() usage.
 * Added qbasename() implementation.
 *
 * Revision 1.10  2005/05/16 13:13:20  mitry
 * Added more rubust message handling
 *
 * Revision 1.9  2005/05/06 20:16:58  mitry
 * Changed end-of-session detection
 *
 * Revision 1.8  2005/04/09 01:08:44  mitry
 * Removed doubling total mail size send to remote
 *
 * Revision 1.7  2005/04/07 12:58:13  mitry
 * Changed TIME parsing and format to send
 *
 * Revision 1.6  2005/03/28 15:07:46  mitry
 * New non-blocking version of binkp
 *
 * Revision 1.5  2005/02/25 16:27:05  mitry
 * Revert last change
 *
 * Revision 1.4  2005/02/21 16:33:42  mitry
 * Changed tty_hangedup to tty_online
 *
 */

#include "headers.h"
#ifdef WITH_BINKP

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "binkp.h"
#include "tty.h"
#include "tcp.h"
#include "crc.h"
#include "md5q.h"


#define CRYPT(_x_) ( _x_->opt_cr == O_YES ? " (crypted)" : "" )

#define ERR_CLOSE(_x_) \
	if ( _x_->send_file )			\
		txclose( &txfd, FOP_ERROR );	\
	if ( _x_->recv_file )			\
		rxclose( &rxfd, FOP_ERROR );


#define xN(i) (n[i] - '0')
#define BP_VER(_x_) (_x_->ver_major * 100 + _x_->ver_minor)

static char *mess[] = {
    "NUL", "ADR", "PWD", "FILE", "OK", "EOB", "GOT",
    "ERR", "BSY", "GET", "SKIP", "RESERVED", "CHAT"
};

static BPS *bps = NULL;

static const char *month_names[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char *weekday_names[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};



static int binkp_init(int to)
{
	char *p;

	if ( !bps ) {
		bps = xcalloc( sizeof( BPS ), 1 );
		if ( !bps ) {
			write_log("Can't allocate session data (bps)");
			return ERROR;
		}
	}

	bps->to = to;
	bps->rx_buf = (byte *) xcalloc( BP_BUFFER, 1 );
	bps->rx_ptr = 0;
	bps->rx_size = -1;
	bps->is_msg = -1;
	bps->ver_major = 1;
	bps->ver_minor = 0;

	bps->tx_buf = (byte *) xcalloc( BP_BUFFER, 1 );
	bps->tx_ptr = bps->tx_left = 0;

	bps->init = 1;

	for( p = cfgs( CFG_BINKPOPT ); *p; p++ ) {
		switch(tolower(*p)) {
		case 'p':					/* Force password digest */
			bps->opt_md |= O_NEED;

		case 'm':					/* Want password digest */
			bps->opt_md |= O_WANT;
			break;

		case 'c':					/* Can crypt */
			bps->opt_cr |= O_WE;
			break;

		case 'd':					/* No dupes mode */
			bps->opt_nd |= O_NO; /*mode?O_THEY:O_NO;*/

		case 'r':					/* Non-reliable mode */
			bps->opt_nr |= to ? O_WANT : O_NO;
			break;

		case 'b':					/* Multi-batch mode */
			bps->opt_mb |= O_WANT;
			break;

		case 't':					/* Chat */
			bps->opt_cht |= O_WANT;
			break;

		default:
			write_log( "Binkp: unknown binkpopt: '%c'", *p );
		}
	}

	if ( !to && !tcp_setsockopts( tty_fd ))
		return ERROR;

	bps->MD_chal = NULL;

	return OK;
}


static void binkp_deinit(void)
{
	int i;

	xfree( bps->tx_buf );
	xfree( bps->rx_buf );
	xfree( bps->MD_chal );
	xfree( bps->rfname );
	for( i = 0; i < bps->nmsgs; i++ )
		if ( bps->mqueue[i].msg )
			xfree( bps->mqueue[i].msg );
	xfree( bps->mqueue );

	xfree( bps );
}


static void mkheader(byte *buf, word l)
{
	buf[0] = (byte) (( l >> 8 ) & 0xff);
	buf[1] = (byte) (l & 0xff);
}


static int msgs(char id, char *str, ...)
{
	va_list	args;
	size_t	len;
	char	msg_body[513] = {0};

	if ( str ) {
		va_start( args, str );
		vsnprintf( msg_body, 512, str, args );
		va_end( args );
	}

	len = strlen( msg_body ) + 1;

	DEBUG(('B',1,"send msg %s '%s'", mess[(int) id], msg_body));

	bps->mqueue = xrealloc( bps->mqueue, sizeof( BPMSG ) * ( bps->nmsgs + 1 ));
	bps->mqueue[bps->nmsgs].id = id;
	bps->mqueue[bps->nmsgs].len = len;
	bps->mqueue[bps->nmsgs].msg = xmalloc( len + BLK_HDR_SIZE + 1 );

	mkheader( bps->mqueue[bps->nmsgs].msg, len | 0x8000);
	bps->mqueue[bps->nmsgs].msg[2] = (byte) id;
	xstrcpy( bps->mqueue[bps->nmsgs].msg + 3, msg_body, len );
	bps->mqueue[bps->nmsgs].len += BLK_HDR_SIZE;

	if ( bps->opt_cr == O_YES )
		encrypt_buf( (void *) bps->mqueue[bps->nmsgs].msg,
			bps->mqueue[bps->nmsgs].len, bps->keys_out );

	bps->nmsgs++;
	bps->mib++;

	return 1;
}


static int file_parse(void *s, char **fn, long *sz, time_t *tm, long *offs)
{
	char *n = (char *) s, *name, *size, *time, *off = NULL;

	DEBUG(('B',4,"pars: in '%s'",n));
	name = strsep( &n, " " );
	size = strsep( &n, " " );
	time = strsep( &n, " " );

	if ( offs )
		off = strsep( &n, " " );

	DEBUG(('B',4,"pars: name=%s, size=%s, time=%s, offs=%s",
		name, size, time, off));
	if ( name && size && time && ( !offs || off )) {
		*fn = name;
		*sz = atol( size );
		*tm = atol( time );
		if ( offs )
			*offs = atol( off );
		DEBUG(('B',3,"pars: name=%s, size=%lu, time=%lu, offs=%ld",
			*fn, *sz, *tm, offs ? *offs : -3l ));
		return 1;
	}
	return 0;
}


int binkp_devfree(void)
{
	return ( bps->opt_cht == O_YES );
}


int binkp_devsend(byte *str, word len)
{
	if ( bps->opt_cht == O_YES ) {
		msgs( BPM_CHAT, "%s", (char *) str );
		return 1;
	}
	return 0;
}


static void binkp_devrecv(byte *data)
{
    c_devrecv( data, strlen( (char *) data ) + 1 );
}


/*
 * Process M_NUL commands
 */
static int M_nul(BPS *bp, byte *arg)
{
	char *n;
	char *buf = (char *) arg;

	DEBUG(('B',3,"NUL: %s", buf));

	recode_to_local( buf );

	if ( !strncmp( buf, "SYS ", 4 ))
		restrcpy( &rnode->name, skip_blanks( buf + 4 ));
	else if( !strncmp( buf, "ZYZ ", 4 ))
		restrcpy( &rnode->sysop, skip_blanks( buf + 4 ));
	else if( !strncmp( buf, "LOC ", 4 ))
		restrcpy( &rnode->place, skip_blanks( buf + 4 ));
	else if( !strncmp( buf, "NDL ", 4 )) {
		long spd = atol( skip_blanks( buf + 4 ));
		char *comma = strchr( buf + 4, ',' );

		restrcpy( &rnode->flags, comma ? comma + 1 : NULL );

		if ( spd >= 300 )
			rnode->speed = spd;
		else {
			comma = skip_blanks( buf + 4 );
			while( comma && *comma && isdigit( *comma ))
				comma++;
			if ( !comma || !*comma )
				rnode->speed = TCP_SPEED;
			else if ( tolower( *comma ) == 'k' )
				rnode->speed = spd * 1024;
			else if ( tolower( *comma ) == 'm' )
				rnode->speed = spd * 1024 * 1024;
			else
				rnode->speed = TCP_SPEED;
		}
	} else if( !strncmp( buf, "TIME ", 5 )) {
		long		gmt = 0;
		size_t		len = strlen( buf + 5 );
		struct tm	tt;
		time_t		ti, TZOFFSET;

		ti = time( NULL );
		TZOFFSET = gmtoff( ti );
		memcpy( &tt, localtime( &ti ), sizeof( struct tm ));

#define BMALTL ((size_t)((byte *) buf - (byte *) arg ) < len )

		buf = skip_blanks( buf + 5 );

		if ( isalpha( *buf ))			/* Skip day of week name "Mon, " */
			buf = skip_blanks( buf + 5 );

		DEBUG(('B',9,"1..... buf '%s'", buf));
		if ( BMALTL && isdigit( *buf ) && ( isdigit( buf[1] ) || isspace( buf[1] ))) {
			tt.tm_mday = atoi( buf );
			buf = skip_blanks( buf + 2 );

			DEBUG(('B',9,"2..... buf '%s'", buf));
			if ( BMALTL && isalpha( *buf )) {
				buf[3] = '\0';
				for( gmt = 0; gmt < 12; gmt++ ) {
					if ( !strncasecmp( month_names[gmt], buf, 3 )) {
						tt.tm_mon = gmt;
						break;
					}
				}

				DEBUG(('B',9,"3..... gmt %ld, buf '%s'", gmt, buf + 4));
				if ( gmt < 12 ) {
					buf = skip_blanks( buf + 4 );

					DEBUG(('B',9,"4..... buf '%s'", buf));
					if ( BMALTL && isdigit( *buf )) {
						gmt = atol( buf );
						if ( errno != ERANGE ) {
							if ( gmt > 1900 )
								tt.tm_year = (int) (gmt - 1900);
							else if ( gmt < 50 )
								tt.tm_year = (int) (gmt + 100);
							else
								tt.tm_year = (int) gmt;

							DEBUG(('B',9,"5..... gmt %ld", gmt));
							gmt = 0;
							n = strchr( buf , ':' ) - 2;
							if ( n ) {
								if ( *n == ' ' )
									n++;

								DEBUG(('B',9,"6..... n '%s'", n));
								tt.tm_hour = atoi( n );

								n = strchr( buf , ':' ) + 1;
								DEBUG(('B',9,"7..... n '%s'", n));
								if ( n ) {
									tt.tm_min = atoi( n );
									n += 2;
									DEBUG(('B',9,"8..... n '%s'", n));
									if ( *n == ':' ) {
										tt.tm_sec = atoi( n + 1 );
										n++;
									}

									DEBUG(('B',9,"9..... n '%s'", n));
									while( *n && *n++ != ' ' );

									DEBUG(('B',9,"A..... n '%s'", n));
									if ( *n && isdigit( n[1] ) && isdigit( n[2] )
										&& isdigit( n[3] ) && isdigit( n[4] ))
									{
										gmt = ((xN(1) * 10 + xN(2)) * 60 + (xN(3) * 10 + xN(4))) * 60;
										if ( *n == '-' )
											gmt = -gmt;
									} else if ( !strncmp( n, "GMT", 3 ) || !strncmp( n, "UT", 2 )) {
										gmt = -TZOFFSET;
									}
								}
							}
						}
					}
				} else
					gmt = 0;
			}
		}
#undef BMALTL

		DEBUG(('B',9,"gmt %ld", gmt));
		tt.tm_sec += ( TZOFFSET - gmt );
		rnode->time = mktime( &tt ) + gmt;

	} else if( !strncmp( buf, "VER ", 4 )) {
		char *min;
		restrcpy( &rnode->mailer, skip_blanks( buf + 4 ));
		if (( n = strstr( buf, BP_PROT "/" )) && ( min = strchr( n, '.' )) && ( min - n > 6 )) {
			bp->ver_major = atoi( n + 6 );
			bp->ver_minor = atoi( min + 1 );
		} else {
			write_log("Binkp: got bad VER message: %s", buf );
		}
		if ( BP_VER( bp ) <= 100 )
			bp->delay_eob = 1;
	} else if ( !strncmp( buf, "TRF ", 4 )) {
		n = skip_blanks( buf + 4 );
		if ( n && *n && isdigit( *n )) {
			rnode->netmail = atoi( n );
			n = skip_blanks( strchr( n, ' ' ));
			if ( n && *n && isdigit( *n ))
				rnode->files = atoi( n );
		}
		if ( rnode->netmail + rnode->files )
			bp->delay_eob = 1;
	} else if( !strncmp( buf, "FREQ", 4 )) {
		bp->delay_eob = 1;
	} else if( !strncmp( buf, "PHN ", 4 )) {
		restrcpy( &rnode->phone, skip_blanks( buf + 4 ));
	} else if ( !strncmp( buf, "OPM ", 4 )) {
		write_log( "Message for operator: %s", skip_blanks( buf + 4 ));
	} else if ( !strncmp( buf, "OPT ", 4 )) {
		char *p;

		n = skip_blanks( buf + 4 );
		while( n && ( p = strsep( &n, " " ))) {
			if ( !strcmp( p, "NR" ))
				bp->opt_nr |= O_WE;
			else if ( !strcmp( p, "MB" ))
				bp->opt_mb |= O_WE;
			else if( !strcmp( p, "ND" ))
				bp->opt_nd |= O_WE;
			else if( !strcmp( p, "NDA" ))
				bp->opt_nd |= O_EXT;
			else if( !strcmp( p, "CHAT" ))
				bp->opt_cht |= O_WE;
			else if( !strcmp( p, "CRYPT" ))
				bp->opt_cr |= O_THEY;
			else if( !strncmp( p, "CRAM-MD5-", 9 ) && bp->to && bp->opt_md ) {
				if ( strlen( p + 9 ) > 64 )
					write_log( "Binkp: got too long challenge string" );
				else {
					DEBUG(('B',2,"Remote requests MD mode"));
					xfree( bp->MD_chal );
					bp->MD_chal = md5_challenge((unsigned char *) p );
					if ( bp->MD_chal )
						bp->opt_md |= O_THEY;
				}
				if (( bp->opt_md & ( O_THEY | O_WANT )) == ( O_THEY | O_WANT ))
					bp->opt_md = O_YES;
			} else /* if ( strcmp( p, "GZ" ) || strcmp( p, "BZ2" ) || strcmp( p, "EXTCMD" )) */ {
				DEBUG(('B',2,"got unsupported option '%s'",p));
			}
		}
	} else
		write_log( "Binkp: got unknown NUL: \"%s\"", buf );

	return 1;
}


/*
 * Process M_ADR command
 */
static int M_adr(BPS *bp, byte *arg)
{
	char		*buf = skip_blanks((char *) arg );
	char		*rem_aka, *rem_pwd = NULL;
	FTNADDR_T	( fa );
	falist_t	*our_addrs = cfgal( CFG_ADDRESS );
	int		rc = 0;
    
	DEBUG(('B',3,"ADR: %s", buf));

	falist_kill( &rnode->addrs );
	totalm = totalf = 0;

	while(( rem_aka = strsep( &buf, " " ))) {
		DEBUG(('B',4,"parsing: %s", rem_aka));
		if ( parseftnaddr( rem_aka, &fa, NULL, 0 )) {
			if ( has_addr( &fa, our_addrs )) {
				falist_add( &rnode->addrs, &fa );
				log_rinfo( rnode );
				write_log( "Remote has our aka %s", ftnaddrtoa( &fa ));
				msgs( BPM_ERR, "Sorry, you have one of my akas");
				bp->rc = S_FAILURE;
				return 0;
			}

			/* Search for duplicated remote akas */
			if ( !falist_find( rnode->addrs, &fa )) {
				if ( outbound_locknode( &fa, LCK_s )) {
                    
					DEBUG(('B',4,"locked: %s", rem_aka));

					if ( !bp->to && !rem_pwd ) {
						rem_pwd = findpwd( &fa );
						if ( rem_pwd ) {
							DEBUG(('B',4,"found pwd '%s' for %s", rem_pwd, rem_aka));
						}
					}

					rc++;
					falist_add( &rnode->addrs, &fa );
					makeflist( &fl, &fa, bp->to );

					DEBUG(('B',4,"totalm: %lu, totalf: %lu", totalm, totalf));

				} else
					write_log( "Can't lock outbound for %s", ftnaddrtoa( &fa ));
			} else
				DEBUG(('B',4,"removed duplicated aka: %s", rem_aka));
		} else {
			log_rinfo( rnode );
			write_log( "Remote sent bad address %s", rem_aka );
			msgs( BPM_ERR, "Bad address %s", rem_aka );
			bp->rc = S_FAILURE;
			return 0;
		}
	}

	if ( rc == 0 ) {
		log_rinfo( rnode );
		msgs( BPM_BSY, "All akas are busy" );
		bp->rc = ( bp->to ? S_REDIAL | S_ADDTRY : S_BUSY );
		return 0;
	}

	if ( bp->to ) {
		if ( !has_addr( bp->remaddr, rnode->addrs )) {
			log_rinfo( rnode );
			write_log( "Called the wrong system" );
			msgs( BPM_ERR, "Sorry, you are not who I need" );
			bp->rc = S_FAILURE|S_ADDTRY;
			return 0;
		}

		msgs( BPM_NUL, "TRF %lu %lu", totalm, totalf );

		DEBUG(('B',3,"state: sendpwd"));

		rem_pwd = findpwd( bp->remaddr );
			
		if ( rem_pwd == NULL )
			rem_pwd = "-";

		DEBUG(('B',4,"pwd: '%s'",rem_pwd));
		if ( bp->MD_chal ) {
			char *dig = md5_digest( rem_pwd, bp->MD_chal );

			if ( dig == NULL ) {
				msgs( BPM_ERR, "Can't build digest" );
				bp->rc = S_REDIAL | S_ADDTRY;
				return 0;
			} else
				msgs( BPM_PWD, "CRAM-MD5-%s", dig );
			xfree( dig );
		} else if ( bp->opt_md == O_YES ) {
			msgs( BPM_ERR, "Can't use plaintext password" );
			bp->rc = S_FAILURE|S_ADDTRY;
			return 0;
		} else
			msgs( BPM_PWD, "%s", rem_pwd );
	} else if ( rem_pwd == NULL )
		rem_pwd = "-";

	restrcpy( &rnode->pwd, rem_pwd );
	if ( !strcmp( rnode->pwd, "-" ))
		rnode->options &= ~O_PWD;
	else
		rnode->options |= O_PWD;

	return 1;
}


/*
 * Completes binkp handshake
 */
static int binkp_hsdone(BPS *bp)
{
	char *p, tmp[256];

	if ( bp->opt_nd == O_WE || bp->opt_nd == O_THEY )
		bp->opt_nd = O_NO;

	if ( !rnode->phone || !*rnode->phone )
		restrcpy( &rnode->phone, "-Unpublished-" );
	if ( !( rnode->options & O_PWD ) || bp->opt_md != O_YES )
		bp->opt_cr = O_NO;
	if (( bp->opt_cr & O_WE ) && ( bp->opt_cr & O_THEY )) {
		unsigned long *keys;

		DEBUG(('B',2,"enable crypting messages"));
		bp->opt_cr = O_YES;
		if ( bp->to ) {
			init_keys( bp->keys_out, rnode->pwd );
			init_keys( bp->keys_in, "-" );
			keys = bp->keys_in;
		} else {
			init_keys( bp->keys_in, rnode->pwd );
			init_keys( bp->keys_out, "-" );
			keys = bp->keys_out;
		}
		for( p = rnode->pwd; *p; p++ )
			update_keys( keys, (int) *p );
	}

	log_rinfo( rnode );
	write_log( "we have: %d%c mail, %d%c files",
		SIZES( totalm ), SIZEC( totalm ), SIZES( totalf ), SIZEC( totalf ));
	rnode->starttime = time( NULL );
	if ( cfgi( CFG_MAXSESSION ))
		alarm( cci * 60 );
	DEBUG(('S',1,"Maxsession: %d",cci));

	if ( bp->opt_cht & O_WANT )
		chatinit( 0 );
    
	if ( bp->opt_nd & O_WE || ( bp->to && ( bp->opt_nr & O_WANT) && BP_VER( bp ) >= 101 ))
		bp->opt_nr |= O_WE;

	if (( bp->opt_cht & O_WE ) && ( bp->opt_cht & O_WANT ))
		bp->opt_cht = O_YES;

	if ( BP_VER( bp ) > 100 || ( bp->opt_mb & O_WE ))
		bp->opt_mb = O_YES;
	else
		bp->opt_mb = O_NO;

	if ( BP_VER( bp ) > 100 )
		bp->delay_eob = 0;

	snprintf( tmp, 255, "Binkp%s%s%s%s%s%s%s%s%s",
		( rnode->options & O_LST ) ? "/LST" : "",
		( rnode->options & O_PWD ) ? "/PWD" : "",
		( bp->opt_nr & O_WE ) ? "/NR" : "",
		(( bp->opt_nd & O_WE ) && ( bp->opt_nd & O_THEY )) ? "/ND" : "",
		(( bp->opt_nd & O_WE ) && !( bp->opt_nd & O_THEY ) && (bp->opt_nd & O_EXT )) ? "/NDA" : "",
		( bp->opt_mb == O_YES ) ? "/MB" : "",
		( bp->opt_md == O_YES ) ? "/MD5" : "/Plain",
		( bp->opt_cr == O_YES ) ? "/CRYPT" : "",
		( bp->opt_cht == O_YES ) ? "/Chat" :"" );
	write_log( "options: %s", tmp );
	IFPerl(if(perl_on_session(tmp)!=S_OK) return 0);
    
	title( "%sbound session %s", bp->to ? "Out" : "In", ftnaddrtoa( &rnode->addrs->addr ));
	qemsisend( rnode );
	qpreset( 0 );
	qpreset( 1 );

	sendf.allf = totaln;
	sendf.ttot = totalf + totalm;
	recvf.ttot = rnode->netmail + rnode->files;
	effbaud = rnode->speed;
	bp->oflist = fl;
	outbound_getstatus( &rnode->addrs->addr, &bp->ostatus );

	sline( "Binkp session" );
    
	bp->mib = 0;
	bp->init = 0;

	DEBUG(('B',1,"established binkp/%d.%d session. (nr,nd,md,mb,cr,cht)=(%d,%d,%d,%d,%d,%d)",
		bp->ver_major,
		bp->ver_minor,
		bp->opt_nr,
		bp->opt_nd,
		bp->opt_md,
		bp->opt_mb,
		bp->opt_cr,
		bp->opt_cht));

	return 1;
}


/*
 * Process M_FILE command
 */
static int M_file(BPS *bp, byte *arg)
{
	char	*buf = (char *) arg, *fname = NULL;
	long	fsize, foffs;
	int	i, n;
	time_t	ftime;

	DEBUG(('B',3,"FILE %s", buf));

	if ( bp->sent_eob && bp->recv_eob )
		bp->sent_eob = 0;
	bp->cls = 1;
	bp->recv_eob = 0;

	if ( bp->recv_file ) {
		rxclose( &rxfd, FOP_OK );
		bp->recv_file = 0;
		qpfrecv();
	}

	if ( !file_parse( buf, &fname, &fsize, &ftime, &foffs )) {
		msgs( BPM_ERR, "M_FILE: unparsable file info: \"%s\"", buf );
		write_log( "Binkp: got unparsable file info: \"%s\"", buf );
		if ( bp->send_file )
			txclose( &txfd, FOP_ERROR );
		bp->rc = ( bp->to ? S_REDIAL | S_ADDTRY : S_BUSY );
		return 0;
	}

	if ( bp->rfname && !strncasecmp( fname, bp->rfname, MAX_PATH )
		&& bp->rmtime == ftime && recvf.ftot == fsize && recvf.soff == foffs )
	{
		bp->recv_file = 1;
		bp->rxpos = foffs;
		return 1;
	}

	xfree( bp->rfname );
	bp->rmtime = ftime;
	bp->rfname = xstrdup( qbasename( fname ));

	for( i = 0, n = 0; i < (int) strlen( fname ); ) {
		if ( fname[i] == '\\' && tolower( fname[i+1] ) == 'x' ) {
			fname[n++] = hexdcd( fname[i+2], fname[i+3] );
			i += 4;
		} else
			fname[n++] = fname[i++];
	}

	switch( rxopen( fname, ftime, fsize, &rxfd )) {
	case FOP_ERROR:
		write_log( "Binkp: error open file for write \"%s\"", recvf.fname );

	case FOP_SUSPEND:
		DEBUG(('B',2,"trying to suspend file \"%s\"", recvf.fname ));
		msgs( BPM_SKIP, "%s %lu %lu", bp->rfname, (long) recvf.ftot, bp->rmtime );
		break;

	case FOP_SKIP:
		DEBUG(('B',2,"trying to skip file \"%s\"", recvf.fname ));
		msgs( BPM_GOT, "%s %lu %lu", bp->rfname, (long) recvf.ftot, bp->rmtime );
		break;

	case FOP_OK:
		if ( foffs != -1 ) {
			if ( !( bp->opt_nr & O_THEY ))
				bp->opt_nr |= O_THEY;
			bp->recv_file = 1;
			bp->rxpos = 0;
			break;
		}

	case FOP_CONT:
		DEBUG(('B',2,"trying to get file \"%s\" from offset %ld",
			recvf.fname, (long) recvf.soff ));
		msgs( BPM_GET, "%s %lu %lu %ld", bp->rfname, (long) recvf.ftot,
			bp->rmtime, (long) recvf.soff );
		break;
	}

	return 1;
}


/*
 * Process M_PWD command
 */
static int M_pwd(BPS *bp, byte *arg)
{
	char	*buf = skip_blanks((char *) arg );
	char	*exp_pwd = NULL, *got_pwd = NULL, tmp[129];
	int	have_CRAM = !strncasecmp( buf, "CRAM-MD5-", 9 );
	int	have_pwd = ((rnode->options & O_PWD) != 0 );
	int	bad_pwd;

	DEBUG(('B',3,"PWD %s", buf));

	if ( bp->to ) {
		DEBUG(('B',1,"unexpected password (%s) from remote on outgoing call", buf));
		bp->rc = S_FAILURE;
		return 0;
	}

	DEBUG(('B',4,"have_CRAM: %d, have_pwd: %d, rn_pwd: '%s', MD_chal: %.8p", have_CRAM, have_pwd,
		rnode->pwd, bp->MD_chal));

	if ( bp->MD_chal ) {
		if ( have_CRAM ) {
			got_pwd = buf + 9;
			exp_pwd = md5_digest( rnode->pwd, bp->MD_chal );
			bp->opt_md |= O_THEY;
		} else if ( bp->opt_md & O_NEED ) {
			log_rinfo( rnode );
			write_log( "Remote does not support MD5" );
			msgs( BPM_ERR, "You must support MD5" );
			bp->rc = S_FAILURE;
			return 0;
		}
	}
    
	if ( !bp->MD_chal || ( !have_CRAM && !( bp->opt_md & O_NEED ))) {
		got_pwd = buf;
		exp_pwd = xstrdup( rnode->pwd );
	}

	bad_pwd = strcasecmp( exp_pwd, got_pwd );
	DEBUG(('B',4,"exp_pwd: '%s', got_pwd: '%s', bad_pwd: %d", exp_pwd, got_pwd, bad_pwd ));

	if ( have_pwd ) {
		if ( bad_pwd ) {
			log_rinfo( rnode );
			write_log( "Bad password '%s'", buf );
			msgs( BPM_ERR, "Security violation" );
			xfree( exp_pwd );
			rnode->options |= O_BAD;
			bp->rc = S_FAILURE;
			return 0;
		}
	} else if ( bad_pwd ) {
		write_log( "Remote proposed password for us '%s'", buf );
	}

	if (( bp->opt_md & ( O_THEY | O_WANT )) == ( O_THEY | O_WANT ))
		bp->opt_md = O_YES;

	if ( !have_pwd || bp->opt_md != O_YES )
		bp->opt_cr = O_NO;

	snprintf( tmp, 128, "%s%s%s%s%s%s",
		( bp->opt_nr & O_WANT ) ? " NR" : "",
		( bp->opt_nd & O_THEY ) ? " ND" : "",
		( bp->opt_mb & O_WANT ) ? " MB" : "",
		( bp->opt_cht & O_WANT ) ? " CHAT" : "",
		(( !( bp->opt_nd & O_WE )) != ( !( bp->opt_nd & O_THEY ))) ? " NDA": "",
		(( bp->opt_cr & O_WE ) && ( bp->opt_cr & O_THEY )) ? " CRYPT" : "" );
	if ( strlen( tmp ))
		msgs( BPM_NUL, "OPT%s", tmp );

	msgs( BPM_NUL, "TRF %lu %lu", totalm, totalf );
	msgs( BPM_OK, "%ssecure", have_pwd ? "" : "non-" );

	return binkp_hsdone( bp );
}


/*
 * Process M_OK command
 */
static int M_ok(BPS *bp, byte *arg)
{
	char *buf = (char *) arg;

	DEBUG(('B',3,"OK %s", buf));

	if ( !bp->to ) {
		DEBUG(('B',1,"unexpected M_OK (%s) from remote on incoming call", buf));
		bp->rc = S_FAILURE;
		return 0;
	}

	buf = skip_blanks( buf );

	if (( rnode->options & O_PWD ) && buf ) {
		char *t;

		while(( t = strsep( &buf, " \t" )))
			if ( strcmp( t, "non-secure" ) == 0 ) {
				bp->opt_cr = O_NO;
				rnode->options &= ~(O_PWD);
				break;
			}
	}

	return binkp_hsdone( bp );
}


/*
 * Process M_EOB command
 */
static int M_eob(BPS *bp, byte *arg)
{
	DEBUG(('B',3,"EOB"));

	/*
	if ( chattimer && bp->sent_eob==2 && bp->recv_eob==2 ) {
		bp->opt_cht = O_NO;
	}
	*/

	if ( bp->recv_file ) {
		rxclose( &rxfd, FOP_OK );
		bp->recv_file = 0;
		qpfrecv();
	}

	bp->recv_eob = 1;
	bp->delay_eob = 0;

	if ( !bp->oflist && bp->nofiles ) {
		for( bp->oflist = fl; bp->oflist && !bp->oflist->sendas; bp->oflist = bp->oflist->next );

		if ( bp->oflist ) {
			bp->oflist = fl;
			bp->nofiles = 0;
		}
	}

	DEBUG(('B',4,"cls=%d, sent_eob=%d, recv_eob=%d", bp->cls, bp->sent_eob, bp->recv_eob));

	return 1;
}


/*
 * Process M_ERR command
 */
static int M_err(BPS *bp, byte *arg)
{
	char *buf = (char *) arg;

	DEBUG(('B',3,"ERR %s", buf));
	write_log("Binkp error: \"%s\"", buf );
    
	ERR_CLOSE( bp );
	bp->rc = ( bp->to ? S_REDIAL | S_ADDTRY : S_BUSY );

	return 0;
}


/*
 * Process M_BSY command
 */
static int M_bsy(BPS *bp, byte *arg)
{
	char *buf = (char *) arg;
    
	DEBUG(('B',3,"BSY %s", buf));
	write_log( "Binkp busy: \"%s\"", buf );

	ERR_CLOSE( bp );
	bp->rc = ( bp->to ? S_REDIAL | S_ADDTRY : S_BUSY );

	return 0;
}


/*
 * Process M_GET command
 */
static int M_get(BPS *bp, byte *arg)
{
	char	*buf = (char *) arg, *fname;
	long	fsize, ftime, foffs;

	DEBUG(('B',3,"GET %s", buf));
    
	if ( file_parse( buf, &fname, &fsize, &ftime, &foffs )) {
		if ( bp->send_file && sendf.fname
			&& !strncasecmp( fname,sendf. fname, MAX_PATH)
			&& sendf.mtime == ftime && sendf.ftot == fsize )
		{
			if ( fseeko( txfd, foffs, SEEK_SET ) < 0 ) {
				write_log( "can't send file from requested offset" );
				msgs( BPM_ERR, "Can't send file from requested offset" );
				txclose( &txfd, FOP_ERROR );
				bp->send_file = 0;
			} else {
				sendf.soff = sendf.foff = bp->txpos = foffs;
				bp->wait_for_get = 0;
				msgs( BPM_FILE, "%s %lu %ld %lu", sendf.fname, (long) sendf.ftot,
					sendf.mtime, (long) foffs );
			}
		} else
			write_log( "Binkp: got M_GET for unknown file" );
	} else
		DEBUG(('B',1,"got unparsable fileinfo"));

	return 1;
}


/*
 * Process M_GOT/M_SKIP commands
 */
static int M_gotskip(BPS *bp, byte *arg)
{
	char 	*buf = (char *) arg;
	char 	*fname;
	int 	id = *bp->rx_buf;
	long 	fsize, ftime;

	DEBUG(('B',3,"%s %s", mess[id], buf));

	if ( file_parse( buf, &fname, &fsize, &ftime, NULL )) {
        	if ( sendf.fname && !strncasecmp( fname, sendf.fname, MAX_PATH )
			&& sendf.mtime == ftime && sendf.ftot == fsize )
		{
			if ( bp->send_file ) {
				DEBUG(('B',1,"file %s %s",
					sendf.fname,
					( id == BPM_GOT ) ? "skipped" : "suspended"));

				txclose( &txfd, ( id == BPM_GOT ) ? FOP_SKIP : FOP_SUSPEND );
				bp->ticskip = ( id == BPM_GOT ) ? 1 : 2;
				bp->oflist->suspend = ( id == BPM_SKIP );
				flexecute( bp->oflist );
				bp->send_file = 0;
				qpfsend();
				return 1;
			}
        
			if ( bp->wait_got ) {
				DEBUG(('B',1,"file %s %s",
					sendf.fname,
					( id == BPM_GOT ) ? "done" : "suspended"));
				
				bp->wait_got = 0;
				txclose( &txfd, ( id == BPM_GOT ) ? FOP_OK : FOP_SUSPEND );
            
				bp->ticskip = ( id == BPM_GOT ) ? 0 : 2;
				bp->oflist->suspend = ( id == BPM_SKIP );
				flexecute( bp->oflist );
				qpfsend();
			} else
				write_log( "Binkp: got M_%s for unknown file", mess[id] );
		}
#if 0        
        if ( bp->send_file && sendf.fname && !strncasecmp( fname, sendf.fname, MAX_PATH )
            && sendf.mtime == ftime && sendf.ftot == fsize ) {

            DEBUG(('B',1,"file %s %s", sendf.fname, ( id == BPM_GOT ) ? "skipped" : "suspended"));
            txclose( &txfd, ( id == BPM_GOT ) ? FOP_SKIP : FOP_SUSPEND );
            if ( id == BPM_GOT ) {
                flexecute( bp->oflist );
                bp->ticskip = 1;
            } else
                bp->ticskip = 2;
            bp->send_file = 0;
            qpfsend();
            return 1;
        }

        if ( bp->wait_got && sendf.fname && !strncasecmp( fname, sendf.fname, MAX_PATH )
            && sendf.mtime == ftime && sendf.ftot == fsize ) {

            DEBUG(('B',1,"file %s %s", sendf.fname, ( id == BPM_GOT ) ? "done" : "suspended"));
            bp->wait_got = 0;
            txclose( &txfd, ( id == BPM_GOT ) ? FOP_OK : FOP_SUSPEND );
            
            if ( id == BPM_GOT ) {
                flexecute( bp->oflist );
                bp->ticskip = 0;
            } else
                bp->ticskip = 2;
            qpfsend();
        } else
            write_log( "Binkp: got M_%s for unknown file", mess[id] );
#endif
	} else
		DEBUG(('B',1,"got unparsable fileinfo"));
	return 1;
}


/*
 * Process M_CHAT command
 */
static int M_chat(BPS *bp, byte *arg)
{
	DEBUG(('B',3,"CHAT"));
	if ( bp->opt_cht == O_YES )
		binkp_devrecv( arg );
	else
		DEBUG(('B',1,"got chat msg with disabled chat"));
	return 1;
}

static int M_template(BPS *bp, byte *arg)
{
	/* char *buf = (char *) arg; */
	return 0;
}

typedef int M_cmd(BPS *bp, byte *arg);

static M_cmd *M_cmds[] = {
    M_nul, M_adr, M_pwd, M_file, M_ok, M_eob, M_gotskip,
    M_err, M_bsy, M_get, M_gotskip, M_template, M_chat
};


static int binkp_send(BPS *bp)
{
	byte	i;
	int	rc = 0;

	DEBUG(('B',4,"send-> nmsgs: %d, tx_ptr: %d, tx_left: %d",
		bp->nmsgs, bp->tx_ptr, bp->tx_left));

	if ( bp->tx_left == 0 ) {			/* tx buffer is empty */
		bp->tx_ptr = bp->tx_left = 0;

		if ( bp->nmsgs ) {			/* there are unsent messages */
			for( i = 0; i < bp->nmsgs; i++ ) {
				if ( bp->mqueue[i].msg ) {
					if ( (unsigned int) (bp->mqueue[i].len + bp->tx_left) > MAX_BLKSIZE )
						break;

					memcpy( (void *) (bp->tx_buf + bp->tx_left),
						bp->mqueue[i].msg, bp->mqueue[i].len );
					bp->tx_left += bp->mqueue[i].len;
					xfree( bp->mqueue[i].msg );
					DEBUG(('B',3,"put msg to tx buf, %d", bp->mqueue[i].len));
				}
			}

			if ( i >= bp->nmsgs ) {		/* all messages were put in tx buf */
				xfree( bp->mqueue );
				bp->nmsgs = 0;
			}
		} else if ( bp->send_file && txfd && !bp->wait_for_get ) {
			int blksz = MIN( BP_BLKSIZE, sendf.ftot - bp->txpos );

			if (( rc = fread( bp->tx_buf + BLK_HDR_SIZE, 1, blksz, txfd )) < 0 ) {
				sline("Binkp: file read error at pos %lu", bp->txpos);
				DEBUG(('B',1,"Binkp: file read error at pos %lu", bp->txpos));
				txclose( &txfd, FOP_ERROR );
				bp->send_file = 0;
			} else {
				DEBUG(('B',2,"read %d bytes of %d (data)", rc, blksz));
				if ( rc ) {
					mkheader( bp->tx_buf, rc );
					if ( bp->opt_cr == O_YES )
						encrypt_buf( (void *) bp->tx_buf, (size_t) (rc + BLK_HDR_SIZE), bp->keys_out );
					bp->txpos += rc;
					sendf.foff += rc;
					bp->tx_left = rc + BLK_HDR_SIZE;
					qpfsend();
				}
				
				if ( rc < BP_BLKSIZE && bp->txpos == sendf.ftot ) {
					bp->wait_got = 1;
					bp->send_file = 0;
				}
			}
		}
	} else {

		DEBUG(('B',4,"send: tx_ptr: %d, tx_left: %d", bp->tx_ptr, bp->tx_left));
		rc = send( tty_fd, bp->tx_buf + bp->tx_ptr, bp->tx_left, 0 );

		bp->saved_errno = errno;

		if ( rc < 0 ) {
			if ( EWBOEA() )
				return 1;
			else {
				bp->error = 1;
				return 0;
			}
		}

		DEBUG(('B',3,"sent: rc=%d (%d)",rc,bp->tx_left,CRYPT(bps)));

		bp->tx_ptr += rc;
		bp->tx_left -= rc;
	}

	return 1;
}


/*
 * Store received data to file
 */
static int store_data(BPS *bp)
{
	char	tmp[512];
	int	n;

	DEBUG(('B',4,"got: data %d",bp->rx_size));
	DEBUG(('B',4,"data: rxpos %lu, recvf.foff %lu", (long) bp->rxpos, (long) recvf.foff ));

	snprintf( tmp, 511, "%s %lu %lu", bp->rfname, (long) recvf.ftot, bp->rmtime );

	if ( rxstatus ) {
		msgs(( rxstatus == RX_SUSPEND ) ? BPM_SKIP : BPM_GOT, tmp );
		rxclose( &rxfd, ( rxstatus == RX_SUSPEND ) ? FOP_SUSPEND : FOP_SKIP );
		bp->recv_file = 0;
		return 1;
	}

	if (( n = fwrite( bp->rx_buf, 1, bp->rx_size, rxfd )) < 0 ) {
		bp->recv_file = 0;
		sline( "Binkp: file write error" );
		write_log( "can't write to %s, suspended", recvf.fname );
		rxclose( &rxfd, FOP_ERROR );
		msgs( BPM_SKIP, tmp );
	} else {
		bp->rxpos += bp->rx_size;
		recvf.foff += bp->rx_size;
		DEBUG(('B',5,"data -> rxpos: %lu, recvf.foff: %lu", (long) bp->rxpos, (long) recvf.foff ));

		qpfrecv();
        
		if ( bp->rxpos > (int) recvf.ftot ) {
			bp->recv_file = 0;
			write_log( "Binkp: got too many data (%lu of %lu expected)",
				(long) bp->rxpos, (long) recvf.ftot );
			rxclose( &rxfd, FOP_SUSPEND );
			msgs( BPM_SKIP, tmp );
		} else if ( bp->rxpos == (int) recvf.ftot) {
			bp->recv_file = 0;
			if ( rxclose( &rxfd, FOP_OK ) != FOP_OK ) {
				msgs( BPM_SKIP, tmp );
				sline( "Binkp: file close error" );
				write_log( "can't close %s, suspended", recvf.fname );
			} else {
				msgs( BPM_GOT, tmp );
			}
			qpfrecv();
		}
	}

	return 1;
}


#define HMD(_x_) ( _x_->is_msg ? "msg" : "data" )

static int binkp_recv(BPS *bp)
{
	int	rc = BPM_NONE, readsz;
	int	blksz = bp->rx_size == -1 ? BLK_HDR_SIZE : bp->rx_size;

	DEBUG(('B',4,"recv-> blksz: %d, rx_ptr: %d, rx_size: %d", blksz, bp->rx_ptr, bp->rx_size));

	if ( blksz == 0 )
		readsz = 0;
	else {
		getevt();

		readsz = recv( tty_fd, &bp->rx_buf[bp->rx_ptr], (size_t) (blksz - bp->rx_ptr), 0 );

		bp->saved_errno = errno;

		if ( readsz < 0 ) {
			if ( EWBOEA() )
				return 1;
			else {
				bp->error = 1;
				return 0;
			}
		} else if ( readsz == 0 ) {
			write_log( "rcvd: connection closed by remote host" );
			bp->error = -2;
			return 0;
		}

		if ( bp->opt_cr == O_YES )
			decrypt_buf( (void *) &bp->rx_buf[bp->rx_ptr], (size_t) readsz, bp->keys_in );
	}

	bp->rx_ptr += readsz;
	DEBUG(('B',3,"rcvd: %d bytes", readsz ));

	if ( bp->rx_ptr == blksz ) {			/* Received complete block */
		if ( bp->rx_size == -1 ) {		/* Header */
			bp->is_msg = ((byte) *bp->rx_buf) >> 7;
			bp->rx_size = ((bp->rx_buf[0] & 0x7f) << 8 ) + bp->rx_buf[1];
			bp->rx_ptr = 0;
			DEBUG(('B',2,"rcvd header: %d (%s)", bp->rx_size, HMD(bp)));

			if ( bp->rx_size == 0 )
				goto ZeroLen;

			rc = 1;
		} else {				/* Next block */
ZeroLen:
			DEBUG(('B',2,"rcvd block: %d/%d (%s)", bp->rx_size, blksz, HMD(bp)));

			if ( bp->is_msg ) {
				bp->mib++;
                
				if ( bp->rx_size == 0 ) {	/* Handle zero length block */
					DEBUG(('B',1,"Zero length msg - dropped"));
					bp->rx_size = -1;
					bp->rx_ptr = 0;
					return 1;
				}

				rc = *bp->rx_buf;
				bp->rx_buf[bp->rx_ptr] = 0;

				if ( rc > BPM_MAX ) {
					DEBUG(('B',1,"Unknown msg: %d%s, skipped",rc,CRYPT(bps)));
					rc = 1;
				} else {
					DEBUG(('B',2,"rcvd %s '%s'%s",mess[rc],bp->rx_buf + 1,CRYPT(bps)));
					rc = M_cmds[rc]( bps, bp->rx_buf + 1 );
				}
			} else {
				DEBUG(('B',2,"rcvd data, len=%d%s", bp->rx_size, CRYPT(bps)));
				if ( bp->recv_file) {
					rc = store_data( bps );
				} else {
					DEBUG(('B',1,"Ignore received data block"));
					rc = 1;
				}
			}

			bp->rx_ptr = 0;
			bp->rx_size = -1;
		}
	} else
		rc = 1;
    
	return rc;
}
#undef HMD


static void binkp_hs(BPS *bp)
{
	char		tmp[512];
	time_t		tt;
	struct tm	*tm;
	int		offset, negative;
	falist_t	*pp = NULL;
	ftnaddr_t	*ba = NULL;

	DEBUG(('B',1,"Binkp handshake"));

	if ( !bp->to && bp->opt_md ) {
		bp->MD_chal = md5_challenge( NULL );
	}

	if ( !bp->to && bp->MD_chal && ( bp->opt_md & O_WANT )) {
		char chall[129];
		bin2strhex((unsigned char *) chall, bp->MD_chal + 1, bp->MD_chal[0] );
		msgs( BPM_NUL, "OPT CRAM-MD5-%s", chall );
	}

	recode_to_remote( cfgs( CFG_STATION ));
	msgs( BPM_NUL, "SYS %s", ccs );

	recode_to_remote( cfgs( CFG_SYSOP ));
	msgs( BPM_NUL, "ZYZ %s", ccs );

	recode_to_remote( cfgs( CFG_PLACE ));
	msgs( BPM_NUL, "LOC %s", ccs );

	recode_to_remote( cfgs( CFG_FLAGS ));
	msgs( BPM_NUL, "NDL %d,%s", cfgi( CFG_SPEED ), ccs );

	recode_to_remote( cfgs( CFG_PHONE ));
	if ( ccs && *ccs )
		msgs( BPM_NUL, "PHN %s", ccs );

	tt = time( NULL );
	tm = localtime( &tt );
	offset = gmtoff( tt );
	if ( offset < 0 ) {
		negative = 1;
		offset = -offset;
	} else
		negative = 0;

	msgs( BPM_NUL, "TIME %s, %2d %s %04d %02d:%02d:%02d %c%02d%02d",
		weekday_names[tm->tm_wday],
		tm->tm_mday,
		month_names[tm->tm_mon],
		tm->tm_year + 1900,
		tm->tm_hour, tm->tm_min, tm->tm_sec,
		negative ? '-' : '+', offset / 3600, offset % 3600 );

	snprintf( tmp, 511 , "%s-%s/%s", progname, version, osname );
	strtr( tmp, ' ', '-' );
	msgs( BPM_NUL, "VER %s %s", tmp, BP_PROT "/" BP_VERSION );

	if ( bp->to ) {
		msgs( BPM_NUL, "OPT NDA%s%s%s%s%s",
			( bp->opt_nr  & O_WANT ) ? " NR"    : "",
			( bp->opt_nd  & O_THEY ) ? " ND"    : "",
			( bp->opt_mb  & O_WANT ) ? " MB"    : "",
			( bp->opt_cr  & O_WE   ) ? " CRYPT" : "",
			( bp->opt_cht & O_WANT ) ? " CHAT"  : "" );
	}

	pp = cfgal( CFG_ADDRESS );
	if ( bp->to ) {
		ba = akamatch( bp->remaddr, pp );
		xstrcpy( tmp, ftnaddrtoda( ba ), 511 );
	} else {
		xstrcpy( tmp, ftnaddrtoda( &pp->addr ), 511 );
		pp = pp->next;
		ba = NULL;
	}

	for( ; pp; pp = pp->next )
		if ( &pp->addr != ba ) {
			xstrcat( tmp, " ", 511 );
			xstrcat( tmp, ftnaddrtoda( &pp->addr ), 511 );
		}

	msgs( BPM_ADR, tmp );
}


int binkpsession(int mode, ftnaddr_t *remaddr)
{
	int		rc = 1, n = 0;
	boolean		rd, wd;
	struct timeval	tv;

	if ( binkp_init( mode ) != OK ) {
		binkp_deinit();
		if ( mode )
			return (S_REDIAL | S_ADDTRY);
		else
			return S_FAILURE;
	}

	bps->remaddr = remaddr;
	xfree( rnode->phone );

	rxstatus = 0;
	totaln = totalf = totalm = 0;
	got_req = 0;
	receive_callback = receivecb;

	write_log( "starting %sbound Binkp session", mode ? "out" : "in" );
	sline( "Binkp handshake" );

	binkp_hs( bps );

	while ( 1 ) {

		DEBUG(('B',3,"init: %d, send_file: %d, sent_eob: %d, txfd: %.8p, nofiles: %d",
			bps->init, bps->send_file, bps->sent_eob, txfd, bps->nofiles));

		if ( !bps->init && !bps->send_file && !bps->sent_eob && !txfd && !bps->nofiles ) {
			DEBUG(('B',2,"finding files"));
			DEBUG(('B',3,"oflist: %.8p, fl: %.8p", bps->oflist, fl));

			if ( bps->oflist && bps->oflist != fl )
				bps->oflist = bps->oflist->next;

			if ( bps->oflist )
				rc = cfgi( CFG_AUTOTICSKIP ) ? bps->ticskip : 0;

			while( bps->oflist && !txfd ) {
				if ( !bps->oflist->sendas 
					|| ( rc && istic( bps->oflist->tosend ))
					|| !( txfd = txopen( bps->oflist->tosend, bps->oflist->sendas )))
				{
					if ( rc && istic( bps->oflist->tosend )) {
						write_log( "tic file '%s' auto-%sed", bps->oflist->tosend,
							rc == 1 ? "skipp" : "suspend" );
						bps->oflist->suspend = ( rc == 2 );
					}
					flexecute( bps->oflist );
					bps->oflist = bps->oflist->next;
					rc = 0;
				}
			}

			bps->ticskip = 0;
            
			if ( bps->oflist && txfd ) {
				DEBUG(('B',1,"found: %s",sendf.fname));

				bps->send_file = 1;
				bps->txpos = ( bps->wait_for_get = ( bps->opt_nr & O_WE )) ? -1 : 0;
				msgs( BPM_FILE, "%s %lu %lu %ld", sendf.fname, (long) sendf.ftot, sendf.mtime, bps->txpos );
                
				if ( bps->wait_for_get )
					DEBUG(('B',2,"waiting for M_GET"));
				bps->sent_eob = 0;
				bps->cls = 1;
			} else
				bps->nofiles = 1;
		}
        
		if ( !bps->init && bps->nofiles && !bps->wait_got && !bps->sent_eob && !bps->delay_eob ) {
			msgs( BPM_EOB, NULL );
			bps->sent_eob = 1;
		}

		bps->rc = S_OK;

		if ( bps->sent_eob && bps->recv_eob ) {
			DEBUG(('B',4,"mib=%d", bps->mib));
			if ( bps->mib < 3 || BP_VER( bps ) <= 100 )
				break;
			bps->mib = bps->sent_eob = bps->recv_eob = 0;
		}

		wd = ( bps->nmsgs || bps->tx_left || ( bps->send_file && txfd && !bps->wait_for_get));
		rd = TRUE;
		tv.tv_sec = BP_TIMEOUT;
		tv.tv_usec = 0;
		DEBUG(('B',4,"select: r=%d, w=%d, sec=%d, usec=%d", rd, wd, tv.tv_sec, tv.tv_usec));
		rc = tty_select( &rd, &wd, &tv );
		bps->saved_errno = errno;
		DEBUG(('B',4,"select done: %d (r=%d, w=%d)",rc, rd, wd));

		bps->rc = ( bps->to ? S_REDIAL | S_ADDTRY : S_BUSY );
		if ( tty_gothup ) {
			ERR_CLOSE( bps );
			bps->error = -2;
			break;
		} else if ( rc <= 0 ) {
			ERR_CLOSE( bps );
			bps->error = ( rc == 0 ) ? -1 : 1;
			break;
		}

		if ( rd && !binkp_recv( bps ))
			break;

		if ( wd && !binkp_send( bps ))
			break;

		check_cps();
	}
    
	if ( chattimer && bps->sent_eob && bps->recv_eob ) {
		DEBUG(('S',3,"Binkp chat autoclosed (%s)", timer_expired(chattimer) ? "timeout" : "hangup" ));
		msgs( BPM_EOB, NULL );
	}

	rc = bps->rc;
	if ( bps->error == -1 )
		write_log( "timeout" );
	else if ( bps->error > 0 )
		write_log( "%s", strerror( bps->saved_errno ));

	flkill( &fl, rc == S_OK );

	DEBUG(('B',1,"session done, rc=%d", rc));

	while( !bps->error ) {
		if (( n = recv( tty_fd, bps->rx_buf, MAX_BLKSIZE, 0 )) == 0 )
			break;
		if ( n < 0 ) {
			if ( !EWBOEA() )
				bps->error = 1;
			break;
		}
		DEBUG(('B',4,"Purged %d bytes from input stream",n));
	}
        
	while( !bps->error && ( bps->nmsgs || bps->tx_left ) && binkp_send( bps ));
    
	binkp_deinit();

	return rc;
}

#endif
