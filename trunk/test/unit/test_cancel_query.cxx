#include <test_helpers.hxx>

using namespace PGSTD;
using namespace pqxx;

namespace
{
void test_cancel_query(transaction_base &t)
{
  // Calling cancel_query() while none is in progress has no effect.
  t.conn().cancel_query();

  // Nothing much is guaranteed about cancel_query, except that it doesn't make
  // the process die in flames.
  pipeline p(t, "test_cancel_query");
  p.retain(0);
  p.insert("SELECT pg_sleep(1)");
  t.conn().cancel_query();
}
} // namespace

PQXX_REGISTER_TEST(test_cancel_query)
