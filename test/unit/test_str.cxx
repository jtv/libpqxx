#include "../test_helpers.hxx"

namespace
{
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


/// Get the standard, guaranteed-correct string representation of t.
template<typename T> std::string represent(T t)
{
  std::stringstream output;
  output << t;
  return output.str();
}


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

  for (T i{2}; i < 127; ++i)
    PQXX_CHECK_EQUAL(
	std::string{pqxx::str<T>{i}.view()},
	represent(i),
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

    constexpr T bottom{std::numeric_limits<T>::min()};
    PQXX_CHECK_EQUAL(
	std::string{pqxx::str<T>{bottom}},
	represent(bottom),
	"Smallest " + type + " did not convert right.");

    for (T i{-2}; i > -128; --i)
      PQXX_CHECK_EQUAL(
	  std::string{pqxx::str<T>{i}.view()},
	  represent(i),
	  "Bad " + type + " conversion.");
  }

  constexpr T top = std::numeric_limits<T>::max();
  PQXX_CHECK_EQUAL(
	std::string{pqxx::str<T>{top}},
	represent(top),
	"Largest " + type + " did not convert right.");
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
} // namespace
