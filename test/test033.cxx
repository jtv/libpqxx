#include <iostream>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
namespace
{
void test_033(transaction_base &T)
{
  result R( T.exec("SELECT * FROM pg_tables") );

  for (result::const_iterator c = R.begin(); c != R.end(); ++c)
  {
    string N;
    c[0].to(N);

    cout << '\t' << to_string(c.num()) << '\t' << N << endl;
  }

  // "Commit" the non-transaction.  This doesn't really do anything since
  // nontransaction doesn't start a backend transaction.
  T.commit();
}
} // namespace

PQXX_REGISTER_TEST_CT(test_033, lazyconnection, nontransaction)
