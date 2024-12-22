#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>

#include <pqxx/internal/header-pre.hxx>

#include <pqxx/internal/wait.hxx>

#include <pqxx/internal/header-post.hxx>

#include <pqxx/transaction>
#include <pqxx/transactor>

#include "test_helpers.hxx"

// Example program for libpqxx.  Send notification to self.

namespace
{
void test_004()
{
  auto const channel{"pqxx_test_notif"};
  pqxx::connection cx;
  int backend_pid{0};
  cx.listen(channel, [&backend_pid](pqxx::notification n) noexcept {
    backend_pid = n.backend_pid;
  });

  // Trigger our notification receiver.
  pqxx::perform([&cx, &channel] {
    pqxx::work tx(cx);
    tx.notify(channel);
    tx.commit();
  });

  int notifs{0};
  for (int i{0}; (i < 20) and (backend_pid == 0); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notifications.");
    // Sleep for one second.  I'm not proud of this, but how does one inject
    // a change to the built-in clock in a static language?
    pqxx::internal::wait_for(1000u);
    notifs = cx.get_notifs();
  }

  PQXX_CHECK_EQUAL(
    backend_pid, cx.backendpid(),
    "Did not get our notification from our own backend.");
  PQXX_CHECK_EQUAL(notifs, 1, "Got too many notifications.");
}


PQXX_REGISTER_TEST(test_004);
} // namespace
