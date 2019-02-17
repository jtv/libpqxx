#include <cerrno>
#include <cstring>
#include <ctime>

#include "test_helpers.hxx"

using namespace pqxx;

// Example program for libpqxx.  Send notification to self.

namespace
{

int Backend_PID = 0;


// Sample implementation of notification receiver.
class TestListener final : public notification_receiver
{
  bool m_done;

public:
  explicit TestListener(connection_base &conn) :
	notification_receiver(conn, "listen"), m_done(false) {}

  virtual void operator()(const std::string &, int be_pid) override
  {
    m_done = true;
    PQXX_CHECK_EQUAL(
	be_pid,
	Backend_PID,
	"Notification came from wrong backend process.");
  }

  bool done() const { return m_done; }
};


void test_004()
{
  connection conn;

  TestListener L{conn};
  // Trigger our notification receiver.
  perform(
    [&conn, &L]()
    {
      work tx(conn);
      tx.exec("NOTIFY " + conn.quote_name(L.channel()));
      Backend_PID = conn.backendpid();
      tx.commit();
    });

  int notifs = 0;
  for (int i=0; (i < 20) and not L.done(); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notifications.");

    // Sleep one second using a libpqxx-internal function.  Kids, don't try
    // this at home!  The pqxx::internal namespace is not for third-party use
    // and may change radically at any time.
    pqxx::internal::sleep_seconds(1);
    notifs = conn.get_notifs();
  }

  PQXX_CHECK_NOT_EQUAL(L.done(), false, "No notification received.");
  PQXX_CHECK_EQUAL(notifs, 1, "Got too many notifications.");
}


PQXX_REGISTER_TEST(test_004);
} // namespace
