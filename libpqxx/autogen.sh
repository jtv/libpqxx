#! /bin/sh
# Run this to generate all the initial makefiles, etc.
# Set CONFIG_ARGS to the argument list you wish to pass to configure

set -e

ver=""
if test -x /usr/bin/automake-1.6 ; then
	ver="-1.6"
fi
if test -x /usr/bin/automake-1.7 ; then
	ver="-1.7"
fi

# The VERSION file defines our versioning
PQXXVERSION=`grep '\<PQXXVERSION\>' VERSION | sed -e 's/^[[:space:]A-Z_]*//' | sed -e 's/[[:space:]]*#.*$//'`
ABI_CURRENT=`grep '\<ABI_CURRENT\>' VERSION | sed -e 's/^[[:space:]A-Z_]*//' | sed -e 's/[[:space:]]*#.*$//'`
ABI_REVISION=`grep '\<ABI_REVISION\>' VERSION | sed -e 's/^[[:space:]A-Z_]*//' | sed -e 's/[[:space:]]*#.*$//'`
ABI_AGE=`grep '\<ABI_AGE\>' VERSION | sed -e 's/^[[:space:]A-Z_]*//' | sed -e 's/[[:space:]]*#.*$//'`
echo -n "libpqxx version $PQXXVERSION, " 
echo "ABI $ABI_CURRENT.$ABI_REVISION.$ABI_AGE"

# Generate configure.ac based on current version numbers
sed -e "s/@PQXXVERSION@/$PQXXVERSION/g" configure.ac.in | sed -e "s/@ABI_CURRENT@/$ABI_CURRENT/g" | sed -e "s/@ABI_REVISION@/$ABI_REVISION/g" | sed -e "s/@ABI_AGE@/$ABI_AGE/g" >configure.ac

# Generate test/Makefile.am
./tools/maketestam.pl test >test/Makefile.am

# Generate win32/test.mak (adding carriage returns to make it MS-DOS format)
./tools/maketestvcmak.pl test | sed -e 's/$/\r/' >win32/test.mak

aclocal${ver}
autoheader
libtoolize --force --automake --copy
automake${ver} --verbose --add-missing --copy
autoconf

conf_flags="--enable-maintainer-mode $CONFIG_ARGS"
if test x$NOCONFIGURE = x; then
	echo Running $srcdir/configure $conf_flags "$@" ...
	./configure $conf_flags "$@" \
	&& echo Now type \`make\' to compile $PKG_NAME || exit 1
else
	echo Skipping configure process.
fi

	

