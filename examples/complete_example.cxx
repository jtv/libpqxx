#include <iostream>
#include <pqxx/pqxx>

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
  catch (pqxx::sql_error const &e)
  {
    std::cerr << "SQL error: " << e.what() << std::endl;
    std::cerr << "Query was: " << e.query() << std::endl;
    return 2;
  }
  catch (std::exception const &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
