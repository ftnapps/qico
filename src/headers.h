/**********************************************************
 * All common headers are included here
 **********************************************************/
/*
 * $Id: headers.h,v 1.7 2005/05/16 20:33:14 mitry Exp $
 *
 * $Log: headers.h,v $
 * Revision 1.7  2005/05/16 20:33:14  mitry
 * Added more proper MAX_PATH define
 *
 * Revision 1.6  2005/03/29 20:38:25  mitry
 * Added four more underscores :)
 *
 * Revision 1.5  2005/03/28 17:02:52  mitry
 * Pre non-blocking i/o update. Mostly non working.
 *
 * Revision 1.4  2005/02/21 21:58:12  mitry
 * Added #include <sys/ioctl.h>
 *
 */

#ifndef __HEADERS_H__
#define __HEADERS_H__

#include <config.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <signal.h>
#include <ctype.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <string.h>

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef STDC_HEADERS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif


#ifdef MAXPATHLEN
#  define MAX_PATH MAXPATHLEN
#else
#  define MAX_PATH 1024
#endif


#include "replace.h"
#include "types.h"
#include "defs.h"
#include "qslib.h"
#include "ftn.h"
#include "tools.h"
#include "mailer.h"
#include "globals.h"
#include "qconf.h"
#include "qipc.h"
#include "timer.h"

#endif /* __HEADERS_H */
