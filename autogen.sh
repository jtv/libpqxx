#! /bin/sh
# Run this to generate all the initial makefiles, etc.
# Set CONFIG_ARGS to the argument list you wish to pass to configure

set -e
set -u

# The VERSION file defines our versioning
PQXXVERSION=$(./tools/extract_version)
echo "libpqxx version $PQXXVERSION"

PQXX_ABI=$(./tools/extract_version --abi)
echo "libpqxx ABI version $PQXX_ABI"

PQXX_MAJOR="$(echo "$PQXXVERSION" | sed -e 's/[[:space:]]*\([0-9]*\)\..*$/\1/')"
PQXX_MINOR="$(echo "$PQXXVERSION" | sed -e 's/[^.]*\.\([0-9]*\).*$/\1/')"

substitute() {
	sed -e "s/@PQXXVERSION@/$PQXXVERSION/g" \
		-e "s/@PQXX_MAJOR@/$PQXX_MAJOR/g" \
		-e "s/@PQXX_MINOR@/$PQXX_MINOR/g" \
		-e "s/@PQXX_ABI@/$PQXX_ABI/g" \
		"$1"
}

# Generate configure.ac based on current version numbers
substitute configure.ac.in >configure.ac

# Generate version header.
substitute include/pqxx/version.hxx.template >include/pqxx/version.hxx

# Generate Windows makefiles (adding carriage returns to make it MS-DOS format)
makewinmake() {
	./tools/template2mak.py "$1" | sed -e 's/$/\r/' >"$2"
}

if ! which python >/dev/null
then
	echo "This script requires the python interpreter." >&2
	exit 1
fi

# Use templating system to generate various Makefiles
./tools/template2mak.py test/Makefile.am.template test/Makefile.am
./tools/template2mak.py test/unit/Makefile.am.template test/unit/Makefile.am
makewinmake win32/vc-libpqxx.mak.template win32/vc-libpqxx.mak
makewinmake win32/vc-test.mak.template win32/vc-test.mak
makewinmake win32/vc-test-unit.mak.template win32/vc-test-unit.mak
makewinmake win32/mingw.mak.template win32/MinGW.mak

autoheader

libtoolize --force --automake --copy
aclocal -I . -I config/m4
automake --verbose --add-missing --copy
autoconf

conf_flags="--enable-maintainer-mode ${CONFIG_ARGS:-}"
if [ -z "${NOCONFIGURE:-}" ]
then
	echo Running ./configure $conf_flags "$@" ...
	./configure $conf_flags "$@" \
	&& echo "Now type 'make' to compile libpqxx" || exit 1
else
	echo Skipping configure process.
fi
