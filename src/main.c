/**********************************************************
 * qico's main module.
 **********************************************************/
/*
 * $Id: main.c,v 1.24 2005/08/11 19:15:14 mitry Exp $
 *
 * $Log: main.c,v $
 * Revision 1.24  2005/08/11 19:15:14  mitry
 * Changed code a bit
 *
 * Revision 1.23  2005/05/17 19:49:56  mitry
 * Included modem.h
 *
 * Revision 1.22  2005/05/17 18:17:42  mitry
 * Removed system basename() usage.
 * Added qbasename() implementation.
 *
 * Revision 1.21  2005/05/16 20:31:40  mitry
 * Changed code a bit
 *
 * Revision 1.20  2005/05/06 20:21:55  mitry
 * Changed nodelist handling
 *
 * Revision 1.19  2005/04/07 12:59:10  mitry
 * Removed mallopt call
 *
 * Revision 1.18  2005/04/05 09:34:54  mitry
 * Removed commented out code
 *
 * Revision 1.17  2005/03/31 19:40:38  mitry
 * Update function prototypes and it's duplication
 *
 * Revision 1.16  2005/03/28 16:25:40  mitry
 * Added non-blocking i/o support
 *
 * Revision 1.15  2005/02/25 16:29:07  mitry
 * Another fixing hangup after session
 *
 * Revision 1.14  2005/02/23 21:34:28  mitry
 * Changed checking of CARRIER() to tty_online
 *
 * Revision 1.13  2005/02/22 16:38:42  mitry
 * Added experimental mallopt call to prevent seg fault
 * on buggy glibc library
 *
 * Revision 1.12  2005/02/22 15:47:49  mitry
 * Added 'run in foreground' mode
 *
 * Revision 1.11  2005/02/21 20:58:07  mitry
 * Added tty_online=TRUE on incoming call
 *
 * Revision 1.10  2005/02/19 21:32:06  mitry
 * Changed setlocale setting to "C"
 * Removed superfluous strncasecmp in session detection
 *
 * Revision 1.9  2005/02/18 20:28:08  mitry
 * Changed hangup() to modem_hangup(), stat_collect() to modem_stat_collect()
 *
 * Revision 1.8  2005/02/12 18:59:39  mitry
 * Fixing hangup after session.
 *
 */

#include "headers.h"
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#include <sys/utsname.h>

#if defined(__linux__)
#include <malloc.h>
#endif

#include "clserv.h"
#include "tty.h"
#include "modem.h"
#include "tcp.h"
#include "nodelist.h"


char progname[] = PACKAGE_NAME;

#define __CLS_CONN \
        log_callback = NULL;                                              \
        xsend_cb = NULL;                                                  \
        ssock = cls_conn( CLS_LINE, cfgs( CFG_SERVER ), NULL );           \
        if( ssock < 0)                                                    \
            write_log( "can't connect to server: %s", strerror( errno )); \
        else                                                              \
            log_callback = vlogs;

#define __OUT_INIT \
        rc = outbound_init( cfgs( CFG_ASOOUTBOUND ), cfgs( CFG_BSOOUTBOUND ), \
            cfgs( CFG_QSTOUTBOUND ), cfgal( CFG_ADDRESS )->addr.z );          \
        if ( !rc ) {                                                          \
            write_log("No outbound defined");                                 \
            stopit( S_FAILURE );                                              \
        }


