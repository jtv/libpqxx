SUBDIRS = config doc include src test
EXTRA_DIST = win32/* debian/* Doxyfile

MAINTAINERCLEANFILES=Makefile.in aclocal.m4 config.h.in config.log configure \
	stamp-h.in

includedir=$(prefix)/include/pqxx

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpqxx.pc

dist-hook:
	rm -rf $(distdir)/win32/CVS $(distdir)/debian/CVS
	chmod u+x $(distdir)/debian/rules

