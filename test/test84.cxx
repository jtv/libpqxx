#include <string>

#include <pqxx/cursor>
#include <pqxx/transaction>

#include "helpers.hxx"


// "Adopted SQL Cursor" test program for libpqxx.  Create SQL cursor, wrap it
// in a cursor stream, then use it to fetch data and check for consistent
// results. Compare results against an icursor_iterator so that is tested as
// well.
namespace
{
void test_084(pqxx::test::context &tctx)
{
  pqxx::connection cx;
  pqxx::transaction<pqxx::serializable> tx{cx};

  int const table_size{20};

  // Create an SQL cursor and, for good measure, muddle up its state a bit.
  std::string const cur_name{tctx.make_name("pqxx-cur")},
    query{std::format("SELECT * FROM generate_series(1, {})", table_size)};
  constexpr int initial_skip{2}, get_rows{3};

  tx.exec("DECLARE " + tx.quote_name(cur_name) + " CURSOR FOR " + query)
    .no_rows();
  tx.exec(
      "MOVE " + pqxx::to_string(initial_skip * get_rows) +
      " "
      "IN " +
      tx.quote_name(cur_name))
    .no_rows();

  // Wrap cursor in cursor stream.  Apply some trickery to get its name inside
  // a result field for this purpose.  This isn't easy because it's not
  // supposed to be easy; normally we'd only construct streams around existing
  // SQL cursors if they were being returned by functions.
  pqxx::icursorstream C{
    tx, tx.exec("SELECT $1", pqxx::params{cur_name}).one_field(), get_rows};

  // Create parallel cursor to check results
  pqxx::icursorstream C2{tx, query, "CHECKCUR", get_rows};
  pqxx::icursor_iterator i2{C2};

  // Remember, our adopted cursor is at position (initial_skip*get_rows)
  pqxx::icursor_iterator i3(i2);

  PQXX_CHECK((i3 == i2) and not(i3 != i2));
  PQXX_CHECK(not(i3 > i2) and not(i3 < i2) and (i3 <= i2) and (i3 >= i2));

  i3 += initial_skip;

  PQXX_CHECK(not(i3 <= i2));

  pqxx::icursor_iterator i4;
  pqxx::icursor_iterator const iend;
  PQXX_CHECK(i3 != iend);
  i4 = iend;
  PQXX_CHECK(i4 == iend);

  // Now start testing our new Cursor.
  pqxx::result res;
  C >> res;
  i2 = i3;
  pqxx::result res2(*i2++);

  PQXX_CHECK_EQUAL(
    std::size(res), static_cast<pqxx::result::size_type>(get_rows));

  PQXX_CHECK_EQUAL(pqxx::to_string(res), pqxx::to_string(res2));

  C.get(res);
  res2 = *i2;
  PQXX_CHECK_EQUAL(pqxx::to_string(res), pqxx::to_string(res2));
  i2 += 1;

  C.ignore(get_rows);
  C.get(res);
  res2 = *++i2;

  PQXX_CHECK_EQUAL(pqxx::to_string(res), pqxx::to_string(res2));

  ++i2;
  res2 = *i2++;
  for (int i{1}; C.get(res) and i2 != iend; res2 = *i2++, ++i)
    PQXX_CHECK_EQUAL(
      pqxx::to_string(res), pqxx::to_string(res2),
      "Unexpected result in iteration at " + pqxx::to_string(i));

  PQXX_CHECK(i2 == iend);
  PQXX_CHECK(not(C >> res));
}
} // namespace


PQXX_REGISTER_TEST(test_084);
