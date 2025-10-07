Accessing results and result rows                   {#accessing-results}
=================================

A query produces a result set consisting of rows, and each row consists of
fields.  There are several ways to receive this data.

The fields are _untyped._  That is to say, libpqxx has no opinion on what their
respective types are.  The database sends the data in a very flexible textual
format.  When you read a field, you specify what type you want it to be, and
libpqxx converts the text format to that type for you.

If a value does not conform to the format for the type you specify, the
conversion fails.  For example, if you have strings that all happen to contain
numbers, you can read them as `int`.  But if any of the values is empty, or
it's null (for a type that doesn't support null), or it's some string that does
not look like an integer, or it's too large, you can't convert it to `int`.

So usually, reading result data from the database means not just retrieving the
data; it also means converting it to some target type.


Querying rows of data
---------------------

The simplest way to query rows of data is to call one of a transaction's
"query" functions, passing as template arguments the types of columns you want
to get back (e.g. `int`, `std::string`, `double`, and so on) and as a regular
argument the query itself.

You can then iterate over the result to go over the rows of data:

```cxx
    for (auto [id, value] :
        tx.query<int, std::string>("SELECT id, name FROM item"))
    {
        std::cout << id << '\t' << value << '\n';
    }
```

The "query" functions execute your query, load the complete result data from
the database, and iterate them.  Along the way they convert each row to a tuple
of C++ types that you indicated.

There are different query functions for querying any number of rows (`query()`);
querying just one row of data as a `std::tuple` and throwing an error if it
doesn't produce exactly one row (`query1()`); or querying either zero or one
rows and getting a `std::optional` tuple of the values converted to the
respective types you requested.


Streaming rows
--------------

There's another way to go through the rows coming out of a query.  It's
usually easier and faster if there are a lot of rows, but there are drawbacks.

**One,** you start getting rows before all the data has come in from the
database.  That speeds things up, but what happens if you lose your network
connection while transferring the data?  Your application may already have
processed some of the data before finding out that the rest isn't coming.  If
that is a problem for your application, streaming may not be the right choice.

