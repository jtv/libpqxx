#include <pqxx/pipeline>
#include <pqxx/transaction>

#include "../test_helpers.hxx"

namespace
{
void test_cancel_query()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  // Calling cancel_query() while none is in progress has no effect.
  cx.cancel_query();

  // Nothing much is guaranteed about cancel_query, except that it doesn't make
  // the process die in flames.
  pqxx::pipeline p{tx, "test_cancel_query"};
  p.retain(0);
  p.insert("SELECT pg_sleep(1)");
  cx.cancel_query();
}


PQXX_REGISTER_TEST(test_cancel_query);
} // namespace
