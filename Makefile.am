SUBDIRS = include src test tools win32 config debian doc
EXTRA_DIST = autogen.sh configure.ac.in libpqxx.spec libpqxx.spec.in \
	configitems README-UPGRADE VERSION

MAINTAINERCLEANFILES=Makefile.in aclocal.m4 config.h.in config.log configure \
	stamp-h.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpqxx.pc
bin_SCRIPTS=pqxx-config

TESTS = tools/lint
