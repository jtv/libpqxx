#include <iostream>

#include <pqxx/transaction>

#include "../test_helpers.hxx"

namespace
{
void compare_esc(
  pqxx::connection &c, pqxx::transaction_base &t, char const text[])
{
  std::size_t const len{std::size(std::string{text})};
  PQXX_CHECK_EQUAL(
    c.esc(text, len), t.esc(text, len),
    "Connection & transaction escape differently.");

  PQXX_CHECK_EQUAL(
    t.esc(text, len), t.esc(text), "Length argument to esc() changes result.");

  PQXX_CHECK_EQUAL(
    t.esc(std::string{text}), t.esc(text),
    "esc(std::string()) differs from esc(char const[]).");

  PQXX_CHECK_EQUAL(
    text, t.query_value<std::string>("SELECT '" + t.esc(text, len) + "'"),
    "esc() is not idempotent.");

  PQXX_CHECK_EQUAL(
    t.esc(text, len), t.esc(text), "Oversized buffer affects esc().");
}


void test_esc(pqxx::connection &c, pqxx::transaction_base &t)
{
  PQXX_CHECK_EQUAL(t.esc("", 0), "", "Empty string doesn't escape properly.");
  PQXX_CHECK_EQUAL(t.esc("'", 1), "''", "Single quote escaped incorrectly.");
  PQXX_CHECK_EQUAL(
    t.esc(std::string_view{"hello"}), "hello", "Trivial escape went wrong.");
  char const *const escstrings[]{"x", " ", "", nullptr};
  for (std::size_t i{0}; escstrings[i] != nullptr; ++i)
    compare_esc(c, t, escstrings[i]);
}


void test_quote(pqxx::connection &c, pqxx::transaction_base &t)
{
  PQXX_CHECK_EQUAL(t.quote("x"), "'x'", "Basic quote() fails.");
  PQXX_CHECK_EQUAL(
    t.quote(1), "'1'", "quote() not dealing with int properly.");
  PQXX_CHECK_EQUAL(t.quote(0), "'0'", "Quoting zero is a problem.");
  char const *const null_ptr{nullptr};
  PQXX_CHECK_EQUAL(t.quote(null_ptr), "NULL", "Not quoting NULL correctly.");
  PQXX_CHECK_EQUAL(
    t.quote(std::string{"'"}), "''''", "Escaping quotes goes wrong.");

  PQXX_CHECK_EQUAL(
    t.quote("x"), c.quote("x"),
    "Connection and transaction quote differently.");

  char const *test_strings[]{"",   "x",   "\\", "\\\\", "'",
                             "''", "\\'", "\t", "\n",   nullptr};

  for (std::size_t i{0}; test_strings[i] != nullptr; ++i)
  {
    auto r{t.query_value<std::string>("SELECT " + t.quote(test_strings[i]))};
    PQXX_CHECK_EQUAL(
      r, test_strings[i], "Selecting quoted string does not come back equal.");
  }
}


void test_quote_name(pqxx::transaction_base &t)
{
  PQXX_CHECK_EQUAL(
    "\"A b\"", t.quote_name("A b"), "Escaped identifier is not as expected.");
  PQXX_CHECK_EQUAL(
    std::string{"A b"},
    t.exec("SELECT 1 AS " + t.quote_name("A b")).column_name(0),
    "Escaped identifier does not work in SQL.");
}


void test_esc_raw_unesc_raw(pqxx::transaction_base &t)
{
  constexpr char binary[]{"1\0023\\4x5"};
  std::string const data(binary, std::size(binary));
  std::string const escaped{t.esc_raw(data)};

  for (auto const i : escaped)
    PQXX_CHECK(isascii(i), "Non-ASCII character in escaped data: " + escaped);

  for (auto const i : escaped)
    PQXX_CHECK(
      isprint(i), "Unprintable character in escaped data: " + escaped);

  PQXX_CHECK_EQUAL(
    escaped, "\\x3102335c34783500", "Binary data escaped wrong.");
  PQXX_CHECK_EQUAL(
    std::size(t.unesc_raw(escaped)), std::size(data),
    "Wrong size after unescaping.");
  PQXX_CHECK_EQUAL(
    t.unesc_raw(escaped), data,
    "Unescaping binary data does not undo escaping it.");
}


void test_esc_like(pqxx::transaction_base &tx)
{
  PQXX_CHECK_EQUAL(tx.esc_like(""), "", "esc_like breaks on empty string.");
  PQXX_CHECK_EQUAL(tx.esc_like("abc"), "abc", "esc_like is broken.");
  PQXX_CHECK_EQUAL(tx.esc_like("_"), "\\_", "esc_like fails on underscore.");
  PQXX_CHECK_EQUAL(tx.esc_like("%"), "\\%", "esc_like fails on %.");
  PQXX_CHECK_EQUAL(
    tx.esc_like("a%b_c"), "a\\%b\\_c", "esc_like breaks on mix.");
  PQXX_CHECK_EQUAL(
    tx.esc_like("_", '+'), "+_", "esc_like ignores escape character.");
}


void test_escaping()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  test_esc(conn, tx);
  test_quote(conn, tx);
  test_quote_name(tx);
  test_esc_raw_unesc_raw(tx);
  test_esc_like(tx);
}


PQXX_REGISTER_TEST(test_escaping);
} // namespace
