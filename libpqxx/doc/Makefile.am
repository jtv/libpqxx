MAINTAINERCLEANFILES = Makefile.in

maintainer-clean-local:
	$(RM) -rf html/Reference html/Tutorial

EXTRA_DIST= Doxyfile.in libpqxx.xml reference-stamp tutorial-stamp

all-local: docs

if BUILD_REFERENCE
REFERENCE_DOCS = reference
else
REFERENCE_DOCS =
endif

if BUILD_TUTORIAL
TUTORIAL_DOCS = tutorial
else
TUTORIAL_DOCS =
endif

if MAINTAINER_MODE
REFERENCE_STAMP_DEPENDENCIES = ../src/libpqxx.la
else
REFERENCE_STAMP_DEPENDENCIES =
endif

DOCS = $(REFERENCE_DOCS) $(TUTORIAL_DOCS)

docs: $(DOCS)

reference: reference-stamp
reference-stamp: Doxyfile.in $(REFERENCE_STAMP_DEPENDENCIES)
	if test -d html; then \
	  :; \
	else \
	  $(RM) html; \
	  $(MKDIR) html; \
	fi
	$(DOXYGEN) Doxyfile
	touch $@

../src/libpqxx.la:
	cd ../src; \
	$(MAKE) libpqxx.la

tutorial: tutorial-stamp
tutorial-stamp: libpqxx.xml
	if test -d html; then \
	  :; \
	else \
	  $(RM) -f html; \
	  $(MKDIR) html; \
	fi
	$(XMLTO) -o html/Tutorial xhtml $<
	touch $@

DISTHOOKDIRS = html/Reference html/Tutorial

dist-hook: reference tutorial
	mkdir $(distdir)/html
#	Do a shallow copy of each directory
	for dir in $(DISTHOOKDIRS); do \
	  if test -d $(srcdir)/$$dir; then \
	    mkdir $(distdir)/$$dir; \
	    for dirfile in $(srcdir)/$$dir/*; do \
	      if test -f $$dirfile; then \
		cp -p $$dirfile $(distdir)/$$dir; \
	      fi ; \
	      if test -d $$dirfile ; then \
	        if test "`basename "$$dirfile"`" = "CVS"; then \
	          :; \
	        else \
		  cp -pR $$dirfile $(distdir)/$$dir; \
	        fi \
	      fi \
	    done \
	  fi \
	done
