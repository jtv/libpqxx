#include <cerrno>
#include <cstring>
#include <ctime>
#include <iostream>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/transactor>
#include <pqxx/notify-listen>
#include <pqxx/result>

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
    if (be_pid != Backend_PID)
      throw logic_error("Expected notification from backend process " +
		        to_string(Backend_PID) +
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
    T.exec("NOTIFY \"" + m_notif + "\"");
    Backend_PID = T.conn().backendpid();
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


void test_004()
{
  connection C("");
  cout << "Adding listener..." << endl;
  TestListener L(C);

  cout << "Sending notification..." << endl;
  C.perform(Notify(L.name()));

  int notifs = 0;
  for (int i=0; (i < 20) && !L.Done(); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notifications.");

    // Sleep one second using a libpqxx-internal function.  Kids, don't try
    // this at home!  The pqxx::internal namespace is not for third-party use
    // and may change radically at any time.
    pqxx::internal::sleep_seconds(1);
    notifs = C.get_notifs();
    cout << ".";
  }
  cout << endl;

  PQXX_CHECK_NOT_EQUAL(L.Done(), false, "No notification received.");
  PQXX_CHECK_EQUAL(notifs, 1, "Got too many notifications.");
}

} // namespace


int main()
{
  return test::pqxxtest(test_004);
}

