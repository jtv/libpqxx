Supporting additional data types                              {#datatypes}
================================

Communication with the database mostly happens in a text format.  When you
include an integer value in a query, you use `to_string` to convert it to that
text format.  When you get a query result field "as a float," it converts from
the text format to a floating-point type.  These conversions are everywhere in
libpqxx.

The conversion sydstem supports many built-in types, but it is also extensible.
You can "teach" libpqxx (in the scope of your own application) to convert
additional types of values to and from PostgreSQL's string format.

This is massively useful, but it's not for the faint of heart.  You'll need to
specialise some templates.  And, **the API for doing this can change with any
major libpqxx release.**


Converting types
----------------

In your application, a conversion is driven entirely by a C++ type you specify.
The value's SQL type has nothing to do with it, nor is there anything in the
string that would identify its type.

So, if you've SELECTed a 64-bit integer from the database, and you try to
convert it to a C++ "short," one of two things will happen: either the number
is small enough to fit in your `short` (and it just works), or else it throws a
conversion exception.

Or, your database table might have a text column, but a given field may contain
a string that _looks_ just like a number.  You can convert that value to an
integer type just fine.  Or to a floating-point type.  All that matters to the
conversion is the actual value, and the type.

In some cases the templates for these conversions can tell the type from the
arguments you pass them:

    auto x = to_string(99);

In other cases you may need to instantiate template explicitly:

    auto y = from_string<int>("99");


Supporting a new type
---------------------

Let's say you have some other SQL type which you want to be able to store in,
or retrieve from, the database.  What would it take to support that?

Sometimes you do not need _complete_ support.  You might need a conversion _to_
a string but not _from_ a string, for example.  The conversion is defined at
compile time, so don't be too afraid to be incomplete.  If you leave out one of
these steps, it's not going to crash at run time or mess up your data.  The
worst that can happen is that your code won't build.

So what do you need for a complete conversion?

First off, of course, you need a C++ type.  It may be your own, but it
doesn't have to be.  It could be a type from a third-party library, or even one
from the standard library that libpqxx does not yet support.

You also specialise the `pqxx::type_name` variable to specify the type's name.
This is important for all code which mentions your type in human-readable text,
such as error messages.

Then, does your type have a built-in null value?  You specialise the
`pqxx::nullness` template to specify the details.

Finally, you specialise the `pqxx::string_traits` template.  This is where you
define the actual conversions.

Let's go through these steps one by one.


Your type
---------

You'll need a type for which the conversions are not yet defined, because the
C++ type is what determines the right conversion.  One type, one set of
conversions.

The type doesn't have to be one that you create.  The conversion logic was
designed such that you can build it around any type.  So you can just as
easily build a conversion for a type that's defined somewhere else.  There's
no need to include any special methods or other members inside it.  That's also
how libpqxx can support converting built-in types like `int`.

By the way, if the type is an enum, you don't need to do any of this.  Just
invoke the preprocessor macro `PQXX_DECLARE_ENUM_CONVERSION`, from the global
namespace near the top of your translation unit, and pass the type as an
argument.

The library also provides specialisations for `std::optional<T>`,
`std::shared_ptr<T>`, and `std::unique_ptr<T>`.  If you have conversions for
`T`, you'll also have conversions for those.


Specialise `type_name`
----------------------

When errors happen during conversion, libpqxx will compose error messages for
the user.  Sometimes these will include the name of the type that's being
converted.

To tell libpqxx the name of each type, there's a template variable called
`pqxx::type_name`.  For any given type `T`, it should have a specialisation
that provides that `T`'s human-readable name:

    namespace pqxx
    {
    template<> std::string const pqxx::type_name<T>{"T"};
    }

(Yes, this means that you need to define something inside the pqxx namespace.
Future versions of libpqxx may move this into a separate namespace.)

Define this early on in your translation unit, before any code that might cause
libpqxx to need the name.  That way, the libpqxx code which needs to know the
type's name can see your definition.


Specialise `nullness_traits`
----------------------------

A struct template `pqxx::nullness_traits` defines whether your type has a
natural "null value" built in.  If so, it also provides member functions for
producing and recognising null values.

The simplest scenario is also the most common: most types don't have a null
value built in.  In that case, define your nullness traits from
`pqxx::no_null`:

    namespace pqxx
    {
    template<> struct nullness_traits<T> : pqxx::no_null {};
    }

(Here again you're defining this in the pqxx namespace.)

If your type does have a natural null value, the definition gets a little more
complex:

    namespace pqxx
    {
    template<> struct nullness_traits<T>
    {
      static bool has_null{true};

      static bool is_null(T const &value)
      {
        // TODO: Return whether "value" is null.
      }

      [[nodiscard]] static T null()
      {
        // TODO: Return a null value.
      }
    };
    }

You may be wondering why there's a function to produce a null value, but also a
function to check whether a value is null.  Why not just compare the value to
the result of `null()`?  Because two null values may not be equal.  `T` may
have several different null values.  Or it may override the comparison
operator, similar to SQL where NULL is not equal to NULL.


Specialise `string_traits`
-------------------------

This part is more work.  Specialise the `pqxx::string_traits` template:

    namespace pqxx
    {
    template<> struct string_traits<T>
    {
      static T from_string(std::string_view text);
      static zview to_buf(char *begin, char *end, T const &value);
      static char *into_buf(char *begin, char *end, T const &value);
      static size_t size_buffer(T const &value) noexcept;
    };
    }

You'll also need to write those member functions, or as many of them as needed
to get your code to build.


### `from_string`

We start off simple: `from_string` parses a string as a value of `T`, and
returns that value.

The string may not be zero-terminated; it's just the `string_view` from
beginning to end (exclusive).  In your tests, cover cases where the string
does not end in a zero byte.

It's perfectly possible that the string isn't actually a `T` value.  Mistakes
happen.  In that case, throw a `pqxx::conversion_error`.

(Of course it's also possible that you run into some other error, so it's fine
to throw different exceptions as well.  But when it's definitely "this is not
the right format for a `T`," throw `conversion_error`.)


### `to_buf`

In this function, you convert a value of `T` into a string that the postgres
server will understand.

The caller will provide you with a buffer where you can write the string, if
you need it: from `begin` to `end` exclusive.  It's a half-open interval, so
don't access `*end`.

If the buffer is insufficient for you to do the conversion, throw a
`pqxx::conversion_overrun`.  It doesn't have to be exact: you can be a little
pessimistic and demand a bit more space than you need.  Just be sure to throw
the exception if there's any risk of overrunning the buffer.

You don't _have_ to use the buffer for this function though.  For example,
`pqxx::string_traits<bool>::to_buf` returns a compile-time constant string and
ignores the buffer.

Even if you do use the buffer, your string does not _have_ to start at the
beginning of the buffer.  For example, the integer conversions start by writing
the _least_ significant digit to the _end_ of the buffer, and then writes the
more significant digits before it.  It was just more convenient.

Return a `pqxx::zview`.  This is basically a `std::string_view`, but with one
difference: a `zview` guarantees that there will be a valid zero byte right
after the `string_view`.  The zero byte is not counted as part of its size, but
it will be there.

Expressed in code, this rule must hold:

    void invariant(zview z)
    {
      assert(z[z.size()] == 0);
    }

Make sure you write your trailing zero _before_ the `end`.  If the trailing
zero doesn't fit in the buffer, then there's just not enough room to perform
the conversion.

Beware of locales when converting.  If you use standard library features like
`sprintf`, they may obey whatever locale is currently set on the system.   That
means that a simple integer like 1000000 may come out as "1000000" on your
system, but as "1,000,000" on mine, or as "1.000.000" for somebody else, and on
an Indian system it may be different again.  Check the documentation for such
functions, and set the C locale if necessary.  Or if you can, use the libpqxx
conversions for the pre-existing types.


### `into_buf`

This is a stricter version of `to_buf`.  All the same requirements apply, but
in addition you must write your string into the buffer provided, starting
_exactly_ at `begin`.

That's why this function returns just a simple pointer: the address right
behind the trailing zero.  If the caller wants to use the string, they can
find it at `begin`.  If they want to write a different value into the rest of
the buffer, they can start at the location you returned.


### `size_buffer`

Here you estimate how much buffer space you need for converting a `T` to a
string.  Be precise if you can, but pessimistic if you must.  It's usually
better to waste a few unnecessary bytes than to spend a lot of time computing
the exact buffer space you need.  And failing the conversion because you
under-budgeted the buffer is worst of all.

Include the trailing zero in the buffer size.  If your `to_buf` takes more
space than just what's needed to store the result, include that too.

Make `size_buffer` a `constexpr` function if you can.  It can allow the caller
to allocate the buffer on the stack, with a size known at compile time.
