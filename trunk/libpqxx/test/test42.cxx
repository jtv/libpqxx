#include <algorithm>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <pqxx/connection.h>
#include <pqxx/cursor.h>
#include <pqxx/transaction.h>
#include <pqxx/result.h>

using namespace PGSTD;
using namespace pqxx;


// Cursor test program for libpqxx.  Read table through a cursor, and verify
// that correct move counts are being reported.
//
// Usage: test42 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, of "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

namespace
{

void ExpectMove(Cursor &C, Cursor::size_type N, Cursor::size_type Expect)
{
  const Cursor::size_type Dist = C.Move(N);
  if (Dist != Expect)
    throw logic_error("Expected to move " + ToString(Expect) + " rows, "
	              "found " + ToString(Dist));
}

void ExpectMove(Cursor &C, Cursor::size_type N)
{
  ExpectMove(C, N, N);
}

}


int main(int, char *argv[])
{
  try
  {
    const string Table = "events";

    Connection C(argv[1]);
    Transaction T(C, "test19");

    // Count rows.
    Result R( T.Exec(("SELECT count(*) FROM " + Table).c_str()) );
    int Rows;
    R.at(0).at(0).to(Rows);

    if (Rows <= 10) 
      throw runtime_error("Not enough rows in '" + Table + "' "
		          "for serious testing.  Sorry.");

    int GetRows = 4;
    Cursor Cur(T, ("SELECT * FROM " + Table).c_str(), "tablecur", GetRows);
    Cur >> R;

    if (R.size() != GetRows)
      throw logic_error("Expected " + ToString(GetRows) + " rows, "
		        "got " + ToString(R.size()));

    // Move cursor 1 step forward to make subsequent backwards fetch include
    // current row
    ExpectMove(Cur, 1);

    ExpectMove(Cur, Cursor::BACKWARD_ALL(), -5);

    R = Cur.Fetch(Cursor::NEXT());
    if (R.size() != 1) 
      throw logic_error("NEXT: wanted 1 row, got " + ToString(R.size()));

    ExpectMove(Cur, 3);
    ExpectMove(Cur, -2);

    R = Cur.Fetch(Cursor::PRIOR());
    if (R.size() != 1)
      throw logic_error("PRIOR: wanted 1 row, got " + ToString(R.size()));

    ExpectMove(Cur, 5);
    ExpectMove(Cur, -5);

    // We're at pos 1 now.  Now see that "lower edge" is respected.
    ExpectMove(Cur, -2, -1);
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

