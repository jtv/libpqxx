#include <iostream>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// Example program for libpqxx.  Perform a query and enumerate its output
// using array indexing.
//
// Usage: test002 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

int main(int, char *argv[])
{
  try
  {
    // Set up connection to database
    string ConnectString = (argv[1] ? argv[1] : "");
    connection C(ConnectString);

    // Start transaction within context of connection
    transaction<> T(C, "test2");

    // Perform query within transaction
    result R( T.Exec("SELECT * FROM pg_tables") );

    // Let's keep the database waiting as briefly as possible: commit now,
    // before we start processing results.  We could do this later, or since
    // we're not making any changes in the database that need to be committed,
    // we could in this case even omit it altogether.
    T.Commit();

    // Since we don't need the database anymore, we can be even more 
    // considerate and close the connection now.  This is optional.
    C.Disconnect();

    // Now we've got all that settled, let's process our results.
    for (result::size_type i = 0; i < R.size(); ++i)
      cout << '\t' << ToString(i) << '\t' << R[i][0].c_str() << endl;

  }
  catch (const sql_error &e)
  {
    // If we're interested in the text of a failed query, we can write separate
    // exception handling code for this type of exception
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

