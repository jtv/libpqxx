#include <cerrno>
#include <cstring>
#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Example program for libpqxx.  Send notification to self, using a notification
// name with unusal characters, and without polling.
namespace
{
// Sample implementation of notification receiver.
class TestListener : public notification_receiver
{
  bool m_done;

public:
  explicit TestListener(connection_base &C, string Name) :
    notification_receiver(C, Name), m_done(false)
  {
  }

  virtual void operator()(const string &, int be_pid)
  {
    m_done = true;
    PQXX_CHECK_EQUAL(
	be_pid,
	conn().backendpid(),
	"Got notification from wrong backend process.");

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
    T.exec("NOTIFY \"" + m_notif + "\"");
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


void test_078(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  const string NotifName = "my listener";
  cout << "Adding listener..." << endl;
  TestListener L(C, NotifName);

  cout << "Sending notification..." << endl;
  C.perform(Notify(L.channel()));

  int notifs = 0;
  for (int i=0; (i < 20) && !L.done(); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notifications.");
    cout << ".";
    notifs = C.await_notification();
  }
  cout << endl;

  PQXX_CHECK(L.done(), "No notification received.");
  PQXX_CHECK_EQUAL(notifs, 1, "Got unexpected number of notifications.");
}
} // namespace

PQXX_REGISTER_TEST_T(test_078, nontransaction)
