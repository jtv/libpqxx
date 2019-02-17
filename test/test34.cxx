#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
// This test uses a lazy connection.
namespace
{
void test_034()
{
  lazyconnection conn;

#include <pqxx/internal/ignore-deprecated-pre.hxx>
  // See if deactivate() behaves...
  conn.deactivate();
#include <pqxx/internal/ignore-deprecated-post.hxx>

  const auto r = perform(
    [&conn](){
	return nontransaction{conn}.exec("SELECT generate_series(1, 4)");
    });

  PQXX_CHECK_EQUAL(r.size(), 4u, "Unexpected transactor result.");
}


PQXX_REGISTER_TEST(test_034);
} // namespace
