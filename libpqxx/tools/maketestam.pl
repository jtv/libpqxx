#! /usr/bin/perl -w

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
INCLUDES=-I../include \${POSTGRES_INCLUDE}
MAINTAINERCLEANFILES=Makefile.in

#TESTS_ENVIRONMENT=PGDATABASE=libpqxx
# PGDATABASE, PGHOST, PGPORT, PGUSER

EOF

print "TESTS = @tests\n";

print "check_PROGRAMS = \${TESTS}\n";

foreach my $t (@tests) {
  print "\n$t"."_SOURCES = $t.cxx\n";
  print "$t"."_LDADD = ../src/libpqxx.la \${POSTGRES_LIB}\n"
}

print "\n";

