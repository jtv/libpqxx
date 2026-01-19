#include <iostream>

#include <pqxx/pqxx>

/** Here's an example of how to use PostgreSQL in a C++ application, using
 * libpqxx.
 *
 * In this example we'll populate a small company database, and query it.
 */

namespace
{
/// Create an SQL schema.
/** The tables are all temporary to simplify cleanup.  In a real application
 * of course they wouldn't be `TEMP`.
 */
void create_schema(pqxx::connection &cx)
{
  // We have a connection.  We want to execute some SQL statements.
  //
  // In libpqxx you don't do that directly on the connection.  You create a
  // _transaction,_ and use that to execute statements.
  //
  // Once you're done with your statements, you _commit_ the transaction.  If
  // there's an error, you'll typically get an exception, and destroy the
  // transaction without committing it.  When that happens, the transaction
  // _aborts,_ and its changes are undone.
  //
  // There can be only one regular transaction on a connection at the same
  // time.  You'll have to commit, abort, or destroy it before you can start
  // another.
  //
  // In SQL, "work" is an alias for "transaction."  In libpqxx, `transaction`
  // is a template, letting you choose a transaction isolation level.  To keep
  // it simple, `work` is an alias for the default kind.
  pqxx::work tx{cx};

  // Execute a simple SQL statement: create a table.
  tx.exec(
    "CREATE TEMP TABLE department ("
    "id integer PRIMARY KEY, "
    "name varchar UNIQUE NOT NULL)");

  // When executing a statement that should not return any rows of data, you
  // can indicate this by calling no_rows() on the result.  This checks that
  // there really are no data rows, and throws an exception if there are.
  tx.exec(
      "CREATE TEMP TABLE employee ("
      "id integer PRIMARY KEY, "
      "name varchar NOT NULL, "
      "date_hired timestamp without time zone, "
      "dept_id integer REFERENCES department(id))")
    .no_rows();

  // More generally, you can specify that there should be `n` rows by calling
  // `expect_rows(n)` on the result.
  tx.exec(
      "CREATE TEMP TABLE customer ("
      "id integer PRIMARY KEY, "
      "name varchar NOT NULL)")
    .expect_rows(0);

  // If we got this far without exceptions, that means we're successful.
  // Commit the transaction so that the changes become persistent.
  tx.commit();
}


/// Write sample data to our schema.
void populate_schema(pqxx::connection &cx)
{
  // Again we need to create a transaction.  There are also a few SQL
  // statements that can't be run inside a transaction; for those you'll use a
  // `pqxx::nontransaction`.  That way you get the same, single API for
  // executing SQL regardless of whether you're in a transaction.
  pqxx::work tx{cx};

  // We'll _stream_ our data into the tables.  Each stream is an object, and
  // only one can be active on a transaction at any given time.
  //
  // Here, we set up a stream to write IDs and names to the "department" table.
  //
  // We specify the transaction; a "path" for the table (which in the simplest
  // case consists of just its name); and the column names we're writing.  In
  // any columns we don't write we will get the default values, usually NULL.
  auto depts = pqxx::stream_to::table(tx, {"department"}, {"id", "name"});

  // Now we can feed our data into the table.  This is much faster than
  // executing individual "INSERT" statements.
  //
  // We can feed many kinds of data into the fields.  In this case, we use an
  // integer for the ID and a string for the name.  But we could also use two
  // strings, if we wanted to, so long as the ID string contains only digits.
  //
  // There is one caveat: if there is a clash (e.g. because there already is
  // a department with the same name), this will simply fail with an exception.
  // If you want to resolve conflicts by keeping the row you had, or by
  // overwriting it with the new one, you'll have to write into a temporary
  // table and write your own SQL to move the data over into the destination.
  depts.write_values(1, "accounting");
  depts.write_values(2, "marketing");
  depts.write_values(3, "widgets");

  // Just like we had to commit a transaction to make our work persistent, we
  // need to tell the stream that we're done writing data.  Until we do this,
  // it may still be buffering some data on the client side.
  depts.complete();

  // Now that there are departments, we can populate them with employees.
  auto emps = pqxx::stream_to::table(
    tx, {"employee"}, {"id", "name", "date_hired", "dept_id"});

  // There are many ways to pass a null.  We can use a nullptr, or an empty
  // std::optional, an empty std::unique_ptr or std::shared_ptr, and so on.
  emps.write_values(1, "Piet Hein", nullptr, 1);
  emps.write_values(2, "Hugo de Groot", std::optional<int>{}, 3);
  emps.write_values("3", "Johan de Witt", std::unique_ptr<std::string>{}, 2);
  emps.write_values("4", "Willem van Oranje", std::shared_ptr<float>{}, 3);

  // Again we must tell the stream when we're done feeding data into it.
  emps.complete();

  auto cust = pqxx::stream_to::table(tx, {"customer"}, {"id", "name"});
  cust.write_values(1, "Acme");
  cust.write_values(2, "The Government");
  cust.write_values(3, "Sirius Cybernetics Corp.");
  cust.write_values(4, "A chap I met at the club called Bernard");
  cust.complete();

  // Don't forget to commit the transaction!  Otherwise it was all for nought.
  tx.commit();
}


/// Query and print out departments.
void query_depts(pqxx::connection &cx)
{
  // Again we need a transaction.  In this case we don't really care what
  // kind.  We'll use a `nontransaction`, which just immediately commits every
  // statement you execute.  Its `commit()` and `abort()` functions are no-ops.
  pqxx::nontransaction tx{cx};

  // We execute an SQL command, and get a `pqxx::result` back.  This contains
  // all of the data and metadata resulting from the query, in their SQL
  // textual representation.
  //
  // There are other ways of querying (see below for more), but this one is
  // perfect when we want the metadata, and the amount of data is small enough
  // to retrieve in one go.  We only get the result once the query has
  // executed to completion and we have received all the data.
  pqxx::result const res =
    tx.exec("SELECT name FROM department ORDER BY name");

  // The result has metadata, such as its number of rows.
  std::cout << "Number of departments: " << std::size(res) << '\n';

  // And of course it contains the rows of data.  You can iterate these just
  // like a standard C++ container.
  for (pqxx::row_ref const row : res)
  {
    // Each row contains a series of fields, corresponding to the columns of
    // the result.  You can iterate these as well, or you can address them by
    // index, just like a standard C++ container: `operator[]` if you are sure
    // the field exists, or `at()` if you want error checking.
    //
    // The simplest way to read a field's contents is to call its `view()`.
    // It gives you a `std::string_view` on its value.
    std::cout << '\t' << row.at(0).view() << '\n';
  }
  std::cout << '\n';
}


/// Query and print out employees.
void query_emps(pqxx::connection &cx)
{
  pqxx::work tx{cx};

  // Query employees.  Specify how many columns we expect in the result; if
  // the number is wrong, we'll get an exception.
  pqxx::result const res =
    tx.exec(
        "SELECT employee.id, employee.name, department.name "
        "FROM employee "
        "JOIN department ON department.id = employee.dept_id "
        "ORDER BY employee.name, department.name")
      .expect_columns(3);

  // Print out the results.  This time we convert the fields' contents to
  // various C++ types.  It doesn't matter what type they were in the database;
  // you can convert them to anything so long as the data fits the type.  You
  // can read an integer as a string, or as a floating-point number, and so on.
  std::cout << "Employees:\n";
  for (auto row : res)
  {
    std::cout << '\t' << row[0].as<int>() << '\t'
              << row[1].as<std::string_view>() << '\t'
              << row[2].as<std::string>() << '\n';
  }

  // There is also a completely different way of iterating over a result: you
  // pass a lambda or function to its `for_each()`.  That function will figure
  // out what parameter types your callable expects, and convert the respective
  // columns to those and pass them as arguments.
  //
  // Of course your callable must take exactly the same number of arguments as
  // the result contains.
  //
  // Here, we use that to determine the highest employee ID.
  int top_id{0};
  res.for_each([&top_id](int id, std::string_view, std::string_view) {
    // (Don't worry about the extra parentheses.  Those are just there because
    // std::min() and std::max() confuse Visual Studio.)
    top_id = (std::max)(top_id, id);
  });
  if (top_id > 0)
    std::cout << "The highest employee ID is " << top_id << ".\n\n";
}


/// Query and print customers.  This could be a lot of data.
void query_customers(pqxx::connection &cx)
{
  pqxx::work tx{cx};
  std::cout << "Customers:\n";

  // Query the number of customers.  There's a convenience shortcut for
  // "execute this query, check that it produces a result consisting of a
  // single field (so one row of one column), and convert that one field value
  // to the type I specify":
  auto const num_customers =
    tx.query_value<std::size_t>("SELECT count(*) FROM customer");
  std::cout << "Total customers: " << num_customers << '\n';

  // There's also a convenience shortcut for "execute this query, and iterate
  // over the result rows, converting each to a tuple of the given types."
  // You use this with a "for" loop and structured binding:
  for (auto [id, name] : tx.query<int, std::string_view>(
         "SELECT id, name FROM customer ORDER BY id"))
  {
    std::cout << '\t' << id << '\t' << name << '\n';
  }
  std::cout << "\n\n";

  // Or, if you prefer a callback-based style, for_query() takes a query and a
  // callback.  It executes the query, iterates over the result rows, calling
  // your callback with the row's respective field values as argments.  It
  // detects the parameter types your callback expects, and converts the fields
  // to those respective types.
  std::cout << "That same data again:\n";
  tx.for_query(
    "SELECT id, name FROM customer ORDER BY id",
    [](int id, std::string_view name) {
      std::cout << '\t' << id << '\t' << name << '\n';
    });
}
} // namespace


