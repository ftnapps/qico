/***************************************************************************
 * File: replace.h
 * Replace functions declarations
 * Created at Fri Feb 16 16:52:46 2001 by pqr@yasp.com
 * $Id: replace.h,v 1.4.4.1 2003/01/24 08:59:22 cyrilm Exp $
 ***************************************************************************/
#ifndef __REPLACE_H__
#define __REPLACE_H__

#if !HAVE_BASENAME
extern char *basename (const char *filename);
/*
#define q_basename(c) 	(basename((c)))
#else
#	ifdef HAVE_BROKEN_BASENAME
		extern char *q_basename(char *c);
#	else
#		define q_basename(c) 	(basename((c)))
#	endif
*/
#endif
#if !HAVE_STRSEP
extern char *strsep(char **str, const char *delim);
#endif
#if !HAVE_USLEEP
extern int usleep (unsigned long usec);
#endif
#if !HAVE_STRCSPN
extern size_t strcspn(const char *s1, register const char *s2);
#endif
#if !HAVE_GETSID
extern pid_t getsid(pid_t pid);
#endif

#endif
