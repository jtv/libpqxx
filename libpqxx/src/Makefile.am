lib_LTLIBRARIES = libpqxx.la
libpqxx_la_SOURCES = \
	connection.cxx \
	cursor.cxx \
	nontransaction.cxx \
	result.cxx \
	robusttransaction.cxx \
	tablereader.cxx \
	tablestream.cxx \
	tablewriter.cxx \
	transaction.cxx \
	transactionitf.cxx

libpqxx_la_LDFLAGS = -release ${VERSION} # ${POSTGRES_LIB}

INCLUDES=-I../include ${POSTGRES_INCLUDE}
