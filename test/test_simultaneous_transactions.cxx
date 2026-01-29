#include <pqxx/nontransaction>
#include <pqxx/subtransaction>

#include "helpers.hxx"

namespace
{
void test_simultaneous_transactions(pqxx::test::context &)
{
  pqxx::connection cx;

  pqxx::nontransaction const n1{cx};
  PQXX_CHECK_THROWS(
    pqxx::nontransaction{cx}, pqxx::usage_error,
    "Allowed to open simultaneous nontransactions.");
}


PQXX_REGISTER_TEST(test_simultaneous_transactions);
} // namespace
