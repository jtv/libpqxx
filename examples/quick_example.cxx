#include <iostream>
#include <pqxx/pqxx>

namespace
{
void set_up(pqxx::connection &cx)
{
  pqxx::transaction tx{cx};
  tx.exec(
    "CREATE TEMP TABLE Employee (id integer, name varchar, salary integer)");
  tx.exec(
    "INSERT INTO Employee(id, name, salary) VALUES (1, 'Ichiban', 65432)");
  tx.commit();
}
} // namespace


int main(int, char *argv[])
{
  // (Normally you'd check for valid command-line arguments.)
  char const *const name = ((argv[1] == nullptr) ? "Ichiban" : argv[1]);

  // (Normally you'd pass connection settings to the connection constructor.)
  pqxx::connection cx;

  set_up(cx);

  pqxx::work tx{cx};

  // For querying just one single value, the transaction has a shorthand method
  // query_value().
  //
  // The employee ID shows up in the query as "$1"; that means we'll pass it as
  // a parameter.  Pass all parameters together in a single "params" object.
  int const employee_id = tx.query_value<int>(
    "SELECT id "
    "FROM Employee "
    "WHERE name = $1",
    pqxx::params{name});

  std::cout << "Updating employee #" << employee_id << '\n';

  // Update the employee's salary.  Use exec() to perform the command, and
  // no_rows() to check that it produces no result rows.  If the result does
  // contain data, this will throw an exception.
  tx.exec(
      "UPDATE Employee "
      "SET salary = salary + 1 "
      "WHERE id = $1",
      pqxx::params{employee_id})
    .no_rows();

  // Make our change definite.
  tx.commit();
}
