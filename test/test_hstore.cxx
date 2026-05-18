#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_hstore(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  if (not pqxx::test::have_extension(tx, "hstore"))
    return;

  // XXX: Test!
}
} // namespace


PQXX_REGISTER_TEST(test_hstore);
