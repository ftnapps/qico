# $Id: Makefile.am,v 1.11 2004/04/09 09:51:33 sisoft Exp $
sbin_PROGRAMS = qico
bin_PROGRAMS = qctl @QCC@
EXTRA_PROGRAMS = qcc

noinst_HEADERS = byteop.h crc.h defs.h ftn.h globals.h headers.h hydra.h \
		 binkp.h janus.h ls_zmodem.h mailer.h qcconst.h replace.h \
		 tty.h types.h ver.h xmem.h xstr.h qconf.x qconf.h \
		 tools.h clserv.h qipc.h charset.h cvsdate.h

EXTRA_DIST = x2c.awk

BUILT_SOURCES = qconf.c qconf.h
qico_SOURCES = aso.c bso.c call.c config.c crc.c emsi.c execsh.c binkp.c md5.c \
	       freq.c ftn.c globals.c gmtoff.c hydra.c janus.c log.c ls_zglue.c \
	       ls_zmodem.c ls_zreceive.c ls_zsend.c main.c daemon.c nodelist.c \
	       protfm.c qconf.c clserv.c qnlc.c queue.c session.c tcp.c tty.c \
	       tools.c qipc.c ver.c xmem.c xstr.c flagexp.y flaglex.l
qctl_SOURCES = gmtoff.c qctl.c ver.c xstr.c clserv.c
qcc_SOURCES = qcc.c gmtoff.c ver.c xstr.c clserv.c
qico_LDADD = @LIBOBJS@
qctl_LDADD = @LIBOBJS@
qcc_LDADD = @LIBOBJS@
qcc_LDFLAGS = @QCCLIBS@

AM_YFLAGS = -d

qconf.h qconf.c: qconf.x x2c.awk
	${AWK} -F, -f x2c.awk qconf.x
