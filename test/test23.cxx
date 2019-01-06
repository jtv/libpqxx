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
class TestListener final : public notification_receiver
{
  bool m_done;

public:
  explicit TestListener(connection_base &C) :
	notification_receiver(C, "listen"), m_done(false) {}

  virtual void operator()(const string &, int be_pid) override
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


void test_023()
{
  lazyconnection C;
  cout << "Adding listener..." << endl;
  TestListener L{C};

  cout << "Sending notification..." << endl;
  perform([&C, &L](){ nontransaction{C}.exec("NOTIFY " + L.channel()); });

  int notifs = 0;
  for (int i=0; (i < 20) and not L.done(); ++i)
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


PQXX_REGISTER_TEST(test_023);
} // namespace
