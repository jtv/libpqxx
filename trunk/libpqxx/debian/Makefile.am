@SET_MAKE@

MAINT_CHARSET = latin1

EXTRA_DIST = \
	changelog \
	control.in \
	copyright \
	libpqxx-dev.dirs \
	libpqxx-dev.doc-base.reference \
	libpqxx-dev.doc-base.tutorial \
	libpqxx-dev.docs \
	libpqxx-dev.install \
	libpqxx-dev.preinst \
	libpqxx.docs \
	libpqxx.install \
	rules


## Rules

dist-hook:
# Distribute debian/control (not in EXTRA_DIST to avoid $(srcdir) dependencies
	DEBIAN_VERSION=`grep "(" $(srcdir)/changelog | head -1l | sed "s/^[^(]*(//" | sed "s/).*$$//"` ; \
	UPSTREAM_VERSION=`echo $$DEBIAN_VERSION | sed "s/-[^-]*$$//"` ; \
	LIBRARY_VERSION="-$${UPSTREAM_VERSION}" ; \
	sed -e "s/#UPSTREAM_VERSION#/$${UPSTREAM_VERSION}/g" \
	    -e "s/#LIBRARY_VERSION#/$${LIBRARY_VERSION}/g" \
	    <$(srcdir)/control.in >$(distdir)/control

DEBUILD_OPTIONS = -us -uc
deb: debian/changelog debian/control debian/rules
	$(MAKE) dist
	-$(RM) -r debian-build
	-mkdir debian-build
	cd debian-build ; \
	mv ../$(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)_$(VERSION).orig.tar.gz ; \
	tar xfvz $(PACKAGE)_$(VERSION).orig.tar.gz ; \
	cd $(PACKAGE)-$(VERSION) ; \
	debuild $(DEBUILD_OPTIONS) ; \
	cd .. ; \
	$(RM) -r $(PACKAGE)-$(VERSION)

control:
	cd ..; \
	debian/rules debian/control

MAINTAINERCLEANFILES = \
	Makefile.in

.PHONY: deb