static void usage(char *ex)
{
    printf( "usage: %s [<options>] [<node>]\n", ex );
    puts( "<node>       must be in ftn-style (i.e. zone:net/node[.point])!\n"
          "-h           this help screen\n"
          "-I<config>   override default config" );
    puts( "-d           start in daemon (originate) mode\n"
          "-f           start in foreground (originate) mode\n"
          "-a<type>     start in answer mode with <type> session, type can be:\n"
          "                 auto           - autodetect\n"
          "                 emsi, **emsi, or\n"
          "                 **EMSI_INQC816 - EMSI session without init phase" );
#ifdef WITH_BINKP
    puts( "                 binkp          - Binkp session" );
#endif
    puts( "-i<host>     start TCP/IP connection to <host> (node must be specified!)\n"
          "-c[N|I|A]    force call to <node>\n"
          "             N - normal call\n"
          "             I - call <i>mmediately (don't check node's worktime)" );
    puts( "             A - call on <a>ny free port (don't check cancall setting)\n"
          "             You could specify line after <node>, lines are numbered from 1" );
#ifdef WITH_BINKP
    puts( "-b           call with Binkp (default call ifcico)" );
#endif
    puts( "-n           compile nodelists\n"
          "-t|T         check config file for errors, T is more verbose\n"
          "-v           show version\n" );
    exit(0);
}


void stopit(int rc)
{
    vidle();
    qqreset();
    IFPerl( perl_done( 0 ));
    log_done();
    log_init( cfgs( CFG_LOG ), rnode->tty );
    write_log( "exiting with rc=%d", rc );
    log_done();
    cls_close( ssock );
    cls_shutd( lins_sock );
    cls_shutd( uis_sock );
    /*
    killconfig();
    */

    exit( rc );
}


RETSIGTYPE sigerr(int sig)
{
	signal( sig, SIG_DFL );
	outbound_done();
	write_log( "got SIG%s signal", sigs[sig] );

	if ( cfgs( CFG_PIDFILE ) && getpid() == islocked( ccs ))
		lunlink( ccs );

	IFPerl( perl_done( 1 ));
	log_done();

	if ( tty_online ) {
		if ( is_ip )
			tcp_done();
		else
			modem_done();
	}

	qqreset();
	sline( "" );
	title( "" );
	cls_close( ssock);
	cls_shutd( lins_sock );
	cls_shutd( uis_sock );

	switch( sig ) {
	case SIGSEGV:
	case SIGFPE:
	case SIGBUS:
	case SIGABRT:
		abort();
	default:
		exit( 1 );
	}
}


static void getsysinfo(void)
{
    struct utsname uts;
    char tmp[ MAX_STRING + 1 ];

    if ( uname( &uts ))
        return;

    snprintf( tmp, MAX_STRING, "%s-%s (%s)", uts.sysname, uts.release, uts.machine );
    osname = xstrdup( tmp );
}


