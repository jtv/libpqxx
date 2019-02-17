#include "../test_helpers.hxx"

using namespace pqxx;

namespace
{
template<typename T> T make_infinity()
{
  return std::numeric_limits<T>::infinity();
}

/// Test conversions for some floating-point type.
template<typename T> void infinity_test()
{
  T inf = make_infinity<T>();
  std::string inf_string;
  T back_conversion;

  inf_string = to_string(inf);
  from_string(inf_string, back_conversion);
  PQXX_CHECK_LESS(
	T(999999999),
	back_conversion,
	"Infinity doesn't convert back to something huge.");

  inf_string = to_string(-inf);
  from_string(inf_string, back_conversion);
  PQXX_CHECK_LESS(
	back_conversion,
	-T(999999999),
	"Negative infinity is broken");
}

void test_infinities()
{
  infinity_test<float>();
  infinity_test<double>();
  infinity_test<long double>();
}


PQXX_REGISTER_TEST(test_infinities);
} // namespace
