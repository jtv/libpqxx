#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;

// Test program for libpqxx.  Test string-escaping functions.

#define CHECK(REF, VAL, VDESC) \
	PQXX_CHECK_EQUAL(VAL, REF, "String mismatch for " VDESC)

namespace
{
void esc(transaction_base &t, string str, string expected=string())
{
  if (expected.empty()) expected = str;
  CHECK(expected, t.esc(str), "string");
  CHECK(expected, t.esc(str.c_str()), "const char[]");
  CHECK(expected, t.esc(str.c_str(), str.size()), "const char[],size_t");
  CHECK(expected, t.esc(str.c_str(), 1000), "const char[],1000");
}


void dotests(transaction_base &t)
{
  esc(t, "");
  esc(t, "foo");
  esc(t, "foo bar");
  esc(t, "unquote' ha!", "unquote'' ha!");
  esc(t, "'", "''");
  esc(t, "\\", "\\\\");
  esc(t, "\t");
}


void test_090(transaction_base &N)
{
  connection_base &C(N.conn());
  // Test connection's adorn_name() function for uniqueness
  const string nametest = "basename";
  const string nt1 = C.adorn_name(nametest),
               nt2 = C.adorn_name(nametest);

  PQXX_CHECK_NOT_EQUAL(C.adorn_name(nametest),
	C.adorn_name(nametest),
	"\"Unique\" names are not unique.");

  dotests(N);
  N.abort();

  work W(C, "test90work");
  dotests(W);
  W.abort();
}
} // namespace

PQXX_REGISTER_TEST(test_090)
