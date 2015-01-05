#include <test_helpers.hxx>

using namespace std;
using namespace pqxx;

namespace
{
template<typename T> T make_infinity()
{
  return numeric_limits<T>::infinity();
}

void infinity_test(transaction_base &)
{
  double inf = make_infinity<double>();
  string inf_string;
  double back_conversion;

  inf_string = to_string(inf);
  from_string(inf_string, back_conversion);
  PQXX_CHECK_LESS(
	999999999,
	back_conversion,
	"Infinity doesn't convert back to something huge.");

  inf_string = to_string(-inf);
  from_string(inf_string, back_conversion);
  PQXX_CHECK_LESS(back_conversion, -999999999, "Negative infinity is broken");
}
} // namespace

PQXX_REGISTER_TEST_NODB(infinity_test)
