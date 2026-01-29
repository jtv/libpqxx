#include <pqxx/connection>

#include "helpers.hxx"

// Test program for libpqxx.  Test adorn_name.

namespace
{
void test_090(pqxx::test::context &)
{
  pqxx::connection cx;

  // Test connection's adorn_name() function for uniqueness
  std::string const nametest{"basename"};

  PQXX_CHECK_NOT_EQUAL(cx.adorn_name(nametest), cx.adorn_name(nametest));
}
} // namespace


PQXX_REGISTER_TEST(test_090);
