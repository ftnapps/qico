/*	$OpenBSD: basename.c,v 1.4 1999/05/30 17:10:30 espie Exp $	*/
/*	$FreeBSD: src/lib/libc/gen/basename.c,v 1.1.2.2 2001/07/23 10:13:04 dd Exp $	*/
/*	$Id: basename.c,v 1.2.4.2 2003/01/25 21:14:35 cyrilm Exp $ */

/*
 * Copyright (c) 1997 Todd C. Miller <Todd.Miller@courtesan.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if !HAVE_BASENAME
#ifndef lint
static char openbsd_rcsid[] = "$OpenBSD: basename.c,v 1.4 1999/05/30 17:10:30 espie Exp $";
static char freebsd_rcsid[] = "$FreeBSD: src/lib/libc/gen/basename.c,v 1.1.2.2 2001/07/23 10:13:04 dd Exp $";
static char qico_rcsid[] = "$Id";
#endif /* not lint */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#if HAVE_ERRNO_H
# include <errno.h>
#endif
#if HAVE_LIBGEN_H
# include <libgen.h>
#endif
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

char *
basename(path)
	const char *path;
{
	static char bname[MAXPATHLEN];
	register const char *endp, *startp;

	/* Empty or NULL string gets treated as "." */
	if (path == NULL || *path == '\0') {
		(void)strcpy(bname, ".");
		return(bname);
	}

	/* Strip trailing slashes */
	endp = path + strlen(path) - 1;
	while (endp > path && *endp == '/')
		endp--;

	/* All slashes becomes "/" */
	if (endp == path && *endp == '/') {
		(void)strcpy(bname, "/");
		return(bname);
	}

	/* Find the start of the base */
	startp = endp;
	while (startp > path && *(startp - 1) != '/')
		startp--;

	if (endp - startp + 2 > sizeof(bname)) {
		errno = ENAMETOOLONG;
		return(NULL);
	}
	(void)strncpy(bname, startp, endp - startp + 1);
	bname[endp - startp + 1] = '\0';
	return(bname);
}
#endif
