Streams						{#streams}
=======

Most of the time it's fine to retrieve data from the database using `SELECT`
queries, and store data using `INSERT`.  But for those cases where efficiency
matters, there are two classes to help you do this better: `stream_from` and
`stream_to`.  They're less flexible than SQL queries, but you get speed and
memory efficiencies in return.


`stream_from`
-------------

Use `stream_from` to read data directly from a database table.  It's likely to
be faster than a `SELECT`, but also, you won't need to keep your full result
set in memory.  That can really matter with larger data sets.

So, use `stream_from` when you need to...
 * read all rows in a table,
 * in an arbitrary order,
 * without any computation done on them.

You can select which _columns_ you want though, and in which order.

Every row is converted to a tuple type of your choice as you read it.  You
stream into the tuple:

    pqxx::stream_from stream{
        tx,
        "score",
        std::vector<std::string>{"name", "points"}};
    std::tuple<std::string, int> row;
    while (stream >> row)
      process(row);
    stream.complete();

As the stream reads each row, it converts that row's data to your tuple, and
then promptly forgets it.  This means you can easily read more data than will
fit in memory.


`stream_to`
-----------

Use `stream_to` to write data directly to a database table.  This saves you
having to perform an `INSERT` for every row, and so it can be significantly
faster for larger insertions.

As with `stream_from`, you can specify the table and the columns, and not much
else.  You insert tuple-like objects of your choice:

    pqxx::stream_to stream{
        tx,
        "score",
        std::vector<std::string>{"name", "points"}};
    for (const auto &entry: scores)
        stream << entry;
    stream.complete();

Each row is processed as you provide it, and not retained in memory after that.

The call to `complete()` is more important here than it is for `stream_from`.
It's a lot like a "commit" or "abort" at the end of a transaction.  If you omit
it, it will be done automatically during the stream's destructor.  But since
destructors can't throw exceptions, any failures at that stage won't be visible
in your code.  So, always call `complete()` on a `stream_to` to close it off
properly!
