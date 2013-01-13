/**********************************************************
 * client/server tools
 **********************************************************/
/*
 * $Id: clserv.c,v 1.6 2005/08/22 17:12:45 mitry Exp $
 *
 * $Log: clserv.c,v $
 * Revision 1.6  2005/08/22 17:12:45  mitry
 * Close socket fd if didn't make connection
 *
 * Revision 1.5  2005/05/16 11:17:30  mitry
 * Updated function prototypes. Changed code a bit.
 *
 * Revision 1.4  2005/04/04 19:37:49  mitry
 * Changed arg 2 of shutdown()
 *
 */

#include "headers.h"

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include "clserv.h"

#ifdef DEBUG
    #undef DEBUG
#endif
#define DEBUG(p)

xsend_cb_t xsend_cb = NULL;


int cls_conn(int type, const char *port, const char *addr)
{
	int			rc, f = 1;
	struct sockaddr_in	sa;
	struct servent		*se;

	sa.sin_family = AF_INET;

        DEBUG(('I',1,"cls_conn: type=%d, port=%s, addr=%s", type, port, addr));

	if ( port && atoi( port ))
		sa.sin_port = htons( atoi( port ));
	else if( port && ( se = getservbyname( port, (type & CLS_UDP ) ? "udp" : "tcp" )))
		sa.sin_port = se->s_port;
	else if( !port && ( se = getservbyname( "qicoui", ( type & CLS_UDP ) ? "udp" : "tcp" )))
		sa.sin_port = se->s_port;
	else if( !port )
		sa.sin_port = htons( DEF_SERV_PORT );
	else {
		errno = EINVAL;
		return -1;
	}
	
	if ( addr ) {
		int		a1;
		struct hostent	*he;

		if ( sscanf( addr, "%d.%d.%d.%d", &a1, &a1, &a1, &a1 ) == 4 )
			sa.sin_addr.s_addr = inet_addr( addr );
		else if (( he = gethostbyname( addr )))
			memcpy( &sa.sin_addr, he->h_addr, he->h_length );
		else {
			DEBUG(('I',1,"cls_conn: unknown address: %s", addr));
			return -1;
		}
	} else
		sa.sin_addr.s_addr = htonl(( type & CLS_UDP ) ? INADDR_LOOPBACK : INADDR_ANY );

	rc = socket( AF_INET, type & CLS_UDP ? SOCK_DGRAM : SOCK_STREAM, 0 );
    	if ( rc < 0 ) {
		DEBUG(('I',1,"cls_conn: can't create socket: %s", strerror( errno )));
		return rc;
    	}

	if ( type & CLS_SERVER ) {
		if ( setsockopt( rc, SOL_SOCKET, SO_REUSEADDR, &f, sizeof( f )) < 0 ) {
			close( rc );
			return -1;
		}

		f = fcntl( rc, F_GETFL, 0);
		if ( f >= 0 )
			fcntl( rc, F_SETFL, f | O_NONBLOCK );

		if ( bind( rc, (struct sockaddr *) &sa, sizeof( sa )) < 0 ) {
			DEBUG(('I',1,"cls_conn: can't bind address (%s): %s",
				addr, strerror( errno )));
			close( rc );
			return -1;
		}
		if ( !( type & CLS_UDP )) {
			if ( listen( rc, 4 ) < 0) {
				DEBUG(('I',1,"cls_conn: listen error: %s", strerror( errno )));
				close( rc );
				return -1;
			}
			xsend_cb = xsend;
			return rc;
		}
		xsend_cb = xsend;
		return rc;
	}

	if ( connect( rc, (struct sockaddr *) &sa, sizeof( sa )) < 0 ) {
		DEBUG(('I',1,"cls_conn: connect error: %s", strerror( errno )));
		close( rc );
		return -1;
	}
	xsend_cb = xsend;
	return rc;
}


void cls_close(int sock)
{
	if ( sock >= 0 )
		close( sock );
}


void cls_shutd(int sock)
{
#ifdef HAVE_SHUTDOWN
	if ( sock >= 0 )
		shutdown( sock, SHUT_RDWR );
#endif
	cls_close( sock );
}


int xsendto(int sock, const char *buf, size_t len, struct sockaddr *to)
{
	int		rc;
	char		*b;
	unsigned short	l = H2I16( len );

	if ( sock < 0 ) {
		errno = EBADF;
		return -1;
	}
	if ( !len )
		return 0;
	b = xmalloc( len + 2 );
	STORE16( b, l );
	memcpy( b + 2, buf, len );
	rc = sendto( sock, b, len + 2, 0, to, sizeof( struct sockaddr ));
	xfree( b );
	return rc;
}


int xsend(int sock, const char *buf, size_t len)
{
	return xsendto( sock, buf, len, NULL );
}


int xrecv(int sock, char *buf, size_t len, int wait)
{
	unsigned short	l = 0;
	int		rc;
	fd_set		rfd;
	struct timeval	tv;

	if ( sock < 0 ) {
		errno = EBADF;
		return -1;
	}
	
	if ( !wait ) {
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO( &rfd );
		FD_SET( sock, &rfd );
		rc = select( sock + 1, &rfd, NULL, NULL, &tv );
		if ( rc < 1 ) {
			if ( !rc )
				errno = EAGAIN;
			return -1;
		}
	}
	rc = recv( sock, &l, 2, MSG_PEEK | MSG_WAITALL );
	if ( rc <= 0 )
		return rc;
	if ( rc == 2 ) {
		l = I2H16( l );
		if ( !l )
			return 0;
		if ( l > len )
			l = len;
		rc = recv( sock, buf, MIN( l + 2, len ), MSG_WAITALL );
		if ( rc <= 0 )
			return rc;
		rc = MIN( rc - 2, FETCH16( buf ));
		if ( rc < 1 )
			return 0;
		if ( rc >= len ) rc = len - 2;
		memcpy( buf, buf + 2, rc );
		return rc;
	}
	return 0;
}
