#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

#include <pqxx/connection>
#include <pqxx/tablewriter>
#include <pqxx/transaction>

#include "test_helpers.hxx"

#include <pqxx/config-internal-libpq.h>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Test querying of result column origins.
namespace
{
void test_093(connection_base &C, transaction_base &T)
{
  result R, X;

  {
    T.exec("CREATE TEMP TABLE pqxxfoo (x varchar, y integer, z integer)");
    T.exec("INSERT INTO pqxxfoo VALUES ('xx', 1, 2)");
    R = T.exec("SELECT z,y,x FROM pqxxfoo");
    X = T.exec("SELECT x,y,z,99 FROM pqxxfoo");

    if (!C.supports(connection_base::cap_table_column))
    {
      cout << "No support for querying table columns.  Skipping." << endl;
      return;
    }

    // T and C are closed here; result objects remain
  }

#ifdef PQXX_HAVE_PQFTABLECOL
  int x = R.table_column(2),
      y = R.table_column(1),
      z = R.table_column(int(0));

  PQXX_CHECK_EQUAL(x, 0, "Wrong column number.");
  PQXX_CHECK_EQUAL(y, 1, "Wrong column number.");
  PQXX_CHECK_EQUAL(z, 2, "Wrong column number.");

  x = R.table_column("x");
  y = R.table_column("y");
  z = R.table_column("z");

  PQXX_CHECK_EQUAL(x, 0, "Wrong number for named column.");
  PQXX_CHECK_EQUAL(y, 1, "Wrong number for named column.");
  PQXX_CHECK_EQUAL(z, 2, "Wrong number for named column.");

  int xx = X[0].table_column(int(0)),
      yx = X[0].table_column(result::tuple::size_type(1)),
      zx = X[0].table_column("z");

  PQXX_CHECK_EQUAL(xx, 0, "Bad result from table_column(int).");
  PQXX_CHECK_EQUAL(yx, 1, "Bad result from table_column(size_type).");
  PQXX_CHECK_EQUAL(zx, 2, "Bad result from table_column(string).");

  for (result::tuple::size_type i=0; i<R[0].size(); ++i)
    PQXX_CHECK_EQUAL(
	R[0][i].table_column(),
	R.table_column(i),
	"Bad result from column_table().");

  PQXX_CHECK_THROWS(
	R.table_column(3),
	exception,
	"table_column() with invalid index didn't fail.");

  PQXX_CHECK_THROWS(
	R.table_column("nonexistent"),
	exception,
	"table_column() with invalid column name didn't fail.");

  PQXX_CHECK_THROWS(
	X.table_column(3),
	exception,
	"table_column() on non-table didn't fail.");
#endif	// PQXX_HAVE_PQFTABLECOL
}
} // namespace

PQXX_REGISTER_TEST(test_093)
