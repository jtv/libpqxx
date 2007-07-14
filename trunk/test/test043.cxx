#include <algorithm>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <pqxx/connection>
#include <pqxx/cursor>
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

void ExpectPos(abscursor &C, cursor_base::difference_type Pos)
{
  if (C.pos() != Pos)
    throw logic_error("Expected to find cursor at " + to_string(Pos) + ", "
	              "got " + to_string(C.pos()));
}


void MoveTo(abscursor &C,
	cursor_base::difference_type N,
	cursor_base::difference_type NewPos)
{
  const result::difference_type OldPos = C.pos();
  cursor_base::difference_type rows = 0;
  cout << "Moving " << N << " row(s) from position " << OldPos << endl;
  C.move(N, rows);
  ExpectPos(C, NewPos);
  if (OldPos + rows != NewPos)
    throw logic_error("Inconsistent move: " + to_string(rows) + " rows "
	              "from " + to_string(OldPos) + " "
		      "got us to " + to_string(NewPos));
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
    result R( T.exec("SELECT count(*) FROM " + Table) );
    int Rows;
    R.at(0).at(0).to(Rows);

    if (Rows <= 10)
      throw runtime_error("Not enough rows in '" + Table + "' "
		          "for serious testing.  Sorry.");

    int GetRows = 4;
    abscursor Cur(&T, "SELECT * FROM " + Table, "tablecur");
    ExpectPos(Cur, 0);
    R = Cur.fetch(GetRows);
    ExpectPos(Cur, GetRows);

    if (R.size() != result::size_type(GetRows))
      throw logic_error("Expected " + to_string(GetRows) + " rows, "
		        "got " + to_string(R.size()));

    // Move cursor 1 step forward to make subsequent backwards fetch include
    // current row
    MoveTo(Cur, 1, GetRows+1);

    MoveTo(Cur, -Cur.pos(), 0);

    R = Cur.fetch(cursor_base::next());
    if (R.size() != 1)
      throw logic_error("NEXT: wanted 1 row, got " + to_string(R.size()));
    ExpectPos(Cur, 1);

    MoveTo(Cur, 3, 4);
    MoveTo(Cur, -2, 2);

    R = Cur.fetch(cursor_base::prior());
    if (R.size() != 1)
      throw logic_error("PRIOR: wanted 1 row, got " + to_string(R.size()));
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

