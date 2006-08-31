#include <pqxx/compiler-internal.hxx>

#include <iostream>

#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Test result::swap()
//
// Usage: test077 [connect-string]
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
    nontransaction T(C, "test77");
    result RFalse = T.exec("SELECT 1=0"),
	   RTrue  = T.exec("SELECT 1=1");
    bool f, t;
    from_string(RFalse[0][0], f);
    from_string(RTrue[0][0],  t);
    if (f || !t)
      throw runtime_error("Booleans converted incorrectly");

    RFalse.swap(RTrue);
    from_string(RFalse[0][0], f);
    from_string(RTrue[0][0],  t);
    if (!f || t) throw runtime_error("result::swap() failed");
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

