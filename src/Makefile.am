# $Id: Makefile.am,v 1.16 2004/05/31 13:15:39 sisoft Exp $
sbin_PROGRAMS = qico
bin_PROGRAMS = qctl @QCC@
EXTRA_PROGRAMS = qcc

noinst_HEADERS = binkp.h byteop.h charset.h clserv.h crc.h cvsdate.h defs.h \
		 ftn.h globals.h headers.h hydra.h janus.h ls_zmodem.h \
		 mailer.h qcconst.h replace.h tty.h types.h \
		 qconf.x qconf.h qipc.h qslib.h tools.h

EXTRA_DIST = x2c.awk

BUILT_SOURCES = qconf.c qconf.h
qico_SOURCES = aso.c binkp.c call.c clserv.c config.c crc.c daemon.c \
	       emsi.c freq.c ftn.c globals.c hydra.c janus.c log.c \
	       ls_zglue.c ls_zmodem.c ls_zreceive.c ls_zsend.c main.c md5.c \
	       modem.c nodelist.c protfm.c qconf.c qipc.c qnlc.c queue.c \
	       qslib.c session.c tcp.c tools.c tty.c flagexp.y flaglex.l
qctl_SOURCES = qctl.c clserv.c md5.c qslib.c
qcc_SOURCES = qcc.c clserv.c md5.c qslib.c
qico_LDADD = @LIBOBJS@
qctl_LDADD = @LIBOBJS@
qcc_LDADD = @LIBOBJS@
qcc_LDFLAGS = @QCCLIBS@

AM_YFLAGS = -d

qconf.h qconf.c: qconf.x x2c.awk
	${AWK} -F, -f x2c.awk qconf.x
