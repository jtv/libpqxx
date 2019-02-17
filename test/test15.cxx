#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.

namespace
{
void test_015()
{
  connection conn;

  // See if deactivate() behaves...
#include <pqxx/internal/ignore-deprecated-pre.hxx>
  conn.deactivate();
#include <pqxx/internal/ignore-deprecated-post.hxx>

  perform(
    [&conn]()
    {
      nontransaction tx{conn};
      const auto r = tx.exec("SELECT * FROM generate_series(1, 5)");
      PQXX_CHECK_EQUAL(r.size(), 5ul, "Weird query result.");
      tx.commit();
    });
}


PQXX_REGISTER_TEST(test_015);
} // namespace
