lib_LTLIBRARIES = libpqxx.la
libpqxx_la_SOURCES = binarystring.cxx \
	cachedresult.cxx \
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
	transaction_base.cxx \
	util.cxx

# Uncomment to build an unversioned library (version in soname)
libpqxx_version = -release $(PQXXVERSION)
# Uncomment to build a versioned library
#libpqxx_version = -version-info $(ABI_CURRENT):$(ABI_REVISION):$(ABI_AGE)
libpqxx_la_LDFLAGS = $(libpqxx_version) \
	-rpath $(libdir) \
	${POSTGRES_LIB}

INCLUDES=-I$(srcdir)/../include -I../include ${POSTGRES_INCLUDE}

MAINTAINERCLEANFILES=Makefile.in

