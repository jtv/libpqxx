MAINTAINERCLEANFILES = Makefile.in

maintainer-clean-local:
	$(RM) -rf doxygen-html
	$(MKDIR) doxygen-html

EXTRA_DIST = Doxyfile

all-local: docs

if BUILD_DOCS
DOXYGEN = doxygen
# Doxygen config needs the source files in its local tree.  That means the
# build tree.  If that differs from the source tree, copy the files.
docs:
	if ! [ "$(srcdir)/../src" -ef ../src ]; then \
	    cp "$(srcdir)"/../src/*.[ch]xx ../src/ ; \
	    cp -r "$(srcdir)"/../include/pqxx/* ../include/pqxx/ ; \
	    cp -r "$(srcdir)"/../test ../test/ ; \
	fi
	$(DOXYGEN) "$(srcdir)"/Doxyfile
else
docs:
endif

dist-hook:
	if [ -d doxygen-html ]; then \
	    cp -pR doxygen-html $(distdir)/html ; \
	fi
