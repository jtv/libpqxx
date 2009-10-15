#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Test result::swap()
namespace
{
void test_077(transaction_base &T)
{
  result RFalse = T.exec("SELECT 1=0"),
	 RTrue  = T.exec("SELECT 1=1");
  bool f, t;
  from_string(RFalse[0][0], f);
  from_string(RTrue[0][0],  t);
  PQXX_CHECK(!f && t, "Booleans converted incorrectly; can't trust this test.");

  RFalse.swap(RTrue);
  from_string(RFalse[0][0], f);
  from_string(RTrue[0][0],  t);
  PQXX_CHECK(f && !t, "result::swap() is broken.");
}
} // namespace

PQXX_REGISTER_TEST_T(test_077, nontransaction)
