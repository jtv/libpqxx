#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_read_transaction(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::read_transaction tx{cx};
  PQXX_CHECK_EQUAL(tx.query_value<int>("SELECT 1"), 1);

  PQXX_CHECK_THROWS(
    tx.exec("CREATE TABLE should_not_exist(x integer)"), pqxx::sql_error);
}


PQXX_REGISTER_TEST(test_read_transaction);
} // namespace
