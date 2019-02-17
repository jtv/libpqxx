#include "../test_helpers.hxx"

using namespace pqxx;


namespace
{
void test_stateless_cursor_provides_random_access(connection_base &conn)
{
  work tx{conn};
  stateless_cursor<cursor_base::read_only, cursor_base::owned> c{
	tx,
	"SELECT * FROM generate_series(0, 3)",
	"count",
	false};

  auto r = c.retrieve(1, 2);
  PQXX_CHECK_EQUAL(r.size(), 1u, "Wrong number of rows from retrieve().");
  PQXX_CHECK_EQUAL(r[0][0].as<int>(), 1, "Cursor retrieved wrong data.");

  r = c.retrieve(3, 10);
  PQXX_CHECK_EQUAL(r.size(), 1u, "Expected 1 row retrieving past end.");
  PQXX_CHECK_EQUAL(r[0][0].as<int>(), 3, "Wrong data retrieved at end.");

  r = c.retrieve(0, 1);
  PQXX_CHECK_EQUAL(r.size(), 1u, "Wrong number of rows back at beginning.");
  PQXX_CHECK_EQUAL(r[0][0].as<int>(), 0, "Wrong data back at beginning.");
}


void test_stateless_cursor_ignores_trailing_semicolon(connection_base &conn)
{
  work tx{conn};
  stateless_cursor<cursor_base::read_only, cursor_base::owned> c{
	tx,
	"SELECT * FROM generate_series(0, 3)  ;; ; \n \t  ",
	"count",
	false};

  auto r = c.retrieve(1, 2);
  PQXX_CHECK_EQUAL(r.size(), 1u, "Trailing semicolon confused retrieve().");

}


void test_cursor()
{
  connection conn;
  test_stateless_cursor_provides_random_access(conn);
  test_stateless_cursor_ignores_trailing_semicolon(conn);
}


PQXX_REGISTER_TEST(test_cursor);
} // namespace
