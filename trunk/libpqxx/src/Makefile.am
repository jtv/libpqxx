lib_LTLIBRARIES = libpqxx.la
libpqxx_la_SOURCES = cachedresult.cxx \
	connection_base.cxx \
	connection.cxx \
	cursor.cxx \
	largeobject.cxx \
	nontransaction.cxx \
	result.cxx \
	robusttransaction.cxx \
	tablereader.cxx \
	tablestream.cxx \
	tablewriter.cxx \
	transaction.cxx \
	transaction_base.cxx

libpqxx_la_LDFLAGS = -version-info @sonumber@ ${POSTGRES_LIB}

INCLUDES=-I$(srcdir)/../include -I../include ${POSTGRES_INCLUDE}

MAINTAINERCLEANFILES=Makefile.in

