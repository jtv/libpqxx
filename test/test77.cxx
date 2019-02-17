#include "test_helpers.hxx"

using namespace pqxx;


// Test program for libpqxx.  Test result::swap()
namespace
{
void test_077()
{
  connection conn;
  nontransaction tx{conn};

  row
	RFalse = tx.exec1("SELECT 1=0"),
	RTrue  = tx.exec1("SELECT 1=1");
  bool f, t;
  from_string(RFalse[0], f);
  from_string(RTrue[0],  t);
  PQXX_CHECK(
	not f and t,
	"Booleans converted incorrectly; can't trust this test.");

  RFalse.swap(RTrue);
  from_string(RFalse[0], f);
  from_string(RTrue[0],  t);
  PQXX_CHECK(f and not t, "result::swap() is broken.");
}
} // namespace


PQXX_REGISTER_TEST(test_077);
