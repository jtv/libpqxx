lib_LTLIBRARIES = libpqxx.la
libpqxx_la_SOURCES = binarystring.cxx \
	connection_base.cxx \
	connection.cxx \
	cursor.cxx \
	dbtransaction.cxx \
	errorhandler.cxx \
	except.cxx \
	field.cxx \
	largeobject.cxx \
	nontransaction.cxx \
	notification.cxx \
	pipeline.cxx \
	prepared_statement.cxx \
	result.cxx \
	robusttransaction.cxx \
	statement_parameters.cxx \
	strconv.cxx \
	subtransaction.cxx \
	tablereader.cxx \
	tablestream.cxx \
	tablewriter.cxx \
	transaction.cxx \
	transaction_base.cxx \
	row.cxx \
	util.cxx

libpqxx_version = -release $(PQXX_ABI)

libpqxx_la_LDFLAGS = $(libpqxx_version) \
	-rpath $(libdir) \
	${POSTGRES_LIB}

AM_CPPFLAGS=-I$(top_srcdir)/include -I$(top_builddir)/include ${POSTGRES_INCLUDE}
# Override automatically generated list of default includes.  It contains only
# unnecessary entries, and incorrectly mentions include/pqxx directly.
DEFAULT_INCLUDES=

MAINTAINERCLEANFILES=Makefile.in
