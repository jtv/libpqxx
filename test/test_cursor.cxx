#include <pqxx/cursor>
#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_stateless_cursor_provides_random_access(pqxx::connection &cx)
{
  pqxx::work tx{cx};
  pqxx::stateless_cursor<
    pqxx::cursor_base::read_only, pqxx::cursor_base::owned>
    c{tx, "SELECT * FROM generate_series(0, 3)", "count", false};

  auto r{c.retrieve(1, 2)};
  PQXX_CHECK_EQUAL(std::size(r), 1);
  PQXX_CHECK_EQUAL(r[0][0].as<int>(), 1);

  r = c.retrieve(3, 10);
  PQXX_CHECK_EQUAL(std::size(r), 1, "Expected 1 row retrieving past end.");
  PQXX_CHECK_EQUAL(r[0][0].as<int>(), 3);

  r = c.retrieve(0, 1);
  PQXX_CHECK_EQUAL(std::size(r), 1);
  PQXX_CHECK_EQUAL(r[0][0].as<int>(), 0);
}


void test_stateless_cursor_ignores_trailing_semicolon(pqxx::connection &cx)
{
  pqxx::work tx{cx};
  pqxx::stateless_cursor<
    pqxx::cursor_base::read_only, pqxx::cursor_base::owned>
    c{tx, "SELECT * FROM generate_series(0, 3)  ;; ; \n \t  ", "count", false};

  auto r{c.retrieve(1, 2)};
  PQXX_CHECK_EQUAL(std::size(r), 1, "Trailing semicolon confused retrieve().");
}


void test_cursor()
{
  pqxx::connection cx;
  test_stateless_cursor_provides_random_access(cx);
  test_stateless_cursor_ignores_trailing_semicolon(cx);
}


PQXX_REGISTER_TEST(test_cursor);
} // namespace
