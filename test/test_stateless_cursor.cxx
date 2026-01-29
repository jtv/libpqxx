#include <pqxx/cursor>
#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_stateless_cursor(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  pqxx::stateless_cursor<
    pqxx::cursor_base::read_only, pqxx::cursor_base::owned>
    empty(tx, "SELECT generate_series(0, -1)", "empty", false);

  auto rows{empty.retrieve(0, 0)};
  PQXX_CHECK_EQUAL(std::empty(rows), true);
  rows = empty.retrieve(0, 1);
  PQXX_CHECK_EQUAL(std::size(rows), 0);

  PQXX_CHECK_EQUAL(empty.size(), 0);

  PQXX_CHECK_THROWS(empty.retrieve(1, 0), pqxx::range_error);

  pqxx::stateless_cursor<
    pqxx::cursor_base::read_only, pqxx::cursor_base::owned>
    stateless(tx, "SELECT generate_series(0, 9)", "stateless", false);

  PQXX_CHECK_EQUAL(stateless.size(), 10);

  // Retrieve nothing.
  rows = stateless.retrieve(1, 1);
  PQXX_CHECK_EQUAL(std::size(rows), 0);

  // Retrieve two rows.
  rows = stateless.retrieve(1, 3);
  PQXX_CHECK_EQUAL(std::size(rows), 2);
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 1);
  PQXX_CHECK_EQUAL(rows[1][0].as<int>(), 2);

  // Retrieve same rows in reverse.
  rows = stateless.retrieve(2, 0);
  PQXX_CHECK_EQUAL(std::size(rows), 2);
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 2);
  PQXX_CHECK_EQUAL(rows[1][0].as<int>(), 1);

  // Retrieve beyond end.
  rows = stateless.retrieve(9, 13);
  PQXX_CHECK_EQUAL(std::size(rows), 1);
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 9);

  // Retrieve beyond beginning.
  rows = stateless.retrieve(0, -4);
  PQXX_CHECK_EQUAL(std::size(rows), 1);
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 0);

  // Retrieve entire result set backwards.
  rows = stateless.retrieve(10, -15);
  PQXX_CHECK_EQUAL(std::size(rows), 10);
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 9);
  PQXX_CHECK_EQUAL(rows[9][0].as<int>(), 0);

  // Normal usage pattern: step through result set, 4 rows at a time.
  rows = stateless.retrieve(0, 4);
  PQXX_CHECK_EQUAL(std::size(rows), 4);
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 0);
  PQXX_CHECK_EQUAL(rows[3][0].as<int>(), 3);

  rows = stateless.retrieve(4, 8);
  PQXX_CHECK_EQUAL(std::size(rows), 4);
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 4);
  PQXX_CHECK_EQUAL(rows[3][0].as<int>(), 7);

  rows = stateless.retrieve(8, 12);
  PQXX_CHECK_EQUAL(std::size(rows), 2);
  PQXX_CHECK_EQUAL(rows[0][0].as<int>(), 8);
  PQXX_CHECK_EQUAL(rows[1][0].as<int>(), 9);
}


PQXX_REGISTER_TEST(test_stateless_cursor);
} // namespace
