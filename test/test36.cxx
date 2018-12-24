#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
namespace
{
void test_036(transaction_base &)
{
  lazyconnection C;
  const auto r = perform(
    [&C](){ return nontransaction{C}.exec("SELECT generate_series(1, 8)"); });
  PQXX_CHECK_EQUAL(r.size(), 8u, "Unexpected transactor result.");
}
} // namespace

PQXX_REGISTER_TEST_NODB(test_036)
