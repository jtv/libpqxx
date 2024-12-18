#include <cerrno>
#include <cstring>
#include <iostream>

#include <pqxx/nontransaction>
#include <pqxx/transactor>

#include "test_helpers.hxx"


// Example program for libpqxx.  Send notification to self, using a
// notification name with unusal characters, and without polling.
namespace
{
void test_078()
{
  pqxx::connection cx;
  bool done{false};

  std::string const channel{"my listener"};
  cx.listen(channel, [&done](pqxx::notification) noexcept { done = true; });

  pqxx::perform([&cx, &channel] {
    pqxx::nontransaction tx{cx};
    tx.notify(channel);
    tx.commit();
  });

  int notifs{0};
  for (int i{0}; (i < 20) and not done; ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notifications.");
    std::cout << ".";
    notifs = cx.await_notification();
  }
  std::cout << std::endl;

  PQXX_CHECK(done, "No notification received.");
  PQXX_CHECK_EQUAL(notifs, 1, "Got unexpected number of notifications.");
}
} // namespace


PQXX_REGISTER_TEST(test_078);
