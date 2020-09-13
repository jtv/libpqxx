Accessing results and result rows                   {#accessing-results}
---------------------------------

When you execute a query using one of the transaction `exec` functions, you
normally get a `result` object back.  A `result` is a container of `row`s.

(There are exceptions.  The `exec0` functions are for when you expect no result
data, so they don't return anything.  The `exec1` functions expect exactly one
row, so they return just the `row`.)

Result objects are an all-or-nothing affair.  The `exec` function waits until
it's received all the result data, and then gives it to you in the form of the
`result`.  (There is another way of doing things, so see "streaming rows" below
as well.)

For example, your code might do:

    pqxx::result r = tx.exec("SELECT * FROM mytable");

Now, how do you access the data inside `r`?

Result sets act as standard C++ containers of rows.  Rows act as standard
C++ containers of fields.  So the easiest way to go through them is:

    for (auto const &row: r)
    {
       for (auto const &field: row) std::cout << field.c_str() << '\t';
       std::cout << std::endl;
    }

But results and rows also support other kinds of access.  Array-style
indexing, for instance, such as `r[rownum]`:

    int const num_rows = std::size(r);
    for (int rownum=0; rownum < num_rows; ++rownum)
    {
      pqxx::row const row = r[rownum];
      int const num_cols = std::size(row);
      for (int colnum=0; colnum < num_cols; ++colnum)
      {
        pqxx::field const field = row[colnum];
        std::cout << field.c_str() << '\t';
      }

      std::cout << std::endl;
    }

And of course you can use classic "begin/end" loops:

    for (auto row = std::begin(r); row != std::end(r); row++)
    {
      for (auto field = std::begin(row); field != std::end(row); field++)
        std::cout << field->c_str() << '\t';
        std::cout << std::endl;
    }

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


Streaming rows
--------------

There's another way to go through the rows coming out of a query.  It's
easier and faster, but there are drawbacks.

One, you start getting rows before all the data has come in from the database.
So if there's an error while you're receiving and processing data then you're
stuck in the middle.  You might lose your connection to the database when
you've already started processing the incoming data.

Two, you can't do everything in a streaming query that you can in a regular
one.  Your query gets wrapped in a PostgreSQL `COPY` command, and `COPY` only
supports some queries: `SELECT`, `VALUES`, `or an `INSERT`, `UPDATE`, or
`DELETE` with a `RETURNING` clause.  See the `COPY` documentation here:
https://www.postgresql.org/docs/current/sql-copy.html

Now for the good news.  Streaming does make it very easy to query data and loop
over it:

    for (auto [id, name, x, y] :
        tx.stream<int, std::string_view, float, float>(
            "SELECT id, name, x, y FROM point"))
      process(id + 1, "point-" + name, x * 10.0, y * 10.0);

The conversion to C++ types (here `int`, `std::string_view`, and two `float`)
is built in.  You don't see the `row` objects, the `field` objects, the
iterators, or the conversion methods.  You just put in your query and you
receive your data.
