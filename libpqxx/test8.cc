#include <iostream>
#include <stdexcept>
#include <vector>

#include "pg_connection.h"
#include "pg_tablereader.h"
#include "pg_transaction.h"

using namespace PGSTD;
using namespace Pg;


// Simple test program for libpqxx.  Read a table using a TableReader, which 
// may be faster than a conventional query.  A TableReader is really a frontend
// for a COPY command.
//
// Usage: test8 [connect-string] [table]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a backend 
// running on host foo.bar.net, logging in as user smith.
//
// The default table name is "events" as used by other test programs.
// PostgreSQL currently implements pg_tables as a view, which cannot be read by
// using the COPY command.  Otherwise, pg_tables would have made a better 
// default value here.
int main(int argc, char *argv[])
{
  try
  {
    // Set up a connection to the backend
    Connection C(argv[1] ? argv[1] : "");

    string Table = "events";
    if (argc > 2) Table = argv[2];

    // Begin a transaction acting on our current connection
    Transaction T(C, "test8");

    // Set up a TableReader stream to read data from table pg_tables
    TableReader Stream(T, Table);

    // Read results into string vectors and print them
    vector<string> R;
    for (int n=0; (Stream >> R); ++n)
    {
      cout << n << ": ";
      for (vector<string>::const_iterator i = R.begin(); i != R.end(); ++i)
        cout << *i << '\t';
      cout << endl;
      R.clear();
    }
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

