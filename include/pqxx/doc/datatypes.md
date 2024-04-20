Supporting additional data types                              {#datatypes}
================================

Communication with the database mostly happens in a text format.  When you
include an integer value in a query, either you use `to_string` to convert it
to that text format, or under the bonnet, libpqxx does it for you.  When you
get a query result field "as a float," libpqxx converts from the text format to
a floating-point type.  These conversions are everywhere in libpqxx.

The conversion system supports many built-in types, but it is also extensible.
You can "teach" libpqxx (in the scope of your own application) to convert
additional types of values to and from PostgreSQL's string format.

This is massively useful, but it's not for the faint of heart.  You'll need to
specialise some templates.  And, **the API for doing this can change with any
major libpqxx release.**

If that happens, your code may fail to compile with the newer libpqxx version,
and you'll have to go through the `NEWS` file to find the API changes.  Usually
it'll be a small change, like an additional function you need to implement, or
a constant you need to define.


Converting types
----------------

In your application, a conversion is driven entirely by a C++ type you specify.
The value's SQL type on the database side has nothing to do with it.  Nor is
there anything in the string that would identify its type.  Your code says
"convert to this type" and libpqxx does it.

So, if you've SELECTed a 64-bit integer from the database, and you try to
convert it to a C++ `short,` one of two things will happen: either the number
is small enough to fit in your `short` and it just works, or else it throws a
conversion exception.  Similarly, if you try to read a 32-bit SQL `int` as a
C++ 32-bit `unsigned int`, that'll work fine, unless the value happens to be
negative.  In such cases the conversion will throw a `conversion_error`.

Or, your database table might have a text column, but a given field may contain
a string that _looks_ just like a number.  You can convert that value to an
integer type just fine.  Or to a floating-point type.  All that matters to the
conversion is the actual value, and the type your code specifies.

In some cases the templates for these conversions can tell the type from the
arguments you pass them:

```cxx
    auto x = to_string(99);
```

In other cases you may need to instantiate template explicitly:

```cxx
    auto y = from_string<int>("99");
```


Supporting a new type
---------------------

Let's say you have some other SQL type which you want to be able to store in,
or retrieve from, the database.  What would it take to support that?

Sometimes you do not need _complete_ support.  You might need a conversion _to_
a string but not _from_ a string, for example.  You write out the conversion at
compile time, so don't be too afraid to be incomplete.  If you leave out one of
these steps, it's not going to crash at run time or mess up your data.  The
worst that can happen is that your code won't build.

So what do you need for a complete conversion?

First off, of course, you need a C++ type.  It may be your own, but it doesn't
have to be.  It could be a type from a third-party library, or even one from
the standard library that libpqxx does not yet support.

First thing to do is specialise the `pqxx::type_name` variable to give the type
a human-readable name.  That allows libpqxx error messages and such to talk
about the type.  If you don't define a name, libpqxx will try to figure one out
with some help from the compiler, but it may not always be easy to read.

Then, does your type have a built-in null value?  For example, a `char *` can
be null on the C++ side.  Or some types are _always_ null, such as `nullptr`.
You specialise the `pqxx::nullness` template to specify the details.

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
no need to include any special methods or other members inside the type itself.
That's also why libpqxx can convert built-in types like `int`.

By the way, if the type is an enum, you don't need to do any of this.  Just
invoke the preprocessor macro `PQXX_DECLARE_ENUM_CONVERSION`, from the global
namespace near the top of your translation unit, and pass the type as an
argument.

The library also provides specialisations for `std::optional<T>`,
`std::shared_ptr<T>`, and `std::unique_ptr<T>`.  If you have conversions for
`T`, you'll also automatically have conversions for those.


Specialise `type_name`
----------------------

When errors happen during conversion, libpqxx will compose error messages for
the user.  Sometimes these will include the name of the type that's being
converted.

To tell libpqxx the name of each type, there's a template variable called
`pqxx::type_name`.  For any given type `T`, it should have a specialisation
that provides that `T`'s human-readable name:

```cxx
    // T is your type.
    namespace pqxx
    {
    template<> std::string const type_name<T>{"My T type's name"};
    }
```

(Yes, this means that you need to define something inside the pqxx namespace.
Future versions of libpqxx may move this into a separate namespace.)

Define this early on in your translation unit, before any code that might cause
libpqxx to need the name.  That way, the libpqxx code which needs to know the
type's name can see your definition.


Specialise `nullness`
---------------------

A struct template `pqxx::nullness` defines whether your type has a natural
"null value" built in.  If so, it also provides member functions for producing
and recognising null values.

The simplest scenario is also the most common: most types don't have a null
value built in.  There is no "null `int`" in C++.  In that kind of case, just
derive your nullness traits from `pqxx::no_null` as a shorthand:

```cxx
    // T is your type.
    namespace pqxx
    {
    template<> struct nullness<T> : pqxx::no_null<T> {};
    }
```

(Here again you're defining this in the pqxx namespace.)

If your type does have a natural null value, the definition gets a little more
complex:

```cxx
    namespace pqxx
    {
    // T is your type.
    template<> struct nullness<T>
    {
      // Does T have a value that should translate to an SQL null?
      static constexpr bool has_null{true};

      // Does this C++ type always denote an SQL null, like with nullptr_t?
      static constexpr bool always_null{false};

      static bool is_null(T const &value)
      {
        // Return whether "value" is null.
        return ...;
      }

      [[nodiscard]] static T null()
      {
        // Return a null value.
        return ...;
      }
    };
    }
```

You may be wondering why there's a function to produce a null value, but also a
function to check whether a value is null.  Why not just compare the value to
the result of `null()`?  Because two null values may not be equal (like in SQL,
where `NULL <> NULL`).  Or `T` may have multiple different null values.  Or `T`
may override the comparison operator to behave in some unusual way.

As a third case, your type may be one that _always_ represents a null value.
This is the case for `std::nullptr_t` and `std::nullopt_t`.  In that case, you
set `nullness<TYPE>::always_null` to `true` (as well as `has_null` of course),
and you won't need to define any actual conversions.


Specialise `string_traits`
-------------------------

This part is the most work.  You can skip it for types that are _always_ null,
but those will be rare.

The APIs for doing this are designed so that you don't need to allocate memory
on the free store, also known as "the heap": `new`/`delete`.  Memory allocation
can be hidden inside `std::string`, `std::vector`, etc.  The conversion API
allows you to use `std::string` for convenience, or memory buffers for speed.

Start by specialising the `pqxx::string_traits` template.  You don't absolutely
have to implement all parts of this API.  Generally, if it compilers, you're OK
for the time being.  Just bear in mind that future libpqxx versions may change
the API — or how it uses the API internally.

```cxx
    namespace pqxx
    {
    // T is your type.
    template<> struct string_traits<T>
    {
      // Do you support converting T to PostgreSQL string format?
      static constexpr bool converts_to_string{true};
      // Do you support converting PostgreSQL string format to T?
      static constexpr bool converts_from_string{true};

      // If converts_to_string is true:

      // Write string version into buffer, or return a constant string.
      static zview to_buf(char *begin, char *end, T const &value);

      // Write string version into buffer.
      static char *into_buf(char *begin, char *end, T const &value);

      // Converting value to string may require this much buffer space at most.
      static std::size_t size_buffer(T const &value) noexcept;

      // If converts_from_string is true:

      // Parse text as a T value.
      static T from_string(std::string_view text);
    };
    }
```

You'll also need to write those member functions, or as many of them as needed
to get your code to build.


### `from_string`

We start off simple: `from_string` parses a string as a value of `T`, and
returns that value.

The string may or may not be zero-terminated; it's just the `string_view` from
beginning to end (with `end` being exclusive).  In your tests, be sure to cover
cases where the string does not end in a zero byte!

It's perfectly possible that the string doesn't actually represent a `T` value.
Mistakes happen.  There can be corner cases.  When you run into this, throw a
`pqxx::conversion_error`.

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
completely ignores the buffer.

Even if you do use the buffer, your string does not _have_ to start at the
beginning of the buffer.  For example, the integer conversions may work from
right to left, if that's easier: they can start by writing the _least_
significant digit to the _end_ of the buffer, divide the remainder by 10, and
repeat for the next digit.

Return a `pqxx::zview`.  This is basically a `std::string_view`, but with one
difference: when you create a `zview` you _guarantee_ that there is a valid
zero byte right after the `string_view`.  The zero byte does not count as part
of its size, but it has to be there.

Expressed in code, this rule must hold:

```cxx
    void invariant(zview z)
    {
      assert(z[std::size(z)] == 0);
    }
```

The trailing zero should not go inside the `zview`, but if you convert into the
buffer, do make sure you that trailing stays inside the buffer, i.e. before the
`end`.  (If there's no room for that zero inside the buffer, throw
`pqxx::conversion_error`).

Beware of locales when converting.  If you use standard library features like
`sprintf`, they may obey whatever locale is currently set on the system where
the code runs.   That means that a simple integer like 1000000 may come out as
"1000000" on your system, but as "1,000,000" on mine, or as "1.000.000" for
somebody else, and on an Indian system it may be "1,00,000".  Don't let that
happen, or it will confuse things.  Use only non-locale-sensitive library
functions.  Values coming from or going to the database should be in fixed,
non-localised formats.

If your conversions need to deal with fields in types that libpqxx already
supports, you can use the conversion functions for those: `pqxx::from_string`,
`pqxx::to_string`, `pqxx::to_buf`.  They in turn will call the `string_traits`
specialisations for those types.  Or, you can call their `string_traits`
directly.


### `into_buf`

This is a stricter version of `to_buf`.  All the same requirements apply, but
in addition you must write your string _into the given buffer,_ starting
_exactly_ at `begin`.

That's why this function returns just a simple pointer: the address right
behind the trailing zero.  If the caller wants to use the string, they can
find it at `begin`.  If they want to write another value into the rest of the
buffer, they can continue writing at the location you returned.


### `size_buffer`

Here you estimate how much buffer space you need for converting a `T` to a
string.  Be precise if you can, but pessimistic if you must.  It's usually
better to waste a few bytes of space than to spend a lot of time computing
the exact buffer space you need.  And failing the conversion because you
under-budgeted the buffer is worst of all.

Include the trailing zero in the buffer size.  If your `to_buf` takes more
space than just what's needed to store the result, include that too.

Make `size_buffer` a `constexpr` function if you can.  It can allow the caller
to allocate the buffer on the stack, with a size known at compile time.


Optional: Specialise `is_unquoted_safe`
---------------------------------------

When converting arrays or composite values to strings, libpqxx may need to
quote values and escape any special characters.  This takes time.

Some types though, such as integral or floating-point types, can never have
any special characters such as quotes, commas, or backslashes in their string
representations.  In such cases, there's no need to quote or escape such values
in SQL arrays or composite types.

If your type is like that, you can tell libpqxx about this by defining:

```cxx
    namespace pqxx
    {
    // T is your type.
    template<> inline constexpr bool is_unquoted_safe<T>{true};
    }
```

The code that converts this type of field to strings in an array or a composite
type can then use a simpler, more efficient variant of the code.  It's always
safe to leave this out; it's _just_ an optimisation for when you're completely
sure that it's safe.

Do not do this if a string representation of your type may contain a comma;
semicolon; parenthesis; brace; quote; backslash; newline; or any other
character that might need escaping.


Optional: Specialise `param_format`
-----------------------------------

This one you don't generally need to worry about.  Read on if you're writing a
type which represents raw binary data, or if you're writing a template where
_some specialisations_ may contain raw binary data.

When you call parameterised statements, or prepared statements with parameters,
libpqxx needs to pass your parameters on to libpq, the underlying C-level
PostgreSQL client library.

There are two formats for doing that: _text_ and _binary._  In the first, we
represent all values as strings in the PostgreSQL text format, and the server
then converts them into its own internal binary representation.  That's what
those string conversions above are all about, and it's what we do for almost
all types of parameters.

But we do it differently when the parameter is a contiguous series of raw bytes
and the corresponding SQL type is `BYTEA`.  There is a text format for those,
but we bypass it for efficiency.  The server can use the binary data in the
exact same form, without any conversion or extra processing.  The binary data
is also twice as compact during transport.

(People sometimes ask why we can't just treat all types as binary.  However the
general case isn't so clear-cut.  The binary formats are not documented, there
are no guarantees that they will be platform-independent or that they will
remain stable across postgres releases, and there's no really solid way to
detect when we might get the format wrong.  On top of all that, the conversions
aren't necessarily as straightforward and efficient as they sound.  So, for the
general case, libpqxx sticks with the text formats.  Raw binary data alone
stands out as a clear win.)

Long story short, the machinery for passing parameters needs to know: is this
parameter a binary string, or not?  In the normal case it can assume "no," and
that's what it does.  The text format is always a safe choice; we just try to
use the binary format where it's faster.

The `param_format` function template is what makes the decision.  We specialise
it for types which may be binary strings, and use the default for all other
types.

"Types which _may_ be binary"?  You might think we know whether a type is a
binary type or not.  But there are some complications with generic types.

Templates like `std::shared_ptr`, `std::optional`, and so on act like
"wrappers" for another type.  A `std::optional<T>` is binary if `T` is binary.
Otherwise, it's not.  If you're building support for a template of this nature,
you'll probably want to implement `param_format` for it.

The decision to use binary format is made based on a given object, not
necessarily based on the type in general.  Look at `std::variant`.  If you have
a `std::variant` type which can hold an `int` or a binary string, is that a
binary parameter?  We can't decide without knowing the individual object.

Containers are another hard case.  Should we pass `std::vector<T>` in binary?
Even when `T` is a binary type, we don't currently have any way to pass an
array in binary format, so we always pass it as text.
