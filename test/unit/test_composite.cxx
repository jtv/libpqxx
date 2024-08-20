#include "../test_helpers.hxx"

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

  int a;
  std::string b;
  pqxx::parse_composite(f.view(), a, b);

  PQXX_CHECK_EQUAL(a, 5, "Integer composite field came back wrong.");
  PQXX_CHECK_EQUAL(b, "hello", "String composite field came back wrong.");
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
  int nonnull;
  r = tx.exec("SELECT '()'::pqxxnull").one_row();
  PQXX_CHECK_THROWS(
    pqxx::parse_composite(r[0].view(), nonnull), pqxx::conversion_error,
    "No conversion error when reading a null into a nulless variable.");
  std::optional<int> nullable{5};
  pqxx::parse_composite(r[0].view(), nullable);
  PQXX_CHECK(
    not nullable.has_value(), "Null integer came out as having a value.");

  tx.exec("CREATE TYPE pqxxnulls AS (a integer, b integer)").no_rows();
  std::optional<int> a{2}, b{4};
  r = tx.exec("SELECT '(,)'::pqxxnulls").one_row();
  pqxx::parse_composite(r[0].view(), a, b);
  PQXX_CHECK(not a.has_value(), "Null first integer stored as value.");
  PQXX_CHECK(not b.has_value(), "Null second integer stored as value.");
}


void test_composite_renders_to_string()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  char buf[1000];

  pqxx::composite_into_buf(
    std::begin(buf), std::end(buf), 355, "foo", "b\na\\r");
  PQXX_CHECK_EQUAL(
    std::string{buf}, "(355,\"foo\",\"b\na\\\\r\")",
    "Composite was not rendered as expected.");

  tx.exec("CREATE TYPE pqxxcomp AS (a integer, b text, c text)").no_rows();
  auto const f{
    tx.exec("SELECT '" + std::string{buf} + "'::pqxxcomp").one_field()};

  int a;
  std::string b, c;
  bool const nonnull{f.composite_to(a, b, c)};
  PQXX_CHECK(nonnull, "Mistaken nullness.");
  PQXX_CHECK_EQUAL(a, 355, "Int came back wrong.");
  PQXX_CHECK_EQUAL(b, "foo", "Simple string came back wrong.");
  PQXX_CHECK_EQUAL(c, "b\na\\r", "Escaping went wrong.");
}


PQXX_REGISTER_TEST(test_composite);
PQXX_REGISTER_TEST(test_composite_escapes);
PQXX_REGISTER_TEST(test_composite_handles_nulls);
PQXX_REGISTER_TEST(test_composite_renders_to_string);
} // namespace
