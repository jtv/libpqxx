Prepared statements                    {#prepared}
===================

Prepared statements are SQL queries that you define once and then invoke
as many times as you like, typically with varying parameters.  It's a lot like
a function that you can define ad hoc, within the scope of one connection.

If you have an SQL statement that you're going to execute many times in
quick succession, it _may_ (but see below!) be more efficient to prepare it
once and reuse it.  This saves the database backend the effort of parsing the
SQL and figuring out an efficient execution plan.


Preparing a statement
---------------------

You create a prepared statement by preparing it on the connection (using the
`pqxx::connection::prepare` functions), passing an identifying name for the
statement, and its SQL text.

The statement's name should consist of ASCII letters, digits, and underscores
only, and start with an ASCII letter.  The name is case-sensitive.

```cxx
    void prepare_my_statement(pqxx::connection &cx)
    {
      cx.prepare(
          "my_statement",
          "SELECT * FROM Employee WHERE name = 'Xavier'");
    }
```

Once you've done this, you'll be able to call `my_statement` from any
transaction you execute on the same connection.  For this, call
`pqxx::transaction_base::exec()` and pass a `pqxx::prepped` object instead of
an SQL statement string.  The `pqxx::prepped` type is just a wrapper that tells
the library "this is not SQL text, it's the name of a prepared statement."

```cxx
    pqxx::result execute_my_statement(pqxx::transaction_base &t)
    {
      return t.exec(pqxx::prepped{"my_statement"});
    }
```


Parameters
----------

You can pass parameters to a prepared statemet, just like you can with a
regular statement.  The query text can contain `$1`, `$2` etc. as placeholders
for parameter values that you will provide when you invoke the prepared
satement.

See @ref parameters for more about this.  And here's a simple example of
preparing a statement and invoking it with parameters:

```cxx
    void prepare_find(pqxx::connection &cx)
    {
      // Prepare a statement called "find" that looks for employees with a
      // given name (parameter 1) whose salary exceeds a given number
      // (parameter 2).
      cx.prepare(
        "find",
        "SELECT * FROM Employee WHERE name = $1 AND salary > $2");
    }
```

This example looks up the prepared statement "find," passes `name` and
`min_salary` as parameters, and invokes the statement with those values:

```cxx
    pqxx::result execute_find(
      pqxx::transaction_base &tx, std::string name, int min_salary)
    {
      return tx.exec(pqxx::prepped{"find"}, name, min_salary);
    }
```


A special prepared statement
----------------------------

There is one special case: the _nameless_ prepared statement.  You may prepare
a statement without a name, i.e. whose name is an empty string.  The unnamed
statement can be redefined at any time, without un-preparing it first.


Performance note
----------------

Don't _assume_ that using prepared statements will speed up your application.
There are cases where prepared statements are actually slower than plain SQL.

The reason is that the backend can often produce a better execution plan when
it knows the statement's actual parameter values.

For example, say you've got a web application and you're querying for users
with status "inactive" who have email addresses in a given domain name X.  If
X is a very popular provider, the best way for the database engine to plan the
query may be to list the inactive users first and then filter for the email
addresses you're looking for.  But in other cases, it may be much faster to
find matching email addresses first and then see which of their owners are
"inactive."  A prepared statement must be planned to fit either case, but a
direct query will be optimised based on table statistics, partial indexes, etc.

So, as with any optimisation... measure where your real performance problems
are before you start making changes, and then afterwards, measure whether your
changes actually helped.  Don't complicate your code unless it solves a real
problem.  Knuth's Law applies.


Zero bytes
----------

@warning Beware of zero ("nul") bytes!

Since libpqxx is a wrapper around libpq, the C-level client library, most
strings you pass to the library should be compatible with C-style strings.  So
they must end with a single byte with value 0, and the text within them cannot
contain any such zero bytes.

(The `pqxx::zview` type exists specifically to tell libpqxx: "this is a
C-compatible string, containing no zero bytes but ending in a zero byte.")

One example is prepared statement names.  But the same also goes for the
parameters values.  Any string you pass as a parameter will end at the _first
char with value zero._  If you pass a string that contains a zero byte, the
last byte in the value will be the one just before the zero.

So, if you need a zero byte in a string, consider that it's really a _binary
string,_ which is not the same thing as a text string.  SQL represents binary
data as the `BYTEA` type, or in binary large objects ("blobs").

In libpqxx, you represent binary data as a range of `std::byte`.  They must be
contiguous in memory, so that libpqxx can pass pointers to the underlying C
library.  So you might use `pqxx::bytes`, or `pqxx::bytes_view`, or
`std::vector<std::byte>`.
