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


// "Adopted SQL Cursor" test program for libpqxx.  Create SQL cursor, wrap it in
// a cursor stream, then use it to fetch data and check for consistent results.
// Compare results against an icursor_iterator so that is tested as well.
//
// Usage: test084 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, of "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

namespace
{
void dump(const result &R)
{
  for (result::const_iterator r(R.begin()); r != R.end(); ++r)
  {
    for (result::tuple::const_iterator f(r->begin()); f != r->end(); ++f)
	cerr << '\t' << f;
    cerr << endl;
  }
}

void compare_results(const result &lhs, const result &rhs, string desc)
{
  if (lhs != rhs)
  {
    cerr << "Outputs at " << desc << ':' << endl;
    cerr << "lhs:" << endl;
    dump(lhs);
    cerr << "rhs:" << endl;
    dump(rhs);
    throw logic_error("Different results at " + desc);
  }
}
}	// namespace

int main(int, char *argv[])
{
  try
  {
    const string Table = "pg_tables", Key = "tablename";

    connection c(argv[1]);
    transaction<serializable> T(c, "test84");

    // Count rows.
    result R( T.exec("SELECT count(*) FROM " + Table) );

    if (R.at(0).at(0).as<long>() <= 20) 
      throw runtime_error("Not enough rows in '" + Table + "' "
		          "for serious testing.  Sorry.");

    // Create an SQL cursor and, for good measure, muddle up its state a bit.
    const string CurName = "MYCUR",
	  	 Query = "SELECT * FROM " + Table + " ORDER BY " + Key;
    const int InitialSkip = 2, GetRows = 3;

    T.exec("DECLARE \"" + CurName + "\" CURSOR FOR " + Query);
    T.exec("MOVE " + to_string(InitialSkip*GetRows) + " IN \""+CurName+'\"');


    // Wrap cursor in cursor stream.  Apply some trickery to get its name inside
    // a result field for this purpose.  This isn't easy because it's not 
    // supposed to be easy; normally we'd only construct streams around existing
    // SQL cursors if they were being returned by functions.
    icursorstream C(T, T.exec("SELECT '"+sqlesc(CurName)+"'")[0][0], GetRows);

    // Create parallel cursor to check results
    icursorstream C2(T, Query, "CHECKCUR", GetRows);
    icursor_iterator i2(C2);

    // Remember, our adopted cursor is at position (InitialSkip*GetRows)
    icursor_iterator i3(i2);
    i3 += InitialSkip;

    icursor_iterator iend, i4;
    if (i3 == iend)
      throw logic_error("Early end to icursor_iterator iteration!");
    i4 = iend;
    if (i4 != iend)
      throw logic_error("Assigning empty icursor_iterator fails");

    // Now start testing our new Cursor.
    C >> R;
    i2 = i3;
    result R2( *i2++ );

    if (R.size() > result::size_type(GetRows))
      throw logic_error("Expected " + to_string(GetRows) + " rows, "
		        "got " + to_string(R.size()));

    if (R.size() < result::size_type(GetRows))
      cerr << "Warning: asked for " << GetRows << " rows, "
	      "got only " << R.size() << endl;

    compare_results(R, R2, "[1]");

    C.get(R);
    R2 = *i2;
    compare_results(R, R2, "[2]");

    C.ignore(2);
    ++i2;
    ++i2;

    C.get(R);
    R2 = *i2;
    compare_results(R, R2, "[3]");

    for (int i=1; C && i2 != iend; C.get(R), R2 = *i2++, ++i)
      compare_results(R, R2, "iteration " + to_string(i));

    if (i2 != iend) throw logic_error("Adopted cursor terminated early");
    if (C) throw logic_error("icursor_iterator terminated early");
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

