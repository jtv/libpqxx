#!/bin/sh
# Run this to generate all the initial makefiles, etc.

aclocal
autoheader
libtoolize --automake
automake --add-missing --copy
autoconf
