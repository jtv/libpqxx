#include <iostream>

#include <unistd.h>

#include "pg_connection.h"
#include "pg_transaction.h"
#include "pg_trigger.h"
#include "pg_result.h"

using namespace PGSTD;
using namespace Pg;


// Example program for libpqxx.  Send notification to self.
//
// Usage: test4 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

// Sample implementation of trigger handler
class TestTrig : public Trigger
{
  bool m_Done;

public:
  explicit TestTrig(Connection &C) : Trigger(C, "trig"), m_Done(false) {}

  virtual void operator()(int be_pid)
  {
    m_Done = true;
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

  void operator()(Transaction &T)
  {
    T.Exec(("NOTIFY " + m_Trigger).c_str());
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


int main(int argc, char *argv[])
{
  try
  {
    Connection C(argv[1] ? argv[1] : "");
    cout << "Adding trigger..." << endl;
    TestTrig Trig(C);

    cout << "Sending notification..." << endl;
    C.Perform(Notify(Trig.Name()));

    for (int i=0; (i < 20) && !Trig.Done(); ++i)
    {
      sleep(1);
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

