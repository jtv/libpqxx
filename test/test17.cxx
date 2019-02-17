#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
namespace
{
void test_017()
{
  connection conn;
  perform(
    [&conn]()
    {
      nontransaction tx{conn};
      const auto r = tx.exec("SELECT * FROM generate_series(1, 4)");
      PQXX_CHECK_EQUAL(r.size(), 4ul, "Weird query result.");
      tx.commit();
    });
}


PQXX_REGISTER_TEST(test_017);
} // namespace
