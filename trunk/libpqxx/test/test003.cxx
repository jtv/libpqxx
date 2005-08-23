#include <cstdio>
#include <iostream>
#include <stdexcept>

#include <pqxx/connection>
#include <pqxx/cursor>
#include <pqxx/transaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Read list of tables through a cursor,
// fetching <blocksize> rows at a time.  Default blocksize is 1; use 0 to
// read all rows at once.  Negative blocksizes read backwards.
//
// Usage: test003 [connect-string] [blocksize]
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
    if (BlockSize == 0) BlockSize = cursor_base::all();

    // Set up a connection to the backend
    connection C(argv[1] ? argv[1] : "");

    // Enable all sorts of debug output
    C.trace(stdout);

    // Begin a transaction acting on our current connection
    transaction<serializable> T(C, "test3");

    // Declare a cursor for the list of database tables
    cursor Cur(&T, "SELECT * FROM pg_tables", "tablecur");
    cout << "Created cursor " << Cur.name() << endl;

    // If we want to read backwards, move to the last tuple first
    if (BlockSize < 0) Cur.move(cursor_base::all());

    // Stop generating debug output
    C.trace(0);


    for (result R(Cur.fetch(BlockSize)); !R.empty(); R=Cur.fetch(BlockSize))
    {
      // Out of sheer curiosity, see if Cursor is consistent in the stream
      // status it reports with operator bool() and operator !() (1)
      if (!Cur) throw logic_error("Inconsistent cursor state!");

      // Received a block of rows.  Note that this may be less than the 
      // blocksize we requested if we're at the end of our query.
      cout << "* Got " << R.size() << " row(s) *" << endl;

      // Another sanity check on Cursor: must never return too many rows,
      // even though returning too few is permitted
      if (R.size() > unsigned(abs(BlockSize)))
        throw logic_error("Cursor returned " + to_string(R.size()) + " rows, "
			  "when " + to_string(abs(BlockSize)) + " "
			  "was all I asked for!");

      // Process each successive result tuple
      for (result::const_iterator c = R.begin(); c != R.end(); ++c)
      {
        // Read value of column 0 into a string N
        string N;
        c[0].to(N);

        // Dump tuple number and column 0 value to cout
        cout << '\t' << to_string(c.num()) << '\t' << N << endl;
      }
    }

    // Out of sheer curiosity, see if Cursor is consistent in the stream
    // status it reports with operator bool() and operator !() (2)
    if (!!Cur) throw logic_error("Inconsistent cursor state!");

    // Tell the transaction that it has been successful
    T.commit();
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

