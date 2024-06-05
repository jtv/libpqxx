MAINTAINERCLEANFILES = Makefile.in

maintainer-clean-local:
	$(RM) -rf doxygen-html
	$(MKDIR) doxygen-html

EXTRA_DIST = Doxyfile

all-local: docs

# Doxygen config needs the input files in its local tree.  Copy them.
docs:
	if ! [ "$(srcdir)/../src" -ef ../src ]; then \
	    cp "$(srcdir)"/../src/*.cxx "$(srcdir)"/../src/*.hxx ../src/ ; \
	    cp -r "$(srcdir)"/../include/pqxx/* ../include/pqxx/ ; \
	    cp -r "$(srcdir)"/../test ../test/ ; \
	fi
	doxygen "$(srcdir)"/Doxyfile

dist-hook:
	if [ -d doxygen-html ]; then \
	    cp -pR doxygen-html $(distdir)/html ; \
	fi
