#! /usr/bin/perl -w
#
# Generates the highly repetitive automake input file test/Makefile.am for 
# libpqxx when new tests have been added.
#

my $dir = shift;
if ($dir eq '') {
  $dir = "."
}
my $files = `ls $dir/test???\.cxx`;
$files =~ s/\s\s*/ /g;
$files =~ s/\.cxx//g;
$files =~ s/[^ \/]*\///g;
my @tests = split / /, $files;

print <<EOF;
# AUTOMATICALLY GENERATED--DO NOT EDIT
# This file is generated automatically for automake whenever test programs are
# added to libpqxx, using the Perl script "maketestam.pl" found in the tools
# directory.

INCLUDES=-I\$(top_builddir)/include -I\$(top_srcdir)/include \${POSTGRES_INCLUDE}
CLEANFILES=pqxxlo.txt
MAINTAINERCLEANFILES=Makefile.in

#TESTS_ENVIRONMENT=PGDATABASE=libpqxx
# PGDATABASE, PGHOST, PGPORT, PGUSER

EOF

print "TESTS = @tests\n";

print "check_PROGRAMS = \${TESTS}\n";

foreach my $t (@tests) {
  print "\n$t"."_SOURCES = $t.cxx\n";
  print "$t"."_LDADD = \$(top_builddir)/src/libpqxx.la \${POSTGRES_LIB}\n"
}

print "\n";

