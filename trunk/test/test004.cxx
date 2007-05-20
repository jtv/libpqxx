#include <cerrno>
#include <cstring>
#include <ctime>
#include <iostream>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/transactor>
#include <pqxx/notify-listen>
#include <pqxx/result>


using namespace PGSTD;
using namespace pqxx;

// Example program for libpqxx.  Send notification to self.
//
// Usage: test004 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

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

} // namespace


int main(int, char *argv[])
{
  try
  {
    connection C(argv[1]);
    cout << "Adding listener..." << endl;
    TestListener L(C);

    cout << "Sending notification..." << endl;
    C.perform(Notify(L.name()));

    int notifs = 0;
    for (int i=0; (i < 20) && !L.Done(); ++i)
    {
      if (notifs)
	throw logic_error("Got " + to_string(notifs) +
	    " unexpected notification(s)!");
      // Sleep one second using a libpqxx-internal function.  Kids, don't try
      // this at home!  The pqxx::internal namespace is not for third-party use
      // and may change radically at any time.
      pqxx::internal::sleep_seconds(1);
      notifs = C.get_notifs();
      cout << ".";
    }
    cout << endl;

    if (!L.Done())
    {
      cout << "No notification received!" << endl;
      return 1;
    }
    if (notifs != 1)
      throw logic_error("Expected 1 notification, got " + to_string(notifs));
  }
  catch (const sql_error &e)
  {
    // If we're interested in the text of a failed query, we can write separate
    // exception handling code for this type of exception
    cerr << "SQL error: " << e.what() << endl
         << "Query was: '" << e.query() << "'" << endl;
    return 1;
  }
  catch (const exception &e)
  {
    // All exceptions thrown by libpqxx are derived from std::exception
    cerr << "Exception: " << e.what() << endl;
    return 2;
  }
  catch (...)
  {
    // This is really unexpected (see above)
    cerr << "Unhandled exception" << endl;
    return 100;
  }

  return 0;
}

