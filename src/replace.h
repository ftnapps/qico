/***************************************************************************
 * Replace functions declarations
 * $Id: replace.h,v 1.1.1.1 2003/07/12 21:27:16 sisoft Exp $
 ***************************************************************************/
#ifndef __REPLACE_H__
#define __REPLACE_H__

#if HAVE_CONFIG_H
# include "config.h"
#endif
#if !HAVE_BASENAME
extern char *basename (const char *filename);
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
