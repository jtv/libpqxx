MAINTAINERCLEANFILES = Makefile.in

maintainer-clean-local:
	$(RM) -rf html/Reference
	$(RM) reference-stamp
	$(MKDIR) html/Reference

EXTRA_DIST= Doxyfile.in libpqxx.xml reference-stamp

all-local: docs

if BUILD_REFERENCE
DOCS = reference
else
DOCS =
endif

if MAINTAINER_MODE
REFERENCE_STAMP_DEPENDENCIES = ../src/libpqxx.la
else
REFERENCE_STAMP_DEPENDENCIES =
endif

docs: $(DOCS)

reference: reference-stamp
reference-stamp: Doxyfile.in $(REFERENCE_STAMP_DEPENDENCIES)
	if [ -x "$(DOXYGEN)" ]; then \
	  $(MKDIR_P) html/Reference; \
	  $(DOXYGEN) Doxyfile; \
	  touch $@; \
	else \
	  echo >&2; \
	  echo >&2 "*****************************************************"; \
	  echo >&2; \
	  echo >&2 "Doxygen not found."; \
	  echo >&2 "Install it, or configure with --disable-documentation"; \
	  echo >&2; \
	  echo >&2 "*****************************************************"; \
	  exit 1; \
	fi

../src/libpqxx.la:
	cd ../src; \
	$(MAKE) libpqxx.la


DISTHOOKDIRS = html/Reference

dist-hook: reference
	if [ -d $(srcdir)/html/Reference ]; then \
	  $(MKDIR_P) $(distdir)/html ; \
	  cp -pR html/Reference $(distdir)/html/ ; \
	fi
