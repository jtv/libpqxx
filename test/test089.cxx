#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx.  Attempt to perform nested queries on various types
// of connections.
namespace
{
void do_test(connection_base &C, const string &desc)
{
  cout << "Testing " << desc << ":" << endl;

  // Trivial test: create subtransactions, and commit/abort
  work T0(C, "T0");
  cout << T0.exec("SELECT 'T0 starts'")[0][0].c_str() << endl;
  subtransaction T0a(T0, "T0a");
  T0a.commit();
  subtransaction T0b(T0, "T0b");
  T0b.abort();
  cout << T0.exec("SELECT 'T0 ends'")[0][0].c_str() << endl;
  T0.commit();

  // Basic functionality: perform query in subtransaction; abort, continue
  work T1(C, "T1");
  cout << T1.exec("SELECT 'T1 starts'")[0][0].c_str() << endl;
  subtransaction T1a(T1, "T1a");
    cout << T1a.exec("SELECT '  a'")[0][0].c_str() << endl;
    T1a.commit();
  subtransaction T1b(T1, "T1b");
    cout << T1b.exec("SELECT '  b'")[0][0].c_str() << endl;
    T1b.abort();
  subtransaction T1c(T1, "T1c");
    cout << T1c.exec("SELECT '  c'")[0][0].c_str() << endl;
    T1c.commit();
  cout << T1.exec("SELECT 'T1 ends'")[0][0].c_str() << endl;
  T1.commit();
}


void test_089(transaction_base &)
{
  asyncconnection A1;
  do_test(A1, "asyncconnection (virgin)");

  asyncconnection A2;
  A2.activate();

  do_test(A2, "asyncconnection (initialized)");

  lazyconnection L1;
  do_test(L1, "lazyconnection (virgin)");

  lazyconnection L2;
  L2.activate();
  do_test(L2, "lazyconnection (initialized)");

  connection C;
  C.activate();
  C.deactivate();
  do_test(C, "connection (deactivated)");
}
} // namespace

PQXX_REGISTER_TEST_NODB(test_089)
