#! /bin/sh
# Run this to generate all the initial makefiles, etc.
# Set CONFIG_ARGS to the argument list you wish to pass to configure

set -e

# Run in most basic of locales to avoid performance overhead (and risk of bugs)
# involved in localization, encoding issues etc.  We only do ASCII here.
export LC_ALL=C

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
makewinmake() {
	./tools/template2mak.py "$1" | sed -e 's/$/\r/' >"$2"
}

if which python >/dev/null ; then
	makewinmake win32/vc-libpqxx.mak.template win32/libpqxx.mak
	makewinmake win32/vc-test.mak.template win32/test.mak
	makewinmake win32/mingw.mak.template win32/MinGW.mak
else
	echo "Python not available--not generating Visual C++ makefiles."
fi

autoheader

libtoolize --force --automake --copy
aclocal${ver} -I . -I config/m4
automake${ver} --verbose --add-missing --copy
autoconf

conf_flags="--enable-maintainer-mode $CONFIG_ARGS"
if test -z "$NOCONFIGURE" ; then
	echo Running $srcdir/configure $conf_flags "$@" ...
	./configure $conf_flags "$@" \
	&& echo Now type \`make\' to compile $PKG_NAME || exit 1
else
	echo Skipping configure process.
fi

