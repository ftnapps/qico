#!/bin/sh

aclocal &&
    automake --copy --add-missing --force-missing &&
	autoconf --force &&
	    autoheader
