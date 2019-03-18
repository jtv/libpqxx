Getting started                              {#getting-started}
===============

The most basic three types in libpqxx are the _connection_, the _transaction_,
and the _result_.

They fit together as follows:
* You connect to the database by creating a connection object (see
  @ref connection).

  The connection type you'll usually want is `pqxx::connection`.

* You create a transaction object (see @ref transaction) operating on that
  connection.  You'll usually want the `pqxx::work` variety.

  Once you're done you call the transaction's `commit` function to make its
  work final.  If you don't call this, the work will be rolled back when the
  transaction object is destroyed.

* Until then, use the transaction's `exec` functions to execute queries, which
  you pass in as simple strings.

* Most of the `exec` functions return a `pqxx::result` object, which acts
  as a standard container of rows.

  Each row in a result, in turn, acts as a container of fields.  You can use
  array indexing or iterators to access either.

* The field's data is stored internally as a text string.

  You can read it as such using its `c_str` function, or convert it to other
  types using its `as` and `to` member functions.  These are templated on the
  destination type: `myfield.as<int>();` or `myfield.to(myint);`

* After you've closed the transaction, the connection is free to run a next
  transaction.

Here's a very basic example.  It connects to the default database (you'll
need to have one set up), queries it for a very simple result, converts it to
an `int`, and prints it out.  It also contains some basic error handling.

    #include <iostream>
    #include <pqxx/pqxx>

    int main()
    {
      try
      {
        // Connect to the database.  In practice we may have to pass some
        // arguments to say where the database server is, and so on.
        // The constructor parses options exactly like libpq's
        // PQconnectdb/PQconnect, see:
        // https://www.postgresql.org/docs/10/static/libpq-connect.html
        pqxx::connection c;

        // Start a transaction.  In libpqxx, you always work in one.
        pqxx::work w(c);

        // work::exec1() executes a query returning a single row of data.
        // We'll just ask the database to return the number 1 to us.
        pqxx::row r = w.exec1("SELECT 1");

        // Commit your transaction.  If an exception occurred before this
        // point, execution will have left the block, and the transaction will
        // have been destroyed along the way.  In that case, the failed
        // transaction would implicitly abort instead of getting to this point.
        w.commit();

        // Look at the first and only field in the row, parse it as an integer,
        // and print it.
        std::cout << r[0].as<int>() << std::endl;
      }
      catch (const std::exception &e)
      {
        std::cerr << e.what() << std::endl;
        return 1;
      }
    }

This prints the number 1.  Notice that you can keep the result object
around after the transaction (or even the connection) has been closed.  There
are situations where you can't do it, but generally it's fine.  If you're
interested: you can install your own callbacks for receiving error messages
from the database, and in that case you'll have to keep the connection object
alive.  But otherwise, it's nice to be able to "fire and forget" your
connection and deal with the data.

Here's a slightly more complicated example.  It takes an argument from the
command line and retrieves a string with that value.  The interesting part is
that it uses the escaping-and-quoting function `quote` to embed this
string value in SQL safely.  It also reads the result field's value as a
plain C-style string using its `c_str` function.

    #include <iostream>
    #include <stdexcept>
    #include <pqxx/pqxx>

    int main(int argc, char *argv[])
    {
      try
      {
        if (!argv[1]) throw std::runtime_error("Give me a string!");

        pqxx::connection c;
        pqxx::work w(c);

        // work::exec() returns a full result set, which can consist of any
        // number of rows.
        pqxx::result r = w.exec("SELECT " + w.quote(argv[1]));

        // End our transaction here.  We can still use the result afterwards.
        w.commit();

        // Print the first field of the first row.  Read it as a C string,
        // just like std::string::c_str() does.
        std::cout << r[0][0].c_str() << std::endl;
      }
      catch (const std::exception &e)
      {
        std::cerr << e.what() << std::endl;
        return 1;
      }
    }

You can find more about converting field values to native types, or
converting values to strings for use with libpqxx, under
@ref stringconversion.  More about getting to the rows and fields of a
result is under @ref accessing-results.

If you want to handle exceptions thrown by libpqxx in more detail, for
example to print the SQL contents of a query that failed, see @ref exception.
