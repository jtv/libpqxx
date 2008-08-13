#include <cerrno>
#include <cstring>
#include <iostream>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/transactor>
#include <pqxx/notify-listen>
#include <pqxx/result>

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
    if (be_pid != Conn().backendpid())
      throw logic_error("Expected notification from backend process " +
		        to_string(Conn().backendpid()) +
			", but got one from " +
			to_string(be_pid));

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


void test_079(connection_base &C, transaction_base &orgT)
{
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
