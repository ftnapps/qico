/**********************************************************
 * timer math
 **********************************************************/
/*
 * $Id: timer.h,v 1.4 2005/06/03 14:13:49 mitry Exp $
 *
 * $Log: timer.h,v $
 * Revision 1.4  2005/06/03 14:13:49  mitry
 * Changed timer_expired()
 *
 * Revision 1.3  2005/03/31 20:10:00  mitry
 * Changed timer_expired_in()
 *
 */

#ifndef __TIMER_H__
#define __TIMER_H__

#define timer_start()			( time( NULL ))
#define timer_time( timer )		( time( NULL ) - ( timer ))
#define timer_set( expire )		( time( NULL ) + ( expire ))
#define timer_expired( timer )		( time( NULL ) >= ( timer ))
#define timer_rest( timer )		(( timer ) - time( NULL ))
#define timer_expired_in( timer, diff )	(( timer_rest( timer )) >= ( diff ))
#define timer_running( timer )		(( timer ) != 0L )
#define timer_reset()			( 0L )

#endif
