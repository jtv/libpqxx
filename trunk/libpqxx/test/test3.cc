#include <cstdio>
#include <iostream>
#include <stdexcept>

#include "pqxx/connection.h"
#include "pqxx/cursor.h"
#include "pqxx/transaction.h"
#include "pqxx/result.h"

using namespace PGSTD;
using namespace Pg;


// Simple test program for libpqxx.  Read list of tables through a cursor,
// fetching <blocksize> rows at a time.  Default blocksize is 1; use 0 to
// read all rows at once.  Negative blocksizes read backwards.
//
// Usage: test3 [connect-string] [blocksize]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, of "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int argc, char *argv[])
{
  try
  {
    int BlockSize = 1;
    if ((argc > 2) && argv[2] && (sscanf(argv[2],"%d",&BlockSize) != 1))
      throw invalid_argument("Expected number for second argument");
    if (BlockSize == 0) BlockSize = Cursor::ALL();

    // Set up a connection to the backend
    Connection C(argv[1] ? argv[1] : "");

    // Enable all sorts of debug output
    C.Trace(stdout);

    // Begin a transaction acting on our current connection
    Transaction T(C, "test3");

    // Declare a cursor for the list of database tables
    Cursor Cur(T, "SELECT * FROM pg_tables", "tablecur", BlockSize);

    // If we want to read backwards, move to the last tuple first
    if (BlockSize < 0) Cur.Move(Cursor::ALL());

    Result R;
    while ((Cur >> R))
    {
      // Out of sheer curiosity, see if Cursor is consistent in the stream
      // status it reports with operator bool() and operator !() (1)
      if (!Cur) throw logic_error("Inconsistent cursor state!");

      // Received a block of rows.  Note that this may be less than the 
      // blocksize we requested if we're at the end of our query.
      cout << "* Got " << R.size() << " row(s) *" << endl;

      // Another sanity check on Cursor: must never return too many rows,
      // even though returning too few is permitted
      if (R.size() > abs(BlockSize))
        throw logic_error("Cursor returned " + ToString(R.size()) + " rows, "
			  "when " + ToString(abs(BlockSize)) + " "
			  "was all I asked for!");

      // Process each successive result tuple
      for (Result::const_iterator c = R.begin(); c != R.end(); ++c)
      {
        // Read value of column 0 into a string N
        string N;
        c[0].to(N);

        // Dump tuple number and column 0 value to cout
        cout << '\t' << ToString(c.num()) << '\t' << N << endl;
      }
    }

    // Out of sheer curiosity, see if Cursor is consistent in the stream
    // status it reports with operator bool() and operator !() (2)
    if (!!Cur) throw logic_error("Inconsistent cursor state!");


    // Stop generating debug output
    C.Untrace();

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

