#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
namespace
{
void test_036()
{
  lazyconnection conn;
  const auto r = perform(
    [&conn]()
    {
      return nontransaction{conn}.exec("SELECT generate_series(1, 8)");
    });
  PQXX_CHECK_EQUAL(r.size(), 8u, "Unexpected transactor result.");
}


PQXX_REGISTER_TEST(test_036);
} // namespace
