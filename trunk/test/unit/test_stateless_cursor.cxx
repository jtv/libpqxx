#include <test_helpers.hxx>

using namespace PGSTD;
using namespace pqxx;

namespace
{
void test_stateless_cursor(connection_base &, transaction_base &trans)
{
  stateless_cursor<cursor_base::read_only, cursor_base::owned> empty(
	trans,
	pqxx::test::select_series(trans.conn(), 0, -1),
	"empty",
	false);

  result rows = empty.retrieve(0, 0);
  PQXX_CHECK_EQUAL(rows.empty(), true, "Empty result not empty");
  rows = empty.retrieve(0, 1);
  PQXX_CHECK_EQUAL(rows.size(), 0, "Empty result returned rows");

  PQXX_CHECK_EQUAL(empty.size(), 0, "Empty cursor not empty");

  PQXX_CHECK_THROWS(
    empty.retrieve(1, 0),
    out_of_range,
    "Empty cursor tries to retrieve");

  stateless_cursor<cursor_base::read_only, cursor_base::owned> stateless(
	trans,
	pqxx::test::select_series(trans.conn(), 0, 9),
	"stateless",
	false);

  PQXX_CHECK_EQUAL(stateless.size(), 10, "stateless_cursor::size() mismatch");

  // Retrieve nothing.
  rows = stateless.retrieve(1, 1);
  PQXX_CHECK_EQUAL(rows.size(), 0, "1-to-1 retrieval not empty");

  // Retrieve two rows.
  rows = stateless.retrieve(1, 3);
  PQXX_CHECK_EQUAL(rows.size(), 2, "Retrieved wrong number of rows");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 1, "Data/position mismatch");
  PQXX_CHECK_EQUAL(rows[1][0].as<int>(), 2, "Data/position mismatch");

  // Retrieve same rows in reverse.
  rows = stateless.retrieve(2, 0);
  PQXX_CHECK_EQUAL(rows.size(), 2, "Retrieved wrong number of rows");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 2, "Data/position mismatch");
  PQXX_CHECK_EQUAL(rows[1][0].as<int>(), 1, "Data/position mismatch");

  // Retrieve beyond end.
  rows = stateless.retrieve(9, 13);
  PQXX_CHECK_EQUAL(rows.size(), 1, "Row count wrong at end");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 9, "Data/pos mismatch at end");

  // Retrieve beyond beginning.
  rows = stateless.retrieve(0, -4);
  PQXX_CHECK_EQUAL(rows.size(), 1, "Row count wrong at beginning");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 0, "Data/pos mismatch at beginning");

  rows = stateless.retrieve(10, -15);
  PQXX_CHECK_EQUAL(rows.size(), 10, "Reverse complete retrieval is broken");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 9, "Data mismatch");
  PQXX_CHECK_EQUAL(rows[9][0].as<int>(), 0, "Data mismatch");
}
} // namespace

int main()
{
  test::TestCase<> test("stateless_cursor", test_stateless_cursor);
  return test::pqxxtest(test);
}

