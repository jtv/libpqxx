#include <test_helpers.hxx>

using namespace PGSTD;
using namespace pqxx;

template<typename T> T make_infinity()
{
  return
#ifdef PQXX_HAVE_LIMITS
    numeric_limits<T>::infinity();
#else
    INFINITY;
#endif
}

namespace
{
void infinity_test()
{
  double inf = make_infinity<double>();
  PQXX_CHECK_EQUAL(to_string(inf), "infinity", "Infinity not as expected")
  PQXX_CHECK_EQUAL(to_string(-inf), "-infinity", "Negative infinity is broken")
}
}

int main()
{
  return pqxx::test::pqxxtest(infinity_test);
}

