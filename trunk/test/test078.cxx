#include <cerrno>
#include <cstring>
#include <iostream>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Example program for libpqxx.  Send notification to self, using a notification
// name with unusal characters, and without polling.
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
	"Got notification from wrong backend process.");

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
    T.exec("NOTIFY \"" + m_notif + "\"");
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


void test_078(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  const string NotifName = "my listener";
  cout << "Adding listener..." << endl;
  TestListener L(C, NotifName);

  cout << "Sending notification..." << endl;
  C.perform(Notify(L.name()));

  int notifs = 0;
  for (int i=0; (i < 20) && !L.Done(); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notifications.");
    cout << ".";
    notifs = C.await_notification();
  }
  cout << endl;

  PQXX_CHECK(L.Done(), "No notification received.");
  PQXX_CHECK_EQUAL(notifs, 1, "Got unexpected number of notifications.");
}
} // namespace

PQXX_REGISTER_TEST_T(test_078, nontransaction)
