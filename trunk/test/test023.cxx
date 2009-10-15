#include <cerrno>
#include <cstring>
#include <iostream>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Example program for libpqxx.  Send notification to self, using deferred
// connection.
namespace
{

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
	conn().backendpid(),
	"Notification came from wrong backend process.");

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


void test_023(transaction_base &)
{
  lazyconnection C;
  cout << "Adding listener..." << endl;
  TestListener L(C);

  cout << "Sending notification..." << endl;
  C.perform(Notify(L.name()));

  int notifs = 0;
  for (int i=0; (i < 20) && !L.Done(); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notifications.");
    pqxx::internal::sleep_seconds(1);
    notifs = C.get_notifs();
    cout << ".";
  }
  cout << endl;

  PQXX_CHECK(L.Done(), "No notification received.");

  PQXX_CHECK_EQUAL(notifs, 1, "Unexpected number of notifications.");
}
} // namespace

PQXX_REGISTER_TEST_NODB(test_023)
