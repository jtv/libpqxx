#!/bin/sh
# Run this to generate all the initial makefiles, etc.

aclocal
autoheader
libtoolize --automake --copy
automake --verbose --add-missing --copy
autoconf

./configure --enable-maintainer-mode

