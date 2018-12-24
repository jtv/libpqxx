#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
// This test uses a lazy connection.
namespace
{
void test_034(transaction_base &T)
{
  connection_base &C(T.conn());
  T.abort();

#include <pqxx/internal/ignore-deprecated-pre.hxx>
  // See if deactivate() behaves...
  C.deactivate();
#include <pqxx/internal/ignore-deprecated-post.hxx>

  const auto r = perform(
    [&C](){
	return nontransaction{C}.exec("SELECT generate_series(1, 4)");
    });

  PQXX_CHECK_EQUAL(r.size(), 4u, "Unexpected transactor result.");
}
} // namespace

PQXX_REGISTER_TEST_CT(test_034, lazyconnection, nontransaction)
