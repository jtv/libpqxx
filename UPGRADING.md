Changes in 8.0
==============

Many things have changed.  Most code written against 7.x will still work, but
there may now be nicer ways of doing things.  Some of your code may need
updating, and I'm sorry about that!  But I hope you'll be happy overall.

Let's go through the main things.


C++20
-----

You will need at least C++20 to compile libpqxx.  Support does not need to be
absolutely complete, but the basics must be there: concepts, various utility
functions in the standard library, the language's new object lifetime rules,
and so on.

Thanks to `std::span`, there are now very few raw pointers in the API (or
even internally).  This makes libpqxx not only safer, but also easier to
_prove_ safe.  It is important to be able to run static analysis tools that
may detect bugs and vulnerabilities.  But, in the C++ tradition, it shouldn't
cost us anything in terms of speed.


PostgreSQL & libpq versions
---------------------------

When you connect to a database, libpqxx checks that it is compatible with the
PostgreSQL server version.  If not, it refuses to connect in order to avoid
nasty surprises later.

This wasn't very noticeable before because the minimum version was set to 8.0
and I never remembered to update it.  Now I've bumped it up.  This will happen
from time to time.

If the bump causes you problems, please seriously consider upgrading your
server.


Retiring deprecated items
-------------------------

Some types and functions were already deprecated and are now gone:

* `binarystring`.  Use `blob` instead.
* `connection_base` was an alias.  Use just regular `connection`.
* `encrypt_password()`.  Use the equivalent member functions in `connection`.
* `dynamic_params`.  Use `params`.
* `stream_from`.  Use `transaction_base::stream()` instead.
* Some `stream_to` constructors.  Use static "factory" member functions.
* `transaction_base::unesc_raw()`.  Use `unesc_bin()`.
* `transaction_base::quote_raw()`.  Use `quote()`, passing a `bytes_view`.
* Some `field` constructors.
* Result row splicing.  I don't think anyone ever used it.


Result iterators
----------------

_Lifetimes:_ `result` and `row` iterators no longer reference-count their
`result` object.  In libpqxx 7, a `result` object's data would stay alive in
memory for as long as you had an iterator referring to it.  It seemed like a
good idea once, many years ago, but it made iteration staggeringly inefficient.

So, that's changed now.  Just like any other iterator in C++, `result` and
`row` iterators no longer keep your container alive.  It's your own problem to
ensure that.  However `result` objects are still reference-counted smart
pointers to the underlying data, so copying them is relatively cheap.

_Semantics:_  Iterators for the `result` and `row` classes have never
fully complied with the standard requirements for iterators.

There was a good reason for this: I wanted these iterators to be convenient and
work a lot like C pointers.  I wanted you to be able to dereference them as if
a `result` object were just an array of pointers to arrays.

But it really started getting in the way.  Much as I hate it, the standard
requirements say that for any iterator `i`, `i[n]` should mean `*(i + n)`.  In
libpqxx, if you had a `result` iterator `i`, `i[n]` meant "field `n` in the row
to which `i` points."  Really convenient, but not compliant with the standard.

So, that is no longer the case.  If you want "field `n` in the row to which `i`
points," say `(*i)[n]`, or `i->at(n)`.

By the way, these changes mean that result and row iterators are now proper,
standard random-access iterators.  Some algorithms from the standard library
may magically become more efficient when they detect this.


Comparing results, rows, and fields
-----------------------------------

When you compare two `result`, `row`, or `field` objects with `==` or `!=`, it
now only checks whether they refer to (the same row/field in) the same
underlying data object.  It does not look at the data inside.

These comparisons were meant to be helpful but they were never very well
defined.  And if you don't know exactly what you're getting, why would you want
to invest the compute time?


Row and field references
------------------------

The `row` and `field` classes were cumbersome, inefficient, and hopelessly
intertwined with iterators.

To avoid all that, use `row_ref` instead of `row` and `field_ref` instead of
`field`.  These assume that you keep the original `result` around, and in a
stable location in memory.  Unlike `row` and `field`, they do not keep the
underlying data object alive through reference-counting.  But neither do any of
the standard C++ containers, so I hope you'll find this intuitive.  It's
certainly more efficient.

_The indexing operations now return `row_ref` and `field_ref` instead of `row`
and `field` respectively._  It probably won't affect your code, but you're
running static analysis and instrumented builds to check for these things,
right?


Exceptions
----------

The exception hierarchy has changed radically.  An exception no longer tries
to tell your application everything it needs to know through its _type;_ there
are _functions_ to tell you things like "can I still use my connection after
this?" and "what should I call this type of exception?"

That comes with a reorganisation of the hierarchy itself.  It used to be that
an exception that breaks your connection was always derived from
`broken_connection`.  _That is no longer the case._

