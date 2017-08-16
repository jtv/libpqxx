#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Simple test program for libpqxx's asyncconnection.  Asynchronously open a
// connection to the database, start a transaction, and perform a query inside
// it.
namespace
{
void test_063(transaction_base &T)
{
  result R( T.exec("SELECT * FROM pg_tables") );
  PQXX_CHECK(!R.empty(), "No tables found.  Cannot test.");

  for (const auto &c: R)
    cout << '\t' << to_string(c.num()) << '\t' << c[0].as(string()) << endl;

  T.commit();
}
} // namespace

PQXX_REGISTER_TEST_C(test_063, asyncconnection)
