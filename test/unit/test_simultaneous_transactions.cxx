#include <pqxx/nontransaction>
#include <pqxx/subtransaction>

#include "../test_helpers.hxx"

namespace
{
void test_simultaneous_transactions()
{
  pqxx::connection cx;

  pqxx::nontransaction n1{cx};
  PQXX_CHECK_THROWS(
    pqxx::nontransaction n2{cx}, std::logic_error,
    "Allowed to open simultaneous nontransactions.");
}


PQXX_REGISTER_TEST(test_simultaneous_transactions);
} // namespace