There is now a single base class for all libpqxx exceptions, and it comes with
all the properties that any of the libpqxx exceptions might need.  Some of
them will be irrelevant for some exceptions, in which case they return an
empty string.

So what should your `catch` code look like now?

In some situations you'll need to catch a very specific error.  That's where
you still have the individual libpqxx exception classes, which you can catch
individually or through inheritance grouping.

But for a more general `try` block, write `catch` clauses in this order:

1. Any non-libpqxx exceptions, most specific ones first.
2. If needed, any _specific_ libpqxx exceptions that you want to separate out.
3. A single `catch` block on `pqxx::failure`.

That final block can sort out the details by calling the exception's member
functions, and checking its type using `typeid` and `dynamic_cast` if needed.
What do I tell the user?  What do I log?  What SQL were we executing when this
happened?  Where in my application did it happen?  Can I find more background
on this SQL problem?  Should I continue my transaction?  Is it even worth
trying to recover using the same connection?

Handling exceptions can be messy and unclear at the best of times, but the new
exception arrangement tries to give you something reasonable to work with.


Binary data
-----------

As the documentation predicted, the alias `pqxx::bytes` has changed to stand
for `std::vector<std::byte>`.  It used to be `std::basic_string<std::byte>`.
And `pqxx::bytes_view` is now an alias for `std::span<std::byte>`.

This may require changes to your code.  The APIs for `std::basic_string` and
`std::vector` differ, perhaps in more places than they should.  Do not read
the data using the `c_str()` member function; use `data()` instead.

Hate to do this to you.  However there were real problems with using
`std::basic_string` the way we did.  The `basic_string` template wasn't built
for binary data, and there was no guarantee that it would work with any given
compiler.  Even where it did, we had to work around differences between
compilers and compiler versions.  That's not healthy.

But there's also good news!  Thanks to C++20's Concepts, most functions that
previously only accepted a `pqxx::bytes` argument will now accept just about
_any_ block of binary data, such as `std::array<std::byte>` or a C-style array.


String conversion API
---------------------

This is a whole chapter in itself.  If you were specialising `pqxx::type_name`
or `pqxx::string_traits`, things just got simpler, safer, and more efficient.
But there's also a change in lifetime rules.  This can be important for writing
safe and correct code, so read on!


### Type names

Let's get the easiest part out of the way first: `type_name` is now deprecated.
Instead of specialising `type_name`, you now specialise  a _function_ called
`name_type()`.  It returns a `std::string_view`, so the underlying data can be
a string literal instead of a `std::string`.  Some static analysis tools would
report false positives about static deallocation of the `type_name` strings.


### Defining a string conversion

Then there's `pqxx::string_traits`.  Several changes here.  If you were
defining your own conversions to/from SQL strings, the 8.x API is...

1. _leaner,_ losing several members but mainly `into_string()`.
2. _safer,_ using views and spans rather than raw C-style pointers.
3. _simpler,_ dropping the terminating zero at the end of various strings.
4. _friendlier,_ accepting `std::source_location` for better error reporting.
5. _richer,_ capable of dealing with different text encodings.
6. _faster,_ because of the lifetime rule changes (described below).

Your existing conversions may still work without changes, but that's only
thanks to some specific compatibility shims.  These will go away in libpqxx 9
at the latest, so I urge you to update your code.


### Lifetime rule changes

In libpqxx 7.x, when you converted a C++ value to an SQL string or _vice
versa,_ you could count on having a valid output for at least as long as you
kept the object in which you stored it.  The details depend on the type, e.g.
`pqxx::string_traits<bool>::to_string()` returns a string constant, either
"`true`" or "`false`".

We're tightening that up in 8.x.  Now, the output remains valid for at least as
long as you keep the object in which you stored it, _and also keep the
original in place._ This streamlines some trivial conversions.  For example,
converting a `std::string_view` or a C-style string to SQL is now just a matter
of returning a view on that same data.  It's just the same data in both C++ and
SQL.

It also enables some conversions that weren't possible before.  You can now
convert an SQL string to a `std::string_view` or a C-style string pointer,
because the conversion is allowed to refer to its input data.

Most code won't need to care about this change.  A calling function is usually
either done very quickly with its converted value, or it immediately arranges
for more permanent storage.  If you call `pqxx::to_string()` for example, you
get a `std::string` containing the SQL string.  All that changes in that case
is the conversion process skipping an internal copy step, making it a bit
faster.


### Using conversions

There is now a richer, more complete set of global conversion functions.  This
means you can convert C++ values to SQL strings without calling their
`pqxx::string_traits` functions yourself.

The options are, from easiest to most efficient:

