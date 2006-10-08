#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

#include <pqxx/connection>
#include <pqxx/tablewriter>
#include <pqxx/transaction>

#include <pqxx/config-internal-libpq.h>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Test querying of result column origins.
//
// Usage: test093 [connect-string]
//
// Where the connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a backend
// running on host foo.bar.net, logging in as user smith.

int main(int, char *argv[])
{
  try
  {
    const char *ConnStr = argv[1];

    result R, X;

    {
      connection C(ConnStr);
      work T(C, "test9");
      T.exec("CREATE TEMP TABLE pqxxfoo (x varchar, y integer, z integer)");
      T.exec("INSERT INTO pqxxfoo VALUES ('xx', 1, 2)");
      R = T.exec("SELECT z,y,x FROM pqxxfoo");
      X = T.exec("SELECT x,y,z,99 FROM pqxxfoo");
      // T and C are closed here; result objects remain
    }

#ifdef PQXX_HAVE_PQFTABLECOL
    int x = R.table_column(2),
	y = R.table_column(1),
	z = R.table_column(int(0));

    if (x != 0 || y != 1 || z != 2)
      throw logic_error("Table column numbers are wrong: "
        "(2,1,0), mapped to "
	"(" + to_string(x) + "," + to_string(y) + "," + to_string(z) + ")");

    x = R.table_column("x");
    y = R.table_column("y");
    z = R.table_column("z");

    if (x != 0 || y != 1 || z != 2)
      throw logic_error("Named table column numbers are wrong: "
        "(x,y,z) should map to (0,1,2) but became "
	"(" + to_string(x) + "," + to_string(y) + "," + to_string(z) + ")");

    int xx = X[0].table_column(int(0)),
      yx = X[0].table_column(result::tuple::size_type(1)),
      zx = X[0].table_column("z");

    if (xx != 0)
      throw logic_error("Column table_column(int) returned " + 
	to_string(xx) + " instead of 0");

    if (yx != 1)
      throw logic_error("Column table_column(size_type) returned " +
        to_string(yx) + " instead of 1");

    if (zx != 2)
      throw logic_error("Column table_column(const string &) returned " +
        to_string(zx) + " instead of 2");

    for (result::tuple::size_type i=0; i<R[0].size(); ++i)
      if (R[0][i].table_column() != R.table_column(i))
        throw logic_error("Field column_table(" + to_string(i) + ") returned " +
		to_string(R[0][i].table_column()) + " "
		"instead of " + to_string(R.table_column(i)));

    bool failed = false;
    int f = 0;
    try
    {
      f = R.table_column(3);
    }
    catch (const exception &e)
    {
      cout << "(Expected) " << e.what() << endl;
      failed = true;
    }
    if (!failed)
      throw logic_error("table_column() with invalid index 3 "
	"returned " + to_string(f) + " instead of failing");

    try
    {
      f = R.table_column("nonexistent");
    }
    catch (const exception &e)
    {
      cout << "(Expected) " << e.what() << endl;
      failed = true;
    }
    if (!failed)
      throw logic_error("table_column() with invalid column name "
	"returned " + to_string(f) + " instead of failing");

    try
    {
      f = X.table_column(3);
    }
    catch (const exception &e)
    {
      cout << "(Expected) " << e.what() << endl;
      failed = true;
    }
    if (!failed)
      throw logic_error("table_column() on non-table column returned " +
	to_string(f) + " instead of failing");
#endif	// PQXX_HAVE_PQFTABLECOL
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

