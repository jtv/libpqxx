# Quick & ugly makefile hack, edit to fit your own environment

# Options for the "dist" target (source distribution):
# What to call the source distribution tarball
ARCHIVE_SRC=libpqxx.tar.gz
# What the source directory (where this file resides) is called
SRCDIR=libpqxx

# Options used to connect to test database when running the self test.  Suit to 
# your own environment, but best keep it out of the way of any database you 
# actually use for anything else.  Recommended practice is to set up a database 
# with this name locally.
DBNAME=libpqxx
CONNSTR="dbname=$(DBNAME)"

# Set SQL dialect to use
# Comment out to use standard SQL; uncomment to use PostgreSQL dialect.  Using 
# PostgreSQL dialect will make some features faster.
# Some code still ignores this setting.
SQL_DIALECT=-DDIALECT_POSTGRESQL

# Set different namespace for standard library (defaults to std)
#STD=-DPGSTD=_STL


# Commands and command line options used by GNU Make's default rules
CXX=colorgcc
CXXFLAGS=-Wall -pedantic -ffor-scope -Wshadow -Wcast-align -Wpointer-arith \
	-Wwrite-strings -Woverloaded-virtual -Wtraditional -O $(EXTRACXX)

CPPFLAGS=-I/usr/include/postgresql -DLACK_ITERATOR -DLACK_LIMITS \
	$(SQL_DIALECT) $(STD)
LD=g++
AR=ar
LDFLAGS=-L/usr/lib/postgresql -L.
LOADLIBES=-lpq
RM=rm -f


# What to call the library file, and how to link it into an executable.
LIBNAME=libpqxx.a
LIBOPT=-lpqxx


# *****************************************************************************
# No more configuration items below this point--any editing from here on is
# customization at best.  Which is not saying it'll never be necessary.
# *****************************************************************************


EXECS=test1 test2 test3 test4 test5 test6 test7 test8 test9 test10 test11 \
	test12
EXECOBJS=test1.o test2.o test3.o test4.o test5.o test6.o test7.o test8.o \
	test9.o test10.o test11.o test12.o
LIBOBJS=pg_connection.o pg_cursor.o pg_result.o pg_tablereader.o \
	pg_tablestream.o pg_tablewriter.o pg_transaction.o 


.PHONY: all clean distclean dist

all: $(EXECS)

test1: test1.o $(LIBNAME)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOPT) $(LOADLIBES)

test2: test2.o $(LIBNAME)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOPT) $(LOADLIBES)

test3: test3.o $(LIBNAME)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOPT) $(LOADLIBES)

test4: test4.o $(LIBNAME)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOPT) $(LOADLIBES)

test5: test5.o $(LIBNAME)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOPT) $(LOADLIBES)

test6: test6.o $(LIBNAME)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOPT) $(LOADLIBES)

test7: test7.o $(LIBNAME)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOPT) $(LOADLIBES)

test8: test8.o $(LIBNAME)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOPT) $(LOADLIBES)

test9: test9.o $(LIBNAME)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOPT) $(LOADLIBES)

test10: test10.o $(LIBNAME)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOPT) $(LOADLIBES)

test11: test11.o $(LIBNAME)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOPT) $(LOADLIBES)

test12: test12.o $(LIBNAME)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOPT) $(LOADLIBES)


$(LIBNAME): $(LIBOBJS)
	$(AR) -r $(LIBNAME) $(LIBOBJS)

check: $(EXECS)
	./test1 $(CONNSTR)
	./test2 $(CONNSTR)
	./test3 $(CONNSTR) 10
	./test4 $(CONNSTR)
	./test5 $(CONNSTR)
	./test6 $(CONNSTR) orgevents events
	./test7 $(CONNSTR)
	./test8 $(CONNSTR)
	./test9 $(CONNSTR)
	./test10 $(CONNSTR)
	./test11 $(CONNSTR)
	./test12 $(CONNSTR)

clean: 
	$(RM) $(LIBOBJS) $(EXECOBJS) a.out

distclean: clean
	$(RM) $(LIBNAME) $(EXECS)

dist: distclean
	tar -C.. -czf ../$(ARCHIVE_SRC) $(SRCDIR)
	tar tvzf ../$(ARCHIVE_SRC)

