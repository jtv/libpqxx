#include <algorithm>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <pqxx/connection>
#include <pqxx/cursor.h>
#include <pqxx/transaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// Cursor test program for libpqxx.  Scan through a table using a cursor, and 
// verify that correct cursor positions are being reported.
//
// Usage: test043 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, of "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

namespace
{

void ExpectPos(Cursor &C, Cursor::size_type Pos)
{
  if (C.Pos() != Pos)
    throw logic_error("Expected to find cursor at " + ToString(Pos) + ", "
	              "got " + ToString(C.Pos()));
}


void MoveTo(Cursor &C, Cursor::size_type N, Cursor::size_type NewPos)
{
  const result::size_type OldPos = C.Pos();
  const result::size_type Dist = C.Move(N);
  if (OldPos + Dist != NewPos)
    throw logic_error("Inconsistent move: " + ToString(Dist) + " rows "
	              "from " + ToString(OldPos) + " "
		      "got us to " + ToString(NewPos));
  ExpectPos(C, NewPos);
}

}


int main(int, char *argv[])
{
  try
  {
    const string Table = "pqxxevents";

    connection C(argv[1]);
    transaction<serializable> T(C, "test19");

    // Count rows.
    result R( T.Exec("SELECT count(*) FROM " + Table) );
    int Rows;
    R.at(0).at(0).to(Rows);

    if (Rows <= 10) 
      throw runtime_error("Not enough rows in '" + Table + "' "
		          "for serious testing.  Sorry.");

    int GetRows = 4;
    Cursor Cur(T, ("SELECT * FROM " + Table).c_str(), "tablecur", GetRows);
    ExpectPos(Cur, 0);
    Cur >> R;
    ExpectPos(Cur, GetRows);

    if (R.size() != GetRows)
      throw logic_error("Expected " + ToString(GetRows) + " rows, "
		        "got " + ToString(R.size()));

    // Move cursor 1 step forward to make subsequent backwards fetch include
    // current row
    MoveTo(Cur, 1, GetRows+1);

    MoveTo(Cur, Cursor::BACKWARD_ALL(), 0);

    R = Cur.Fetch(Cursor::NEXT());
    if (R.size() != 1) 
      throw logic_error("NEXT: wanted 1 row, got " + ToString(R.size()));
    ExpectPos(Cur, 1);

    MoveTo(Cur, 3, 4);
    MoveTo(Cur, -2, 2);

    R = Cur.Fetch(Cursor::PRIOR());
    if (R.size() != 1)
      throw logic_error("PRIOR: wanted 1 row, got " + ToString(R.size()));
    ExpectPos(Cur, 1);

    MoveTo(Cur, 5, 6);
    MoveTo(Cur, -5, 1);

    // Try to move back beyond starting point
    MoveTo(Cur, -2, 0);

    MoveTo(Cur, 4, 4);
  }
  catch (const sql_error &e)
  {
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

