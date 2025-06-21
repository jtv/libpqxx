#include "helpers.hxx"

#include "pqxx/composite"
#include "pqxx/transaction"

namespace
{
void test_composite()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  tx.exec("CREATE TYPE pqxxfoo AS (a integer, b text)").no_rows();
  auto const f{tx.exec("SELECT '(5,hello)'::pqxxfoo").one_field()};

  int a{};
  std::string b;
  pqxx::parse_composite(f.view(), a, b);

  PQXX_CHECK_EQUAL(a, 5);
  PQXX_CHECK_EQUAL(b, "hello");
}


void test_composite_escapes()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  tx.exec("CREATE TYPE pqxxsingle AS (x text)").no_rows();
  std::string s;

  pqxx::row r;
  r = tx.exec(R"--(SELECT '("a""b")'::pqxxsingle)--").one_row();
  pqxx::parse_composite(r[0].view(), s);
  PQXX_CHECK_EQUAL(
    s, "a\"b", "Double-double-quotes escaping did not parse correctly.");

  r = tx.exec(R"--(SELECT '("a\"b")'::pqxxsingle)--").one_row();
  pqxx::parse_composite(r[0].view(), s);
  PQXX_CHECK_EQUAL(s, "a\"b", "Backslash escaping did not parse correctly.");
}


void test_composite_handles_nulls()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::row r;

  tx.exec("CREATE TYPE pqxxnull AS (a integer)").no_rows();
  int nonnull{};
  r = tx.exec("SELECT '()'::pqxxnull").one_row();
  PQXX_CHECK_THROWS(
    pqxx::parse_composite(r[0].view(), nonnull), pqxx::conversion_error);
  std::optional<int> nullable{5};
  pqxx::parse_composite(r[0].view(), nullable);
  PQXX_CHECK(not nullable.has_value());

  tx.exec("CREATE TYPE pqxxnulls AS (a integer, b integer)").no_rows();
  std::optional<int> a{2}, b{4};
  r = tx.exec("SELECT '(,)'::pqxxnulls").one_row();
  pqxx::parse_composite(r[0].view(), a, b);
  PQXX_CHECK(not a.has_value());
  PQXX_CHECK(not b.has_value());
}


void test_composite_renders_to_string()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  char buf[1000];

  pqxx::composite_into_buf(buf, 355, "foo", "b\na\\r");
  PQXX_CHECK_EQUAL(std::string{std::data(buf)}, "(355,\"foo\",\"b\na\\\\r\")");

  tx.exec("CREATE TYPE pqxxcomp AS (a integer, b text, c text)").no_rows();
  auto const f{
    tx.exec("SELECT '" + std::string{std::data(buf)} + "'::pqxxcomp")
      .one_field()};

  int a{};
  std::string b, c;
  bool const nonnull{f.composite_to(a, b, c)};
  PQXX_CHECK(nonnull);
  PQXX_CHECK_EQUAL(a, 355);
  PQXX_CHECK_EQUAL(b, "foo");
  PQXX_CHECK_EQUAL(c, "b\na\\r");
}


PQXX_REGISTER_TEST(test_composite);
PQXX_REGISTER_TEST(test_composite_escapes);
PQXX_REGISTER_TEST(test_composite_handles_nulls);
PQXX_REGISTER_TEST(test_composite_renders_to_string);
} // namespace
