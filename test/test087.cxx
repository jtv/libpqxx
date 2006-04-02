#include <cctype>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <iostream>

#if defined(PQXX_HAVE_SYS_SELECT_H)
#include <sys/select.h>
#else
#include <sys/types.h>
#if defined(PQXX_HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(_WIN32)
#define NOMINMAX
#include <winsock2.h>
#endif
#endif // PQXX_HAVE_SYS_SELECT_H

#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/transactor>
#include <pqxx/trigger>
#include <pqxx/result>


using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Send notification to self, and select() on socket
// as returned by the connection to wait for it to come in.  Normally one would
// use connection_base::await_notification() for this, but the socket may be
// needed for event loops waiting on multiple sources of events.
//
// Usage: test087


namespace
{

// Sample implementation of trigger handler
class TestTrig : public trigger
{
  bool m_Done;

public:
  explicit TestTrig(connection_base &C, string Name) : 
    trigger(C, Name), m_Done(false) 
  {
  }

  virtual void operator()(int be_pid)
  {
    m_Done = true;
    if (be_pid != Conn().backendpid())
      throw logic_error("Expected notification from backend process " +
		        to_string(Conn().backendpid()) + 
			", but got one from " +
			to_string(be_pid));

    cout << "Received notification: " << name() << " pid=" << be_pid << endl;
  }

  bool Done() const { return m_Done; }
};


// A transactor to trigger our trigger handler
class Notify : public transactor<nontransaction>
{
  string m_Trigger;

public:
  explicit Notify(string TrigName) : 
    transactor<nontransaction>("Notifier"), m_Trigger(TrigName) { }

  void operator()(argument_type &T)
  {
    T.exec("NOTIFY \"" + m_Trigger + "\"");
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

extern "C" 
{
static void set_fdbit(fd_set &s, int b)
{
  FD_SET(b,&s);
}
}

} // namespace

int main()
{
  try
  {
    const string TrigName = "my trigger";
    connection C;
    cout << "Adding trigger..." << endl;
    TestTrig Trig(C, TrigName);

    cout << "Sending notification..." << endl;
    C.perform(Notify(Trig.name()));

    int notifs = 0;
    for (int i=0; (i < 20) && !Trig.Done(); ++i)
    {
      if (notifs)
	throw logic_error("Got " + to_string(notifs) + " "
	    "unexpected notification(s)!");
      cout << ".";
      const int fd = C.sock();
      fd_set fds;
      FD_ZERO(&fds);
      set_fdbit(fds,fd);
      timeval timeout = { 1, 0 };
      select(fd+1, &fds, 0, &fds, &timeout);
      notifs = C.get_notifs();
    }
    cout << endl;

    if (!Trig.Done()) 
    {
      cout << "No notification received!" << endl;
      return 1;
    }
    if (notifs != 1)
      throw logic_error("Expected 1 notification, got " + to_string(notifs));
  }
  catch (const sql_error &e)
  {
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

