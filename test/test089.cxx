#include <cassert>
#include <iostream>

#include <pqxx/connection>
#include <pqxx/subtransaction>
#include <pqxx/transaction>

using namespace PGSTD;
using namespace pqxx;

namespace
{
void test(connection_base &C, const string &desc)
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

bool test_and_catch(connection_base &C, const string &desc)
{
  bool ok = false;
  try
  {
    test(C,desc);
    ok = true;
  }
  catch (const broken_connection &)
  {
    throw;
  }
  catch (const logic_error &)
  {
    throw;
  }
  catch (const exception &)
  {
    if (C.supports(connection_base::cap_nested_transactions))
      throw;
    cout << "Backend does not support nested transactions." << endl;
  }

  return ok;
}

} // namespace

// Test program for libpqxx.  Attempt to perform nested queries on various types
// of connections.
//
// Usage: test089
int main()
{
  try
  {
    asyncconnection A1;
    const bool ok = test_and_catch(A1, "asyncconnection (virgin)");

    asyncconnection A2;
    A2.activate();
    if (!A2.supports(connection_base::cap_nested_transactions))
    {
      if (ok)
	throw logic_error("Initialized asyncconnection doesn't support "
	    "nested transactions, but a virgin one does!");
      cout << "Backend does not support nested transactions.  Skipping test."
	   << endl;
      return 0;
    }

    if (!ok)
      throw logic_error("Virgin asyncconnection supports nested transactions, "
	  "but initialized one doesn't!");

    test(A2, "asyncconnection (initialized)");

    lazyconnection L1;
    test(L1, "lazyconnection (virgin)");

    lazyconnection L2;
    L2.activate();
    test(L2, "lazyconnection (initialized)");

    connection C;
    C.activate();
    C.deactivate();
    test(C, "connection (deactivated)");
  }
  catch (const sql_error &e)
  {
    cerr << "SQL error: " << e.what() << endl
         << "Query was: " << e.query() << endl;
    return 1;
  }
  catch (const exception &e)
  {
    cerr << "Exception: " << e.what() << endl;
    return 2;
  }
  catch (...)
  {
    cerr << "Unhandled exception" << endl;
    return 100;
  }

  return 0;
}


