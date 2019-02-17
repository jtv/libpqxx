#include "../test_helpers.hxx"

using namespace pqxx;

namespace
{
void test_simultaneous_transactions()
{
  connection conn;

  nontransaction n1{conn};
  PQXX_CHECK_THROWS(
	nontransaction n2{conn},
	std::logic_error,
	"Allowed to open simultaneous nontransactions.");
}


PQXX_REGISTER_TEST(test_simultaneous_transactions);
} // namespace
