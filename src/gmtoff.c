/**********************************************************
 * Patch for systems without tm.tm_gmtoff
 * $Id: gmtoff.c,v 1.1.1.1 2003/07/12 21:26:39 sisoft Exp $
 **********************************************************/
#include "headers.h"

time_t gmtoff(time_t tt,int mode)
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
	if(lt.tm_isdst>0&&mode)offset++;
	offset*=60;
	offset+=gt.tm_min-lt.tm_min;
	offset*=60;
	offset+=gt.tm_sec-lt.tm_sec;
	return -offset;
#else
	lt=*localtime(&tt);
	return lt.tm_gmtoff-(lt.tm_isdst>0)*mode;
#endif
}
