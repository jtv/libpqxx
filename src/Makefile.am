lib_LTLIBRARIES = libpqxx.la
libpqxx_la_SOURCES = binarystring.cxx \
	connection_base.cxx \
	connection.cxx \
	cursor.cxx \
	dbtransaction.cxx \
	except.cxx \
	largeobject.cxx \
	nontransaction.cxx \
	notify-listen.cxx \
	pipeline.cxx \
	prepared_statement.cxx \
	result.cxx \
	robusttransaction.cxx \
	strconv.cxx \
	subtransaction.cxx \
	tablereader.cxx \
	tablestream.cxx \
	tablewriter.cxx \
	transaction.cxx \
	transaction_base.cxx \
	util.cxx

# Uncomment to build an unversioned library (version in soname)
libpqxx_version = -release $(PQXXVERSION)
# Uncomment to build a versioned library
#libpqxx_version = -version-info $(ABI_CURRENT):$(ABI_REVISION):$(ABI_AGE)
libpqxx_la_LDFLAGS = $(libpqxx_version) \
	-rpath $(libdir) \
	${POSTGRES_LIB}

INCLUDES=-I$(top_srcdir)/include -I$(top_builddir)/include ${POSTGRES_INCLUDE}
# Override automatically generated list of default includes.  It contains only
# unnecessary entries, and incorrectly mentions include/pqxx directly.
DEFAULT_INCLUDES=

MAINTAINERCLEANFILES=Makefile.in

