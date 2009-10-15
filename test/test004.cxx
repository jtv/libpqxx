#include <cerrno>
#include <cstring>
#include <ctime>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;

// Example program for libpqxx.  Send notification to self.

namespace
{

int Backend_PID = 0;


// Sample implementation of notification listener
class TestListener : public notify_listener
{
  bool m_Done;

public:
  explicit TestListener(connection_base &C) :
	notify_listener(C, "listen"), m_Done(false) {}

  virtual void operator()(int be_pid)
  {
    m_Done = true;
    PQXX_CHECK_EQUAL(
	be_pid,
	Backend_PID,
	"Notification came from wrong backend process.");
  }

  bool Done() const { return m_Done; }
};


// A transactor to trigger our notification listener
class Notify : public transactor<>
{
  string m_notif;

public:
  explicit Notify(string NotifName) :
    transactor<>("Notifier"), m_notif(NotifName) { }

  void operator()(argument_type &T)
  {
    T.exec("NOTIFY \"" + m_notif + "\"");
    Backend_PID = T.conn().backendpid();
  }
};


void test_004(transaction_base &T)
{
  T.abort();

  TestListener L(T.conn());

  T.conn().perform(Notify(L.name()));

  int notifs = 0;
  for (int i=0; (i < 20) && !L.Done(); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notifications.");

    // Sleep one second using a libpqxx-internal function.  Kids, don't try
    // this at home!  The pqxx::internal namespace is not for third-party use
    // and may change radically at any time.
    pqxx::internal::sleep_seconds(1);
    notifs = T.conn().get_notifs();
  }

  PQXX_CHECK_NOT_EQUAL(L.Done(), false, "No notification received.");
  PQXX_CHECK_EQUAL(notifs, 1, "Got too many notifications.");
}

} // namespace

PQXX_REGISTER_TEST_T(test_004, nontransaction)
