/***************************************************************************
 * File: replace.h
 * Replace functions declarations
 * Created at Fri Feb 16 16:52:46 2001 by pqr@yasp.com
 * $Id: replace.h,v 1.1 2001/02/16 14:45:56 aaz Exp $
 ***************************************************************************/
#ifndef __REPLACE_H__
#define __REPLACE_H__

#ifndef HAVE_BASENAME
extern char *basename (const char *filename);
#endif
#ifndef HAVE_STRSEP
extern char *strsep(char **str, const char *delim);
#endif
#ifndef HAVE_USLEEP
extern int usleep (unsigned long usec);
#endif

#endif
