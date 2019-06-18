#include "../test_helpers.hxx"

namespace
{
#if __has_include(<charconv>)
void test_str_bool()
{
  const pqxx::str f{false}, t{true};
  PQXX_CHECK_EQUAL(
	std::string{f.view()},
	"false",
	"Failed to convert false.");
  PQXX_CHECK_EQUAL(
	std::string{t.view()},
	"true",
	"Failed to convert true.");
}


template<typename T> constexpr std::size_t max_digits = std::max(
	std::numeric_limits<T>::digits10,
	std::numeric_limits<T>::max_digits10);


template<typename T> void test_str_integral()
{
  const std::string type = pqxx::type_name<T>;

  const pqxx::str
	zero{T{0}},
	one{T{1}},
	ten{T{10}};

  PQXX_CHECK_EQUAL(
	std::string{zero.view()},
	"0",
	"Bad " + type + " conversion.");
  PQXX_CHECK_EQUAL(
	std::string{one.view()},
	"1",
	"Bad " + type + " conversion.");
  PQXX_CHECK_EQUAL(
	std::string{ten.view()},
	"10",
	"Bad " + type + " conversion.");

  if constexpr (std::numeric_limits<T>::is_signed)
  {
    const pqxx::str
	mone{T{-1}},
	mten{T{-10}};

    PQXX_CHECK_EQUAL(
	std::string{mone.view()},
	"-1",
	"Bad " + type + " conversion.");
    PQXX_CHECK_EQUAL(
	std::string{mten.view()},
	"-10",
	"Bad " + type + " conversion.");
  }

  const pqxx::str max{std::numeric_limits<T>::max()};
  PQXX_CHECK_GREATER_EQUAL(
	max.view().size(),
	max_digits<T>,
	"Largest " + type + " came out too short.");
  PQXX_CHECK_LESS_EQUAL(
	max.view().size(),
	max_digits<T> + 1,
	"Largest " + type + " came out too short.");
}


void test_str_integral_types()
{
  test_str_integral<short>();
  test_str_integral<int>();
  test_str_integral<long>();
  test_str_integral<long long>();
  test_str_integral<unsigned short>();
  test_str_integral<unsigned>();
  test_str_integral<unsigned long>();
  test_str_integral<unsigned long long>();
}


PQXX_REGISTER_TEST(test_str_bool);
PQXX_REGISTER_TEST(test_str_integral_types);
#endif
} // namespace
