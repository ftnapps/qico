/**********************************************************
 * All common headers are included here
 * $Id: headers.h,v 1.8 2004/03/20 16:04:16 sisoft Exp $
 **********************************************************/
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

#ifndef HAVE_STDXXX_FILENO
#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2
#endif

#ifndef HAVE_EIDRM
#define EIDRM EINVAL
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val)>>8)
#endif

#include "replace.h"
#include "types.h"
#include "defs.h"
#include "xmem.h"
#include "xstr.h"
#include "ftn.h"
#include "tools.h"
#include "byteop.h"
#include "mailer.h"
#include "globals.h"
#include "qconf.h"
#include "qipc.h"
#include "ver.h"
