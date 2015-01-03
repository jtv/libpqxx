#include <test_helpers.hxx>

using namespace std;
using namespace pqxx;

namespace
{
void test_stateless_cursor(transaction_base &trans)
{
  stateless_cursor<cursor_base::read_only, cursor_base::owned> empty(
	trans,
	pqxx::test::select_series(trans.conn(), 0, -1),
	"empty",
	false);

  result rows = empty.retrieve(0, 0);
  PQXX_CHECK_EQUAL(rows.empty(), true, "Empty result not empty");
  rows = empty.retrieve(0, 1);
  PQXX_CHECK_EQUAL(rows.size(), 0u, "Empty result returned rows");

  PQXX_CHECK_EQUAL(empty.size(), 0u, "Empty cursor not empty");

  PQXX_CHECK_THROWS(
    empty.retrieve(1, 0),
    out_of_range,
    "Empty cursor tries to retrieve");

  stateless_cursor<cursor_base::read_only, cursor_base::owned> stateless(
	trans,
	pqxx::test::select_series(trans.conn(), 0, 9),
	"stateless",
	false);

  PQXX_CHECK_EQUAL(stateless.size(), 10u, "stateless_cursor::size() mismatch");

  // Retrieve nothing.
  rows = stateless.retrieve(1, 1);
  PQXX_CHECK_EQUAL(rows.size(), 0u, "1-to-1 retrieval not empty");

  // Retrieve two rows.
  rows = stateless.retrieve(1, 3);
  PQXX_CHECK_EQUAL(rows.size(), 2u, "Retrieved wrong number of rows");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 1, "Data/position mismatch");
  PQXX_CHECK_EQUAL(rows[1][0].as<int>(), 2, "Data/position mismatch");

  // Retrieve same rows in reverse.
  rows = stateless.retrieve(2, 0);
  PQXX_CHECK_EQUAL(rows.size(), 2u, "Retrieved wrong number of rows");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 2, "Data/position mismatch");
  PQXX_CHECK_EQUAL(rows[1][0].as<int>(), 1, "Data/position mismatch");

  // Retrieve beyond end.
  rows = stateless.retrieve(9, 13);
  PQXX_CHECK_EQUAL(rows.size(), 1u, "Row count wrong at end");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 9, "Data/pos mismatch at end");

  // Retrieve beyond beginning.
  rows = stateless.retrieve(0, -4);
  PQXX_CHECK_EQUAL(rows.size(), 1u, "Row count wrong at beginning");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 0, "Data/pos mismatch at beginning");

  // Retrieve entire result set backwards.
  rows = stateless.retrieve(10, -15);
  PQXX_CHECK_EQUAL(rows.size(), 10u, "Reverse complete retrieval is broken");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 9, "Data mismatch");
  PQXX_CHECK_EQUAL(rows[9][0].as<int>(), 0, "Data mismatch");

  // Normal usage pattern: step through result set, 4 rows at a time.
  rows = stateless.retrieve(0, 4);
  PQXX_CHECK_EQUAL(rows.size(), 4u, "Wrong batch size");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 0, "Batch in wrong place");
  PQXX_CHECK_EQUAL(rows[3][0].as<int>(), 3, "Batch in wrong place");

  rows = stateless.retrieve(4, 8);
  PQXX_CHECK_EQUAL(rows.size(), 4u, "Wrong batch size");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 4, "Batch in wrong place");
  PQXX_CHECK_EQUAL(rows[3][0].as<int>(), 7, "Batch in wrong place");

  rows = stateless.retrieve(8, 12);
  PQXX_CHECK_EQUAL(rows.size(), 2u, "Wrong batch size");
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 8, "Batch in wrong place");
  PQXX_CHECK_EQUAL(rows[1][0].as<int>(), 9, "Batch in wrong place");

}
} // namespace

PQXX_REGISTER_TEST(test_stateless_cursor)
