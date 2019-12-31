#include <iostream>

#include "../test_helpers.hxx"

namespace
{
void compare_esc(
	pqxx::connection_base &c, pqxx::transaction_base &t, const char text[])
{
  const size_t len = std::string{text}.size();
  PQXX_CHECK_EQUAL(c.esc(text, len),
	t.esc(text, len),
	"Connection & transaction escape differently.");

  PQXX_CHECK_EQUAL(t.esc(text, len),
	t.esc(text),
	"Length argument to esc() changes result.");

  PQXX_CHECK_EQUAL(t.esc(std::string{text}),
	t.esc(text),
	"esc(std::string()) differs from esc(const char[]).");

  PQXX_CHECK_EQUAL(text,
	t.exec1("SELECT '" + t.esc(text, len) + "'")[0].as<std::string>(),
	"esc() is not idempotent.");

  PQXX_CHECK_EQUAL(t.esc(text, len),
	t.esc(text),
	"Oversized buffer affects esc().");
}


void test_esc(pqxx::connection_base &c, pqxx::transaction_base &t)
{
  PQXX_CHECK_EQUAL(t.esc("", 0), "", "Empty string doesn't escape properly.");
  PQXX_CHECK_EQUAL(t.esc("'", 1), "''", "Single quote escaped incorrectly.");
  const char *const escstrings[] = {
    "x",
    " ",
    "",
    nullptr
  };
  for (size_t i=0; escstrings[i]; ++i) compare_esc(c, t, escstrings[i]);
}


void test_quote(pqxx::connection_base &c, pqxx::transaction_base &t)
{
  PQXX_CHECK_EQUAL(t.quote("x"), "'x'", "Basic quote() fails.");
  PQXX_CHECK_EQUAL(t.quote(1), "'1'", "quote() not dealing with int properly.");
  PQXX_CHECK_EQUAL(t.quote(0), "'0'", "Quoting zero is a problem.");
  const char *const null_ptr = nullptr;
  PQXX_CHECK_EQUAL(t.quote(null_ptr), "NULL", "Not quoting NULL correctly.");
  PQXX_CHECK_EQUAL(
	t.quote(std::string{"'"}),
	"''''",
	"Escaping quotes goes wrong.");

  PQXX_CHECK_EQUAL(t.quote("x"),
	c.quote("x"),
	"Connection and transaction quote differently.");

  const char *test_strings[] = {
    "",
    "x",
    "\\",
    "\\\\",
    "'",
    "''",
    "\\'",
    "\t",
    "\n",
    nullptr
  };

  for (size_t i=0; test_strings[i]; ++i)
  {
    auto r = t.exec1("SELECT " + t.quote(test_strings[i]));
    PQXX_CHECK_EQUAL(
	r[0].as<std::string>(),
	test_strings[i],
	"Selecting quoted string does not come back equal.");
  }
}


void test_quote_name(pqxx::transaction_base &t)
{
  PQXX_CHECK_EQUAL(
	"\"A b\"",
	t.quote_name("A b"),
	"Escaped identifier is not as expected.");
  PQXX_CHECK_EQUAL(
	std::string{"A b"},
	t.exec("SELECT 1 AS " + t.quote_name("A b")).column_name(0),
	"Escaped identifier does not work in SQL.");
}


void test_esc_raw_unesc_raw(pqxx::transaction_base &t)
{
  const char binary[] = "1\0023\\4x5";
  const std::string data(binary, sizeof(binary));
  const std::string escaped = t.esc_raw(data);

  for (const auto i: escaped)
    PQXX_CHECK(isascii(i), "Non-ASCII character in escaped data: " + escaped);

  for (const auto i: escaped)
    PQXX_CHECK(
	isprint(i),
	"Unprintable character in escaped data: " + escaped);

  PQXX_CHECK_EQUAL(
	t.unesc_raw(escaped),
	data,
	"Unescaping binary data does not undo escaping it.");
}


void test_esc_like(pqxx::transaction_base &tx)
{
  PQXX_CHECK_EQUAL(tx.esc_like(""), "", "esc_like breaks on empty string.");
  PQXX_CHECK_EQUAL(tx.esc_like("abc"), "abc", "esc_like is broken.");
  PQXX_CHECK_EQUAL(tx.esc_like("_"), "\\_", "esc_like fails on underscore.");
  PQXX_CHECK_EQUAL(tx.esc_like("%"), "\\%", "esc_like fails on %.");
  PQXX_CHECK_EQUAL(
	tx.esc_like("a%b_c"),
	"a\\%b\\_c",
	"esc_like breaks on mix.");
  PQXX_CHECK_EQUAL(
	tx.esc_like("_", '+'),
	"+_",
	"esc_like ignores escape character.");
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
