/**********************************************************
 * outgoing call implementation
 **********************************************************/
/*
 * $Id: call.c,v 1.10 2005/08/16 20:23:23 mitry Exp $
 *
 * $Log: call.c,v $
 * Revision 1.10  2005/08/16 20:23:23  mitry
 * Pre 0.57.1: minor changes
 *
 * Revision 1.9  2005/05/16 20:27:57  mitry
 * Changed code a bit
 *
 * Revision 1.8  2005/05/06 20:21:55  mitry
 * Changed nodelist handling
 *
 * Revision 1.7  2005/03/31 20:31:17  mitry
 * Added MC_CANCEL to modem_dial() result check
 *
 * Revision 1.6  2005/03/28 17:02:52  mitry
 * Pre non-blocking i/o update. Mostly non working.
 *
 * Revision 1.5  2005/02/18 20:29:13  mitry
 * Changed mdm_dial() to modem_dial(), mdm_done() to modem_done().
 *
 */

#include "headers.h"
#include "qipc.h"
#include "tty.h"
#include "modem.h"
#include "tcp.h"
#include "nodelist.h"


int do_call(ftnaddr_t *fa, char *site, char *port)
{
    int rc;

    if ( cfgs( CFG_RUNONCALL )) {
        char buf[MAX_PATH];
        snprintf( buf, MAX_PATH, "%s %s %s", ccs, ftnaddrtoa( fa ), port ? port : site );
        if (( rc = execsh( buf )))
            write_log( "exec '%s' returned rc=%d", buf, rc);
    }

    IFPerl( if (( rc = perl_on_call( fa, site, port )) != S_OK ) return rc );

    DEBUG(('S',4,"call.c: port: '%s', site: '%s'", port, site ));

    if ( port ) { /* Modem call */
        rc = modem_dial( site, port );

        switch( rc ) {
            case MC_OK:
                rc = -1;
                break;

            case MC_RING:
            case MC_BUSY:
                rc = S_BUSY;
                break;

            case MC_ERROR:
            case MC_FAIL:
            case MC_CANCEL:
                rc = S_REDIAL | S_ADDTRY;
                break;

            case MC_NODIAL:
                rc = S_NODIAL;
                break;

            case MC_BAD:
                rc = S_OK;
                break;
        }
    } else {
        rc = ( tcp_dial( fa, site ) >= 0 ) ? -1 : ( S_REDIAL | S_ADDTRY );
    }

    if ( rc == -1 ) {
        if ( cfgs( CFG_LOGINSCRIPT )) {
            char buf[MAX_PATH];

            if ( *ccs == '!' )
                xstrcat( buf, ccs + 1, MAX_PATH - 1 );
            else
                snprintf( buf, MAX_PATH, "%s %s %s", ccs, ftnaddrtoa( fa ), port ? port : site );

            switch (( rc = execsh( buf ))) {
                case EXT_OK:
                case EXT_YOURSELF:
                    rc = -1;
                    break;

                case EXT_DID_WORK:
                    rc = 0;
                    break;

                default:
                    write_log( "Login script '%s' failed. rc=%d", buf, rc );
                    rc = ( S_REDIAL | S_ADDTRY );
                    break;
            }
        }

        if ( rc == -1 ) {
            rc = session( SM_OUTBOUND, bink ? SESSION_BINKP : SESSION_AUTO, fa,
                port ? atoi( connstr + strcspn( connstr, "0123456789" )) : TCP_SPEED );

            if (( rc & S_MASK ) == S_REDIAL && cfgi( CFG_FAILPOLLS )) {
                write_log( "creating poll for %s", ftnaddrtoa( fa ));
                outbound_poll( fa, F_ERR );
            }
        }

        sline( "Hanging up..." );
        if ( port )
            modem_done();
        else
            tcp_done();
        
    }

    title( "Waiting..." );
    vidle();
    sline( "" );
    return rc;
}

int force_call(ftnaddr_t *fa, int line, int flags)
{
    int rc;
    char *port = NULL;
    slist_t *ports = NULL;

    rc = nodelist_query( fa, &rnode );

    if ( rc != 1 )
        write_log( ndl_errors[0 - rc] );

    if ( !rnode ) {
        rnode = xcalloc( 1, sizeof( ninfo_t ));
        falist_add( &rnode->addrs, fa );
        rnode->name = xstrdup( "Unknown" );
        rnode->phone = xstrdup( "" );
    }

    rnode->tty = NULL;
    ports = cfgsl( CFG_PORT );

    if (( flags & FC_ANY ) != FC_ANY ) {
        do {
            if ( !ports )
                return S_FAILURE;

            port = tty_findport( ports, cfgs( CFG_NODIAL ));
            if ( !port )
                return S_FAILURE;
            if ( rnode->tty )
                xfree( rnode->tty );

            rnode->tty = xstrdup( baseport( port ));
            ports = ports->next;
        } while( !checktimegaps( cfgs( CFG_CANCALL )));

        if ( !checktimegaps( cfgs( CFG_CANCALL )))
            return S_FAILURE;
    } else {
        if (( port = tty_findport( ports, cfgs( CFG_NODIAL )))) {
            rnode->tty = xstrdup( baseport( port ));
        } else
            return S_FAILURE;
    }

    if ( !cfgi( CFG_TRANSLATESUBST ))
        phonetrans( &rnode->phone, cfgsl( CFG_PHONETR ));

    if ( line ) {
        applysubst( rnode, psubsts );
        
        while( rnode->hidnum && line != rnode->hidnum && applysubst( rnode, psubsts )
            && rnode->hidnum != 1 );

        if ( line != rnode->hidnum ) {
            write_log( "%s doesn't have line #%d", ftnaddrtoa(fa), line );
            return S_NODIAL;
        }

        if ( !can_dial( rnode, ( flags & FC_IMMED ) == FC_IMMED )) {
            write_log( "We should not call to %s #%d at this time", ftnaddrtoa( fa ), line );
            return S_NODIAL;
        }
    } else {
        if ( !find_dialable_subst( rnode,(( flags & FC_IMMED ) == FC_IMMED), psubsts )) {
            write_log( "We should not call to %s at this time", ftnaddrtoa( fa ));
            return S_NODIAL;
        }
    }

    if ( cfgi( CFG_TRANSLATESUBST ))
        phonetrans( &rnode->phone, cfgsl( CFG_PHONETR ));

    if( !log_init( cfgs( CFG_LOG ), rnode->tty )) {
        write_log( "can't open log %s", ccs );
        return S_FAILURE;
    }

    if( rnode->hidnum )
        write_log( "calling %s #%d, %s (%s)",
            rnode->name, rnode->hidnum, ftnaddrtoa( fa ), rnode->phone );
    else
        write_log( "calling %s, %s (%s)",
            rnode->name, ftnaddrtoa( fa ), rnode->phone );

    rc = do_call( fa, rnode->phone, port );
    return rc;
}
