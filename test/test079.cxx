#include <cerrno>
#include <cstring>
#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Example program for libpqxx.  Test waiting for notification with timeout.
namespace
{
// Sample implementation of notification receiver.
class TestListener PQXX_FINAL : public notification_receiver
{
  bool m_done;

public:
  explicit TestListener(connection_base &C, string Name) :
    notification_receiver(C, Name), m_done(false)
  {
  }

  virtual void operator()(const string &, int be_pid) PQXX_OVERRIDE
  {
    m_done = true;
    PQXX_CHECK_EQUAL(
	be_pid,
	conn().backendpid(),
	"Notification came from wrong backend.");

    cout << "Received notification: " << channel() << " pid=" << be_pid << endl;
  }

  bool done() const { return m_done; }
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
    T.exec("NOTIFY " + m_notif);
  }

  void on_abort(const char Reason[]) PQXX_NOEXCEPT
  {
    try
    {
      cerr << "Notify failed!" << endl;
      if (Reason) cerr << "Reason: " << Reason << endl;
    }
    catch (const exception &)
    {
    }
  }
};


void test_079(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  const string NotifName = "mylistener";
  cout << "Adding listener..." << endl;
  TestListener L(C, NotifName);

  // First see if the timeout really works: we're not expecting any notifs
  int notifs = C.await_notification(0, 1);
  PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notification.");

  cout << "Sending notification..." << endl;
  C.perform(Notify(L.channel()));

  for (int i=0; (i < 20) && !L.done(); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got notifications, but no handler called.");
    cout << ".";
    notifs = C.await_notification(1,0);
  }
  cout << endl;

  PQXX_CHECK(L.done(), "No notifications received.");
  PQXX_CHECK_EQUAL(notifs, 1, "Got unexpected notifications.");
}
} // namespace

PQXX_REGISTER_TEST_T(test_079, nontransaction)
