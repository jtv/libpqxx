#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <pqxx/connection>
#include <pqxx/cursor>
#include <pqxx/transaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Read list of tables through a cursor, starting
// with a deferred connection.  Default blocksize is 1; use 0 to read all rows
// at once.  Negative blocksizes read backwards.
//
// Usage: test022 [connect-string] [blocksize]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, of "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int argc, char *argv[])
{
  try
  {
    const string Table = "pqxxevents";

    int BlockSize = 1;
    if ((argc > 2) && argv[2] && (sscanf(argv[2],"%d",&BlockSize) != 1))
      throw invalid_argument("Expected number for second argument");
    if (BlockSize == 0) BlockSize = cursor_base::all();

    lazyconnection C(argv[1]);

    // Enable all sorts of debug output.  C will remember this setting until it
    // gets to the point where it actually needs to connect to the database.
    C.trace(stdout);

    transaction<serializable> T(C, "test22");

    cursor Cur(&T, "SELECT * FROM " + Table, "tablecur");
    if (BlockSize < 0) Cur.move(cursor_base::all());

    C.trace(0);


    for (result R=Cur.fetch(BlockSize); Cur; R=Cur.fetch(BlockSize))
    {
      if (!Cur) throw logic_error("Inconsistent cursor state!");

      if (R.size() > unsigned(abs(BlockSize)))
        throw logic_error("Cursor returned " + to_string(R.size()) + " rows, "
			  "when " + to_string(abs(BlockSize)) + " "
			  "was all I asked for!");

      for (result::const_iterator c = R.begin(); c != R.end(); ++c)
      {
        string N;
        c[0].to(N);

        cout << '\t' << to_string(c.num()) << '\t' << N << endl;
      }
    }

    if (!!Cur) throw logic_error("Inconsistent cursor state!");

    T.commit();
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

