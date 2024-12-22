#include <cerrno>
#include <cstring>
#include <iostream>

#include <pqxx/transaction>
#include <pqxx/transactor>

#include "test_helpers.hxx"


// Example program for libpqxx.  Test waiting for notification with timeout.
namespace
{
void test_079()
{
  pqxx::connection cx;

  std::string const channel{"mylistener"};
  int backend_pid{0};

  cx.listen(channel, [&backend_pid](pqxx::notification n) noexcept {
    backend_pid = n.backend_pid;
  });

  // First see if the timeout really works: we're not expecting any notifs
  int notifs{cx.await_notification(0, 1)};
  PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notification.");

  pqxx::perform([&cx, &channel] {
    pqxx::work tx{cx};
    tx.notify(channel);
    tx.commit();
  });

  for (int i{0}; (i < 20) and (backend_pid == 0); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got notifications, but no handler called.");
    std::cout << ".";
    notifs = cx.await_notification(1, 0);
  }
  std::cout << std::endl;

  PQXX_CHECK_EQUAL(backend_pid, cx.backendpid(), "Wrong backend.");
  PQXX_CHECK_EQUAL(notifs, 1, "Got unexpected notifications.");
}
} // namespace


PQXX_REGISTER_TEST(test_079);
