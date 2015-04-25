#include <cerrno>
#include <cstring>
#include <ctime>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;

// Example program for libpqxx.  Send notification to self.

namespace
{

int Backend_PID = 0;


// Sample implementation of notification receiver.
class TestListener PQXX_FINAL : public notification_receiver
{
  bool m_done;

public:
  explicit TestListener(connection_base &C) :
	notification_receiver(C, "listen"), m_done(false) {}

  virtual void operator() PQXX_OVERRIDE (const string &, int be_pid)
  {
    m_done = true;
    PQXX_CHECK_EQUAL(
	be_pid,
	Backend_PID,
	"Notification came from wrong backend process.");
  }

  bool done() const { return m_done; }
};


// A transactor to trigger our notification receiver.
class Notify : public transactor<>
{
  string m_channel;

public:
  explicit Notify(string channel) :
    transactor<>("Notifier"), m_channel(channel) { }

  void operator()(argument_type &T)
  {
    T.exec("NOTIFY \"" + m_channel + "\"");
    Backend_PID = T.conn().backendpid();
  }
};


void test_004(transaction_base &T)
{
  T.abort();

  TestListener L(T.conn());

  T.conn().perform(Notify(L.channel()));

  int notifs = 0;
  for (int i=0; (i < 20) && !L.done(); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notifications.");

    // Sleep one second using a libpqxx-internal function.  Kids, don't try
    // this at home!  The pqxx::internal namespace is not for third-party use
    // and may change radically at any time.
    pqxx::internal::sleep_seconds(1);
    notifs = T.conn().get_notifs();
  }

  PQXX_CHECK_NOT_EQUAL(L.done(), false, "No notification received.");
  PQXX_CHECK_EQUAL(notifs, 1, "Got too many notifications.");
}

} // namespace

PQXX_REGISTER_TEST_T(test_004, nontransaction)
