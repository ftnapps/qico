# 
# $Id: Makefile,v 1.1 2000/07/18 12:37:18 lev Exp $
#
include	../CONFIG

STRIP=0
#DEFS   += -DEMSI_LOG # log emsi in /tmp/emsi.log
#DEFS   += -DH_DEBUG # Hydra
#DEFS   += -DZ_DEBUG # ZModem
#DEFS   += -DE_DEBUG # EMSI
#DEFS   += -DS_DEBUG # Session flow
#DEFS   += -DQ_DEBUG # Queue management
#DEFS   += -DC_DEBUG # Config & Substs
#DEFS   += -DY_DEBUG #-DYYERROR_VERBOSE=1 -DYYDEBUG=100

DEFS    += -DSHELL=\"$(SHELL)\" -DCONFIG=\"$(CONF)\"
CFLAGS	= $(DEFS) -Wall -m486 -O2 -D_GNU_SOURCE=1 -D_BSD_SOURCE=1
ifeq (1,$(STRIP))
IFLAGS  = -s
else
LFLAGS  += -g
CFLAGS  += -g
endif
#DLIBS   = -lccmalloc -ldl
LIBS   +=
QCLIBS  = -L/usr/local/lib -lncurses

OBJS	= ftn.o nodelist.o main.o config.o bso.o log.o ver.o queue.o qconf.o \
          tty.o globals.o emsi.o crc.o zm.o zmr.o \
          zrecv.o zsend.o session.o call.o qnlc.o qipc.o qcconst.o tcp.o \
	  hydra.o flagexp.o execsh.o protfm.o

QCCOBJS	= qcc.o ver.o qcconst.o crc.o

SRCS    = ftn.c nodelist.c main.c config.c bso.c log.c queue.c \
          tty.c globals.c emsi.c crc.c zm.c zmr.c \
          zrecv.c zsend.c session.c call.c qnlc.c qipc.c qcconst.c tcp.c \
	  hydra.c execsh.c protfm.c

AUTOH = qconf.h 
AUTOC = qconf.c flagexp.c flaglex.c ver.c

.c.o:
	$(CC) -c $(CFLAGS) -o $*.o $< 

###############################################################################
ifeq (1,$(MORDA))
DEFS    += -DMORDA

all:	qico qcc
else
all:	qico
endif

qico:	$(OBJS) 
	$(CC) $(LFLAGS) -o qico $(OBJS) $(LIBS) $(DLIBS)

qcc:	$(QCCOBJS)
	$(CC) $(LFLAGS) -o qcc $(QCCOBJS) $(QCLIBS) $(DLIBS)

ver.c:	ver.h ../CONFIG
	echo -e "#include \"ver.h\"\n\nchar progname[]=\"qico\";\nchar version[]=\"$(VERSION)\";\n" > ver.c

qconf.c: qconf.x qconf.h
	$(AWK) -F, -f x2c.awk qconf.x > qconf.c

qconf.h: qconf.x
	$(AWK) -F, -f x2h.awk qconf.x > qconf.h

flaglex.c: flaglex.l
	$(LEX) flaglex.l
	mv -f lex.yy.c flaglex.c

flagexp.c: flagexp.y flaglex.c
	$(YACC) flagexp.y
	mv -f y.tab.c flagexp.c

$(SRCS): $(AUTOH)

install: qico qcc
	$(INSTALL) $(IFLAGS) -m $(BPERM) -o $(OWNER) -g $(GROUP) qico $(BINDIR)
	$(INSTALL) $(IFLAGS) -m $(BPERM) -o $(OWNER) -g $(GROUP) qcc $(BINDIR)

clean:
	rm -f *.o core qico qcc $(AUTOH) $(AUTOC) *~

