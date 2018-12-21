#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
namespace
{
void test_017(transaction_base &T)
{
  connection_base &C(T.conn());
  T.abort();
  perform(
    [&C]()
    {
      nontransaction T{C};
      const auto r = T.exec("SELECT * FROM generate_series(1, 4)");
      PQXX_CHECK_EQUAL(r.size(), 4ul, "Weird query result.");
      T.commit();
    });
}
} // namespace

PQXX_REGISTER_TEST_T(test_017, nontransaction)
