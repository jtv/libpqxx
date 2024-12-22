#include "pqxx/config-public-compiler.h"
#include <cctype>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <iostream>

#include <pqxx/internal/header-pre.hxx>

#include <pqxx/internal/wait.hxx>

#include <pqxx/internal/header-post.hxx>

#include <pqxx/transaction>
#include <pqxx/transactor>

#include "test_helpers.hxx"


// Test program for libpqxx.  Send notification to self, and wait on the
// socket's connection for it to come in.  In a simple situation you'd use
// connection::await_notification() for this, but that won't let you wait for
// multiple sockets.
namespace
{
void test_087()
{
  pqxx::connection cx;

  std::string const channel{"my notification"};
  int backend_pid{0};

  cx.listen(channel, [&backend_pid](pqxx::notification n) noexcept {
    backend_pid = n.backend_pid;
  });

  pqxx::perform([&cx, &channel] {
    pqxx::work tx{cx};
    tx.notify(channel);
    tx.commit();
  });

  int notifs{0};
  for (int i{0}; (i < 20) and (backend_pid == 0); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notifications.");

    std::cout << ".";

    pqxx::internal::wait_fd(cx.sock(), true, false);
    notifs = cx.get_notifs();
  }
  std::cout << std::endl;

  PQXX_CHECK_EQUAL(
    backend_pid, cx.backendpid(), "Notification came from wrong backend.");
  PQXX_CHECK_EQUAL(notifs, 1, "Got unexpected number of notifications.");
}
} // namespace


PQXX_REGISTER_TEST(test_087);
