SUBDIRS = include src test tools win32 config debian doc
EXTRA_DIST = autogen.sh configure.ac.in README-UPGRADE VERSION

MAINTAINERCLEANFILES=Makefile.in aclocal.m4 config.h.in config.log configure \
	stamp-h.in

includedir=$(prefix)/include/pqxx

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpqxx.pc
