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


void test_cursor(pqxx::test::context &)
{
  pqxx::connection cx;
  test_stateless_cursor_provides_random_access(cx);
  test_stateless_cursor_ignores_trailing_semicolon(cx);
}


void test_cursor_constants(pqxx::test::context &tctx)
{
  PQXX_CHECK_GREATER(pqxx::cursor_base::all(), tctx.make_num());
  PQXX_CHECK_LESS(pqxx::cursor_base::backward_all(), -tctx.make_num());
}


void test_icursorstream_tracks_creation_location(pqxx::test::context &)
{
  std::source_location const loc{pqxx::sl::current()};
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::icursorstream const s{
    tx, "SELECT * FROM generate_series(1, 3)", "mycur", 1, loc};
  PQXX_CHECK_EQUAL(
    std::string{s.created_loc().file_name()}, std::string{loc.file_name()});
  PQXX_CHECK_EQUAL(s.created_loc().line(), loc.line());
}


PQXX_REGISTER_TEST(test_cursor);
PQXX_REGISTER_TEST(test_cursor_constants);
PQXX_REGISTER_TEST(test_icursorstream_tracks_creation_location);
} // namespace
