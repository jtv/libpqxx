#include <iostream> // XXX: DEBUG
#include <pqxx/connection>
#include <pqxx/transaction>

#include <pqxx/internal/wait.hxx>

#include "../test_helpers.hxx"


namespace
{
void test_nonblocking_connect()
{
  std::clog << "Start test...\n"; // XXX: DEBUG
  pqxx::connecting nbc;
  std::clog << "(connecting)\n"; // XXX: DEBUG
  while (not nbc.done())
  {
    std::clog<<"  (wait)"; // XXX: DEBUG
    pqxx::internal::wait_fd(
      nbc.sock(), nbc.wait_to_read(), nbc.wait_to_write());
    std::clog<<"  (process)"; // XXX: DEBUG
    nbc.process();
  }
  std::clog<<"  (connected)"; // XXX: DEBUG

  pqxx::connection conn{std::move(nbc).produce()};
  pqxx::work tx{conn};
  PQXX_CHECK_EQUAL(tx.query_value<int>("SELECT 10"), 10, "Bad value!?");
}


PQXX_REGISTER_TEST(test_nonblocking_connect);
} // namespace