static void answer_mode(int type)
{
    int rc, spd;
    char *cs;
    char host[ MAXHOSTNAMELEN + 1 ];
    struct sockaddr_storage sa;
    socklen_t ss = sizeof( sa );
    TIO tio;
    sts_t sts;

    if ( cfgs( CFG_ROOTDIR ) && ccs[0] )
        chdir( ccs );

    rnode = xcalloc( 1, sizeof( ninfo_t ));
    is_ip = !isatty( 0 );

    xstrcpy( ip_id, "ipline", 10 );
    rnode->tty = xstrdup( is_ip ? ( bink ? "binkp" : "tcpip" ) : qbasename( ttyname( 0 )));
    rnode->options |= O_INB;

    if ( !log_init( cfgs( CFG_LOG ), rnode->tty )) {
        printf( "can't open log %s!\n", ccs );
        exit( S_FAILURE );
    }

    signal( SIGINT, SIG_IGN );
    signal( SIGTERM, sigerr );
    signal( SIGSEGV, sigerr );
    signal( SIGFPE, sigerr );
    signal( SIGPIPE, SIG_IGN );
    IFPerl( perl_init( cfgs( CFG_PERLFILE ), 0 ));

    __CLS_CONN

    __OUT_INIT

    write_log( "answering incoming call" );
    
    vidle();
    
    tty_fd = 0;
    tty_online = TRUE;
    if( is_ip && getpeername( 0, (struct sockaddr*) &sa, &ss ) == 0 ) {
        get_hostname( &sa, ss, host, sizeof( host ));
        write_log( "remote is %s", host );
        spd = TCP_SPEED;
        tcp_setsockopts( tty_fd );
    } else {
        cs = getenv( "CONNECT" );
        spd = cs ? atoi( cs ) : 0;
        xfree( connstr );
        connstr = xstrdup( cs );
        if ( cs && spd )
            strcpy( host, cs );
        else {
            strcpy( host, "Unknown" );
            spd = DEFAULT_SPEED;
        }
        write_log( "*** CONNECT %s", host );
        if (( cs = getenv( "CALLER_ID" )) && strcasecmp( cs, "none" ) && strlen( cs ) > 3 )
            write_log( "caller-id: %s", cs );

        tio_get( tty_fd, &tty_stio );
        memcpy( &tio, &tty_stio, sizeof( TIO ));

        DEBUG(('M',4,"answering: tio_get_speed %d", tio_get_speed( &tio )));

        tio_raw_mode( &tio );
        tio_local_mode( &tio, FALSE );
        /* tio_set_speed( &tio, spd ); */

        if ( tio_set( tty_fd, &tio ) == ERROR ) {
            DEBUG(('M',1,"answering: tio_set error - %s", strerror( errno )));
        }
        fd_set_nonblock( tty_fd, true );
    }

    if ( !tty_gothup )
        rc = session( SM_INBOUND, type, NULL, spd );

    sline( "Hanging up..." );
    if ( is_ip )
        tcp_done();
    else {
        modem_done();
    }

    if(( S_OK == ( rc & S_MASK )) && cfgi( CFG_HOLDONSUCCESS )) {
        log_done();
        log_init( cfgs( CFG_MASTERLOG ), NULL );
        outbound_getstatus( &rnode->addrs->addr, &sts );
        sts.flags |= ( Q_WAITA | Q_WAITR | Q_WAITX );
        sts.htime = MAX( timer_set( cci * 60 ), sts.htime );
        write_log( "calls to %s delayed for %d min after successful incoming session",
            ftnaddrtoa( &rnode->addrs->addr ), cci );
        outbound_setstatus( &rnode->addrs->addr, &sts );
        log_done();
        log_init( cfgs( CFG_LOG ), rnode->tty );
    }

    title( "Waiting..." );
    vidle();
    sline( "" );
    outbound_done();
    stopit( rc );
}

