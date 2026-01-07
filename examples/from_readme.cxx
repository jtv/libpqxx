#include <iostream>
#include <pqxx/pqxx>

namespace
{
void set_up(pqxx::connection &cx)
{
  pqxx::work tx{cx};
  tx.exec("CREATE TEMP TABLE employee(name varchar, salary money)");
  tx.exec("INSERT INTO employee(name, salary) VALUES ('Me', 5432)");
  tx.commit();
}
} // namespace


int main()
{
  try
  {
    // Connect to the database.  You can have multiple connections open
    // at the same time, even to the same database.
    pqxx::connection cx;
    std::cout << "Connected to " << cx.dbname() << '\n';

    set_up(cx);

    // Start a transaction.  A connection can only have one transaction
    // open at the same time, but after you finish a transaction, you
    // can start a new one on the same connection.
    pqxx::work tx{cx};

    // Query data of two columns, converting them to std::string and
    // int respectively.  Iterate the rows.
    for (auto const &[name, salary] : tx.query<std::string, int>(
           "SELECT name, floor(salary::numeric) FROM employee ORDER BY name"))
    {
      std::cout << name << " earns " << salary << ".\n";
    }

    // For large amounts of data, "streaming" the results is more
    // efficient.  It does not work for all types of queries though.
    //
    // You can read fields as std::string_view here, which is not
    // something you can do in most places.  A string_view becomes
    // meaningless when the underlying string ceases to exist.  In this
    // particular situation, you can convert a field to string_view and
    // it will be valid for just that one iteration of the loop.  The
    // next iteration may overwrite or deallocate its buffer space.
    for (auto [name, salary] : tx.stream<std::string_view, int>(
           "SELECT name, floor(salary::numeric) FROM employee"))
    {
      std::cout << name << " earns " << salary << ".\n";
    }

    // Execute a statement, and check that it returns 0 rows of data.
    // This will throw pqxx::unexpected_rows if the query returns rows.
    std::cout << "Doubling all employees' salaries...\n";
    tx.exec("UPDATE employee SET salary = salary*2").no_rows();

    // Shorthand: conveniently query a single value from the database,
    // and convert it to an `int`.
    int const my_salary = tx.query_value<int>(
      "SELECT floor(salary::numeric) FROM employee WHERE name = 'Me'");
    std::cout << "I now earn " << my_salary << ".\n";

    // Or, query one whole row.  This function will throw an exception
    // unless the result contains exactly 1 row.
    auto [top_name, top_salary] = tx.query1<std::string, int>(
      R"(
                    SELECT name, floor(salary::numeric)
                    FROM employee
                    WHERE salary = (SELECT max(salary) FROM employee)
                    LIMIT 1
                )");
    std::cout << "Top earner is " << top_name << " with a salary of "
              << top_salary << ".\n";

    // If you need to access the result metadata, not just the actual
    // field values, use `exec()`.  It returns a pqxx::result object.
    pqxx::result const res = tx.exec("SELECT * FROM employee");
    std::cout << "Columns:\n";
    for (pqxx::row_size_type col = 0; col < res.columns(); ++col)
      std::cout << res.column_name(col) << '\n';

    // Commit the transaction.  If you don't do this, the database will
    // undo any changes you made in the transaction.
    std::cout << "Making changes definite: ";
    tx.commit();
    std::cout << "OK.\n";
  }
  catch (std::exception const &e)
  {
    std::cerr << "ERROR: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
