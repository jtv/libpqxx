#include <iostream>

#include "pqxx/connection.h"
#include "pqxx/transaction.h"
#include "pqxx/result.h"

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
    // Set up connection to database
    Connection C(argv[1] ? argv[1] : "");

    // Start transaction within context of connection
    Transaction T(C, "test2");

    // Perform query within transaction
    Result R( T.Exec("SELECT * FROM pg_tables") );

    // Let's keep the database waiting as briefly as possible: commit now,
    // before we start processing results.  We could do this later, or since
    // we're not making any changes in the database that need to be committed,
    // we could in this case even omit it altogether.
    T.Commit();

    // Since we don't need the database anymore, we can be even more 
    // considerate and close the connection now.  This is optional.
    C.Disconnect();

    // Now we've got all that settled, let's process our results.
    for (Result::size_type i = 0; i < R.size(); ++i)
      cout << '\t' << ToString(i) << '\t' << R[i][0].c_str() << endl;

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

