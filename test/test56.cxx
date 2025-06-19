#include <pqxx/transaction>

#include "helpers.hxx"


// Simple test program for libpqxx.  Issue invalid query and handle error.
namespace
{
void test_056()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  pqxx::quiet_errorhandler const d(cx);
#include "pqxx/internal/ignore-deprecated-post.hxx"

  PQXX_CHECK_THROWS(
    tx.exec("DELIBERATELY INVALID TEST QUERY..."), pqxx::sql_error);
}


PQXX_REGISTER_TEST(test_056);
} // namespace
