/***************************************************************************
 * File: replace.h
 * Replace functions declarations
 * Created at Fri Feb 16 16:52:46 2001 by pqr@yasp.com
 * $Id: replace.h,v 1.2 2001/05/29 19:13:34 lev Exp $
 ***************************************************************************/
#ifndef __REPLACE_H__
#define __REPLACE_H__

#ifndef HAVE_BASENAME
extern char *basename (const char *filename);
#else
#	ifdef HAVE_BROKEN_BASENAME
#	define q_basename(c) 	(basename((c))+1)
#	else
#	define q_basename(c) 	(basename((c)))
#	endif
#endif
#ifndef HAVE_STRSEP
extern char *strsep(char **str, const char *delim);
#endif
#ifndef HAVE_USLEEP
extern int usleep (unsigned long usec);
#endif
#ifndef HAVE_STRCSPN
extern size_t strcspn(const char *s1, register const char *s2);
#endif
#ifndef HAVE_GETSID
extern pid_t getsid(pid_t pid);
#endif

#endif
