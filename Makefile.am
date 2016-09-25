SUBDIRS = include src test tools win32 config doc
EXTRA_DIST = \
    autogen.sh configure.ac.in configitems README.md README-UPGRADE VERSION

MAINTAINERCLEANFILES = \
    Makefile.in aclocal.m4 config.h.in config.log configure stamp-h.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpqxx.pc
bin_SCRIPTS=pqxx-config

TESTS = tools/lint


# We use README.md, but automake expects plain README.
README: README.md
	ln -s $< $@
