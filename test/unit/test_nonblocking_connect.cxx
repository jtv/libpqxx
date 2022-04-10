#include <iostream> // XXX: DEBUG
#include <pqxx/connection>
#include <pqxx/transaction>

#include <pqxx/internal/wait.hxx>

#include "../test_helpers.hxx"


namespace
{
void test_nonblocking_connect()
{
  pqxx::connecting nbc;
  while (not nbc.done())
  {
    pqxx::internal::wait_fd(
      nbc.sock(), nbc.wait_to_read(), nbc.wait_to_write());
    nbc.process();
  }

std::clog<<"Connected.\n"; // XXX: DEBUG
  pqxx::connection conn{std::move(nbc).produce()};
std::clog<<"Produced.\n"; // XXX: DEBUG
  pqxx::work tx{conn};
std::clog<<"Transacting.\n"; // XXX: DEBUG
  PQXX_CHECK_EQUAL(tx.query_value<int>("SELECT 10"), 10, "Bad value!?");
std::clog<<"Done.\n"; // XXX: DEBUG
}


PQXX_REGISTER_TEST(test_nonblocking_connect);
} // namespace
