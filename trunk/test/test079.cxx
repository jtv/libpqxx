#include <cerrno>
#include <cstring>
#include <iostream>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Example program for libpqxx.  Test waiting for notification with timeout.
namespace
{
// Sample implementation of notification listener
class TestListener : public notify_listener
{
  bool m_Done;

public:
  explicit TestListener(connection_base &C, string Name) :
    notify_listener(C, Name), m_Done(false)
  {
  }

  virtual void operator()(int be_pid)
  {
    m_Done = true;
    PQXX_CHECK_EQUAL(
	be_pid,
	Conn().backendpid(),
	"Notification came from wrong backend.");

    cout << "Received notification: " << name() << " pid=" << be_pid << endl;
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
    T.exec("NOTIFY " + m_notif);
  }

  void on_abort(const char Reason[]) throw ()
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
  C.perform(Notify(L.name()));

  for (int i=0; (i < 20) && !L.Done(); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got notifications, but no handler called.");
    cout << ".";
    notifs = C.await_notification(1,0);
  }
  cout << endl;

  PQXX_CHECK(L.Done(), "No notifications received.");
  PQXX_CHECK_EQUAL(notifs, 1, "Got unexpected notifications.");
}
} // namespace

PQXX_REGISTER_TEST_T(test_079, nontransaction)
