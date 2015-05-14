dnl Process this file with autoconf to produce a configure script.
AC_INIT([qico],[0.59.1],[jame at rocasa dot us])
AC_PREREQ(2.53)
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE([qico],[0.59.1])
AM_CONFIG_HEADER([config.h])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_AWK
AC_PROG_RANLIB
AC_PROG_INSTALL
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_MAKE_SET
AC_PROG_GCC_TRADITIONAL

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_HEADER_TIOCGWINSZ
AC_CHECK_HEADERS([arpa/inet.h errno.h fcntl.h libgen.h libutil.h locale.h malloc.h])
AC_CHECK_HEADERS([netdb.h netinet/in.h sgtty.h stddef.h stdlib.h])
AC_CHECK_HEADERS([string.h sys/fs/s5param.h sys/ioctl.h sys/param.h])
AC_CHECK_HEADERS([sys/socket.h sys/statfs.h sys/statvfs.h sys/time.h])
AC_CHECK_HEADERS([sys/vfs.h syslog.h termio.h termios.h unistd.h utime.h])


dnl Checks for library functions.
AC_FUNC_FNMATCH
AC_FUNC_FORK
AC_FUNC_FSEEKO
AC_FUNC_MEMCMP
AC_FUNC_STRFTIME
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF

AC_CHECK_FUNCS([alarm dup2 getcwd])
AC_CHECK_FUNCS([getpagesize getpass gettimeofday])
AC_CHECK_FUNCS([memcpy memmove memset mkdir recv rmdir])
AC_CHECK_FUNCS([select send setlocale statfs statvfs])
AC_CHECK_FUNCS([strcasecmp strchr strerror])
AC_CHECK_FUNCS([strncasecmp strrchr strspn strstr])
AC_CHECK_FUNCS([strtol strtoul uname utime vsnprintf waitpid])

dnl Socket functions are in libnsl.a on SunOS 5.5
AC_CHECK_FUNCS([accept],,[AC_CHECK_LIB(socket,accept)])
AC_CHECK_FUNCS([bind],,[AC_CHECK_LIB(socket,bind)])
AC_CHECK_FUNCS([connect],,[AC_CHECK_LIB(socket,connect)])
AC_CHECK_FUNCS([gethostbyaddr],,[AC_CHECK_LIB(socket,gethostbyaddr)])
AC_CHECK_FUNCS([gethostbyname],,[AC_CHECK_LIB(nsl,gethostbyname)])
AC_CHECK_FUNCS([getpeername],,[AC_CHECK_LIB(socket,getpeername)])
AC_CHECK_FUNCS([getservbyname],,[AC_CHECK_LIB(socket,getservbyname)])
AC_CHECK_FUNCS([inet_addr],,[AC_CHECK_LIB(nsl,inet_addr)])
AC_CHECK_FUNCS([inet_ntoa],,[AC_CHECK_LIB(nsl,inet_ntoa)])
AC_CHECK_FUNCS([listen],,[AC_CHECK_LIB(socket,listen)])
AC_CHECK_FUNCS([shutdown],,[AC_CHECK_LIB(socket,shutdown)])
AC_CHECK_FUNCS([socket],,[AC_CHECK_LIB(socket,socket)])

