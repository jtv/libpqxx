#include <iostream>
#include <pqxx/pqxx>

/* Example for using libpqxx.
 *
 * This shows a bit of how a real C++ application might access a PostgreSQL
 * database using libpqxx.
 *
 * It sets up a minimal database schema, and then executes some queries on it;
 * and handles any errors that might crop up in the process.
 */
namespace
{
void set_up(pqxx::connection &cx)
{
  pqxx::work tx{cx};
  tx.exec("CREATE TEMP TABLE Employee(name varchar, salary integer)");
  tx.exec("INSERT INTO Employee(name, salary) VALUES ('Someone', 4632)");
  tx.commit();
}
} // namespace


/// Query employees from database.  Return result.
pqxx::result query()
{
  pqxx::connection cx;
  set_up(cx);

  pqxx::work tx{cx};

  // Silly example: Add up all salaries.  Normally you'd let the database do
  // this for you.
  long total = 0;
  for (auto [salary] : tx.query<int>("SELECT salary FROM Employee"))
    total += salary;
  std::cout << "Total salary: " << total << '\n';

  // Execute and process some data.
  pqxx::result r{tx.exec("SELECT name, salary FROM Employee")};
  for (auto row : r)
    std::cout
      // Address column by name.  Use c_str() to get C-style string.
      << row["name"].c_str()
      << " makes "
      // Address column by zero-based index.  Use as<int>() to parse as int.
      << row[1].as<int>() << "." << std::endl;

  // Not really needed, since we made no changes, but good habit to be
  // explicit about when the transaction is done.
  tx.commit();

  // Connection object goes out of scope here.  It closes automatically.
  // But the result object remains valid.
  return r;
}


/// Query employees from database, print results.
int main()
{
  try
  {
    pqxx::result const r{query()};

    // Results can be accessed and iterated again.  Even after the connection
    // has been closed.
    for (auto row : r)
    {
      std::cout << "Row: ";
      // Iterate over fields in a row.
      for (auto field : row) std::cout << field.c_str() << " ";
      std::cout << std::endl;
    }
  }
  // Catch libpqxx exceptions.  There are different subclasses for various
  // types of errors, but we really don't need to care about those differences
  // unless we're trying to catch one very specific error.
  //
  // This central base libpqxx exception class, pqxx::failure, is derived from
  // std::exception.  So if we're going to catch both, we'll have to catch
  // pqxx::failure first.
  //
  // All the API for libpqxx exceptions is present right from this base class
  // on down.  Some of the more specific exception types will have additional
  // data, such as which SQL query triggered the error.  But the function to
  // read that query is present even in the base class.  When not applicable,
  // it will simply return an empty string.
  catch (pqxx::failure const &e)
  {
    std::cerr << e.name() << ": " << e.what() << '\n';
    std::cerr << "Happened in " << pqxx::source_loc(e.location()) << ".\n";
    if (not std::empty(e.query()))
      std::cerr << "Query was:\n" << e.query() << '\n';
    if (not std::empty(e.sqlstate()))
      std::cerr << "SQLSTATE " << e.sqlstate() << '\n';
    return 2;
  }
  // Catch other exceptions.
  catch (std::exception const &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
