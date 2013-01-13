# $Id: Makefile.am,v 1.6 2005/08/23 11:18:18 mitry Exp $
AUTOMAKE_OPTIONS = foreign dist-bzip2 1.5

SUBDIRS = lib src man stuff
EXTRA_DIST = FAQ Changes autogen.sh \
	qico.conf.sample qico.substs.sample qico.passwd.sample \
	qico.spec \
	freebsd/Makefile        freebsd/pkg-install \
	freebsd/pkg-descr       freebsd/pkg-plist \
	debian/README.Debian		debian/qico-common.postinst \
	debian/changelog		debian/qico-common.postrm \
	debian/compat			debian/qico-qcc.dirs \
	debian/control			debian/qico-qcc.install \
	debian/copyright		debian/qico.dirs \
	debian/qico-common.dirs		debian/qico.install \
	debian/qico-common.docs		debian/rules \
	debian/qico-common.init		debian/services \
	debian/qico-common.install

distclean-hook:
	-rm -rf $(top_builddir)/autom4te.cache

deb: debian/rules debian/control
	debian/rules binary

.PHONY: deb
