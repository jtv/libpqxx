#include <iostream>

#include "pg_connection.h"
#include "pg_transaction.h"
#include "pg_result.h"

using namespace PGSTD;
using namespace Pg;


// Example program for libpqxx.  Perform a query and enumerate its output
// using array indexing.
//
// Usage: test2 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

int main(int argc, char *argv[])
{
  try
  {
    Connection C(argv[1] ? argv[1] : "");
    Transaction T(C, "test2");

    Result R( T.Exec("SELECT * FROM pg_tables") );

    for (Result::size_type i = 0; i < R.size(); ++i)
      cout << '\t' << ToString(i) << '\t' << R[i][0].c_str() << endl;

    T.Commit();
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

