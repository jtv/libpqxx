#include <cerrno>
#include <cstring>
#include <iostream>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/transactor>
#include <pqxx/trigger>
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

#ifdef WIN32
#include <windows.h>
#else
extern "C"
{
#ifdef PQXX_HAVE_SYS_SELECT_H
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

int Backend_PID = 0;


#ifndef _WIN32
// Reasonably portable way to sleep for a given number of milliseconds
/* This definition is not needed on Windows, which provides its own Sleep()
 * function with the same semantics.
 */
void Sleep(int ms)
{
  fd_set F;
  FD_ZERO(&F);
  struct timeval timeout;
  timeout.tv_sec = ms / 1000;
  timeout.tv_usec = (ms % 1000) * 1000;
  if (select(0, &F, &F, &F, &timeout) == -1)
    throw runtime_error(strerror(errno));
}
#endif	// _WIN32

// Sample implementation of trigger handler
class TestTrig : public trigger
{
  bool m_Done;

public:
  explicit TestTrig(connection_base &C) : trigger(C, "trig"), m_Done(false) {}

  virtual void operator()(int be_pid)
  {
    m_Done = true;
    if (be_pid != Backend_PID)
      throw logic_error("Expected notification from backend process " +
		        ToString(Backend_PID) +
			", but got one from " +
			ToString(be_pid));

    cout << "Received notification: " << name() << " pid=" << be_pid << endl;
  }

  bool Done() const { return m_Done; }
};


// A transactor to trigger our trigger handler
class Notify : public transactor<>
{
  string m_Trigger;

public:
  explicit Notify(string TrigName) : 
    transactor<>("Notifier"), m_Trigger(TrigName) { }

  void operator()(argument_type &T)
  {
    T.exec("NOTIFY " + m_Trigger);
    Backend_PID = T.conn().backendpid();
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
    connection C(argv[1]);
    cout << "Adding trigger..." << endl;
    TestTrig Trig(C);

    cout << "Sending notification..." << endl;
    C.perform(Notify(Trig.name()));

    for (int i=0; (i < 20) && !Trig.Done(); ++i)
    {
      Sleep(500);
      C.get_notifs();
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

