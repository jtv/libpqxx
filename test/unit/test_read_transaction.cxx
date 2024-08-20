#include <pqxx/transaction>

#include "../test_helpers.hxx"

namespace
{
void test_read_transaction()
{
  pqxx::connection cx;
  pqxx::read_transaction tx{cx};
  PQXX_CHECK_EQUAL(
    tx.query_value<int>("SELECT 1"), 1, "Bad result from read transaction.");

  PQXX_CHECK_THROWS(
    tx.exec("CREATE TABLE should_not_exist(x integer)"), pqxx::sql_error,
    "Read-only transaction allows database to be modified.");
}


PQXX_REGISTER_TEST(test_read_transaction);
} // namespace
