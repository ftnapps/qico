/**********************************************************
 * File: headers.h
 * Created at Sun Nov 26 15:26:57 MSK 2000 by lev // lev@serebryakov.spb.ru
 * All common headers are included here
 * $Id: headers.h,v 1.3 2001/02/16 14:45:56 aaz Exp $
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


#include "replace.h"
#include "types.h"
#include "ftn.h"
#include "qcconst.h"
#include "qconf.h"
#include "mailer.h"
#include "globals.h"
#include "ver.h"
