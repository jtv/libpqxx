Building using CMake
====================

First of all, the libpqxx build requires the full PostgreSQL development
package.  You must have that package installed before you can build libpqxx.

The instructions will assume that you're working from a command-line shell.
If you prefer to work from an IDE, you'll have to know how your IDE likes to
do things, and you'll want to follow the shell instructions as a guide.

I'm not too familiar with CMake, and this build relies heavily on contributions
from users.  If you see something wrong here, please file a bug and explain, in
simple words, what needs changing and why.

There are two ways to use libpqxx in your project with CMake:

* _(A)_ copy and build the libpqxx source tree.
* Or _(B),_ make use of a separately installed libpqxx.

We'll go through both of these.


Option (A): Copy and build the libpqxx source tree
--------------------------------------------------

This option is nice and portable.  If you want others to be able to compile
your project on their own systems, this method probably requires a bit less
setup than the other option.  It's easier to keep compatible compilation
options between libpqxx and your project.

On the flip side, building your project will take a bit longer, because it will
build libpqxx along the way.  And of course, you'll need to make sure that
there's a copy of the libpqxx tree somewhere.  You could do that by having the
libpqxx git repo as a submodule inside yours.

As mentioned, you'll need to have the PostgreSQL dev libraries installed on
your system, including their headers.

Let's say you have a copy of the libpqxx source tree inside your source tree as
`deps/libpqxx`, and you want to want to build libpqxx in a subdirectory of your
build directory called `build-pqxx`.

Given that, make sure you have a line like this in your `CMakeLists.txt`:

```cmake
    add_subdirectory(deps/libpqxx build-pqxx)
```

You'll also need to mention libpqxx in your `target_link_libraries`, so that
your code will link with libpqxx.  For example, if your project is named
`ausom` and links to no other libraries:

```cmake
    target_link_libraries(ausom PRIVATE pqxx)
```

So a simple working `CMakeLists.txt` might be:

```cmake
cmake_minimum_required(VERSION 3.18)
project(ausom)
set(CMAKE_CXX_STANDARD 20)
add_executable(ausom src/ausom.cxx)

add_subdirectory(deps/libpqxx build-pqxx)
target_link_libraries(ausom PRIVATE pqxx)
```


Option (B): Make use of a separately installed libpqxx
------------------------------------------------------

If libpqxx is already installed, you can just make use of that in your own
`CMakeLists.txt`:

```cmake
    find_package(libpqxx REQUIRED)
```

For this to work, there must be a configuration file `libpqxxConfig.cmake` or
`libpqxx-config.cmake` that CMake can find.  If this file exists but CMake
can't find it, you may have to set `libpqxx_DIR` in your `CMakeLists.txt` to
point to a directory containing that configuration file.

You'll also need to mention libpqxx in your `target_link_libraries`, so that
your code will link with libpqxx.  For example, if your project is named
`ausom` and links to no other libraries:

```cmake
    target_link_libraries(ausom PRIVATE pqxx)
```

So a simple working `CMakeLists.txt` might be:

```cmake
cmake_minimum_required(VERSION 3.18)
project(ausom)
set(CMAKE_CXX_STANDARD 20)
add_executable(ausom src/ausom.cxx)

find_package(libpqxx REQUIRED)
target_link_libraries(ausom PRIVATE pqxx)
```

Of course that does mean that you need libpqxx installed on your system.  We'll
look into that next.


Installing a libpqxx OS package
-------------------------------

Your operating system provider may have a packaged version of libpqxx ready for
you to install.  For example, on a Debian or Ubuntu system you might run:

```shell
    sudo apt-get install libpqxx-dev
```

But the exact name of the package you need may depend on the system.  In a
Debian-flavoured system, the `-dev` suffix is a convention for saying that you
want not just a binary library installed, but its header files as well.  You
definitely need those.  On a RedHat-flavoured system, the convention is a
suffix of `-devel` after the name, i.e. `libpqxx-devel`.