**Two,** streaming only works for some types of query.  The `stream()` function
wraps your query in a PostgreSQL `COPY` command, and `COPY` only supports a few
commands: `SELECT`, `VALUES`, or an `INSERT`, `UPDATE`, or `DELETE` with a
`RETURNING` clause.  See the `COPY` documentation here:
[
    https://www.postgresql.org/docs/current/sql-copy.html
](https://www.postgresql.org/docs/current/sql-copy.html).

**Three,** when you convert a field to a "view" type (such as
`std::string_view` or `pqxx::bytes_view`), the view points to underlying data
which only stays valid until you iterate to the next row or exit the loop.  So
if you want to use that data for longer than a single iteration of the
streaming loop, you'll have to store it somewhere yourself.

Now for the good news.  Streaming does make it very easy to query data and loop
over it, and often faster than with the "query" or "exec" functions:

```cxx
    for (auto [id, name, x, y] :
        tx.stream<int, std::string_view, float, float>(
            "SELECT id, name, x, y FROM point"))
      process(id + 1, "point-" + name, x * 10.0, y * 10.0);
```

The conversion to C++ types (here `int`, `std::string_view`, and two `float`s)
is built into the function.  You never even see `row` objects, `field` objects,
iterators, or conversion methods.  You just put in your query and you receive
your data.



The `result` class
------------------

Sometimes you want more from a query result than just rows of data.  You may
need to know how many rows your `UPDATE` statement has affected, or the names
of the columns, and so on.

For that, use the transaction's `exec()` function.  It returns a `pqxx::result`
object.  A `result` is really a smart pointer to a data structure managed by
the C library, libpq.  But seen from the outside, it _acts_ like a
random-access container of rows of data, which you can iterate like any other
container, or index them like you would index an array.  Each row in turn is a
container of field values.  And finally, each field holds a value.

Since `result` is at its core just a smart pointer, it is fairly cheap to
copy: it doesn't copy the data, it just creates an extra reference to the same
underlying data structure.  When you destroy the last copy of the original
`result` object you got back from the database, that will destroy the
underlying data structure as a side effect.


### Rows and fields

A field does not know what type of data it contains.  It holds the data in
string form.  So, _you_ specify the type when you read the value (using
functions like `as()`), and the field will convert its string representation
to that type.

By the way, there are no separate objects holding the rows or the fields.
That data is all held inside the data structure managed by libpq.  There are
however two classes representing a row of data, and two classes representing a
field:

1. _`row_ref`_: effectively a pointer to the `result`, plus a row number.
2. _`row`_: effectively a _copy_ of the `result`, plus a row number.
3. _`field_ref`_: effectively a pointer to the `result`, with row & column
   numbers.
4. _`field`_: effectively a copy of the result, with row & column numbers.

When you index a `result` or dereference a `result` iterator, you get a
`row_ref`.  When you index a `row_ref` or `row` or dereference an iterator of
either, you get a `field_ref`.

Usually you'll deal with `row_ref` and `field_ref`.  These are effficient,
cheap to copy, and straightforward.  All you really need to do is ensure that
the `result` object does not move in memory, or get destroyed.

In rare cases you may need to reference a row or field but without keeping the
`result`, you can use a `row` or `field` object.  These are less efficient,
but they keep a `result` object so that the underlying data structure does not
get destroyed so long as you stlil have a `row` or `field` referencing it.


### Iterating rows and fields

For example, your code might just read it as raw text using `c_str()`:

```cxx
    pqxx::result r = tx.exec("SELECT * FROM mytable");
    for (auto const &row_ref: r)
    {
       for (auto const &field_ref: row) std::cout << field.c_str() << '\t';
       std::cout << '\n';
    }
```


### Data types

Or you can ask for the field to convert itself to some other C++ type, using
`as<my_type>()`:

```cxx
    pqxx::result r = tx.exec("SELECT number, number * 2 FROM data");
    for (auto const &row: r)
    {
        for (auto const &field: row)
        {
            int n = field.as<int>();
            double f = field.as<double>();
            std::cout << n << '\t << f << '\n';
        }
    }
```


### Indexing

But results and rows also support other kinds of access.  Array-style
indexing, for instance, such as `r[rownum]`:

```cxx
    std::size_t const num_rows = std::size(r);
    for (std::size_t rownum=0u; rownum < num_rows; ++rownum)
    {
      pqxx::row_ref const row = r[rownum];
      std::size_t const num_cols = std::size(row);
      for (std::size_t colnum=0u; colnum < num_cols; ++colnum)
      {
        pqxx::field_ref const field = row[colnum];
        std::cout << field.c_str() << '\t';
      }

      std::cout << '\n';
    }
```

Every row in the result has the same number of columns, so you don't need to
look up the number of fields again for each one:

```cxx
    std::size_t const num_rows = std::size(r);
    std::size_t const num_cols = r.columns();
    for (std::size_t rownum=0u; rownum < num_rows; ++rownum)
    {
      pqxx::row_ref const row = r[rownum];
      for (std::size_t colnum=0u; colnum < num_cols; ++colnum)
      {
        pqxx::field_ref const field = row[colnum];
        std::cout << field.c_str() << '\t';
      }

      std::cout << '\n';
    }
```

You can even address a field by indexing the `row` using the field's _name:_

```cxx
    std::cout << row["salary"] << '\n';
```

But try not to do that if speed matters, because looking up the column by name
takes time.  At least you'd want to look up the column index before your loop
and then use numerical indexes inside the loop.

For C++23 or better, there's also a two-dimensional array access operator,
which is actually a little faster than the classic indexing:

```cxx
    for (std::size_t rownum=0u; rownum < num_rows; ++rownum)
    {
        for (std::size_t colnum=0u; colnum < num_cols; ++colnum)
            std::cout result[rownum, colnum].c_str() << '\t';
        std::cout << '\n';
    }
```


### Going old-school

And of course you can use classic "begin/end" loops:

```cxx
    for (auto row = std::begin(r); row != std::end(r); row++)
    {
      for (auto field = std::begin(row); field != std::end(row); field++)
        std::cout << field->c_str() << '\t';
      std::cout << '\n';
    }
```


Fine print
----------

Result sets are immutable, so all iterators on results and rows are actually
`const_iterator`s.  There are also `const_reverse_iterator` types, which
iterate backwards from `rbegin()` to `rend()` exclusive.

All these iterator types provide one extra bit of convenience that you won't
normally find in C++ iterators: referential transparency.  You don't need to
dereference them to get to the row or field they refer to.  That is, instead
of `row->end()` you can also choose to say `row.end()`.  Similarly, you
may prefer `field.c_str()` over `field->c_str()`.

This becomes really helpful with the array-indexing operator.  With regular
C++ iterators you would need ugly expressions like `(*row)[0]` or
`row->operator[](0)`.  With the iterator types defined by the result and
row classes you can simply say `row[0]`.
