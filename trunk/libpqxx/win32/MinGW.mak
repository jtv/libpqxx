# Project: libpqxx
#
# Makefile for libpqxx with MinGW-3.1.0 (originally based on libpqxx 2.0.0) 
# Contributed by Pasquale Fersini <basquale.fersini@libero.it>

CPP = g++.exe

CC = gcc.exe

WINDRES = windres.exe

RES = 

OBJ = src/transaction_base.o src/binarystring.o src/cachedresult.o src/connection.o src/connection_base.o src/cursor.o src/largeobject.o src/nontransaction.o src/result.o src/robusttransaction.o src/tablereader.o src/tablestream.o src/tablewriter.o src/transaction.o $(RES)

LINKOBJ = src/transaction_base.o src/binarystring.o src/cachedresult.o src/connection.o src/connection_base.o src/cursor.o src/largeobject.o src/nontransaction.o src/result.o src/robusttransaction.o src/tablereader.o src/tablestream.o src/tablewriter.o src/transaction.o $(RES)

LIBS = -L"C:/Devel/Mingw/lib" -L"C:/Devel/Mingw/lib/gcc-lib/mingw32/3.2.3" -L"C:/Devel/Mingw/lib/libpql" --export-all-symbols --add-stdcall-alias -fpic -lpq -lm -lwsock32 

INCS = -I"C:/Devel/Mingw/include" -I"C:/msys/home/pasquale/libpqxx-2.0.0/include" 

CXXINCS = -I"C:/Devel/Mingw/include" -I"C:/Devel/Mingw/include/c++/3.2.3" -I"C:/Devel/Mingw/include/c++/3.2.3/backward" -I"C:/Devel/Mingw/include/c++/3.2.3/bits" -I"C:/Devel/Mingw/include/c++/3.2.3/ext" -I"C:/Devel/Mingw/include/c++/3.2.3/mingw32" -I"C:/Devel/Mingw/lib/wx/include/msw-2.4" -I"C:/msys/home/pasquale/libpqxx-2.0.0/include" 

BIN = libpqxx.dll

CXXFLAGS = $(CXXINCS) -w

CFLAGS = $(INCS)-DBUILDING_DLL=1 -DHAVE_NAMESPACE_STD -DHAVE_CXX_STRING_HEADER -w

.PHONY: all all-before all-after clean clean-custom

all: all-before libpqxx.dll all-after



clean: clean-custom

rm -f $(OBJ) $(BIN)

DLLWRAP=dllwrap.exe

DEFFILE=libpqxx.def

STATICLIB=libpqxx.a

$(BIN): $(LINKOBJ)

$(DLLWRAP) --output-def $(DEFFILE) --driver-name c++ --implib $(STATICLIB) $(LINKOBJ) $(LIBS) -o $(BIN)

src/transaction_base.o: src/transaction_base.cxx

$(CPP) -c src/transaction_base.cxx -o src/transaction_base.o $(CXXFLAGS)

src/binarystring.o: src/binarystring.cxx

$(CPP) -c src/binarystring.cxx -o src/binarystring.o $(CXXFLAGS)

src/cachedresult.o: src/cachedresult.cxx

$(CPP) -c src/cachedresult.cxx -o src/cachedresult.o $(CXXFLAGS)

src/connection.o: src/connection.cxx

$(CPP) -c src/connection.cxx -o src/connection.o $(CXXFLAGS)

src/connection_base.o: src/connection_base.cxx

$(CPP) -c src/connection_base.cxx -o src/connection_base.o $(CXXFLAGS)

src/cursor.o: src/cursor.cxx

$(CPP) -c src/cursor.cxx -o src/cursor.o $(CXXFLAGS)

src/largeobject.o: src/largeobject.cxx

$(CPP) -c src/largeobject.cxx -o src/largeobject.o $(CXXFLAGS)

src/nontransaction.o: src/nontransaction.cxx

$(CPP) -c src/nontransaction.cxx -o src/nontransaction.o $(CXXFLAGS)

src/result.o: src/result.cxx

$(CPP) -c src/result.cxx -o src/result.o $(CXXFLAGS)

src/robusttransaction.o: src/robusttransaction.cxx

$(CPP) -c src/robusttransaction.cxx -o src/robusttransaction.o $(CXXFLAGS)

src/tablereader.o: src/tablereader.cxx

$(CPP) -c src/tablereader.cxx -o src/tablereader.o $(CXXFLAGS)

src/tablestream.o: src/tablestream.cxx

$(CPP) -c src/tablestream.cxx -o src/tablestream.o $(CXXFLAGS)

src/tablewriter.o: src/tablewriter.cxx

$(CPP) -c src/tablewriter.cxx -o src/tablewriter.o $(CXXFLAGS)

src/transaction.o: src/transaction.cxx

$(CPP) -c src/transaction.cxx -o src/transaction.o $(CXXFLAGS)

