Prepared statements                    {#prepared}
===================

Prepared statements are SQL queries that you define once and then invoke
as many times as you like, typically with varying parameters.  It's basically
a function that you can define ad hoc.

If you have an SQL statement that you're going to execute many times in
quick succession, it may be more efficient to prepare it once and reuse it.
This saves the database backend the effort of parsing complex SQL and
figuring out an efficient execution plan.  Another nice side effect is that
you don't need to worry about escaping parameters.  Some corporate coding
standards require all SQL parameters to be passed in this way, to reduce the
risk of programmer mistakes leaving room for SQL injections.


Preparing a statement
---------------------

You create a prepared statement by preparing it on the connection (using the
`pqxx::connection::prepare` functions), passing an identifier and its SQL text.

The identifier is the name by which the prepared statement will be known; it
should consist of ASCII letters, digits, and underscores only, and start with
an ASCII letter.  The name is case-sensitive.

    void prepare_my_statement(pqxx::connection &c)
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


Parameters
----------

Did I mention that prepared statements can have parameters?  The query text
can contain `$1`, `$2` etc. as placeholders for parameter values that you
will provide when you invoke the prepared satement.

    void prepare_find(pqxx::connection &c)
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


Dynamic parameter lists
-----------------------

In rare cases you may just not know how many parameters you'll pass into your
statement when you call it.

For these situations, have a look at `make_dynamic_params()`, which works with
containers or iterator ranges of parameter values.  (This does mean that all of
those parameters have to have the same static C++ type.)

The `make_dynamic_params` factory produces an object which you can pass into
your statement as a normal parameter.  All its parameter values will be
expanded into that position of the statement's overall parameter list.  So if
you call your statement passing a regular parameter `a`, a set of dynamic
parameters containing just a parameter `b`, and another regular parameter `c`,
then your call will pass parameters `a`, `b`, and `c`.  Or if the dynamic
parameters is empty, it will pass just `a` and `b`.  If the dynamic parameters
contain `x` and `y`, your call will pass `a, x, y, c`.

As you can see, you can mix static (regular) and dynamic parameters freely.
Don't go overboard though: complexity is where bugs happen!


A special prepared statement
----------------------------

There is one special case: the _nameless_ prepared statement.  You may prepare
a statement without a name, i.e. whose name is an empty string.  The unnamed
statement can be redefined at any time, without un-preparing it first.


Performance note
----------------

Don't assume that using prepared statements will speed up your application.
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


Zero bytes
----------

@warning Beware of "nul" bytes!

Any string you pass as a parameter will end at the _first char with value
zero._  If you pass a `std::string` that contains a zero byte, the last byte
in the value will be the one just before the zero.

So, if you need a zero byte in a string, consider using `pqxx::binarystring`
and/or SQL's `bytea` type.