Other systems may conventionally include the headers with the library package.
For example, on macOS using HomeBrew, you would just...

```shell
    brew install libpqxx
```

Of course if there is no ready-made libpqxx package for your system, or it's
not up to date, or it does not provide a `libpqxx-config.cmake` configuration
file, another option is to build and install libpqxx using CMake.


Building and installing libpqxx yourself
----------------------------------------

If you just want to get libpqxx itself built and installed quickly, run `cmake`
from the root of the libpqxx source tree.  This configures your build.

Then compile libpqxx by running:

```shell
    cmake --build .
```

To install in the default location:

```shell
    cmake --install .
```

The rest of this document will go deeper into the details of this, but you may
not need it.


Stages
------

I'll explain the main build steps in more detail below, but here's a quick
rundown:

1. Configure
2. Compile
3. Test
4. Install
5. Use

The Test step is optional.


Configure
---------

Run `cmake` to configure your build.  It figures out various parameters, such
as where libpq and its headers are, which C++ features your compiler supports,
and which options your compiler needs.  CMake generates configuration for your
build tool: `Makefile`s for `make`, or a Solution (".sln") file for MSVC's
`msbuild`, and so on.

At this stage you can also override those options yourself. e.g. to instruct
the compiler to look for libpq in a non-standard place, or to use a different
compiler, or pass different compiler flags.  Don't try to specify those while
doing the actual compile; set them once when running `cmake`.

Let's say `$BUILD` is the directory where you want to build libpqxx, and
`$SRC` is where its source code is.  So for example, the readme file will be at
`$SRC/README.md`.

In the simplest case, you just do:

```shell
    cd $BUILD
    cmake $SRC
```

Add CMake options as needed.  There's more about the options below.  I'll also
explain the two directories.


### Cheat sheet

Here are some popular `cmake` options for libpqxx:

* `-DSKIP_BUILD_TEST=on` skips compiling libpqxx's tests.
* `-DBUILD_SHARED_LIBS=on` to build a shared library.
* `-DBUILD_SHARED_LIBS=off` to build a static library.
* `-DBUILD_DOC=on` to build documentation.
* `-DINSTALL_TEST=on` to install test executor binary.

On Windows, I recommend building libpqxx as a shared library and bundling it
with your application.  On other platforms I would prefer a static library.

Building the documentation requires some tools to be installed.  It takes at
least Doxygen, but there's no list of requirements.  The way to get this set up
is to just try it and see what it's missing.


### Generators

You can also choose your own build tool by telling CMake to use a particular
"generator."  For example, here's how to force use of `make`:

```shell
    cmake -G 'Unix Makefiles'
```

Or if you prefer to build using `ninja` instead:

```shell
    cmake -G Ninja
```

There are many more options.  You may prefer yet a different build tool.


### Finding libpq

The CMake step tries to figure out where libpq is, using Cmake's `find_package`
function.  If that doesn't work, or if you want a libpq in a different location
from the one it finds, there are two ways to override it.

The first is to set the individual include and link paths.

To make the build look for the libpq headers in some directory `$DIR`, add
these options:

* `-DPostgreSQL_TYPE_INCLUDE_DIR=$DIR`
* `-DPostgreSQL_INCLUDE_DIR=$DIR`

To make the build look for the libpq library binary in a directory `$DIR`, add
this option:

* `-DPostgreSQL_LIBRARY_DIR=$DIR`

The second, easier way requires CMake 3.12 or better.  Here, you specify a path
to a full PostgreSQL build tree.  You do this (again for some directory `$DIR`)
by simply passing this cmake option: `-DPostgreSQL_ROOT=$DIR`


### Source and Build trees

Where should you run `cmake`?

Two directories matter when building libpqxx: the _source tree_ (where the
libpqxx source code is) and the _build tree_ (where you want your build
artefacts).  Here I will call them `$SRC` and `$BUILD`, but you can call them
anything you like.

They can be one and the same, if you like.  It's convenient, but less clean, as
source code and build artefacts will exist in the same directory tree.  If
you're going to delete the source tree after installing, of course it's fine to
make a mess in there.


