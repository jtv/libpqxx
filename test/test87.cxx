#include <cctype>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <iostream>
#include "pqxx/config-public-compiler.h"

#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#else
#include <sys/types.h>
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(_WIN32)
#define NOMINMAX
#include <winsock2.h>
#endif
#endif // HAVE_SYS_SELECT_H

#include "test_helpers.hxx"

using namespace pqxx;


// Test program for libpqxx.  Send notification to self, and select() on socket
// as returned by the connection to wait for it to come in.  Normally one would
// use connection_base::await_notification() for this, but the socket may be
// needed for event loops waiting on multiple sources of events.
namespace
{
// Sample implementation of notification receiver.
class TestListener final : public notification_receiver
{
  bool m_done;

public:
  explicit TestListener(connection_base &conn, std::string Name) :
    notification_receiver(conn, Name), m_done(false)
  {
  }

  virtual void operator()(const std::string &, int be_pid) override
  {
    m_done = true;
    PQXX_CHECK_EQUAL(
	be_pid,
	conn().backendpid(),
	"Notification came from wrong backend process.");

    std::cout
	<< "Received notification: " << channel() << " pid=" << be_pid
	<< std::endl;
  }

  bool done() const { return m_done; }
};


extern "C"
{
static void set_fdbit(fd_set &s, int b)
{
#ifdef _MSC_VER
// Suppress pointless, unfixable warnings in Visual Studio.
#pragma warning ( push )
#pragma warning ( disable: 4389 ) // Signed/unsigned mismatch.
#pragma warning ( disable: 4127 ) // Conditional expression is constant.
#endif

  // Do the actual work.
  FD_SET(b,&s);

#ifdef _MSV_VER
// Restore prevalent warning settings.
#pragma warning ( pop )
#endif
}
}


void test_087()
{
  connection conn;

  const std::string NotifName = "my notification";
  TestListener L{conn, NotifName};

  perform(
    [&conn, &L]()
    {
      work tx{conn};
      tx.exec0("NOTIFY " + tx.quote_name(L.channel()));
      tx.commit();
    });

  int notifs = 0;
  for (int i=0; (i < 20) and not L.done(); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notifications.");

    std::cout << ".";
    const int fd = conn.sock();

    // File descriptor from which we wish to read.
    fd_set read_fds;
    FD_ZERO(&read_fds);
    set_fdbit(read_fds,fd);

    // File descriptor for which we want to see errors.  We can't just use
    // the same fd_set for reading and errors: they're marked "restrict".
    fd_set except_fds;
    FD_ZERO(&except_fds);
    set_fdbit(except_fds,fd);

    timeval timeout = { 1, 0 };
    select(fd+1, &read_fds, nullptr, &except_fds, &timeout);
    notifs = conn.get_notifs();
  }
  std::cout << std::endl;

  PQXX_CHECK(L.done(), "No notification received.");
  PQXX_CHECK_EQUAL(notifs, 1, "Got unexpected number of notifications.");
}
} // namespace


PQXX_REGISTER_TEST(test_087);
