Streams                                                           {#streams}
=======

Most of the time it's fine to retrieve data from the database using `SELECT`
queries, and store data using `INSERT`.  But for those cases where efficiency
matters, there are two _data streaming_ mechanisms to help you do this more
efficiently: "streaming queries," for reading query results from the
database; and the @ref pqxx::stream_to class, for writing data from the client
into a table.

These are less flexible than SQL queries.  Also, depending on your needs, it
may be a problem to lose your connection while you're in mid-stream, not
knowing that the query may not complete.  But, you get some scalability and
memory efficiencies in return.

Just like regular querying, these streaming mechanisms do data conversion for
you.  You deal with the C++ data types, and the database deals with the SQL
data types.


Interlude: null values
----------------------

So how do you deal with nulls?  It depends on the C++ type you're using.  Some
types may have a built-in null value.  For instance, if you have a
`char const *` value and you convert it to an SQL string, then converting a
`nullptr` will produce a NULL SQL value.

But what do you do about C++ types which don't have a built-in null value, such
as `int`?  The trick is to wrap it in `std::optional`.  The difference between
`int` and `std::optional<int>` is that the former always has an `int` value,
and the latter doesn't have to.

Actually it's not just `std::optional`.  You can do the same thing with
`std::unique_ptr` or `std::shared_ptr`.  A smart pointer is less efficient than
`std::optional` in most situations because they allocate their value on the
heap, but sometimes that's what you want in order to save moving or copying
large values around.

This part is not generic though.  It won't work with just any smart-pointer
type, just the ones which are explicitly supported: `shared_ptr` and
`unique_ptr`.  If you really need to, you can build support for additional
wrappers and smart pointers by copying the implementation patterns from the
existing smart-pointer support.


Streaming data _from a query_
-----------------------------

Use @ref transaction_base::stream to read large amounts of data directly from
the database.  In terms of API it works just like @ref transaction_base::query,
but it's faster than the `exec` and `query` functions For larger data sets.
Also, you won't need to keep your full result set in memory.  That can really
matter with larger data sets.

Another performance advantage is that with a streaming query, you can start
processing your data right after the first row of data comes in from the
server.  With `exec()` or `query()` you need to wait to receive all data, and
only then can you begin processing.  With streaming queries you can be
processing data on the client side while the server is still sending you the
rest.

Not all kinds of queries will work in a stream.  Internally the streams make
use of PostgreSQL's `COPY` command, so see the PostgreSQL documentation for
`COPY` for the exact limitations.  Basic `SELECT` and `UPDATE ... RETURNING`
queries will just work, but fancier constructs may not.

As you read a row, the stream converts its fields to a tuple type containing
the value types you ask for:

```cxx
    for (auto [name, score] :
        tx.stream<std::string_view, int>("SELECT name, points FROM score")
    )
        process(name, score);
```

On each iteration, the stream gives you a `std::tuple` of the column types you
specify.  It converts the row's fields (which internally arrive at the client
in text format) to your chosen types.

The `auto [name, score]` in the example is a _structured binding_ which unpacks
the tuple's fields into separate variables.  If you prefer, you can choose to
receive the tuple instead: `for (std::tuple<int, std::string_view> :`.


### Is streaming right for my query?

Here are the things you need to be aware of when deciding whether to stream a
query, or just execute it normally.

First, when you stream a query, there is no metadata describing how many rows
it returned, what the columns are called, and so on.  With a regular query you
get a @ref result object which contains this metadata as well as the data
itself.  If you absolutely need this metadata for a particular query, then
that means you can't stream the query.

Second, under the bonnet, streaming from a query uses a PostgreSQL-specific SQL
command `COPY (...) TO STDOUT`.  There are some limitations on what kinds of
queries this command can handle.  These limitations may change over time, so I
won't describe them here.  Instead, see PostgreSQL's
[COPY documentation](https://www.postgresql.org/docs/current/sql-copy.html)
for the details.  (Look for the `TO` variant, with a query as the data source.)

Third: when you stream a query, you start receiving and processing data before
you even know whether you will receive all of the data.  If you lose your
connection to the database halfway through, you will have processed half your
data, unaware that the query may never execute to completion.  If this is a
problem for your application, don't stream that query!

The fourth and final factor is performance.  If you're interested in streaming,
obviously you care about this one.

I can't tell you _a priori_ whether streaming will make your query faster.  It
depends on how many rows you're retrieving, how much data there is in those
rows, the speed of your network connection to the database, your client
encoding, how much processing you do per row, and the details of the
client-side system: hardware speed, CPU load, and available memory.

Ultimately, no amount of theory beats real-world measurement for your specific
situation so...  if it really matters, measure.  (And as per Knuth's Law: if
it doesn't really matter, don't optimise.)

That said, here are a few data points from some toy benchmarks:

If your query returns e.g. a hundred small rows, it's not likely to make up a
significant portion of your application's run time.  Streaming is likely to be
_slower_ than regular querying, but most likely the difference just won't
amtter.

If your query returns _a thousand_ small rows, streaming is probably still
going to be a bit slower than regular querying, though "your mileage may vary."

If you're querying _ten thousand_ small rows, however, it becomes more likely
that streaming will speed it up.  The advantage increases as the number of rows
increases.

That's for small rows, based on a test where each row consisted of just one
integer number.  If your query returns larger rows, with more columns,
I find that streaming seems to become more attractive.  In a simple test with 4
columns (two integers and two strings), streaming even just a thousand rows was
considerably faster than a regular query.

If your network connection to the database is slow, however, that may make
streaming a bit _less_ effcient.  There is a bit more communication back and
forth between the client and the database to set up a stream.  This overhead
takes a more or less constant amount of time, so for larger data sets it will
tend to become insignificant compared to the other performance costs.


Streaming data _into a table_
-----------------------------

Use `stream_to` to write data directly to a database table.  This saves you
having to perform an `INSERT` for every row, and so it can be significantly
faster if you want to insert more than just one or two rows at a time.

As with `stream_from`, you can specify the table and the columns, and not much
else.  You insert tuple-like objects of your choice:

```cxx
    pqxx::stream_to stream{
        tx,
        "score",
        std::vector<std::string>{"name", "points"}};
    for (auto const &entry: scores)
        stream << entry;
    stream.complete();
```

Each row is processed as you provide it, and not retained in memory after that.

The call to `complete()` is more important here than it is for `stream_from`.
It's a lot like a "commit" or "abort" at the end of a transaction.  If you omit
it, it will be done automatically during the stream's destructor.  But since
destructors can't throw exceptions, any failures at that stage won't be visible
in your code.  So, always call `complete()` on a `stream_to` to close it off
properly!
