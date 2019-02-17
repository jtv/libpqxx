#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "test_helpers.hxx"

using namespace pqxx;


// "Adopted SQL Cursor" test program for libpqxx.  Create SQL cursor, wrap it in
// a cursor stream, then use it to fetch data and check for consistent results.
// Compare results against an icursor_iterator so that is tested as well.
namespace
{
void test_084()
{
  connection conn;
  transaction<serializable> tx{conn};

  const std::string Table = "pg_tables", Key = "tablename";

  // Count rows.
  result R( tx.exec("SELECT count(*) FROM " + Table) );

  PQXX_CHECK(
	R.at(0).at(0).as<long>() > 20,
	"Not enough rows in " + Table + ", cannot test.");

  // Create an SQL cursor and, for good measure, muddle up its state a bit.
  const std::string CurName = "MYCUR",
	Query = "SELECT * FROM " + Table + " ORDER BY " + Key;
  const int InitialSkip = 2, GetRows = 3;

  tx.exec0("DECLARE " + tx.quote_name(CurName) + " CURSOR FOR " + Query);
  tx.exec0(
	"MOVE " + to_string(InitialSkip*GetRows) + " "
	"IN " + tx.quote_name(CurName));

  // Wrap cursor in cursor stream.  Apply some trickery to get its name inside
  // a result field for this purpose.  This isn't easy because it's not
  // supposed to be easy; normally we'd only construct streams around existing
  // SQL cursors if they were being returned by functions.
  icursorstream C{tx, tx.exec("SELECT '"+tx.esc(CurName)+"'")[0][0], GetRows};

  // Create parallel cursor to check results
  icursorstream C2{tx, Query, "CHECKCUR", GetRows};
  icursor_iterator i2{C2};

  // Remember, our adopted cursor is at position (InitialSkip*GetRows)
  icursor_iterator i3(i2);

  PQXX_CHECK(
	(i3 == i2) and not (i3 != i2),
	"Equality on copy-constructed icursor_iterator is broken.");
  PQXX_CHECK(
	not (i3 > i2) and not (i3 < i2) and (i3 <= i2) and (i3 >= i2),
	"Comparison on identical icursor_iterators is broken.");

  i3 += InitialSkip;

  PQXX_CHECK(not (i3 <= i2), "icursor_iterator operator<=() is broken.");

  icursor_iterator iend, i4;
  PQXX_CHECK(i3 != iend, "Early end to icursor_iterator iteration.");
  i4 = iend;
  PQXX_CHECK(i4 == iend, "Assigning empty icursor_iterator fails.");

  // Now start testing our new Cursor.
  C >> R;
  i2 = i3;
  result R2( *i2++ );

  PQXX_CHECK_EQUAL(
	R.size(),
	result::size_type(GetRows),
	"Got unexpected number of rows.");

  PQXX_CHECK_EQUAL(R, R2, "Unexpected result at [1]");

  C.get(R);
  R2 = *i2;
  PQXX_CHECK_EQUAL(R, R2, "Unexpected result at [2]");
  i2 += 1;

  C.ignore(GetRows);
  C.get(R);
  R2 = *++i2;

  PQXX_CHECK_EQUAL(R, R2, "Unexpected result at [3]");

  ++i2;
  R2 = *i2++;
  for (int i=1; C.get(R) and i2 != iend; R2 = *i2++, ++i)
    PQXX_CHECK_EQUAL(
	R,
	R2,
	"Unexpected result in iteration at " + to_string(i));

  PQXX_CHECK(i2 == iend, "Adopted cursor terminated early.");
  PQXX_CHECK(not (C >> R), "icursor_iterator terminated early.");
}
} // namespace


PQXX_REGISTER_TEST(test_084);