int main(int, char *argv[])
{
  // We may need a connection string setting a database address (either host
  // and port, or path to a Unix domain socket), a database name, username,
  // password, and so on.  Any values not set in the connection string will
  // use either values set in environment variables, or built-in defaults.
  //
  // For connection strings, see:
  // https://postgresql.org/docs/current/libpq-connect.html#LIBPQ-CONNSTRING
  //
  // For the environment variables, see:
  // https://postgresql.org/docs/current/libpq-envars.html
  //
  // Here we'll just assume you pass a connection string on the command line.
  std::string const connect_string{(argv[1] == nullptr) ? "" : argv[1]};

  try
  {
    // Connect to the database.  In libpqxx this is the same thing as "creating
    // a connection."
    //
    // You don't need to check whether this succeeds.  If there's a problem,
    // this will throw an exception.
    pqxx::connection cx{connect_string};

    // We're connected to a database.  Let's set up our schema and populate it.
    create_schema(cx);
    populate_schema(cx);

    // Query the database, and print out various information.
    query_depts(cx);
    query_emps(cx);
    query_customers(cx);
  }
  catch (pqxx::failure const &e)
  {
    // In the case of a libpqxx exception, there's some extra useful
    // information that we can print.
    std::cerr << "*** " << e.name() << " ***: " << e.what() << '\n';
    std::cerr << "Happened in " << pqxx::source_loc(e.location()) << ".\n";
    if (not std::empty(e.query()))
      std::cerr << "\nQuery was:\n" << e.query() << '\n';
    return 1;
  }
  catch (std::exception const &e)
  {
    // We only get here if there was an error.
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
