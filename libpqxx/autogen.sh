#! /bin/sh
# Run this to generate all the initial makefiles, etc.

set -e

ver=""
if test -x /usr/bin/automake-1.6 ; then
	ver="-1.6"
fi
if test -x /usr/bin/automake-1.7 ; then
	ver="-1.7"
fi
aclocal${ver}
autoheader
libtoolize --force --automake --copy
automake${ver} --verbose --add-missing --copy
autoconf

conf_flags="--enable-maintainer-mode --with-postgres=/home/jtv/postgresql-7.4"
if test x$NOCONFIGURE = x; then
	echo Running $srcdir/configure $conf_flags "$@" ...
	./configure $conf_flags "$@" \
	&& echo Now type \`make\' to compile $PKG_NAME || exit 1
else
	echo Skipping configure process.
fi

	

