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


// Cursor test program for libpqxx.  Read table through a cursor, and verify
// that correct move counts are being reported.
//
// Usage: test042 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, of "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

namespace
{

template<typename CURSOR> inline void ExpectMove(CURSOR &C,
    cursor_base::difference_type N,
    cursor_base::difference_type Expect)
{
  cout << "move " << N << endl;
  const cursor_base::difference_type Dist = C.move(N);
  if (Dist != Expect)
    throw logic_error("Expected to move " + to_string(Expect) + " rows, "
	              "found " + to_string(Dist));
}

template<typename CURSOR> inline
  void ExpectMove(CURSOR &C, cursor_base::difference_type N)
{
  ExpectMove(C, N, N);
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
    cursor Cur(&T, "SELECT * FROM " + Table, "tablecur");
    R = Cur.fetch(GetRows);

    if (R.size() != result::size_type(GetRows))
      throw logic_error("Expected " + to_string(GetRows) + " rows, "
		        "got " + to_string(R.size()));

    // Move cursor 1 step forward to make subsequent backwards fetch include
    // current row
    ExpectMove(Cur, 1);

    ExpectMove(Cur, cursor_base::backward_all(), -5);

    R = Cur.fetch(cursor_base::next());
    if (R.size() != 1) 
      throw logic_error("NEXT: wanted 1 row, got " + to_string(R.size()));

    ExpectMove(Cur, 3);
    ExpectMove(Cur, -2);

    R = Cur.fetch(cursor_base::prior());
    if (R.size() != 1)
      throw logic_error("PRIOR: wanted 1 row, got " + to_string(R.size()));

    ExpectMove(Cur, 5);
    ExpectMove(Cur, -5);

    // We're at pos 1 now.  Now see that "lower edge" is respected.
    ExpectMove(Cur, -2, -1);
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

