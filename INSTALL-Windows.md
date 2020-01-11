Welcome to libpqxx, the C++ API to the PostgreSQL database management system.

This file documents building and installation of libpqxx on Windows systems.
Please see the regular README.md for general instructions on building,
installing, and using the library.  If you are using Cygwin, MSYS, or another
Unix-like environment please try the Unix configuration & build procedure
instead of the one described here.  Some help for the MSYS/MinGW combination is
given below, but in general, things are a bit more complicated for Windows than
they are for other environments.

The only Windows compilers currently documented here are:

	Visual C++ (the newer the better)
	MinGW

If you are using a different compiler, you should still be able to get it to
work.  If you do, or if you fail, please report your results so any problems can
be resolved, and so that we can add support for your compiler in the standard
libpqxx distribution.

We'll be using the command line throughout this document.


Obtaining libpq
---------------

Next, make sure you have a working installation of libpq, the C-level client
library included with PostgreSQL.

There are two ways to make libpq available on a Windows system: Install it, or
build it from sources.

Either way, make sure that you get the right build: don't mix 32-bit and 64-bit
libraries, or ones built for "debug" and "release."


Installing libpq
----------------

The easiest way to get libpq on a Windows system, is to install PostgreSQL.  The
PostgreSQL installer from Windows can be obtained from

	http://www.postgresql.org/download/windows

This build procedure has been tested against installes done by the One Click
Installer.  If you opt for the pgInstaller, you will have to make some
adjustments to these instructions and scripts for the build to succeeed.

By default, for some `{Version}` of postgres, the installer sets up libpq
under:

	C:\Program Files\PostgreSQL\{Version}\lib


Building libpq from source
--------------------------

If you prefer to build libpq from source, you can compile a recent version of
PostgreSQL.  Its source tree should then contain libpq in binary form, as well
as the corresponding headers.  Look for these in src/interfaces/libpq.  Visual
C++ will generate separate Debug and Release versions in subdirectories called
Debug and Release, respectively, at that location.  Note these locations; they
will become important in the next section.

The source code for PostgreSQL can be obtained from

	http://postgresql.org/download/

Select a recent version of postgres and download the corrresponding .tar.gz or
.tar.bz2 archive for that version.  Unpack the sources to some directory on your
computer.

Select "Visual Studio Command Prompt" from the Start menu ("Developer Command
Prompt" if building for x86 and "x64 Native Tools Command Prompt" if building
for x64). Alternatively, open a Command Prompt window and run:

    VCVARSALL.BAT amd64

to prepare the environment.

From the Command Prompt window, "cd" into the postgres source tree directory,
and run:

	nmake /f win32.mak

You'll also want to build a Debug-flavour libpq, so then run:

	nmake /f win32.mak DEBUG=1

The libpq binaries will be produced in src/interfaces/libpq/Release and
src/interface/libpq/Debug, respecitively, and the Debug versions will have an
extra "D" in their names.

(Instructions for building PostgreSQL with MinGW are given in the MSYS section
below).


Building libpqxx
----------------

The rest of the procedure depends on your compiler.


### Visual Studio

If you're using Visual C++, use the CMake build.  First, run the vcvars64
script:

    call "<compiler>\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

(Replace `<compiler>` with the path to your compiler's installation location.)

Then, in the directory where you want to do the build, configure for building:

    cmake <source-directory> -DBUILD_SHARED_LIBS=1

Replace `<source-directory>` with the path to where your libpqxx source tree
is.  The `-DBUILD_SHARED_LIBS=1` is only needed if you're building a shared
library.

Finally, compile:

    msbuild libpqxx.sln


### MinGW

If you're using MinGW (but without a Unix-like environment, see above):

1) Run `mingw32-make -f win32/MinGW.mak`

2) Consider installing a Unix-like environment like MSYS to automate all this!


Running the test suite
----------------------

After building libpqxx, it is recommended that you compile and run the
self-test programs included in the package.

Unix, Cygwin, or MSYS users simply type "make check" to build and run the
entire regression test.  For Visual C++ users, the following steps should
accomplish the same:

 1) Make sure a PostgreSQL database is running and accessible, and set up the
    environment variables PGDATABASE, PGHOST, PGPORT, PGUSER and PGPASSWORD as
    described in the libpqxx README.md file so the test program can connect
    without needing to pass a connection string.

 2) Run `runner.exe` from any of the test directories.  When runner.exe runs,
    it executes each of the test programs in order.  Some of the tests will
    produce lots of output, some won't produce any.  Error messages that are
    expected in the course of a normal test run are prefixed with "(Expected)"
    to show that they are not harmful.  All tests should complete successfully.

 3) Similarly, run `unit_runner.exe`.
