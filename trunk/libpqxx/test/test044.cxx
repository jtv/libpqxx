#include <cassert>
#include <iostream>

#include <pqxx/cachedresult.h>
#include <pqxx/connection.h>
#include <pqxx/transaction.h>
#include <pqxx/result.h>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Test Cursor with empty result set.
//
// Usage: test044 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int, char *argv[])
{
  try
  {
    connection C(argv[1]);
    transaction<serializable> T(C, "test44");

    // A query that will not return any data
    const char Query[] = "SELECT * FROM pqxxevents WHERE year <> year";

    Cursor Cur(T, Query, "test44");
    if ((Cur.size() != Cursor::pos_unknown) && 
	(Cur.size() != Cursor::pos_start))
      throw logic_error("Cursor reported size " + ToString(Cur.size()) + ", "
	                "expected " + ToString(int(Cursor::pos_start)) + " "
			"or unknown");

    Cursor::size_type Dist = Cur.Move(2);
    if ((Dist != 0) && (Dist != 1))
      throw logic_error("Move in empty Cursor returned " + ToString(Dist));

    Cur.MoveTo(0);
    Cur.MoveTo(1);
    Cur.MoveTo(2);

    if ((Cur.Pos() != 0) && (Cur.Pos() != 1))
      throw logic_error("Cursor at row " + ToString(Cur.Pos()) + " "
	                "in empty result set");

    if (Cur.size() != 0)
      throw logic_error("Cursor reported size " + ToString(Cur.size()) + ", "
	                "expected 0");
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