AC_REPLACE_FUNCS([strsep strcasestr strcspn getsid])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SIGNAL
AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_SIZEOF([char],[1])
AC_CHECK_SIZEOF([short],[2])
AC_CHECK_SIZEOF([int],[4])
AC_CHECK_SIZEOF([long],[4])
AC_CHECK_TYPE([socklen_t],
	,
	AC_DEFINE(socklen_t,[unsigned int],[Define to `unsigned int' if <sys/types.h> does not define it]))


AC_CHECK_MEMBER([struct statfs.f_bavail],[AC_DEFINE(STATFS_HAVE_F_BAVAIL,1,[Define to 1 if 'struct statfs' has 'f_bavail' field])],,[#include <sys/statfs.h>])
AC_CHECK_MEMBER([struct statvfs.f_bavail],[AC_DEFINE(STATVFS_HAVE_F_BAVAIL,1,[Define to 1 if 'struct statvfs' has 'f_bavail' field])],,[#include <sys/statvfs.h>])


AC_CHECK_DECL([facilitynames],
	[AC_DEFINE(HAVE_SYSLOG_FAC_NAMES,1,[Define to 1 if 'facilitynames' defined in syslog headers])],
	,
	[
#include <stdlib.h>
#define SYSLOG_NAMES
#include <syslog.h>
#ifdef HAVE_SYSLOG_AND_SYS_SYSLOG
# include <sys/syslog.h>
#endif])


AC_CHECK_DECL([prioritynames],
	[AC_DEFINE(HAVE_SYSLOG_PRI_NAMES,1,[Define to 1 if 'prioritynames' defined in syslog headers])],
	,
	[
#include <stdlib.h>
#define SYSLOG_NAMES
#include <syslog.h>
#ifdef HAVE_SYSLOG_AND_SYS_SYSLOG
# include <sys/syslog.h>
#endif])


AC_CHECK_DECL([TIOCSCTTY],
	[AC_DEFINE(HAVE_TIOCSCTTY,1,[Define to 1 if system has 'TIOCSCTTY' ioctl()])],
	,
	[
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>])


AC_CHECK_DECL([EIDRM],
	AC_DEFINE(HAVE_EIDRM,1,[Define to 1 if system defines 'EIDRM' errno value]),
	,
	[
#include <stdlib.h>
#include <errno.h>])


AC_CHECK_DECLS([STDIN_FILENO,STDOUT_FILENO,STDERR_FILENO],
	AC_DEFINE(HAVE_STDXXX_FILENO,1,[Define to 1 if system defines 'STDxxx_FILENO' constants]),
	,
	[
#include <unistd.h>])


dnl Checking for FreeBSD setproctitle in libutil
AC_CHECK_FUNC([setproctitle],
	[AC_DEFINE(HAVE_SETPROCTITLE,1,[Define to 1 if system have setproctitle() call])],
	[AC_CHECK_LIB(util,setproctitle,
		[AC_DEFINE(HAVE_SETPROCTITLE,1,[Define to 1 if system have setproctitle() call])])])


AC_CHECK_LIB(md,[MD5Init])


dnl Signal handling
AC_MSG_CHECKING(for reliable signals)
AC_TRY_RUN([
#include <sys/types.h>
#include <signal.h>

#ifndef SIGCHLD
# define SIGCHLD SIGCLD
#endif
#ifdef USE_SIGSET
# define signal sigset
#endif

int got;

#ifdef SIGVOID
void
#endif
hand()
{
	got++;
}

main()
{
	(void) signal( SIGCHLD, hand );
	kill( getpid(), SIGCHLD );
	kill( getpid(), SIGCHLD );
	if ( got < 2 )
		exit( 1 );
	exit( 0 );
}
], AC_MSG_RESULT(yes),
AC_DEFINE(SYS5SIGNALS,1,[Non-reliable signals]) AC_MSG_RESULT(no),
AC_DEFINE(SYS5SIGNALS,1,[Non-reliable signals]) AC_MSG_RESULT(cross-compiling. No assumptions made)
)


AC_MSG_CHECKING([whether to enable maintainer-specific portions of Makefiles])
dnl maintainer-mode is disabled by default
AC_ARG_ENABLE(maintainer-mode,
	AC_HELP_STRING([--enable-maintainer-mode],
		[enable make rules and dependencies not useful (and sometimes confusing) to the casual installer]),
	USE_MAINTAINER_MODE=$enableval,
	USE_MAINTAINER_MODE=no)
AC_MSG_RESULT([$USE_MAINTAINER_MODE])


dnl Enable optional modules
AC_ARG_ENABLE(dlmalloc,
	AC_HELP_STRING([--enable-dlmalloc],[Compile & use the malloc package by Doug Lea]),
[
 case "$enableval" in
 'yes')
    use_dlmalloc="yes"
    LIBDLMALLOC="libdlmalloc.a"
    LIB_MALLOC="-L../lib -ldlmalloc"
    echo "dlmalloc enabled"
    ;;
 'no')
    use_dlmalloc="no"
    echo "dlmalloc disabled"
    ;;
 *) use_dlmalloc="yes"
    LIB_MALLOC="$enableval"
    echo "dlmalloc enabled with $LIB_MALLOC"
 esac
])
if test "${use_dlmalloc-unset}" = unset; then
    case "$host" in
	i386-*-solaris2.*)
		echo "Enabling dlmalloc for $host"
		use_dlmalloc="yes"
		LIBDLMALLOC="libdlmalloc.a"
		LIB_MALLOC="-L../lib -ldlmalloc"
		;;
	*-sgi-irix*)
		echo "Enabling dlmalloc for $host"
		use_dlmalloc="yes"
		LIBDLMALLOC="libdlmalloc.a"
		LIB_MALLOC="-L../lib -ldlmalloc"
		;;
    esac
fi
if test "x$ac_cv_enabled_dlmalloc" = "xyes"; then
    # Ok. dlmalloc was enabled before, but state may be changed.
    # we have to test these again
    unset ac_cv_func_mallinfo
    unset ac_cv_func_mallocblksize
    unset ac_cv_func_free
    unset ac_cv_func_realloc
    unset ac_cv_func_memalign
    unset ac_cv_func_valloc
    unset ac_cv_func_pvalloc
    unset ac_cv_func_calloc
    unset ac_cv_func_cfree
    unset ac_cv_func_malloc_trim
    unset ac_cv_func_malloc_usable_size
    unset ac_cv_func_malloc_stats
    unset ac_cv_func_mallinfo
    unset ac_cv_func_mallopt
    unset ac_cv_lib_gnumalloc
    unset ac_cv_header_gnumalloc_h
    unset ac_cv_lib_malloc
    unset ac_cv_enabled_dlmalloc
fi
if test "$use_dlmalloc" = yes; then
    ac_cv_func_mallinfo="yes"
    ac_cv_func_mallocblksize="no"
    ac_cv_func_free="yes"
    ac_cv_func_realloc="yes"
    ac_cv_func_memalign="yes"
    ac_cv_func_valloc="yes"
    ac_cv_func_pvalloc="yes"
    ac_cv_func_calloc="yes"
    ac_cv_func_cfree="yes"
    ac_cv_func_malloc_trim="yes"
    ac_cv_func_malloc_usable_size="yes"
    ac_cv_func_malloc_stats="yes"
    ac_cv_func_mallopt="yes"
    ac_cv_lib_gnumalloc="no"
    ac_cv_header_gnumalloc_h="no"
    ac_cv_lib_malloc="no"
    ac_cv_enabled_dlmalloc="yes"
else
AC_FUNC_MALLOC
fi

AC_SUBST(LIBDLMALLOC)
AC_SUBST(LIB_MALLOC)

dnl whether syslog.h and sys/syslog.h may both be included
AC_CHECK_HEADER([sys/syslog.h],
	[AC_DEFINE(HAVE_SYSLOG_AND_SYS_SYSLOG,1,[Could <syslog.h> and <sys/syslog.h> be included in one file])],
	,
	[
#define SYSLOG_NAMES
#include <syslog.h>])


dnl Checking for config path
AC_MSG_CHECKING([for config path])
AC_ARG_WITH([config],
	AC_HELP_STRING([--with-config=config],
		[specifies config filename (default is ${sysconfdir}/qico.conf)]),
	with_config=$withval,
	with_config=NONE)

if test "${with_config}x" = "NONEx"; then
	if test x`eval echo ${sysconfdir}` = "xNONE/etc"; then
	    if test "${prefix}x" = "NONEx"; then
		syscdir="${ac_default_prefix}/etc"
	    else
		syscdir="${prefix}/etc"
	    fi
	else
	    syscdir="${sysconfdir}"
	fi
	with_config="${syscdir}/qico.conf"
fi
with_config=`eval echo ${with_config}`
AC_MSG_RESULT($with_config)


dnl TCP Speed
AC_MSG_CHECKING([for TCP connection speed])
AC_ARG_WITH([tcp-speed],
	AC_HELP_STRING([--with-tcp-speed],[specifies TCP connection speed (default is 115200)]),
	with_tcp_speed=$withval,
	with_tcp_speed=115200)
AC_DEFINE_UNQUOTED(TCP_SPEED,$with_tcp_speed,[Speed for TCP connections in bps])
AC_MSG_RESULT($with_tcp_speed)

changequote(,)dnl
if test "x`expr "$with_tcp_speed" : '[0-9]*'`" != "x`expr "$with_tcp_speed" : '.*'`"; then
	changequote([,])dnl
	AC_MSG_ERROR([Parameter for --with-tcp-speed must be a number])
fi


dnl Default Speed
AC_MSG_CHECKING([for default port locking speed])
AC_ARG_WITH([default-speed],
	AC_HELP_STRING([--with-default-speed],[specifies default port locking speed (default is 9600)]),
	with_default_speed=$withval,
	with_default_speed=9600)
AC_DEFINE_UNQUOTED(DEFAULT_SPEED,$with_default_speed,[Default speed for modem connections])
AC_MSG_RESULT($with_default_speed)

changequote(,)dnl
if test "x`expr "$with_default_speed" : '[0-9]*'`" != "x`expr "$with_default_speed" : '.*'`"; then
	changequote([,])dnl
	AC_MSG_ERROR([Parameter for --with-default-speed must be a number])
fi


dnl Shell
AC_MSG_CHECKING([for shell])
AC_ARG_WITH([shell],
	AC_HELP_STRING([--with-shell],[specifies shell (default is /bin/sh)]),
	with_shell=$withval,
	with_shell=/bin/sh)
AC_DEFINE_UNQUOTED(SHELL,"$with_shell",[Path to system shell])
AC_MSG_RESULT($with_shell)

if test "x`expr "$with_shell" : '/.*'`" != "x`expr "$with_shell" : '.*'`"; then
	AC_MSG_ERROR([Parameter for --with-shell must be absolute pathname])
fi
if test ! \( -x "$with_shell" -a \( -f "$with_shell" -o -L "$with_shell" \) \); then
	AC_MSG_ERROR([Parameter for --with-shell must be name of executable file])
fi


dnl Lock creation style
AC_MSG_CHECKING([for lock creation style])
AC_ARG_WITH([lock-style],
	AC_HELP_STRING([--with-lock-style],[select lock creation style (open or link)]),
	with_lock_style=$withval,
	with_lock_style=LINK)

case "$with_lock_style" in
	link | LINK | NFS | nfs )
		AC_MSG_RESULT(link)
		;;
	open | OPEN | VFAT | vfat )
		AC_MSG_RESULT(open)
		AC_DEFINE(LOCKSTYLE_OPEN,1,[Create lock files with open(), not link()])
		;;
	*)
		AC_MSG_RESULT(unknown)
		AC_MSG_ERROR([Lock style '$with_lock_style' is unknown. Valid ones are 'link' or 'open])
		;;
esac


dnl qcc support
AC_ARG_ENABLE(qcc,
	AC_HELP_STRING([--disable-qcc],[disable building of qcc]),
	enable_qcc=$enableval,
	enable_qcc=yes)

if test "${enable_qcc}x" != "nox"; then
	AC_CHECK_LIB(ncurses,main,[AC_DEFINE(HAVE_LIBNCURSES,1,[Define to 1 if system have ncurses])
	CURSES=ncurses],[AC_CHECK_LIB(curses,main,[AC_DEFINE(HAVE_LIBCURSES,1,[Define to 1 if system have curses])
	CURSES=curses])])
	AC_CHECK_HEADERS([ncurses.h curses.h])
	AC_CHECK_HEADERS([$CURSES.h],,CURSES=)
	if test $CURSES; then
		AC_CHECK_LIB($CURSES,mvvline,[AC_DEFINE(CURS_HAVE_MVVLINE,1,[Define to 1 if system (n)curses have mvvline()])])
		AC_CHECK_LIB($CURSES,resizeterm,[AC_DEFINE(CURS_HAVE_RESIZETERM,1,[Define to 1 if system (n)curses have wresize()])])
		QCC=qcc
		QCCLIBS=-l$CURSES
		AC_DEFINE(QCC,1,[Build qcc (interface program)])
		AC_MSG_CHECKING([for qcc support])
		AC_MSG_RESULT(yes)
	else
		QCC=
		AC_MSG_CHECKING([for qcc support])
		AC_MSG_RESULT(unsupported)
		AC_MSG_WARN([there's no any curses library here])
	fi
else
	AC_MSG_CHECKING([for qcc support])
	AC_MSG_RESULT(disabled)
	QCC=
fi


dnl Compile in debug messages
AC_MSG_CHECKING([for debug logging])
AC_ARG_ENABLE(debug,
	AC_HELP_STRING([--disable-debug],[disable adding debug messages]),
	enable_debug=$enableval,
	enable_debug=yes)

if test "x${enable_debug}" = "xyes"; then
	AC_MSG_RESULT([yes, use loglevels in config to tune])
	AC_DEFINE(NEED_DEBUG,1,[Build debug loglevels support])
else
	AC_MSG_RESULT([no, logleves in config are useless])
fi


dnl Check for DNOTIFY
AC_MSG_CHECKING([for DNOTIFY (for ASO auto-rescan)])
AC_ARG_ENABLE(notify,
	AC_HELP_STRING([--disable-notify],[disable use DNOTIFY for ASO auto-rescan]),
	enable_notify=$enableval,
	enable_notify=yes)

if test "${enable_notify}x" != "nox"; then
	AC_TRY_RUN([
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

volatile int done = 0;
#ifdef SIGVOID
void
#endif
sigrt()
{
	done = 1;
}

int main() {
	int fd, rc = 1;
	FILE *f;

	mkdir("ac_xtst",S_IRWXU);
	fd = open("ac_xtst",O_RDONLY);
	if ( fd < 0 ) {
		rmdir("ac_xtst");
		return 1;
	}
	signal(SIGRTMIN,sigrt);
	if ( fcntl(fd,F_SETSIG,SIGRTMIN) < 0 ||
			fcntl(fd,F_NOTIFY,DN_CREATE|DN_DELETE|DN_MULTISHOT) < 0 )
		goto bad;
	f = fopen("ac_xtst/ac","w");
	if ( !f )
		goto bad;
	fclose( f );
	sleep( 1 );
	if ( !done)
		goto bad;
	done = 0;
	unlink("ac_xtst/ac");
	sleep( 1 );
	if ( done )
		rc = 0;
bad:
	close( fd );
	rmdir("ac_xtst");
	return rc;
}],
AC_MSG_RESULT(yes) AC_DEFINE(HAVE_DNOTIFY,1,[Allow use DNOTIFY for ASO auto-recan]),
AC_MSG_RESULT(no),
AC_MSG_RESULT(no))

else

AC_MSG_RESULT(no)

fi


dnl HYDRA4/8/16K support (disabled by default)
AC_MSG_CHECKING([for Hydra4k/8k/16k support])
AC_ARG_ENABLE([hydra8k],
	AC_HELP_STRING([--enable-hydra8k],[enable Hydra4k/8k/16k protocols support]),
	enable_hydra8k=$enableval,
	enable_hydra8k=no)

if test "x$enable_hydra8k" = "xyes"; then
	AC_DEFINE(HYDRA8K16K,1,[Allow Hydra-4K, Hydra-8K and Hydra-16K protocols])
fi
AC_MSG_RESULT($enable_hydra8k)


dnl Check for Binkp protocol support
AC_MSG_CHECKING([for BinkP protocol support])
AC_ARG_ENABLE(binkp,
	AC_HELP_STRING([--disable-binkp],[disable BinkP protocol support]),
	want_binkp=$enableval,
	want_binkp=yes)

if test "x$want_binkp" = "xyes"; then
	AC_DEFINE(WITH_BINKP,1,[Support for BinkP protocol])
	BINKP_O='binkp.$(OBJEXT)'
fi
AC_MSG_RESULT($want_binkp)


dnl Check for perl support
AC_MSG_CHECKING(for perl support)
AC_ARG_ENABLE(perl,
	AC_HELP_STRING([--enable-perl],[Enable perl hooks support]),
	enable_perl=$enableval,
	enable_perl=no)

if test "${enable_perl}x" != "nox"; then
	AC_MSG_RESULT(yes)
	AC_PATH_PROG(PERL,perl,perl)
	AC_MSG_CHECKING(for libperl)
	PERLDIR=`$PERL -MConfig -e 'print $Config{archlib}' 2>/dev/null`
	if PERLCFLAGS=`$PERL -MExtUtils::Embed -e ccopts 2>/dev/null`; then
		PERLLDFLAGS=`$PERL -MExtUtils::Embed -e ldopts 2>/dev/null`
		case x$PERLDIR in
		    x) AC_MSG_RESULT(yes);;
		    *) AC_MSG_RESULT($PERLDIR);;
		esac
	else
		case x$PERLDIR in
		    x) AC_MSG_RESULT(no);;
		    *)
			PERLCFLAGS="-I $PERLDIR/CORE"
			PERLLDFLAGS="$PERLDIR/auto/DynaLoader/DynaLoader.a -L$PERLDIR/CORE -lperl -ldl -lcrypt -lm"
			AC_CHECK_LIB(dl,main,PERLLDFLAGS="$PERLLDFLAGS -ldl")
			AC_CHECK_LIB(crypt,main,PERLLDFLAGS="$PERLLDFLAGS -lcrypt")
			AC_CHECK_LIB(m,main,PERLLDFLAGS="$PERLLDFLAGS -lm")
			AC_MSG_RESULT($PERLDIR)
			;;
		esac
	fi
	case x$PERLCFLAGS in
		x) ;;
		*)
		    AC_MSG_CHECKING(for working libperl)
		    OLD_LIBS=$LIBS
		    OLD_CFLAGS=$CFLAGS
		    LIBS="$LIBS $PERLLDFLAGS"
		    CFLAGS="$CFLAGS $PERLCFLAGS"
		    AC_TRY_RUN([
#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>
int main(int argc, char** argv, char** env) {
return perl_alloc() ? 0 : 1;
}],
		    AC_DEFINE(WITH_PERL,1,[Support for perl hooks])
		    PERL_O='perl.$(OBJEXT)'
		    AC_MSG_RESULT(yes),AC_MSG_RESULT(no),AC_MSG_RESULT(no))
		    CFLAGS=$OLD_CFLAGS; LIBS=$OLD_LIBS
		    ;;
	esac
else
	AC_MSG_RESULT(no)
fi


dnl calculate real config -- prefix already known, so we could eval config
if test "x`expr "$with_config" : '/.*'`" != "x`expr "$with_config" : '.*'`"; then
	AC_MSG_ERROR([Parameter for --with-config must be absolute filename])
fi
if test -d "$with_config"; then
	AC_MSG_WARN([Parameter for --with-config must be filename, not a directory])
fi
AC_DEFINE_UNQUOTED(CONFIG,"$with_config",[Default path to config])
CONFIG=$with_config


dnl Check for __attribute__((__packed__))
AC_CACHE_CHECK([for __attribute__((__packed__))],
ac_cv_attr_packed,
[AC_TRY_RUN([
typedef struct {
	short	a;
	int	b;
} __attribute__((__packed__)) tst_t;

int main(int argc, char **argv)
{
	return (sizeof( tst_t ) != (sizeof( short ) + sizeof( int )));
}
],
[ac_cv_attr_packed=yes],
[ac_cv_attr_packed=no],
[ac_cv_attr_packed=no])]
)

if test "x$ac_cv_attr_packed" = "xyes"; then
	AC_DEFINE([HAVE_ATTR_PACKED],1,[Define to 1 if compiler has __attribute__((__packed__))])
fi


dnl My defines -- from acconfig.h, hack for new autoconf
AC_DEFINE_UNQUOTED(MAX_STRING,512,[Maximum length of standard string])
AC_DEFINE_UNQUOTED(MAX_NODELIST,32,[Maximum number of compiled nodelists])


dnl My substs
AC_SUBST(QCCLIBS)
AC_SUBST(QCC)
AC_SUBST(TCP_SPEED)
AC_SUBST(DEFAULT_SPEED)
AC_SUBST(SHELL)
AC_SUBST(CONFIG)
AC_SUBST(BINKP_O)
AC_SUBST(PERLCFLAGS)
AC_SUBST(PERLLDFLAGS)
AC_SUBST(PERL_O)


dnl add warn flags in maintainer mode
CFLAGS="$CFLAGS -Wall"
if test "x$USE_MAINTAINER_MODE" = "xyes"; then
    CFLAGS="$CFLAGS -pedantic -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls"
fi


dnl creating files
AC_CONFIG_FILES([
	Makefile
	lib/Makefile
	src/Makefile
	man/Makefile
	stuff/Makefile
])
AC_OUTPUT

echo "Do 'make' and 'make install' to compile and install qico."
echo ""
