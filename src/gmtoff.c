/**********************************************************
 * File: gmtoff.c
 * Created at Sun Nov 26 15:34:28 MSK 2000 by lev // lev@serebryakov.spb.ru
 * Patch for systems without tm.tm_gmtoff
 * $Id: gmtoff.c,v 1.3 2001/01/17 14:17:20 lev Exp $
 **********************************************************/
#include "headers.h"

time_t gmtoff(time_t tt)
{
	struct tm lt;
#ifndef TM_HAVE_GMTOFF
	struct tm gt;
	time_t offset;
	lt=*localtime(&tt);
	gt=*gmtime(&tt);
	offset=gt.tm_yday-lt.tm_yday;
	if (offset > 1) offset=-24;
	else if (offset < -1) offset=24;
	else offset*=24;
	offset+=gt.tm_hour-lt.tm_hour;
	offset*=60;
	offset+=gt.tm_min-lt.tm_min;
	offset*=60;
	offset+=gt.tm_sec-lt.tm_sec;
	return -offset;
#else
	lt=*localtime(&tt);
	return lt.tm_gmtoff;
#endif
}
