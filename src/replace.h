/***************************************************************************
 * File: replace.h
 * Replace functions declarations
 * Created at Fri Feb 16 16:52:46 2001 by pqr@yasp.com
 * $Id: replace.h,v 1.4 2002/04/18 17:02:58 lev Exp $
 ***************************************************************************/
#ifndef __REPLACE_H__
#define __REPLACE_H__

#ifndef HAVE_BASENAME
extern char *basename (const char *filename);
#define q_basename(c) 	(basename((c)))
#else
#	ifdef HAVE_BROKEN_BASENAME
		extern char *q_basename(char *c);
#	else
#		define q_basename(c) 	(basename((c)))
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
