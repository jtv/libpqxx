SUBDIRS = include src test examples tools config doc
EXTRA_DIST = autogen.sh configitems README.md VERSION requirements.json \
	.clang-format .clang-tidy .cmake-format .inferconfig .lcovrc \
	.lgtm.yml .lift .mdlrc .mdl_style.rb .yamllint

MAINTAINERCLEANFILES = \
    Makefile.in aclocal.m4 config.h.in config.log configure stamp-h.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpqxx.pc

TESTS = tools/lint.sh


# Generate ChangeLog from git history.  It goes all the way back through
# the project's git, bzr, svn, and cvs days.
# dist-hook: ChangeLog

ChangeLog: configure.ac
	if [ -d .get ] ; then \
	    git log --stat --name-only --date=short --abbrev-commit >$@ ; \
	fi


# We use README.md, but automake expects plain README.
README: README.md
	ln -s $< $@
