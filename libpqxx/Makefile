# Quick & ugly makefile hack, edit to fit your own environment

DBNAME=libpqxx
CONNSTR="dbname=$(DBNAME)"

# Set SQL dialect to use
# Comment out to use standard SQL; uncomment to use PostgreSQL dialect.
# Using PostgreSQL dialect will keep some SQL a bit shorter.
SQL_DIALECT=-DDIALECT_POSTGRESQL

# Set different namespace for standard library (defaults to std)
#STD=-DPGSTD=_STL

CXX=colorgcc
CXXFLAGS=-Wall -pedantic -ffor-scope -Wshadow -Wcast-align -Wpointer-arith \
	-Wwrite-strings -Woverloaded-virtual -Wtraditional -O $(EXTRACXX)

CPPFLAGS=-I/usr/include/postgresql -DLACK_ITERATOR -DLACK_LIMITS \
	$(SQL_DIALECT) $(STD)
LD=g++
LDFLAGS=-L/usr/lib/postgresql
LOADLIBES=-lpq

EXECS=test1 test2 test3 test4 test5 test6 test7 test8 test9
EXECOBJS=test1.o test2.o test3.o test4.o test5.o test6.o test7.o test8.o test9.o
LIBOBJS=pg_connection.o pg_cursor.o pg_result.o pg_tablereader.o \
	pg_tablestream.o pg_tablewriter.o pg_transaction.o 

all: $(EXECS)


test1: test1.o $(LIBOBJS)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOBJS) $(LOADLIBES)

test2: test2.o $(LIBOBJS)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOBJS) $(LOADLIBES)

test3: test3.o $(LIBOBJS)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOBJS) $(LOADLIBES)

test4: test4.o $(LIBOBJS)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOBJS) $(LOADLIBES)

test5: test5.o $(LIBOBJS)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOBJS) $(LOADLIBES)

test6: test6.o $(LIBOBJS)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOBJS) $(LOADLIBES)

test7: test7.o $(LIBOBJS)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOBJS) $(LOADLIBES)

test8: test8.o $(LIBOBJS)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOBJS) $(LOADLIBES)

test9: test9.o $(LIBOBJS)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBOBJS) $(LOADLIBES)


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

clean: 
	$(RM) $(LIBOBJS) $(EXECOBJS) a.out

distclean: clean
	$(RM) $(EXECS)

dist: distclean
	tar -C.. -czf ../libpqxx.tar.gz libpqxx

