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

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Send notification to self, and select() on socket
// as returned by the connection to wait for it to come in.  Normally one would
// use connection_base::await_notification() for this, but the socket may be
// needed for event loops waiting on multiple sources of events.
namespace
{
// Sample implementation of notification listener
class TestListener : public notify_listener
{
  bool m_Done;

public:
  explicit TestListener(connection_base &C, string Name) :
    notify_listener(C, Name), m_Done(false)
  {
  }

  virtual void operator()(int be_pid)
  {
    m_Done = true;
    PQXX_CHECK_EQUAL(
	be_pid,
	Conn().backendpid(),
	"Notification came from wrong backend process.");

    cout << "Received notification: " << name() << " pid=" << be_pid << endl;
  }

  bool Done() const { return m_Done; }
};


// A transactor to trigger our notification listener
class Notify : public transactor<nontransaction>
{
  string m_notif;

public:
  explicit Notify(string NotifName) :
    transactor<nontransaction>("Notifier"), m_notif(NotifName) { }

  void operator()(argument_type &T)
  {
    T.exec("NOTIFY \"" + m_notif + "\"");
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
#ifdef _MSC_VER 
// Suppress pointless, unfixable warnings in Visual Studio.
#pragma warning ( push ) 
#pragma warning ( disable: 4389 ) // Signed/unsigned mismatch.
#pragma warning ( disable: 4127 ) // Conditional expression is constant.
#endif 

  // Do the actual work.
  FD_SET(b,&s);

#ifdef _MSV_VER
// Restore prevalent warning settings.
#pragma warning ( pop )
#endif
}
}


void test_087(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  const string NotifName = "my notification";
  cout << "Adding listener..." << endl;
  TestListener L(C, NotifName);

  cout << "Sending notification..." << endl;
  C.perform(Notify(L.name()));

  int notifs = 0;
  for (int i=0; (i < 20) && !L.Done(); ++i)
  {
    PQXX_CHECK_EQUAL(notifs, 0, "Got unexpected notifications.");

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

  PQXX_CHECK(L.Done(), "No notification received.");
  PQXX_CHECK_EQUAL(notifs, 1, "Got unexpected number of notifications.");
}
} // namespace

PQXX_REGISTER_TEST_T(test_087, nontransaction)