Compile
-------

To compile, run:

```shell
    cmake --build $BUILD
```

(Where `$BUILD` is again the directory where you wish to do the build.)

This command will invoke your build tool.  Other ways to do the same thing
would be...

* With Unix Makefiles: `make`
* With Ninja: `ninja`
* With Visual Studio: `msbuild libpqxx.sln`
* etc.

Depending on your build tool, you may want to speed this up by adding an option
like `-j 16`, where `16` is an example of how many processes you might want to
run in parallel.  The optimal number depends on your available CPUs and memory.
If you have enough memory, usually the number of CPUs will be a good starting
point for the right number.  Don't use this option with Ninja though.  It
figures things out for itself.


Test
----

Of course libpqxx comes with a test suite, to check that the library is
functioning correctly.

You can run it, but there's one caveat: you need to give it a database where it
can log in, without a password or any other parameters, and try out various
things.

And when I say you need to "give" it a database, I really mean "give."  The
test suite will create and drop tables.  Those will all have names prefixed
with "pqxx", so it's probably safe to use a database you already had, but if
any of the items in your database happen to have names starting with `pqxx`,
tough luck.  They're fair game.

Enter this in your shell to build and run the tests:

```shell
    test/runner
```


### Configuring the test database

But what if you do need a password to log into your test database?  Or, what if
it's running on a different system so you need to pass that machine's address?
What if it's not running on the default port?

You can set these parameters for the test suite, or for any other libpq-based
application, using the following environment variables.  (They only set default
values, so they won't override parameters that the application sets in some
other way.)

* `PGHOST` — the IP address where we can contact the database's socket.  Or
  for a Unix domain socket, its absolute path on the filesystem.
* `PGPORT` — TCP port number on which we can connect to the database.
* `PGDATABASE` — the name of the database to which you wish to connect.
* `PGUSER` — user name under which you wish to log in on the database.
* `PGPASSWORD` — user name's password for accessing the database.

See the full list at [
    https://www.postgresql.org/docs/current/libpq-envars.html
](https://www.postgresql.org/docs/current/libpq-envars.html)

**Be careful with passwords,** by the way.  Depending on your operating system
and configuration, an attacker with access to your machine could try to read
your password if you set it on the command line:

* Your shell may keep a log of the commands you have entered.
* Environment variables may be visible to other users on the system.

If at all possible, rely on postgres "peer authentication."  Once set up, it is
both more secure and more convenient than passwords.


Install
-------

Once you've built libpqxx, CMake can also help you install the library and
headers on your system.  The default installation location will vary from one
operating system to another, but you can set it explicitly.

Let's say you've got your finished build in `$BUILD`, and you want to install
it to your system's default install location.  The command for this is:

```shell
    cmake --install $BUILD
```

But you may want to install to some other location.  Let's call it `$DEST`.
`$DEST` might be something like `/usr/local` on a Unix-like system, or
something like `D:\Software` on a Windows system.

To install to `$DEST`, run:

```shell
    cmake --install $BUILD --prefix $DEST
```


Use
---

Other projects can include libpqxx in their CMake builds.

Here's an example of a `CMakeLists.txt` fragment.  You'll probably still need
to provide details specific to your project.

```cmake
    # (First set LIBVERSION to the libpqxx version you have.)
    set(libpqxxdir "libpqxx-${LIBVERSION}")

    # You can usually skip building the tests.
    set(SKIP_BUILD_TEST ON)

    # On Windows we generally recommend building libpqxx as a shared
    # library.  On other platforms, we recommend a static library.
    IF (WIN32)
        set(BUILD_SHARED_LIBS ON)
    ELSE()
        set(BUILD_SHARED_LIBS OFF)
    ENDIF()

    add_subdirectory(${libpqxxdir})
```

If you are using shared libraries (which is recommended when building on
Windows), you may need to ensure that libpq and the libraries it in turn
requires are all in your dynamic link path.
