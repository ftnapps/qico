# 
# $Id: Makefile,v 1.4 2000/07/29 14:47:59 lev Exp $
#
include	CONFIG

DIRS    = src

CC 	= gcc
AWK	= awk
INSTALL = install

DIST    = qico-$(VERSION).tar.gz

###############################################################################
all:
	for d in ${DIRS}; do (cd $$d && echo $$d && ${MAKE} $@) || exit; done;

clean:
	for d in ${DIRS}; do (cd $$d && echo $$d && ${MAKE} $@) || exit; done;
	rm -f qico-$(VERSION).tar.gz *~

install:
	for d in ${DIRS}; do (cd $$d && echo $$d && ${MAKE} $@) || exit; done;
	$(INSTALL) -m $(PERM) -o $(OWNER) -g $(GROUP) -c qico.conf $(CONF)

release: clean
	mkdir qico-$(VERSION)
	cp -rf src stuff Makefile CONFIG Changes README LICENSE FAQ qico-$(VERSION)/
	rm -f qico-$(VERSION)/src/TAGS
	rm -rf qico-$(VERSION)/src/CVS
	rm -rf qico-$(VERSION)/stuff/CVS
	cp qico.conf qico-$(VERSION)/qico.conf
	tar -chf - qico-$(VERSION) | gzip -f -9 > $(DIST)
	rm -rf qico-$(VERSION)

stat:
	cat src/*.[chlyx] src/*.awk| wc -l 

# i hate typing stupid long words ;)
c:	clean

