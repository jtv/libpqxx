#include <test_helpers.hxx>

using namespace PGSTD;
using namespace pqxx;

namespace
{
void compare_esc(connection_base &c, transaction_base &t, const char str[])
{
  const size_t len = string(str).size();
  PQXX_CHECK_EQUAL(c.esc(str, len),
	t.esc(str, len),
	"Connection & transaction escape differently.");

  PQXX_CHECK_EQUAL(t.esc(str, len),
	t.esc(str),
	"Length argument to esc() changes result.");

  PQXX_CHECK_EQUAL(t.esc(string(str)),
	t.esc(str),
	"esc(std::string()) differs from esc(const char[]).");

  PQXX_CHECK_EQUAL(str,
	t.exec("SELECT '" + t.esc(str, len) + "'")[0][0].as<string>(),
	"esc() is not idempotent.");

  PQXX_CHECK_EQUAL(t.esc(str, len),
	t.esc(str),
	"Oversized buffer affects esc().");
}

void test_esc(connection_base &c, transaction_base &t)
{
  PQXX_CHECK_EQUAL(t.esc("", 0), "", "Empty string doesn't escape properly.");
  PQXX_CHECK_EQUAL(t.esc("'", 1), "''", "Single quote escaped incorrectly.");
  const char *const escstrings[] = {
    "x",
    "",
    "'",
    "''",
    "'''",
    "''''",
    "\\",
    "\\\\",
    "\\'",
    NULL
  };
  for (size_t i=0; escstrings[i]; ++i) compare_esc(c, t, escstrings[i]);
}


void test_quote(connection_base &c, transaction_base &t)
{
  PQXX_CHECK_EQUAL(t.quote("x"), "'x'", "Basic quote() fails.");
  PQXX_CHECK_EQUAL(t.quote(1), "'1'", "quote() not dealing with int properly.");
  PQXX_CHECK_EQUAL(t.quote(0), "'0'", "Quoting zero is a problem.");
  const char *const nullptr = NULL;
  PQXX_CHECK_EQUAL(t.quote(nullptr), "NULL", "Not quoting NULL correctly.");
  PQXX_CHECK_EQUAL(t.quote(string("'")), "''''", "Escaping quotes goes wrong.");

  PQXX_CHECK_EQUAL(t.quote("x"),
	c.quote("x"),
	"Connection and transaction quote differently.");
}


void test_escaping(connection_base &c, transaction_base &t)
{
  test_esc(c, t);
  test_quote(c, t);
}
} // namespace

PQXX_REGISTER_TEST(test_escaping)
