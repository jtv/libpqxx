#include "../test_helpers.hxx"

#include "pqxx/composite"
#include "pqxx/transaction"

namespace
{
void test_composite()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  tx.exec0("CREATE TYPE pqxxfoo AS (a integer, b text)");
  auto const r{tx.exec1("SELECT '(5,hello)'::pqxxfoo")};

  int a;
  std::string b;
  pqxx::parse_composite(r[0].view(), a, b);

  PQXX_CHECK_EQUAL(a, 5, "Integer composite field came back wrong.");
  PQXX_CHECK_EQUAL(b, "hello", "String composite field came back wrong.");
}


void test_composite_escapes()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::row r;
  tx.exec0("CREATE TYPE pqxxsingle AS (x text)");
  std::string s;

  r = tx.exec1(R"--(SELECT '("a""b")'::pqxxsingle)--");
  pqxx::parse_composite(r[0].view(), s);
  PQXX_CHECK_EQUAL(
    s, "a\"b", "Double-double-quotes escaping did not parse correctly.");

  r = tx.exec1(R"--(SELECT '("a\"b")'::pqxxsingle)--");
  pqxx::parse_composite(r[0].view(), s);
  PQXX_CHECK_EQUAL(s, "a\"b", "Backslash escaping did not parse correctly.");
}


void test_composite_handles_nulls()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::row r;

  tx.exec0("CREATE TYPE pqxxnull AS (a integer)");
  int nonnull;
  r = tx.exec1("SELECT '()'::pqxxnull");
  PQXX_CHECK_THROWS(
    pqxx::parse_composite(r[0].view(), nonnull), pqxx::conversion_error,
    "No conversion error when reading a null into a nulless variable.");
  std::optional<int> nullable{5};
  pqxx::parse_composite(r[0].view(), nullable);
  PQXX_CHECK(
    not nullable.has_value(), "Null integer came out as having a value.");

  tx.exec0("CREATE TYPE pqxxnulls AS (a integer, b integer)");
  std::optional<int> a{2}, b{4};
  r = tx.exec1("SELECT '(,)'::pqxxnulls");
  pqxx::parse_composite(r[0].view(), a, b);
  PQXX_CHECK(not a.has_value(), "Null first integer stored as value.");
  PQXX_CHECK(not b.has_value(), "Null second integer stored as value.");
}
} // namespace

PQXX_REGISTER_TEST(test_composite);
PQXX_REGISTER_TEST(test_composite_escapes);
PQXX_REGISTER_TEST(test_composite_handles_nulls);
