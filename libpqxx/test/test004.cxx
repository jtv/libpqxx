#include <cerrno>
#include <cstring>
#include <iostream>

#include <pqxx/connection.h>
#include <pqxx/transaction.h>
#include <pqxx/trigger.h>
#include <pqxx/result.h>


using namespace PGSTD;
using namespace pqxx;

// Example program for libpqxx.  Send notification to self.
//
// Usage: test4 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

#ifdef WIN32
#include <windows.h>
#else
extern "C"
{
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif
}
#endif

namespace
{

// Reasonably portable way to sleep for a given number of seconds
void Sleep(int seconds)
{
  fd_set F;
  FD_ZERO(&F);
  struct timeval timeout;
  timeout.tv_sec = seconds;
  timeout.tv_usec = 0;
  if (select(0, &F, &F, &F, &timeout) == -1)
    throw runtime_error(strerror(errno));
}


// Sample implementation of trigger handler
class TestTrig : public Trigger
{
  bool m_Done;

public:
  explicit TestTrig(ConnectionItf &C) : Trigger(C, "trig"), m_Done(false) {}

  virtual void operator()(int be_pid)
  {
    m_Done = true;
    if (be_pid != Conn().BackendPID())
      throw logic_error("Expected notification from backend process " +
		        ToString(Conn().BackendPID()) + 
			", but got one from " +
			ToString(be_pid));

    cout << "Received notification: " << Name() << " pid=" << be_pid << endl;
  }

  bool Done() const { return m_Done; }
};


// A Transactor to trigger our trigger handler
class Notify : public Transactor
{
  string m_Trigger;

public:
  explicit Notify(string TrigName) : 
    Transactor("Notifier"), m_Trigger(TrigName) { }

  void operator()(argument_type &T)
  {
    T.Exec("NOTIFY " + m_Trigger);
  }

  void OnAbort(const char Reason[]) throw ()
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
    Connection C(argv[1]);
    cout << "Adding trigger..." << endl;
    TestTrig Trig(C);

    cout << "Sending notification..." << endl;
    C.Perform(Notify(Trig.Name()));

    for (int i=0; (i < 20) && !Trig.Done(); ++i)
    {
      Sleep(1);
      C.GetNotifs();
      cout << ".";
    }
    cout << endl;

    if (!Trig.Done()) 
    {
      cout << "No notification received!" << endl;
      return 1;
    }
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

