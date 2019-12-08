lib_LTLIBRARIES = libpqxx.la
libpqxx_la_SOURCES = \
	array.cxx \
	binarystring.cxx \
	connection.cxx \
	cursor.cxx \
	encodings.cxx \
	errorhandler.cxx \
	except.cxx \
	field.cxx \
	largeobject.cxx \
	notification.cxx \
	pipeline.cxx \
	result.cxx \
	robusttransaction.cxx \
	sql_cursor.cxx \
	statement_parameters.cxx \
	strconv.cxx \
	stream_from.cxx \
	stream_to.cxx \
	subtransaction.cxx \
	transaction.cxx \
	transaction_base.cxx \
	row.cxx \
	util.cxx \
	version.cxx

libpqxx_version = -version-info $(PQXX_MAJOR):$(PQXX_MINOR):0

libpqxx_la_LDFLAGS = $(libpqxx_version) \
	-rpath $(libdir) \
	${POSTGRES_LIB}

AM_CPPFLAGS = \
    -I$(top_srcdir)/include -I$(top_builddir)/include ${POSTGRES_INCLUDE}

# Override automatically generated list of default includes.  It contains only
# unnecessary entries, and incorrectly mentions include/pqxx directly.
DEFAULT_INCLUDES=

MAINTAINERCLEANFILES=Makefile.in
