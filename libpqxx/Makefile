# Quick & ugly makefile hack, edit to fit your own environment
#
# Set O variable to determine compiler optimization level; defaults to 1 for
# that little extra code analysis, but O=0 should improve compilation speed.

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
DBUSER=`whoami`
CONNSTR="dbname=$(DBNAME)"

# Set SQL dialect to use
# Comment out to use standard SQL; uncomment to use PostgreSQL dialect.  Using 
# PostgreSQL dialect will make some features faster.
# Some code still ignores this setting.
SQL_DIALECT=-DDIALECT_POSTGRESQL

# Set different namespace for standard library (defaults to std)
#STD=-DPGSTD=_STL

# Location where PostgreSQL headers are found
PGSQLINC=/usr/include/postgresql

# What to call the library file, and how to link it into an executable.
LIBNAME=lib/libpqxx.a
LIBOPT=-lpqxx

# Commands and command line options used by GNU Make's default rules
CXX=colorgcc
CXXFLAGS=-Wall -pedantic -ffor-scope -Wshadow -Wcast-align -Wpointer-arith \
	-Wwrite-strings -Woverloaded-virtual -Wtraditional -O$O $(EXTRACXX)

CPPFLAGS=-I$(PGSQLINC) -I$(PGSQLINC)/internal -Iinclude -DLACK_ITERATOR \
	-DLACK_LIMITS $(SQL_DIALECT) $(STD)
LD=g++
AR=ar
LDFLAGS=-L/usr/lib/postgresql -Llib
LOADLIBES=-lpq $(LIBOPT) -lstdc++
RM=rm -f



# *****************************************************************************
# No more configuration items below this point--any editing from here on is
# customization at best.  Which is not saying it'll never be necessary.
# *****************************************************************************


EXECS=test1 test2 test3 test4 test5 test6 test7 test8 test9 test10 test11 \
	test12 test13 test14 test15 test16 test17 test18 test19 test20
EXECOBJS=test1.o test2.o test3.o test4.o test5.o \
	test6.o test7.o test8.o test9.o test10.o \
	test11.o test12.o test13.o test14.o test15.o \
	test16.o test17.o test18.o test19.o test20.o
LIBOBJS=pg_connection.o pg_cursor.o pg_nontransaction.o \
	pg_result.o pg_robusttransaction.o pg_tablereader.o \
	pg_tablestream.o pg_tablewriter.o pg_transaction.o \
	pg_transactionitf.o


.PHONY: all clean distclean dist check

vpath %.cc src test
vpath %.o obj

all: $(EXECS)

$(EXECS) : $(LIBNAME)


$(LIBNAME): $(LIBOBJS)
	$(AR) -r $(LIBNAME) $(LIBOBJS)

check: $(EXECS)
	@echo "Cleaning up any garbage from previous failed test runs"
	psql $(DBNAME) -c "DELETE FROM events WHERE year=1977"
	@echo
	@echo "======== STARTING TEST RUN ========"
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
	./test13 $(CONNSTR)
	./test14 $(CONNSTR)
	./test15 $(CONNSTR)
	./test16 $(CONNSTR)
	./test17 $(CONNSTR)
	./test18 $(CONNSTR)
	./test19 $(CONNSTR)
	./test20 $(CONNSTR)
	@echo
	@echo "======== TEST RUN COMPLETED ========"
	@echo
	@echo "Checking for remaining transaction records:"
	psql $(DBNAME) -c "SELECT * FROM PQXXLOG_$(DBUSER)"
	@echo "Checking for remaining test garbage:"
	psql $(DBNAME) -c "SELECT * FROM events WHERE year=1977"

clean: 
	$(RM) $(LIBOBJS) $(EXECOBJS) a.out

distclean: clean
	$(RM) $(LIBNAME) $(EXECS)

dist: distclean
	tar -C.. -czf ../$(ARCHIVE_SRC) $(SRCDIR)
	tar tvzf ../$(ARCHIVE_SRC)

