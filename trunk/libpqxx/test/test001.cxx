#include <cassert>
#include <iostream>

#include <pqxx/all.h>

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a transaction, and perform a query inside it.
//
// Usage: test001
int main()
{
  try
  {
    // Set up a connection to the backend.
    connection C;

    // Begin a transaction acting on our current connection.  Give it a human-
    // readable name so the library can include it in error messages.
    transaction<> T(C, "test1");

    // Perform a query on the database, storing result tuples in R.
    result R( T.Exec("SELECT * FROM pg_tables") );

    // Process each successive result tuple
    for (result::const_iterator c = R.begin(); c != R.end(); ++c)
    {
      // Read value of column 0 into a string N
      string N;
      c[0].to(N);

      // Dump tuple number and column 0 value to cout
      cout << '\t' << ToString(c.num()) << '\t' << N << endl;
    }

    // Tell the transaction that it has been successful.  This is not really
    // necessary here, since we didn't make any modifications to the database
    // so there are no changes to commit.
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


