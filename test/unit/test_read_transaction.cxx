#include "../test_helpers.hxx"

using namespace pqxx;

namespace
{
void test_read_transaction()
{
  connection conn;
  read_transaction tx{conn};
  PQXX_CHECK_EQUAL(
	tx.exec("SELECT 1")[0][0].as<int>(),
	1,
	"Bad result from read transaction.");

  PQXX_CHECK_THROWS(
	tx.exec("CREATE TABLE should_not_exist(x integer)"),
	sql_error,
	"Read-only transaction allows database to be modified.");
}


PQXX_REGISTER_TEST(test_read_transaction);
}
