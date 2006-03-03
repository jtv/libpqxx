#! /usr/bin/perl -w
#
# Generates the highly repetitive Visual C++ project Makefile for libpqxx
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

print <<EOF;
# AUTOMATICALLY GENERATED--DO NOT EDIT
# This file is generated automatically by the "makevcmake.pl" script found in
# the tools directory.
# Based on the original by Clinton James, with improvements by various
# contributors
!IF "\$(CFG)" != "dll" && "\$(CFG)" != "dll debug" && "\$(CFG)" != "static" && "\$(CFG)" != "static debug"
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f libpqxx.mak configuration
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "RELEASE"      (release Win32 (x86) Dynamic-Link Library and Static Library)
!MESSAGE "DEBUG"        (debug   Win32 (x86) Dynamic-Link Library and Static Library)
!MESSAGE "DLL"          (release & debug Win32 (x86) Dynamic-Link Library)
!MESSAGE "STATIC"       (release & debug Win32 (x86) Static Library")
!MESSAGE "ALL"          Both the DLL and STATIC builds.
!MESSAGE
!ENDIF

!include common

OUTDIR=lib
INTDIR=obj
LINK32_OBJ_EXTRA=

# Defining extra CCP and LINK options for the four different builds
!IF  "\$(CFG)" == "dll"
BUILDTYPE=DLL
OUTFILE=\$(OUTDIR)\\libpqxx
CPP_EXTRAS=/MD /D "_WINDOWS" /D "NDEBUG" /D "PQXX_SHARED"
LINK32_FLAG_EXTRA=/incremental:no
LINK32_OBJ_EXTRA="\$(INTDIR)\\libpqxx.obj"

!ELSEIF  "\$(CFG)" == "dll debug"
BUILDTYPE=DLL
OUTFILE=\$(OUTDIR)\\libpqxxD
CPP_EXTRAS=/MDd /Gm /ZI /Od /D "_WINDOWS" /D "_DEBUG" /D "PQXX_SHARED" /GZ
LINK32_FLAG_EXTRA=/incremental:yes /debug
LINK32_OBJ_EXTRA="\$(INTDIR)\\libpqxx.obj"

!ELSEIF "\$(CFG)" == "static"
BUILDTYPE=STATIC
OUTFILE=\$(OUTDIR)\\libpqxx_static
CPP_EXTRAS=/MD /D "_LIB" /D "NDEBUG"

!ELSEIF "\$(CFG)" == "static debug"
BUILDTYPE=STATIC
OUTFILE=\$(OUTDIR)\\libpqxx_staticD
CPP_EXTRAS=/MDd /Gm /ZI /Od /D "_LIB" /D "_DEBUG" /GZ

!ENDIF

IAM=\$(MAKEDIR)\\libpqxx.mak /NOLOGO
# Putting the extra options together with the options common to all the
# different builds.
CPP=cl.exe
CPP_PROJ=/nologo /W3 /GX /FD /c \$(CPP_EXTRAS) \\
	/I "../include" /I "\$(PGSQLSRC)/include" /I "\$(PGSQLSRC)/interfaces/libpq" \\
	\$(STD) /D "WIN32" /D "_MBCS" \\
	/Fo"\$(INTDIR)\\\\" /Fd"\$(INTDIR)\\\\" \\

LINK32=link.exe
LINK32_FLAGS=kernel32.lib wsock32.lib advapi32.lib /nologo /dll /machine:I386 \\
	/out:"\$(OUTFILE).dll" /implib:"\$(OUTFILE).lib" \\
	\$(LINK32_FLAG_EXTRA) \$(LIBPATH)

LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"\$(OUTFILE).lib"

LINK32_OBJS= \$(LINK32_OBJ_EXTRA) \\
EOF

foreach my $t (@objs) {
  print "	\"\$(INTDIR)\\"."$t".".obj\" \\\n"
}
print "\n";

print <<EOF;
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"\$(INTDIR)\\libpqxx.res"

RELEASE :
	\@\$(MAKE) /f \$(IAM) CFG="dll" LIBPQXXDLL
	\@\$(MAKE) /f \$(IAM) CFG="static" LIBPQXXSTATIC

DEBUG :
	\@\$(MAKE) /f \$(IAM) CFG="dll debug" LIBPQXXDLL
	\@\$(MAKE) /f \$(IAM) CFG="static debug" LIBPQXXSTATIC

DLL :
	\@\$(MAKE) /f \$(IAM) CFG="dll" LIBPQXXDLL
	\@\$(MAKE) /f \$(IAM) CFG="dll debug" LIBPQXXDLL

STATIC:
	\@\$(MAKE) /f \$(IAM) CFG="static" LIBPQXXSTATIC
	\@\$(MAKE) /f \$(IAM) CFG="static debug" LIBPQXXSTATIC

ALL :
	\@\$(MAKE) /f \$(IAM) CFG="dll" LIBPQXXDLL
	\@\$(MAKE) /f \$(IAM) CFG="dll debug" LIBPQXXDLL
	\@\$(MAKE) /f \$(IAM) CFG="static" LIBPQXXSTATIC
	\@\$(MAKE) /f \$(IAM) CFG="static debug" LIBPQXXSTATIC

CLEAN :
	\@echo Deleting all files from \$(INTDIR) and \$(OUTDIR).
	-\@del "\$(INTDIR)" /Q
	-\@del "\$(OUTDIR)" /Q

!IF "\$(CFG)" == "dll" || "\$(CFG)" == "dll debug" || "\$(CFG)" == "static" || "\$(CFG)" == "static debug"
!MESSAGE
!MESSAGE Building \$(CFG)

LIBPQXXDLL : "\$(OUTFILE).dll"

"\$(OUTFILE).dll" : "\$(OUTDIR)" \$(LINK32_OBJS)
    \$(LINK32) \@<<
  \$(LINK32_FLAGS) \$(LINK32_OBJS)
<<
!IF "\$(CFG)" == "dll debug"
   -\@del "\$(OUTFILE).ilk"
   -\@REM del "\$(OUTFILE).pdb"
!ENDIF
   -\@del "\$(OUTFILE).exp"
   -\@REM del "\$(INTDIR)\\*.?db"
   -\@del "\$(INTDIR)\\*.obj"

LIBPQXXSTATIC : "\$(OUTFILE).lib"

"\$(OUTFILE).lib" : "\$(OUTDIR)" \$(DEF_FILE) \$(LINK32_OBJS)
    \$(LIB32) \@<<
  \$(LIB32_FLAGS) \$(DEF_FLAGS) \$(LINK32_OBJS)
<<
	-\@del "\$(INTDIR)\\*.obj"
	-\@REM del "\$(INTDIR)\\*.?db"

"\$(OUTDIR)" :
    if not exist "\$(OUTDIR)/\$(NULL)" mkdir "\$(OUTDIR)"

"\$(INTDIR)" :
	if not exist "\$(INTDIR)/\$(NULL)" mkdir "\$(INTDIR)"

# Do we want a resource to go with the dll's?  If so what?
#"\$(INTDIR)\\libpqxx.res" : "\$(INTDIR)" libpqxx.rc
#    \$(RSC) \$(RSC_PROJ) libpqxx.rc

EOF

foreach my $t (@objs) {
  print <<EOF
SOURCE=..\\src\\$t.cxx
"\$(INTDIR)\\$t.obj" : \$(SOURCE) "\$(INTDIR)"
	\$(CPP) \$(CPP_PROJ) \$(SOURCE)

EOF
}

print <<EOF;
SOURCE=.\\libpqxx.cxx
"\$(INTDIR)\\libpqxx.obj" : \$(SOURCE) "\$(INTDIR)"
	\$(CPP) \$(CPP_PROJ) \$(SOURCE)

!ENDIF

.cxx{\$(INTDIR)}.obj::
   \$(CPP) \@<<
   \$(CPP_PROJ) \$<
<<
EOF

