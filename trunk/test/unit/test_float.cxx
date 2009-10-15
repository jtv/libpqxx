#include <test_helpers.hxx>

using namespace PGSTD;
using namespace pqxx;

namespace
{
template<typename T> T make_infinity()
{
  return
#ifdef PQXX_HAVE_LIMITS
    numeric_limits<T>::infinity();
#else
    INFINITY;
#endif
}

void infinity_test(transaction_base &)
{
  double inf = make_infinity<double>();
  PQXX_CHECK_EQUAL(to_string(inf), "infinity", "Infinity not as expected");
  PQXX_CHECK_EQUAL(to_string(-inf), "-infinity", "Negative infinity is broken");
}
} // namespace

PQXX_REGISTER_TEST_NODB(infinity_test)
