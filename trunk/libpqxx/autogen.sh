#! /bin/sh
# Run this to generate all the initial makefiles, etc.
# Set CONFIG_ARGS to the argument list you wish to pass to configure

set -e

ver=""
for amv in "1.6" "1.7" "1.8" "1.9" ; do
	if which "automake-$amv" >/dev/null 2>&1; then
		ver="-$amv"
	fi
done

# The VERSION file defines our versioning
PQXXVERSION=`grep '\<PQXXVERSION\>' VERSION | sed -e 's/^[[:space:]A-Z_]*//' -e 's/[[:space:]]*#.*$//'`
echo "libpqxx version $PQXXVERSION" 

# Generate configure.ac based on current version numbers
sed -e "s/@PQXXVERSION@/$PQXXVERSION/g" configure.ac.in >configure.ac

# Generate test/Makefile.am
./tools/maketestam.pl test >test/Makefile.am

# Generate Windows makefiles (adding carriage returns to make it MS-DOS format)
./tools/maketestvcmak.pl test | sed -e 's/$/\r/' >win32/test.mak
./tools/makevcmake.pl src | sed -e 's/$/\r/' >win32/libpqxx.mak
./tools/makemingwmak.pl src | sed -e 's/$/\r/' >win32/MinGW.mak

autoheader

libtoolize --force --automake --copy
aclocal${ver} -I . -I config/m4
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

