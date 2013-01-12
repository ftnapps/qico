/**********************************************************
 * helper stuff for client/server iface.
 **********************************************************/
/*
 * $Id: qipc.c,v 1.4 2005/05/16 11:17:30 mitry Exp $
 *
 * $Log: qipc.c,v $
 * Revision 1.4  2005/05/16 11:17:30  mitry
 * Updated function prototypes. Changed code a bit.
 *
 * Revision 1.3  2005/05/06 20:42:29  mitry
 * Changed setproctitle() code
 *
 * Revision 1.2  2005/04/05 09:25:29  mitry
 * Fix for tty device name > 8 symbols
 *
 */

#include "headers.h"
#ifdef HAVE_LIBUTIL_H
#include <libutil.h>
#endif
#include "clserv.h"

void qsendpkt(char what, const char *line, const char *buff, size_t len)
{
	char	buf[MSG_BUFFER + 5];
	size_t	linelen;

	if ( !xsend_cb )
		return;

        len = MIN( len, MSG_BUFFER );
        linelen = MIN( strlen( line ), QCC_TTYLEN );
	STORE16( buf, (unsigned short) getpid()); /* Hm, what about pids > 64K ??? */
	buf[2] = what;
	if ( buf[2] == QC_CERASE )
		buf[2] = QC_ERASE;
	xstrcpy( buf + 3, line, linelen + 3 );
	buf[3 + linelen] = '\0';
	if ( what == QC_CHAT || what == QC_CERASE ) {
		if ( buf[3] == 'i' && buf[4] == 'p' ) {
			buf[3] = 'I';
			buf[4] = 'P';
		} else {
			buf[3] = 'C';
			buf[4] = 'H';
			buf[5] = 'T';
		}
	}
	memcpy( buf + 4 + linelen, buff, len );
	if ( xsend_cb( ssock, buf, 4 + linelen + len ) < 0 ) {
		if ( errno == ECONNREFUSED )
			xsend_cb = NULL;
		DEBUG(('I',1,"can't send_cb (fd=%d): %s",ssock,strerror(errno)));
	}
}


size_t qrecvpkt(char *str)
{
	int rc;

	if ( !xsend_cb )
		return 0;
	rc = xrecv( ssock, str, MSG_BUFFER - 1, 0 );
	if ( rc < 0 && errno != EAGAIN ) {
		if ( errno == ECONNREFUSED )
			xsend_cb = NULL;
		DEBUG(('I',1,"can't recv (fd=%d): %s",ssock,strerror(errno)));
	}
	if ( rc < 3 || !str[2] )
		return 0;
	str[rc] = '\0';
	return rc;
}


void vlogs(const char *str)
{
	qsendpkt( QC_LOGIT, QLNAME, str, strlen( str ) + 1 );
}


void vlog(const char *str,...)
{
	char	lin[MAX_STRING + 5];
	va_list	args;

	va_start( args, str );
	vsnprintf( lin, MAX_STRING - 1, str, args );
	va_end( args );

	qsendpkt( QC_LOGIT, QLNAME, lin, strlen( lin ) + 1 );
}


void sendrpkt(char what, int sock, const char *fmt, ...)
{
	int	rc;
	char	buf[MSG_BUFFER + 5];
	va_list	args;

	STORE16( buf, 0 );
	buf[2] = what;
	va_start( args, fmt );
	rc = vsnprintf( buf + 3, MSG_BUFFER - 3, fmt, args );
	va_end( args );

	if ( xsend( sock, buf, rc + 4 ) < 0 )
		DEBUG(('I',1,"can't send (fd=%d): %s",sock,strerror(errno)));
}


void sline(const char *str, ...)
{
	char	lin[MAX_STRING + 5];
	va_list	args;

	va_start( args, str );
	vsnprintf( lin, MAX_STRING - 1, str, args );
	va_end( args );

	qsendpkt( QC_SLINE, QLNAME, lin, strlen( lin ) + 1 );
}


void title(const char *str, ...)
{
	char	lin[MAX_STRING + 5];
	va_list	args;

	va_start( args, str );
	vsnprintf( lin, MAX_STRING - 1, str, args );
	va_end( args );

	qsendpkt( QC_TITLE, QLNAME, lin, strlen( lin ) + 1 );
	if ( cfgi( CFG_USEPROCTITLE ))
		setproctitle( lin );
}


#define ST16I(_b_,_d_)	STORE16( _b_, _d_ ); INC16( _b_ )
#define ST32I(_b_,_d_)	STORE32( _b_, _d_ ); INC32( _b_ )

#define INCP(_b_)	_b_ += strlen((char *) _b_) + 1;
#define XCPY(_b_,_s_) \
	xstrcpy((char *) _b_, _s_, MSG_BUFFER - ( _b_ - buf )); \
	INCP(_b_)


void qemsisend(const ninfo_t *e)
{
	unsigned char	buf[MSG_BUFFER + 5], *p = buf;
	falist_t	*a;

	ST32I( p, e->speed );
	ST16I( p, e->opt );
	ST32I( p, e->options );
	ST32I( p, e->starttime );
	XCPY( p, e->name );
	XCPY( p, e->sysop );
	XCPY( p, e->place );
	XCPY( p, e->flags );
	XCPY( p, e->phone );
	*p = '\0';
	for ( a = e->addrs; a; a = a->next ) {
		char *as;

		if ( p + strlen(( as = ftnaddrtoa( &a->addr ))) + 1 > buf + MSG_BUFFER )
			break;
		xstrcat((char *) p, as, MSG_BUFFER - ( p - buf ));
		xstrcat((char *) p, " ", MSG_BUFFER - ( p - buf ));
	}
	INCP( p );
	qsendpkt( QC_EMSID, QLNAME, (char *) buf, p - buf );
}


void qpreset(int snd)
{
	qsendpkt( snd ? QC_SENDD : QC_RECVD, QLNAME, "", 0 );
}


void qpqueue(const ftnaddr_t *a,int mail,int files,int try,int flags)
{
	unsigned char	buf[MSG_BUFFER + 5], *p = buf;
	char		*addr = ftnaddrtoa( a );

	ST32I( p, mail );
	ST32I( p, files );
	ST32I( p, flags );
	ST16I( p, try );
	XCPY( p, addr );

	qsendpkt( QC_QUEUE, QLNAME, (char *) buf, p - buf );
}


void qpproto(char type, const pfile_t *pf)
{
	unsigned char buf[MSG_BUFFER + 5], *p = buf;

	ST32I( p, pf->foff );
	ST32I( p, pf->ftot );
	ST32I( p, pf->toff );
	ST32I( p, pf->ttot );
	ST16I( p, pf->nf );
	ST16I( p, pf->allf );
	ST32I( p, pf->cps );
	ST32I( p, pf->soff );
	ST32I( p, pf->stot );
	ST32I( p, pf->start );
	ST32I( p, pf->mtime );
	XCPY( p, SS( pf->fname ));

	qsendpkt( type, QLNAME, (char *) buf, p - buf );
}


void qpmydata(void)
{
	char buf[MSG_BUFFER + 5];

	xstrcpy( buf, ftnaddrtoa( &DEFADDR ), MSG_BUFFER - 1 );
	qsendpkt( QC_MYDATA, QLNAME, buf, strlen( buf ) + 1 );
}

#undef INCP
#undef XCPY
