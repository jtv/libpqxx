#include <iostream>

#include <pqxx/pqxx>
#include <pqxx/cachedresult.h>

using namespace PGSTD;
using namespace pqxx;

// Test program for libpqxx.  Test cachedresult's empty() and clear() methods.
//
// Usage: test047 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int, char *argv[])
{
  try
  {
    connection C(argv[1]);
    transaction<serializable> T(C, "test47");

    const char Full[] = "SELECT count(*) FROM pg_tables",
               Empty[] = "SELECT * from pg_tables WHERE 1 = 0";

    // Ask for size() first, then check empty()
    cachedresult CR1(T, Full, "CR1");
    if (CR1.size() != 1) 
      throw logic_error("cachedresult had size " + ToString(CR1.size()) + ", "
	                "expected 1");
    if (CR1.empty()) throw logic_error("cachedresult was empty!");

    // Try empty() method without asking for size()
    cachedresult CR2(T, Full, "CR2");
    if (CR2.empty()) throw logic_error("Unexpected empty cachedresult");

    // Now try the same with an empty result
    cachedresult CR3(T, Empty, "CR3");
    if (!CR3.empty()) throw logic_error("cachedresult not empty as expected");

    cachedresult CR4(T, Empty, "CR4");
    if (CR4.size()) 
      throw logic_error("cachedresult had size " + ToString(CR4.size()) + ", "
	                "expected 0");
    if (!CR4.empty()) throw logic_error("cachedresult was not empty!");
  }
  catch (const sql_error &e)
  {
    cerr << "SQL error: " << e.what() << endl
         << "Query was: '" << e.query() << "'" << endl;
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