* `pqxx::to_string()` returns the SQL value as a `std::string`.
* `pqxx::into_buf()` renders the SQL value into a buffer you provide.
* `pqxx::to_buf()` returns the SQL value, in a location of its own choosing.

The `to_buf()` variant can make use of optimisations such as returning a view
or pointer referring to the input you give it, or returning a string constant
(e.g. "`true`" or "`false`").

That concludes the changes to the string conversion API.  On to the new
features that won't require you to change your code, but may improve your
life.


SQL Arrays
----------

You can now generate and parse SQL arrays, just like you can convert so many
other types between their C++ representations and their SQL representations.

The C++ type for this is the `pqxx::array` class template.  You can parse an
array from its SQL string representation in the same way you'd parse any other
type of value: `my_array = pqxx::from_string<pqxx::array<...>>(text)`.  Which
means that a `result` field's `as()` member function will also support array
parsing, just as you would expect.

And in the other direction, you can use `pqxx::to_string(my_array)` to generate
the SQL representation string for your `array` object.

The `to_string()` direction will also work for other containers, by the way.
So you can just convert a `std::vector<int>` or a `std::array<std::string>` in
the same way and get their SQL array representations.

You'll need to know what those template parameters in `array<...>` are:

1. The type for the elements inside the array.
2. The number of dimensions. A simple array has 1 dimension, which is the
   default, but multi-dimensional arrays also work.
3. The separator character between the elements in the SQL array.  It defaults
   to whatever the default is for the element type, but you can override it.
   However it must be a single ASCII byte.

A `pqxx::array` is mostly useful for loading SQL values into C++.  it does not
not support changing or manipulating the array's contents once you've got it.
When going in the other direction however, you can use the standard C++
container or range types: `std::vector`, `std::array`, `std::range`,
`std::view`, and so on.  Nest these inside each other to form multi-dimensional
SQL arrays.


Composite types
---------------

These are syntactically similar to arrays, but a bit more work.  An SQL
composite type is like a C++ `struct` of your own.  You define it in SQL
(whether through libpqxx or in some other way), and also a corresponding C++
type for the client side.

You can then convert these between their C++ form and their SQL form, just like
other types.  To make that work, you'll have to define its `pqxx::string_traits`
and `pqxx::nullness` traits types.  The `composite.hxx` header contains some
functions that you can call to handle most of the work that the conversions in
the `string_traits` need to do.


Source locations
----------------

Most of the functions in the libpqxx API now accept a `std::source_location` as
a final argument.  They will default to the location from where you call them.

(To keep them from getting in the way, I abbreviated the type name to
`pqxx::sl`.  In most functions it just shows up as `sl`.)

In many cases where libpqxx throws an exception, the error message will now
include a reference to that source location to help you debug the error.

Where possible, this will be the most precise location where you called into
libpqxx.  In places where libpqxx functions call other libpqxx functions, they
will (where possible) pass the original source location along, so you're not
left with a source reference that has no meaning to you and is nothing to do
with your application.  By default the source location will show the boundary
where execution went from your code into libpqxx, even if the actual error
happened many layers further down.

You can also override this and pass your own source location in the call.

In some situations it is not possible to pass a source location, but libpqxx
will try to give you at least some useful location, such as the location where
you created the object on which the error occurred.

Expect this whole mechanism to change again in the future as C++ moves towards
full traceback support.


Text encodings
--------------

PostgreSQL lets you communicate with the database in a choice of _client
encodings._  SQL and data that go back and forth between your client and the
server will be represented in this encoding.

For a lot of purposes your code will not need to be aware of the encoding.
For example, all SQL keywords themselves as well as all special characters such
as quotes and commas and various field separators will all be in ASCII.

There are cases where libpqxx needs to know what kind of encoding it's getting.
It doesn't care about the exact encoding, but it needs to know where each
character begins and ends, so it can detect special characters such as closing
quotes, withing being confused by the same byte values occurring inside a
multi-byte character.

This information is now also available to string conversions, to help them
parse SQL data that may contain text in the client encoding.


Build changes
-------------

The build has improved in many ways.  As before, you get a choice of CMake or
the `configure` script.  The CMake build now also supports "unity builds."

All shell scripts now have a `.sh` suffix.  It takes some getting used to, but
it simplifies some things such as running lint checkers on all of them without
having to name each one of them individually.

The build no longer tries to figure out whether you need to link a standard C++
filesystems library.  In most cases this seems to be part of the regular
standard library now, and at any rate libpqxx itself does not use it.  If you
do need to add a link option to get `std::filesystem` to work, you'll have to
pass that option yourself.

In the lint script (now called `tools/lint.sh`), set `PQXX_LINT=skip` to skip
lint checks.  This can speed up `make check` runs, and enable them to run
without network access.
