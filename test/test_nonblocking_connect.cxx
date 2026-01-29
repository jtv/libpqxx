#include <pqxx/transaction>

#include <pqxx/internal/wait.hxx>

#include "helpers.hxx"


namespace
{
void test_nonblocking_connect(pqxx::test::context &)
{
  pqxx::connecting nbc;
  while (not nbc.done())
  {
    pqxx::internal::wait_fd(
      nbc.sock(), nbc.wait_to_read(), nbc.wait_to_write());
    nbc.process();
  }

  pqxx::connection cx{std::move(nbc).produce()};
  pqxx::work tx{cx};
  PQXX_CHECK_EQUAL(tx.query_value<int>("SELECT 10"), 10);
}


PQXX_REGISTER_TEST(test_nonblocking_connect);
} // namespace
