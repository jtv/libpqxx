Accessing results and result rows                   {#accessing-results}
=================================

A query produces a result set consisting of rows, and each row consists of
fields.  There are several ways to receive this data.

The fields are "untyped."  That is to say, libpqxx has no opinion on what their
types are.  The database sends the data in a very flexible textual format.
When you read a field, you specify what type you want it to be, and libpqxx
converts the text format to that type for you.

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
the database, and then as you iterate, convert each row it received to a tuple
of C++ types that you indicated.

There are different query functions for querying any number of rows (`query()`);
querying just one row of data as a `std::tuple` and throwing an error if there's
more than one row (`query1()`); or querying

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
`std::string_view` or `std::basic_string_view<std::byte>`), the view points to
underlying data which only stays valid until you iterate to the next row or
exit the loop.  So if you want to use that data for longer than a single
iteration of the streaming loop, you'll have to store it somewhere yourself.

Now for the good news.  Streaming does make it very easy to query data and loop
over it:

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



Results with metadata
---------------------

Sometimes you want more from a query result than just rows of data.  You may
need to know right away how many rows of result data you received, or how many
rows your `UPDATE` statement has affected, or the names of the columns, etc.

For that, use the transaction's "exec" query execution functions.  Apart from a
few exceptions, these return a `pqxx::result` object.  A `result` is a container
of `pqxx::row` objects, so you can iterate them as normal, or index them like
you would index an array.  Each `row` in turn is a container of `pqxx::field`,
Each `field` holds a value, but doesn't know its type.  You specify the type
when you read the value.

For example, your code might do:

```cxx
    pqxx::result r = tx.exec("SELECT * FROM mytable");
    for (auto const &row: r)
    {
       for (auto const &field: row) std::cout << field.c_str() << '\t';
       std::cout << '\n';
    }
```

But results and rows also support other kinds of access.  Array-style
indexing, for instance, such as `r[rownum]`:

```cxx
    std::size_t const num_rows = std::size(r);
    for (std::size_t rownum=0u; rownum < num_rows; ++rownum)
    {
      pqxx::row const row = r[rownum];
      std::size_t const num_cols = std::size(row);
      for (std::size_t colnum=0u; colnum < num_cols; ++colnum)
      {
        pqxx::field const field = row[colnum];
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
      pqxx::row const row = r[rownum];
      for (std::size_t colnum=0u; colnum < num_cols; ++colnum)
      {
        pqxx::field const field = row[colnum];
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

For C++23 or better, there's also a two-dimensional array access operator:

```cxx
    for (std::size_t rownum=0u; rownum < num_rows; ++rownum)
    {
        for (std::size_t colnum=0u; colnum < num_cols; ++colnum)
            std::cout result[rownum, colnum].c_str() << '\t';
        std::cout << '\n';
    }
```

And of course you can use classic "begin/end" loops:

```cxx
    for (auto row = std::begin(r); row != std::end(r); row++)
    {
      for (auto field = std::begin(row); field != std::end(row); field++)
        std::cout << field->c_str() << '\t';
      std::cout << '\n';
    }
```

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
