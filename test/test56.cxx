#include <pqxx/transaction>

#include "test_helpers.hxx"

using namespace pqxx;


// Simple test program for libpqxx.  Issue invalid query and handle error.
namespace
{
void test_056()
{
  connection cx;
  work tx{cx};
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  quiet_errorhandler d(cx);
#include "pqxx/internal/ignore-deprecated-post.hxx"

  PQXX_CHECK_THROWS(
    tx.exec("DELIBERATELY INVALID TEST QUERY..."), sql_error,
    "SQL syntax error did not raise expected exception.");
}


PQXX_REGISTER_TEST(test_056);
} // namespace
