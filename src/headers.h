/**********************************************************
 * All common headers are included here
 * $Id: headers.h,v 1.1.1.1 2003/07/12 21:26:39 sisoft Exp $
 **********************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

#include <config.h>

#ifdef TIME_WITH_SYS_TIME
#	include <sys/time.h>
#	include <time.h>
#else
#	ifdef HAVE_SYS_TIME_H
#		include <sys/time.h>
#	else
#		include <time.h>
#	endif
#endif

#ifdef HAVE_LIBGEN_H
#	include <libgen.h>
#endif

#ifndef HAVE_STDXXX_FILENO
#	define STDIN_FILENO		0
#	define STDOUT_FILENO	1
#	define STDERR_FILENO	2
#endif

#ifndef HAVE_EIDRM
#	define EIDRM			EINVAL
#endif

#include "replace.h"
#include "xmem.h"
#include "xstr.h"
#include "types.h"
#include "crc.h"
#include "ftn.h"
#include "qipc.h"
#include "qcconst.h"
#include "qconf.h"
#include "mailer.h"
#include "globals.h"
#include "ver.h"
