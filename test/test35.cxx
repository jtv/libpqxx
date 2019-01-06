#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
// This test combines a lazy connection with a robust transaction.
namespace
{
void test_035()
{
  lazyconnection conn;
  robusttransaction<> tx{conn};
  result R( tx.exec("SELECT * FROM pg_tables") );

  for (const auto &c: R)
  {
    string N;
    c[0].to(N);
    cout << '\t' << to_string(c.num()) << '\t' << N << endl;
  }

  tx.commit();
}


PQXX_REGISTER_TEST(test_035);
} // namespace
