#include <test_helpers.hxx>

using namespace std;
using namespace pqxx;

namespace
{
void test_read_transaction(transaction_base &trans)
{
  PQXX_CHECK_EQUAL(
	trans.exec("SELECT 1")[0][0].as<int>(),
	1,
	"Bad result from read transaction.");

  PQXX_CHECK_THROWS(
	trans.exec("CREATE TABLE should_not_exist(x integer)"),
	sql_error,
	"Read-only transaction allows database to be modified.");
}
}

PQXX_REGISTER_TEST_T(test_read_transaction, read_transaction)
