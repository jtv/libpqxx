#!/bin/sh
# Run this to generate all the initial makefiles, etc.

ver=""
if test -x /usr/bin/automake-1.6 ; then
	ver="-1.6"
fi
aclocal${ver}
autoheader
libtoolize --automake --copy
automake${ver} --verbose --add-missing --copy
autoconf

./configure --enable-maintainer-mode

