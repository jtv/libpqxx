libpqxx
=======

Welcome to libpqxx, the C++ API to the PostgreSQL database management system.

Home page: [
    http://pqxx.org/development/libpqxx
](http://pqxx.org/development/libpqxx/)

Find libpqxx on Github: [
    https://github.com/jtv/libpqxx
](https://github.com/jtv/libpqxx)

Documentation on Read The Docs: [
    https://libpqxx.readthedocs.io
](https://libpqxx.readthedocs.io)

Compiling this package requires PostgreSQL to be installed -- or at least the C
headers and library for client development.  The library builds on top of
PostgreSQL's standard C API, libpq, though your code won't notice.

If you're getting the code straight from the Git repo, the head of the `master`
branch represents the current _development version._  Releases are tags on
commits in `master`.  For example, to get version 7.1.1:

```sh
    git checkout 7.1.1
```


Thank you!
----------

Somebody who deserves special mention here: our first Donor-level sponsor is
Klaus ([@klaussilveira](https://github.com/klaussilveira)).  Thank you, Klaus!


Upgrade notes
-------------

**The 8.x versions require at least C++20.**  Make sure your compiler is up to
date.


Building libpqxx
----------------

There are two different ways of building libpqxx from the command line:

1. Using CMake, on any system which supports it.
2. On Unix-like systems, using a `configure` script.

"Unix-like" systems include GNU/Linux, Apple macOS and the BSD family, AIX,
HP-UX, Irix, Solaris, etc.  Even on Microsoft Windows, a Unix-like environment
such as WSL, Cygwin, or MinGW should work.

You'll find detailed build and install instructions in [`BUILDING-configure.md`](./BUILDING-configure.md)
and [`BUILDING-cmake.md`](./BUILDING-cmake.md), respectively.

And if you're working with Microsoft Visual Studio, have a look at Gordon
Elliott's
[
  Easy-PQXX Build for Windows Visual Studio
](https://github.com/GordonLElliott/Easy-PQXX-Build-for-Windows-Visual-Studio)
project.


Documentation
-------------

Building the library, if you have the right tools installed, generates HTML
documentation in the `doc/` directory.  It is based on the headers in
`include/pqxx/` and text in `include/pqxx/doc/`.  This documentation is also
available online at [readthedocs](https://libpqxx.readthedocs.io).


Programming with libpqxx
------------------------

Your first program will involve the libpqxx classes `connection` (see the
`pqxx/connection.hxx` header), and `work` (a convenience alias for
`transaction<>`).  There are various transaction classes, but they all conform
to the interface defined in the `transaction_base` class (in the
`<pqxx/transaction_base.hxx>` header).

These `*.hxx` headers are not the ones you include in your program.  Instead,
include the versions without filename suffix (e.g. `pqxx/connection`).  Those
will include the actual .hxx files for you.  This was done so that includes are
in standard C++ style (as in `<iostream>` etc.), but an editor will still
recognize them as files containing C++ code.  Unless you care a lot about
compilation speed, you'll probably just want to include everything:

```cxx
#include <pqxx/pqxx>
```

Continuing the list of classes, you may also need the `pqxx::result` class
(`pqxx/result.hxx`).  In a nutshell, you create a `pqxx::connection` based on a
PostgreSQL connection string (see below), create a pqxx::work (a transaction
object) in the context of that connection, and run one or more queries or other
SQL commands on that.

Depending on how you execute a query, it can return a stream of `std::tuple`
(each representing one row); or just one tuple or field; or a `pqxx::result`
object which holds both the result data and additional metadata.  The metadata
includes details like how many rows your query returned or modified, what the
column names are, and so on.  A `pqxx::result` acts as a container of
`pqxx::row_ref` objects, and a `pqxx::row_ref` in turn acts as a container of
`pqxx::field_ref` objects.

(They aren't _really_ containers; all these classes are essentially references
to the real data returned by libpq.)

Here's an example with all the basics to get you going:

```c++
    #include <iostream>
    #include <pqxx/pqxx>

    int main()
    {
        try
        {
            // Connect to the database.  You can have multiple connections open
            // at the same time, even to the same database.
            pqxx::connection cx;
            std::cout << "Connected to " << cx.dbname() << '\n';

            // Start a transaction.  A connection can only have one transaction
            // open at the same time, but after you finish a transaction, you
            // can start a new one on the same connection.
            pqxx::work tx{cx};

            // Query data of two columns, converting them to std::string and
            // int respectively.  Iterate the rows.
            for (auto [name, salary] : tx.query<std::string, int>(
                "SELECT name, salary FROM employee ORDER BY name"))
            {
                std::cout << name << " earns " << salary << ".\n";
            }

            // For large amounts of data, "streaming" the results is more
            // efficient.  It does not work for all types of queries though.
            //
            // You can read fields as std::string_view here, which is not
            // something you can do in most places.  A string_view becomes
            // meaningless when the underlying string ceases to exist.  In this
            // particular situation, you can convert a field to string_view and
            // it will be valid for just that one iteration of the loop.  The
            // next iteration may overwrite or deallocate its buffer space.
            for (auto [name, salary] : tx.stream<std::string_view, int>(
                "SELECT name, salary FROM employee"))
            {
                std::cout << name << " earns " << salary << ".\n";
            }

            // Execute a statement, and check that it returns 0 rows of data.
            // This will throw pqxx::unexpected_rows if the query returns rows.
            std::cout << "Doubling all employees' salaries...\n";
            tx.exec("UPDATE employee SET salary = salary*2").no_rows();

            // Shorthand: conveniently query a single value from the database,
            // and convert it to an `int`.
            int my_salary = tx.query_value<int>(
                "SELECT salary FROM employee WHERE name = 'Me'");
            std::cout << "I now earn " << my_salary << ".\n";

            // Or, query one whole row.  This function will throw an exception
            // unless the result contains exactly 1 row.
            auto [top_name, top_salary] = tx.query1<std::string, int>(
                R"(
                    SELECT name, salary
                    FROM employee
                    WHERE salary = (SELECT max(salary) FROM employee)
                    LIMIT 1
                )");
            std::cout << "Top earner is " << top_name << " with a salary of "
                      << top_salary << ".\n";

            // If you need to access the result metadata, not just the actual
            // field values, use `exec()`.  It returns a pqxx::result object.
            pqxx::result res = tx.exec("SELECT * FROM employee");
            std::cout << "Columns:\n";
            for (pqxx::row_size_type col = 0; col < res.columns(); ++col)
                std::cout << res.column_name(col) << '\n';

            // Commit the transaction.  If you don't do this, the database will
            // undo any changes you made in the transaction.
            std::cout << "Making changes definite: ";
            tx.commit();
            std::cout << "OK.\n";
        }
        catch (std::exception const &e)
        {
            std::cerr << "ERROR: " << e.what() << '\n';
            return 1;
        }
        return 0;
    }
```


Connection strings
------------------

Postgres connection strings state which database server you wish to connect to,
under which username, using which password, and so on.  Their format is defined
in the documentation for libpq, the C client interface for PostgreSQL.
Alternatively, these values may be defined by setting certain environment
variables as documented in e.g. the manual for psql, the command line interface
to PostgreSQL.  Again the definitions are the same for libpqxx-based programs.

The connection strings and variables are not fully and definitively documented
here; this document will tell you just enough to get going.  Check the
PostgreSQL documentation for authoritative information.

The connection string consists of attribute=value pairs separated by spaces,
e.g. "user=john password=1x2y3z4".  The valid attributes include:

* `host` —
  Name of server to connect to, or the full file path (beginning with a
  slash) to a Unix-domain socket on the local machine.  Defaults to
  "/tmp".  Equivalent to (but overrides) environment variable PGHOST.
* `hostaddr` —
  IP address of a server to connect to; mutually exclusive with "host".
* `port` —
  Port number at the server host to connect to, or socket file name
  extension for Unix-domain connections.  Equivalent to (but overrides)
  environment variable PGPORT.
* `dbname` —
  Name of the database to connect to.  A single server may host multiple
  databases.  Defaults to the same name as the current user's name.
  Equivalent to (but overrides) environment variable PGDATABASE.
* `user` —
  User name to connect under.  This defaults to the name of the current
  user, although PostgreSQL users are not necessarily the same thing as
  system users.
* `requiressl` —
  If set to 1, demands an encrypted SSL connection (and fails if no SSL
  connection can be created).

Settings in the connection strings override the environment variables, which in
turn override the default, on a variable-by-variable basis.  You only need to
define those variables that require non-default values.


Linking with libpqxx
--------------------

To link your final program, make sure you link to both libpqxx and the
underlying C-level library, libpq, in that order.

With most Unix-style compilers, you'd do this using these options while
linking: `-lpqxx -lpq`

Both libraries must be in your link path, so the linker knows where to find
them.  Any dynamic libraries you use must also be in a place where the loader
can find them when loading your program at runtime.

Sometimes people run into trouble with this when they have more than one
lipqxx binary on their system.  If you get massive link errors, try removing
the "-lpqxx" argument from the command line and replacing it with the name of
the libpqxx library binary instead.  That's typically libpqxx.a, but you'll
have to add the path to its location as well, e.g.
`/usr/local/pqxx/lib/libpqxx.a`.  This will ensure that the linker will use
that exact version of the library rather than one found elsewhere on the
system, and eliminate worries about the exact right version of the library
being installed with your program..

If you get a moderate number fo link errors which include a missing symbol
`check_pqxx_version_X_Y` (where `X` and `Y` are numbers), that's typically a
sign that you're linking to a different version of libpqxx than the one whose
headers your compiler used while compiling.

And Visual Studio users in particular, beware: when linking object files and
libraries, _do not mix Release artefacts with Debug artefacts._  This can be
especially confusing when you install libpqxx from a pre-built package.  Which
kind of binary did you get?

Actually the same thing _can_ happen with other compilers: most compilers have
options that change the code they produce in incompatible ways.  Mixing files
from different kinds of builds can lead to link errors, or worse, crashes.
