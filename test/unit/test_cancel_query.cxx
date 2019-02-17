#include "../test_helpers.hxx"

using namespace pqxx;

namespace
{
void test_cancel_query()
{
  connection conn;
  work tx{conn};
  // Calling cancel_query() while none is in progress has no effect.
  conn.cancel_query();

  // Nothing much is guaranteed about cancel_query, except that it doesn't make
  // the process die in flames.
  pipeline p{tx, "test_cancel_query"};
  p.retain(0);
  p.insert("SELECT pg_sleep(1)");
  conn.cancel_query();
}


PQXX_REGISTER_TEST(test_cancel_query);
} // namespace
