#include <cerrno>
#include <cstring>
#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Example program for libpqxx.  Send notification to self, using deferred
// connection.
namespace
{

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
	conn().backendpid(),
	"Notification came from wrong backend process.");

    cout << "Received notification: " << channel() << " pid=" << be_pid << endl;
  }

  bool done() const { return m_done; }
};


// A transactor to trigger our notification listener
class Notify : public transactor<>
{
  string m_channel;

public:
  explicit Notify(string NotifName) :
    transactor<>("Notifier"), m_channel(NotifName) { }

  void operator()(argument_type &T)
  {
    T.exec("NOTIFY " + m_channel);
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


void test_023(transaction_base &)
{
  lazyconnection C;
  cout << "Adding listener..." << endl;
  TestListener L(C);

  cout << "Sending notification..." << endl;
  C.perform(Notify(L.channel()));

  int notifs = 0;
  for (int i=0; (i < 20) && !L.done(); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notifications.");
    pqxx::internal::sleep_seconds(1);
    notifs = C.get_notifs();
    cout << ".";
  }
  cout << endl;

  PQXX_CHECK(L.done(), "No notification received.");

  PQXX_CHECK_EQUAL(notifs, 1, "Unexpected number of notifications.");
}
} // namespace

PQXX_REGISTER_TEST_NODB(test_023)
