#include <pqxx/types>
#include <pqxx/util>

#include "helpers.hxx"

namespace
{
template<typename T> void test_for(T const &val)
{
  auto const name{pqxx::name_type<T>()};
  auto const sz{std::size(val)};

  std::span<std::byte const> const out{pqxx::binary_cast(val)};

  PQXX_CHECK_EQUAL(
    std::size(out), sz,
    std::format("Got bad size on binary_cast<{}().", name));

  for (std::size_t i{0}; i < sz; ++i)
    PQXX_CHECK_EQUAL(
      static_cast<unsigned>(out[i]), static_cast<unsigned>(val[i]),
      std::format("Mismatch in {} byte {}.", name, i));
}


void test_binary_cast()
{
  std::byte const bytes_c_array[]{
    std::byte{0x22}, std::byte{0x23}, std::byte{0x24}};
  test_for(bytes_c_array);
  test_for("Hello world");
  test_for(std::string{"I'm a string"});
  test_for(std::string_view{"I'm a string_view"});

  test_for(std::vector<char>{'n', 'o', 'p', 'q'});
  test_for(std::vector<unsigned char>{'n', 'o', 'p', 'q'});
  test_for(std::vector<signed char>{'n', 'o', 'p', 'q'});
}


/// Shorthand for `std::source_location::current()` or `pqxx::sl::current()`.
constexpr inline pqxx::sl here(pqxx::sl loc = pqxx::sl::current())
{
  return loc;
}


/// Test @ref pqxx::check_cast() for casting 0 from `FROM` to `TO`.
template<std::integral FROM, std::integral TO> inline void check_val(int n)
{
  PQXX_CHECK_EQUAL(
    pqxx::check_cast<TO>(
      static_cast<FROM>(n), std::format("check_cast failed for value {}.", n),
      here()),
    static_cast<TO>(n),
    std::format("check_fast test failed for integral value {}", n));
}


template<std::integral TO> inline void check_val_to(int n)
{
  check_val<short, TO>(n);
  check_val<int, TO>(n);
  check_val<long, TO>(n);
  check_val<long long, TO>(n);

  if (n >= 0)
  {
    check_val<unsigned short, TO>(n);
    check_val<unsigned int, TO>(n);
    check_val<unsigned long, TO>(n);
    check_val<unsigned long long, TO>(n);
  }
}


/// Test @ref pqxx::check_cast() for casting 0 from `FROM` to `TO`.
template<std::floating_point FROM, std::floating_point TO>
inline void check_val(float n)
{
  auto const cast_result{pqxx::check_cast<TO>(n, "fail", here())};

  // Check that the value we get falls between the immediately neighbouring
  // float values.
  PQXX_CHECK_GREATER(
    cast_result, std::nextafterf(n, -std::numeric_limits<float>::infinity()));
  PQXX_CHECK_LESS(
    cast_result, std::nextafterf(n, std::numeric_limits<float>::infinity()));
}


template<std::floating_point TO> inline void check_val_to(float n)
{
  check_val<float, TO>(n);
  check_val<double, TO>(n);

  // Valgrind doesn't support long double, so skip those tests when building
  // for testing with Valgrind.  Horrible, but there are no good options.
#if !defined(PQXX_VALGRIND)
  check_val<long double, TO>(n);
#endif
}


inline void check_all_casts(int n)
{
  check_val_to<short>(n);
  check_val_to<int>(n);
  check_val_to<long>(n);
  check_val_to<long long>(n);

  if (n >= 0)
  {
    check_val_to<unsigned short>(n);
    check_val_to<unsigned int>(n);
    check_val_to<unsigned long>(n);
    check_val_to<unsigned long long>(n);
  }
}


inline void check_all_casts(float n)
{
  check_val_to<float>(n);
  check_val_to<double>(n);
#if !defined(PQXX_VALGRIND)
  check_val_to<long double>(n);
#endif
}


/// Test @ref pqxx::check_cast() for casting NaNs to type `TO`.
template<std::floating_point TO> inline void check_nan()
{
  PQXX_CHECK(std::isnan(pqxx::check_cast<TO>(std::nanf("1"), "fail", here())));
  PQXX_CHECK(std::isnan(pqxx::check_cast<TO>(std::nan("1"), "fail", here())));
  PQXX_CHECK(std::isnan(pqxx::check_cast<TO>(std::nanl("1"), "fail", here())));
}


/// Test @ref pqxx::check_cast() for casting infinities from `FROM` to `TO`.
template<std::floating_point FROM, std::floating_point TO>
inline void check_inf()
{
  PQXX_CHECK(std::isinf(pqxx::check_cast<TO>(
    std::numeric_limits<FROM>::infinity(), "fail", here())));
  PQXX_CHECK(std::isinf(pqxx::check_cast<TO>(
    -std::numeric_limits<FROM>::infinity(), "fail", here())));
}


/// Test @ref pqxx::check_cast() for casting infinities to `TO`.
template<std::floating_point TO> inline void check_inf_to()
{
  check_inf<float, TO>();
  check_inf<double, TO>();
#if !defined(PQXX_VALGRIND)
  check_inf<long double, TO>();
#endif
}


void test_check_cast()
{
  check_all_casts(0);
  check_all_casts(1);
  check_all_casts(-1);
  check_all_casts(999);
  check_all_casts(-999);
  check_all_casts(32767);
  check_all_casts(-32767);

  check_all_casts(0.0f);
  check_all_casts(-0.0f);
  check_all_casts(-1.0f);
  check_all_casts(1.0f);
  check_all_casts(999.0f);

  PQXX_CHECK_EQUAL(pqxx::check_cast<int>(-1, "fail", here()), -1);
  PQXX_CHECK_EQUAL(pqxx::check_cast<int>(-1LL, "fail", here()), -1);
  PQXX_CHECK_EQUAL(pqxx::check_cast<short>(-1, "fail", here()), -1);
  PQXX_CHECK_EQUAL(pqxx::check_cast<short>(-1LL, "fail", here()), -1);
  PQXX_CHECK_EQUAL(pqxx::check_cast<long>(-1LL, "fail", here()), -1);
  PQXX_CHECK_THROWS(
    pqxx::check_cast<unsigned>(-1, "fail", here()), pqxx::range_error);
  PQXX_CHECK_THROWS(
    pqxx::check_cast<unsigned long>(-1, "fail", here()), pqxx::range_error);
  PQXX_CHECK_THROWS(
    pqxx::check_cast<int>(
      (std::numeric_limits<unsigned>::max)(), "fail", here()),
    pqxx::range_error);

  PQXX_CHECK_THROWS(
    pqxx::check_cast<int>(
      (std::numeric_limits<int>::max)() + 1LL, "fail", here()),
    pqxx::range_error);
  PQXX_CHECK_THROWS(
    pqxx::check_cast<int>(
      std::numeric_limits<int>::lowest() - 1LL, "fail", here()),
    pqxx::range_error);
  PQXX_CHECK_THROWS(
    pqxx::check_cast<float>(
      (std::numeric_limits<float>::max)() * 1.1, "fail", here()),
    pqxx::range_error);
  PQXX_CHECK_THROWS(
    pqxx::check_cast<float>(
      std::numeric_limits<float>::lowest() * 1.1, "fail", here()),
    pqxx::range_error);

  int const threshold{(std::numeric_limits<unsigned short>::max)()};
  if constexpr (std::numeric_limits<int>::max() > threshold)
  {
    PQXX_CHECK_THROWS(
      pqxx::check_cast<unsigned short>(threshold + 1, "fail", here()),
      pqxx::range_error);
  }

  check_nan<float>();
  check_nan<double>();
#if !defined(PQXX_VALGRIND)
  check_nan<long double>();
#endif

  check_inf_to<float>();
  check_inf_to<double>();
#if !defined(PQXX_VALGRIND)
  check_inf_to<long double>();
#endif
}


PQXX_REGISTER_TEST(test_binary_cast);
PQXX_REGISTER_TEST(test_check_cast);
} // namespace
