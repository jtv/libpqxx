#include <iostream>

#include "pg_connection.h"
#include "pg_transaction.h"
#include "pg_result.h"

using namespace PGSTD;
using namespace Pg;


// Simple test program for libpqxx.  Open connection to database, start
// a transaction, and perform a query inside it.
//
// Usage: test1 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int argc, char *argv[])
{
  try
  {
    // Set up a connection to the backend
    Connection C(argv[1] ? argv[1] : "");

    // Begin a transaction acting on our current connection
    Transaction T(C, "test1");

    // Perform a query on the database, storing result tuples in R
    Result R( T.Exec("SELECT * FROM pg_tables") );

    // Process each successive result tuple
    for (Result::const_iterator c = R.begin(); c != R.end(); ++c)
    {
      // Read value of column 0 into a string N
      string N;
      c[0].to(N);

      // Dump tuple number and column 0 value to cout
      cout << '\t' << ToString(c.num()) << '\t' << N << endl;
    }

    // Tell the transaction that it has been successful
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

