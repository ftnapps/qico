# $Id: Makefile.am,v 1.1.1.1 2003/07/12 21:26:27 sisoft Exp $
sbin_PROGRAMS = qico
bin_PROGRAMS = qctl @QCC@
EXTRA_PROGRAMS = qcc

noinst_HEADERS = byteop.h crc.h defs.h ftn.h globals.h headers.h hydra.h \
		janus.h ls_zmodem.h mailer.h opts.h qcconst.h qipc.h replace.h \
		tty.h types.h ver.h xmem.h xstr.h qconf.x qconf.h charset.h

X2C = x2c.awk
X2H = x2h.awk
EXTRA_DIST = $(X2H) $(X2C)

BUILT_SOURCES = qconf.c qconf.h
qico_SOURCES = bso.c call.c config.c crc.c emsi.c execsh.c \
		freq.c ftn.c globals.c gmtoff.c hydra.c janus.c log.c ls_zglue.c \
		ls_zmodem.c ls_zreceive.c ls_zsend.c main.c nodelist.c protfm.c \
		qconf.c qipc.c qnlc.c queue.c session.c tcp.c tty.c ver.c xmem.c \
		xstr.c flagexp.y flaglex.l 
qctl_SOURCES = gmtoff.c qctl.c ver.c xstr.c
qcc_SOURCES = qcc.c gmtoff.c ver.c
qico_LDADD = @LIBOBJS@
qctl_LDADD = @LIBOBJS@
qcc_LDADD = @LIBOBJS@
qcc_LDFLAGS = @QCCLIBS@
DEFS = -DQIPC_KEY=\"$(sbindir)/qico\" @DEFS@
AM_YFLAGS = -d

.x.h:
	${AWK} -F, -f $(X2H) $< > $@

.x.c:
	${AWK} -F, -f $(X2C) $< > $@