int main(int argc, char **argv, char **envp)
{
    int c, daemon = DM_NONE, rc, sesstype = SESSION_EMSI,
	        line = 0, call_flags = FC_NORMAL;
    char *hostname = NULL, *str = NULL;

    FTNADDR_T( fa );

    initsetproctitle( argc, argv, envp );

#ifdef HAVE_SETLOCALE
    setlocale(LC_ALL, "C");
#endif

    while( (c = getopt( argc, argv, "hI:dfa:ni:c:tTbv" )) != EOF ) {
        switch(c) {
            case 'c':
                daemon = DM_CALL;
                str = optarg;
                while( str && *str ) {
                    switch( tolower( *str )) {
                        case 'n':
                            call_flags = FC_NORMAL; break;
                        case 'i':
                            call_flags |= FC_IMMED; break;
                        case 'a':
                            call_flags |= FC_ANY; break;
                        default:
                            write_log( "unknown call option: %c", *optarg );
                            exit( S_FAILURE );
                    }
                    str++;
                }
                break;

            case 'i':
                hostname = xstrdup( optarg );
                break;

            case 'I':
                configname = xstrdup( optarg );
                break;

            case 'f':
                detached = FALSE;

            case 'd':
                daemon = DM_DAEMON;
                break;

            case 'a':
                daemon = DM_ANSWER;
                sesstype = SESSION_AUTO;
                if ( !strncasecmp( optarg, "**emsi", 6 )
                    || !strncasecmp( optarg, "emsi", 4 ))
                    sesstype = SESSION_EMSI;
#ifdef WITH_BINKP
                else if ( !strncasecmp( optarg, "binkp", 5)) {
                    sesstype = SESSION_BINKP;
                    bink = 1;
                }
                break;

            case 'b':
                bink = 1;
#endif
                break;

            case 'n':
                daemon = DM_NODELIST;
                break;

            case 'T':
                verbose_config = 1;

            case 't':
                daemon = DM_CONFIG;
                break;

            case 'v':
                u_vers( progname );
                break;

            default:
                usage( argv[0] );
        }
    }

    if ( !hostname && daemon <= DM_NONE )
        usage( argv[0] );

    getsysinfo();
    ssock = lins_sock = uis_sock = -1;

    rereadconfig( -1 );
/*
    if ( !rereadconfig( -1 )) {
        write_log( "there were some errors parsing '%s', aborting.", configname );
        exit( S_FAILURE );
    }

    if( !log_init( cfgs( CFG_MASTERLOG ), NULL )) {
        write_log( "can't open master log '%s', aborting.", ccs );
        exit( S_FAILURE );
    }
*/

    log_done();

    if ( daemon == DM_CONFIG ) {
        puts( "Config file is OK" );
        exit( S_OK );
    }

    if ( hostname || daemon == DM_CALL ) {
        if ( !parseftnaddr( argv[optind], &fa, &DEFADDR, 0 )) {
            write_log( "can't parse address '%s', aborting.", argv[optind] );
            exit( S_FAILURE );
        }
        optind++;
    }

    if ( hostname ) {
        is_ip = 1;
        rnode = xcalloc( 1, sizeof( ninfo_t ));
        xstrcpy( ip_id, "ipline", 10 );
        rnode->tty = xstrdup( bink ? "binkp" : "tcpip" );
        
        if( !log_init( cfgs( CFG_LOG ), rnode->tty )) {
            write_log( "can't open log '%s', aborting.", ccs );
            exit( S_FAILURE );
        }

        __CLS_CONN

        __OUT_INIT

        if ( outbound_locknode( &fa, LCK_c )) {
            signal( SIGINT, sigerr );
            signal( SIGTERM, sigerr );
            signal( SIGSEGV, sigerr );
            signal( SIGPIPE, SIG_IGN );
            IFPerl( perl_init( cfgs( CFG_PERLFILE ), 0 ));

            rc = do_call( &fa, hostname, NULL );
            outbound_unlocknode( &fa, LCK_x );
        } else
            rc = S_FAILURE;

        if( rc & S_MASK )
            write_log( "can't call to %s", ftnaddrtoa( &fa ));
        outbound_done();
        stopit( rc );
    }

    if ( daemon == DM_CALL ) {
        if ( optind < argc ) {
            if ( sscanf( argv[optind], "%d", &line ) != 1 ) {
                write_log( "can't parse line number '%s'!\n", argv[optind] );
                exit( S_FAILURE );
            }
        }
        else
            line = 0;

        __CLS_CONN

        __OUT_INIT

        if ( outbound_locknode( &fa, LCK_c )) {
            signal( SIGINT, sigerr );
            signal( SIGTERM, sigerr );
            signal( SIGSEGV, sigerr );
            signal( SIGPIPE, SIG_IGN );
            IFPerl( perl_init( cfgs( CFG_PERLFILE ), 0 ));

            rc = force_call( &fa, line, call_flags );
            outbound_unlocknode( &fa, LCK_x );
        }
        else
            rc = S_FAILURE;

        if( rc & S_MASK )
            write_log( "can't call to %s", ftnaddrtoa( &fa ));
        outbound_done();
        stopit( rc );
    }

    switch( daemon ) {
        case DM_DAEMON:
            daemon_mode();
            break;

        case DM_ANSWER:
            answer_mode( sesstype );
            break;

        case DM_NODELIST:
            nodelist_compile( 1 );
            break;
    }
    return S_OK;
}
