#include <pqxx/transaction>

#include "helpers.hxx"


// Simple test and sample code for libpqxx.  Issue invalid query and handle
// error.
namespace
{
void test_056(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  PQXX_CHECK_THROWS(
    tx.exec("DELIBERATELY INVALID TEST QUERY..."), pqxx::sql_error);
}


PQXX_REGISTER_TEST(test_056);
} // namespace
