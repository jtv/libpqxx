@SET_MAKE@

MAINT_CHARSET = latin1

EXTRA_DIST = \
	changelog \
	compat \
	control \
	copyright \
	libpqxx-dev.dirs \
	libpqxx-dev.docs \
	libpqxx-dev.install \
	libpqxx-dev.preinst \
	libpqxx-doc.doc-base.reference \
	libpqxx-doc.doc-base.tutorial \
	libpqxx-doc.install \
	libpqxx.docs \
	libpqxx.install \
	rules \
	watch

## Rules

dist-hook:
# Distribute debian/control (not in EXTRA_DIST to avoid $(srcdir) dependencies
# Do nothing actually, left in for compatibility

DEBUILD_OPTIONS = -us -uc
deb: changelog control rules
	$(MAKE) -C ../ dist
	-$(RM) -r debian-build
	-mkdir debian-build
	cd debian-build ; \
	mv ../../$(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)_$(VERSION).orig.tar.gz ; \
	tar xfvz $(PACKAGE)_$(VERSION).orig.tar.gz ; \
	cd $(PACKAGE)-$(VERSION) ; \
	debuild $(DEBUILD_OPTIONS) ; \
	cd .. ; \
	$(RM) -r $(PACKAGE)-$(VERSION)

MAINTAINERCLEANFILES = \
	Makefile.in

.PHONY: deb
