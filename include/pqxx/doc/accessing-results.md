Accessing results and result rows                   {#accessing-results}
---------------------------------

Let's say you have a result object.  For example, your program may have done:

    pqxx::result r = w.exec("SELECT * FROM mytable");

Now, how do you access the data inside `r`?

Result sets act as standard C++ containers of rows.  Rows act as standard
C++ containers of fields.  So the easiest way to go through them is:

    for (const auto &row: r)
    {
       for (const auto &field: row) std::cout << field.c_str() << '\t';
       std::cout << std::endl;
    }

But results and rows also support other kinds of access.  Array-style
indexing, for instance, such as `r[rownum]`:

    const int num_rows = r.size();
    for (int rownum=0; rownum < num_rows; ++rownum)
    {
      const pqxx::row row = r[rownum];
      const int num_cols = row.size();
      for (int colnum=0; colnum < num_cols; ++colnum)
      {
        const pqxx::field field = row[colnum];
        std::cout << field.c_str() << '\t';
      }

      std::cout << std::endl;
    }

And of course you can use classic "begin/end" loops:

    for (auto row = r.begin(); row != r.end(); row++)
    {
      for (auto field = row.begin(); field != row.end(); field++)
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
