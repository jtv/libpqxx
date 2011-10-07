#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Issue invalid query and handle error.
namespace
{
void test_056(transaction_base &T)
{
  quiet_errorhandler d(T.conn());

  PQXX_CHECK_THROWS(
	T.exec("DELIBERATELY INVALID TEST QUERY...", "invalid_query"),
	sql_error,
	"SQL syntax error did not raise expected exception.");
}
} // namespace

PQXX_REGISTER_TEST(test_056)
