#! /usr/bin/perl -w
#
# Generates the Windows MinGW Makefile for libpqxx
# 
# This script does not add carriage returns at the end of each line, the way
# MS-DOS likes it.  A simple "sed -e 's/$/\r/'" should do the trick.
#

my $dir = shift;
if ($dir eq '') {
  $dir = "."
}
my $files = `ls "$dir"/*.cxx`;
$files =~ s/\s\s*/ /g;
$files =~ s/\.cxx//g;
$files =~ s/[^ \/]*\///g;
my @objs = split / /, $files;

my $pqxxversion = `grep PQXXVERSION VERSION | cut -f 2`;
$pqxxversion =~ s/\n//;

# TODO: Better configuration system for system-dependent variables

print <<EOF;
# AUTOMATICALLY GENERATED--DO NOT EDIT
# Makefile for libpqxx-$pqxxversion with MinGW
# Based on original contribued by Pasquale Fersini <basquale.fersini\@libero.it>

# CONFIGURATION SECTION
# Edit these variables to suit your system.  You hopefully won't need to edit
# the rest of this Makefile.

# MinGW installation location
MINGW = C:/Mingw

# libpqxx installation location
LIBPQXX = \$(MINGW)/local/src/libpqxx-$pqxxversion

# END CONFIGURATION SECTION

CPP = g++.exe

CC = gcc.exe

WINDRES = windres.exe

RES = 

EOF

print "OBJ = ";
foreach my $t (@objs) {
	print "src/$t.o "
}
print "\$(RES)\n";

print "LINKOBJ = ";
foreach my $t (@objs) {
	print "src/$t.o "
}
print "\$(RES)\n";

print <<EOF;

LIBS = -L"\$(MINGW)/local/lib" --export-all-symbols --add-stdcall-alias -fpic -lpq -lm -lwsock32

INCS = -I"\$(MINGW)/include" -I"\$(LIBPQXX)/include" -I"\$(MINGW)/local/include"

CXXINCS = -I"\$(MINGW)/include" -I"\$(LIBPQXX)/include"

BIN = libpqxx.dll

CXXFLAGS = \$(CXXINCS) -w

CFLAGS = \$(INCS) -DBUILDING_DLL -DHAVE_NAMESPACE_STD -DHAVE_CXX_STRING_HEADER -w

.PHONY: all all-before all-after clean clean-custom

all: all-before libpqxx.dll all-after


clean: clean-custom

rm -f \$(OBJ) \$(BIN)

DLLWRAP=dllwrap.exe

DEFFILE=libpqxx.def

STATICLIB=libpqxx.a

\$(BIN): \$(LINKOBJ)

\$(DLLWRAP) --output-def \$(DEFFILE) --driver-name c++ --implib \$(STATICLIB) \$(LINKOBJ) \$(LIBS) -o \$(BIN)

EOF

foreach my $t (@objs) {
	print <<EOF
src/$t.o: src/$t.cxx
	\$(CPP) -c src/$t.cxx -o src/$t.o \$(CXXFLAGS)

EOF
}

