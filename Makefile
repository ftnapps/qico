# 
# $Id: Makefile,v 1.5 2000/08/13 21:16:31 lev Exp $
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
	cp qico.conf qico.conf.sample
	$(INSTALL) -m $(PERM) -o $(OWNER) -g $(GROUP) qico.conf.sample $(CONF)

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

