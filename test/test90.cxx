#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;

// Test program for libpqxx.  Test adorn_name.

namespace
{
void test_090(transaction_base &N)
{
  connection_base &C(N.conn());
  // Test connection's adorn_name() function for uniqueness
  const string nametest = "basename";
  const string nt1 = C.adorn_name(nametest),
               nt2 = C.adorn_name(nametest);

  PQXX_CHECK_NOT_EQUAL(C.adorn_name(nametest),
	C.adorn_name(nametest),
	"\"Unique\" names are not unique.");
}
} // namespace

PQXX_REGISTER_TEST(test_090)
