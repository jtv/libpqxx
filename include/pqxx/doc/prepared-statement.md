Prepared statements                    {#prepared}
===================

Prepared statements are SQL queries that you define once and then invoke
as many times as you like, typically with varying parameters.  It's basically
a function that you can define ad hoc.

If you have an SQL statement that you're going to execute many times in
quick succession, it may be more efficient to prepare it once and reuse it.
This saves the database backend the effort of parsing complex SQL and
figuring out an efficient execution plan.  Another nice side effect is that
you don't need to worry about escaping parameters.

You create a prepared statement by preparing it on the connection
(using the `pqxx::connection_base::prepare` functions), passing an
identifier and its SQL text.  The identifier is the name by which the
prepared statement will be known; it should consist of ASCII letters,
digits, and underscores only, and start with an ASCII letter.  The name is
case-sensitive.

    void prepare_my_statement(pqxx::connection_base &c)
    {
      c.prepare(
          "my_statement",
          "SELECT * FROM Employee WHERE name = 'Xavier'");
    }

Once you've done this, you'll be able to call `my_statement` from any
transaction you execute on the same connection.  For this, use the
`pqxx::transaction_base::exec_prepared` functions.

    pqxx::result execute_my_statement(pqxx::transaction_base &t)
    {
      return t.exec_prepared("my_statement");
    }

Did I mention that prepared statements can have parameters?  The query text
can contain `$1`, `$2` etc. as placeholders for parameter values that you
will provide when you invoke the prepared satement.

    void prepare_find(pqxx::connection_base &c)
    {
      // Prepare a statement called "find" that looks for employees with a
      // given name (parameter 1) whose salary exceeds a given number
      // (parameter 2).
      c.prepare(
  	    "find",
  	    "SELECT * FROM Employee WHERE name = $1 AND salary > $2");
    }

This example looks up the prepared statement "find," passes `name` and
`min_salary` as parameters, and invokes the statement with those values:

    pqxx::result execute_find(
      pqxx::transaction_base &t, std::string name, int min_salary)
    {
      return t.exec_prepared("find", name, min_salary);
    }

There is one special case: the _nameless_ prepared statement.  You may prepare
a statement without a name, i.e. whose name is an empty string.  The unnamed
statement can be redefined at any time, without un-preparing it first.

Never try to prepare, execute, or unprepare a prepared statement
manually using direct SQL queries.  Always use the functions provided by
libpqxx.

Prepared statements are not necessarily defined on the backend right away.
It's usually done lazily.  This means that you can prepare statements before
the connection is fully established, and that it's relatively cheap to
pre-prepare lots of statements that you may or may not not use during the
session.  On the other hand, it also means that errors in a prepared
statement may not show up until you first try to invoke it.  Such an error
may then break the transaction it occurs in.

A performance note: There are cases where prepared statements are actually
slower than plain SQL.  Sometimes the backend can produce a better execution
plan when it knows the parameter values.  For example, say you've got a web
application and you're querying for users with status "inactive" who have
email addresses in a given domain name X.  If X is a very popular provider,
the best way for the database engine to plan the query may be to list the
inactive users first and then filter for the email addresses you're looking
for.  But in other cases, it may be much faster to find matching email
addresses first and then see which of their owners are "inactive."  A
prepared statement must be planned to fit either case, but a direct query
will be optimised based on table statistics, partial indexes, etc.

@warning Beware of "nul" bytes.  Any string you pass as a parameter will
end at the first char with value zero.  If you pass a `std::string` that
contains a zero byte, the last byte in the value will be the one just
before the zero.  If you need a zero byte, consider using
pqxx::binarystring and/or SQL's `bytea` type.
