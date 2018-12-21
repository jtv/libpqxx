#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.

namespace
{
void test_015(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  // See if deactivate() behaves...
#include <pqxx/internal/ignore-deprecated-pre.hxx>
  C.deactivate();
#include <pqxx/internal/ignore-deprecated-post.hxx>

  perform(
    [&C]()
    {
      nontransaction T{C};
      const auto r = T.exec("SELECT * FROM generate_series(1, 5)");
      PQXX_CHECK_EQUAL(r.size(), 5ul, "Weird query result.");
      T.commit();
    });
}
} // namespace

PQXX_REGISTER_TEST_T(test_015, nontransaction)
