/***************************************************************************
 * Replace functions declarations
 * $Id: replace.h,v 1.3 2004/02/06 21:54:46 sisoft Exp $
 ***************************************************************************/
#ifndef __REPLACE_H__
#define __REPLACE_H__
#include <config.h>

#ifndef HAVE_BASENAME
extern char *basename (const char *filename);
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
