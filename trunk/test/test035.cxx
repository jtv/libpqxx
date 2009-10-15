#include <iostream>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
// This test combines a lazy connection with a robust transaction.
namespace
{
void test_035(transaction_base &T)
{
  result R( T.exec("SELECT * FROM pg_tables") );

  for (result::const_iterator c = R.begin(); c != R.end(); ++c)
  {
    string N;
    c[0].to(N);

    cout << '\t' << to_string(c.num()) << '\t' << N << endl;
  }

  // "Commit" the non-transaction.  This doesn't really do anything since
  // NonTransaction doesn't start a backend transaction.
  T.commit();
}
} // namespace

PQXX_REGISTER_TEST_CT(test_035, lazyconnection, robusttransaction<>)
