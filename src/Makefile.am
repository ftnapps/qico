sbin_PROGRAMS = qico
bin_PROGRAMS = qctl @QCC@
EXTRA_PROGRAMS = qcc

noinst_HEADERS = byteop.h crc.h defs.h ftn.h globals.h headers.h hydra.h \
		janus.h ls_zmodem.h mailer.h opts.h qcconst.h qipc.h qipcng.h \
		replace.h tty.h types.h vartree.h ver.h xmem.h xstr.h \
		qconf.x flagexp.h

X2C = x2c.awk
X2H = x2h.awk
EXTRA_DIST = $(X2H) $(X2C)

BUILT_SOURCES = qconf.c qconf.h
qico_SOURCES = bso.c call.c config.c crc.c emsi.c execsh.c \
		freq.c ftn.c globals.c gmtoff.c hydra.c janus.c log.c ls_zglue.c \
		ls_zmodem.c ls_zreceive.c ls_zsend.c main.c nodelist.c protfm.c \
		flagexp.y flaglex.l \
		qconf.c qipc.c qnlc.c queue.c session.c tcp.c tty.c ver.c xmem.c xstr.c

qctl_SOURCES = gmtoff.c qctl.c ver.c xstr.c
qcc_SOURCES = qcc.c ver.c
qcc_LDFLAGS = @QCCLIBS@

.x.h:
	${AWK} -F, -f $(X2H) $< > $@

.x.c:
	${AWK} -F, -f $(X2C) $< > $@
