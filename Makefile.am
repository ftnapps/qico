# $Id: Makefile.am,v 1.10 2004/06/19 22:31:57 sisoft Exp $
AUTOMAKE_OPTIONS = foreign dist-bzip2 1.5

SUBDIRS = src stuff debian
EXTRA_DIST = FAQ Changes qico.conf.sample depcomp autogen.sh TodayFixes qico.spec

distclean-hook:
	-rm -rf $(top_builddir)/autom4te.cache

deb: debian/rules debian/control
	debian/rules binary

.PHONY: deb
