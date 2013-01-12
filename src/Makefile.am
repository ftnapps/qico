# $Id: Makefile.am,v 1.12 2005/08/22 17:21:32 mitry Exp $

sbin_PROGRAMS = qico
bin_PROGRAMS = qctl @QCC@
EXTRA_PROGRAMS = qcc

noinst_HEADERS = binkp.h charset.h clserv.h crc.h cvsdate.h defs.h \
	ftn.h globals.h have.h headers.h hydra.h \
	janus.h ls_zmodem.h \
	mailer.h md5q.h modem.h nodelist.h \
	qcconst.h replace.h slists.h tcp.h timer.h tty.h types.h \
	qconf.x qconf.h qipc.h qslib.h tools.h

EXTRA_DIST = x2c.awk

BUILT_SOURCES = qconf.c qconf.h flagexp.c flaglex.c
qico_SOURCES = call.c clserv.c config.c crc.c daemon.c \
	emsi.c freq.c ftn.c globals.c hydra.c janus.c log.c \
	ls_zglue.c ls_zmodem.c ls_zreceive.c ls_zsend.c main.c md5q.c \
	modem.c nodelist.c outbound.c protfm.c qconf.c qipc.c \
	queue.c qslib.c session.c slists.c tcp.c tools.c tty.c \
	flagexp.y flaglex.l

EXTRA_qico_SOURCES = binkp.c perl.c

qico_LDADD = @LIBOBJS@ @LIB_MALLOC@ @BINKP_O@ @PERL_O@
qico_LDFLAGS = @PERLLDFLAGS@
qico_DEPENDENCIES = @BINKP_O@ @PERL_O@

qctl_SOURCES = qctl.c clserv.c md5q.c qslib.c
qctl_LDADD = @LIBOBJS@ @LIB_MALLOC@

qcc_SOURCES = qcc.c clserv.c md5q.c qslib.c
qcc_LDADD = @LIBOBJS@ @LIB_MALLOC@
qcc_LDFLAGS = @QCCLIBS@

AM_YFLAGS = -d

perl.o: $(srcdir)/perl.c $(srcdir)/headers.h $(CONFIG_HEADER)
	@echo Compiling perl
	@${COMPILE} @PERLCFLAGS@ -c -o perl.o $(srcdir)/perl.c

qconf.h qconf.c: $(srcdir)/qconf.x $(srcdir)/x2c.awk
	@echo Creating qconf.h and qconf.c
	@${AWK} -F, -f $(srcdir)/x2c.awk $(srcdir)/qconf.x
