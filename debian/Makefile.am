# $Id: Makefile.am,v 1.4 2004/06/07 18:50:53 sisoft Exp $
man_MANS = qico.8 qctl.8 qcc.8
EXTRA_DIST = copyright changelog examples rules conffiles inetd.conf \
	     qctl.files qcc.files services control dirs init.d $(man_MANS)

dist-hook:
	-chmod +x rules
