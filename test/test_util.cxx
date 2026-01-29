#include <pqxx/types>
#include <pqxx/util>

#include "helpers.hxx"

namespace
{
void test_make_num(pqxx::test::context &tctx)
{
  bool same{true};
  int last{tctx.make_num()};
  for (int i{0}; same and (i < 10); ++i)
    if (tctx.make_num() != last)
      same = false;
  PQXX_CHECK(
    not same, std::format("Random numbers all came out as {}.", last));

  for (int i{0}; i < 100; ++i) PQXX_CHECK_BOUNDS(tctx.make_num(10), 0, 10);
}


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


void test_binary_cast(pqxx::test::context &)
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


void test_check_cast(pqxx::test::context &)
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


void test_source_loc_renders_real_source_location(pqxx::test::context &)
{
  auto const loc{pqxx::sl::current()};
  auto const loc_text{pqxx::source_loc(loc)};
  PQXX_CHECK_EQUAL(
    loc_text, (std::format(
                "{}:{}:{}: ({})", loc.file_name(), loc.line(), loc.column(),
                loc.function_name())));

  PQXX_CHECK(pqxx::str_contains(
    loc_text, "test_source_loc_renders_real_source_location"));
  PQXX_CHECK(pqxx::str_contains(loc_text, __FILE__));
}


/// Make up an arbitrary source code filename.
std::string make_filename(pqxx::test::context &tctx)
{
  char const *suffix{nullptr};
  switch (tctx.make_num(7))
  {
  case 0: suffix = "cxx"; break;
  case 1: suffix = "cpp"; break;
  case 2: suffix = "cc"; break;
  case 3: suffix = "C"; break;
  case 4: suffix = "hxx"; break;
  case 5: suffix = "hpp"; break;
  case 6: suffix = "h"; break;
  }
  assert(suffix != nullptr);

  return std::format("{}.{}", tctx.make_name("source"), suffix);
}


/// Make up an arbitrary C++ type name.
std::string make_type(pqxx::test::context &tctx)
{
  switch (tctx.make_num(10))
  {
  case 0: return "int";
  case 1: return "char *";
  case 2: return "const char *";
  case 3: return "std::string";
  case 4: return "unsigned int";
  case 5: return "double";
  case 6: return std::format("std::vector<{}> &", make_type(tctx));
  case 7: {
    auto const tp1{make_type(tctx)};
    auto const tp2{make_type(tctx)};
    return std::format("std::map<{}, {}> &", tp1, tp2);
  }
  case 8: return "bool";
  case 9: return "char";
  default: break;
  }
  throw pqxx::internal_error{"Unexpected value from make_num()."};
}


/// Make up an arbitrary C++ parameters list.
std::string make_params(pqxx::test::context &tctx)
{
  switch (tctx.make_num(3))
  {
  case 0: return "";
  case 1: return make_type(tctx);
  case 2: {
    auto const tp1{make_type(tctx)};
    auto const tp2{make_type(tctx)};
    return std::format("{}, {}", tp1, tp2);
  }
  default: break;
  }
  throw pqxx::internal_error{"Unexpected value from make_num()."};
}


/// Make up an arbitrary function name.
std::string make_function(pqxx::test::context &tctx)
{
  std::string rettype{"void"};
  if (tctx.make_num(5) > 0)
    rettype = make_type(tctx);
  auto const name{tctx.make_name("func")};
  return std::format("{} {}({})", rettype, name, make_params(tctx));
}


/// Test double for a `std::source_location`.
struct fake_sl
{
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  std::string fil;
  char const *fun{""};
  std::uint_least32_t lin{0};
  std::uint_least32_t col{0};
  // NOLINTEND(misc-non-private-member-variables-in-classes)

  [[nodiscard]] constexpr char const *file_name() const noexcept
  {
    return fil.c_str();
  }
  [[nodiscard]] constexpr char const *function_name() const noexcept
  {
    return fun;
  }
  [[nodiscard]] constexpr std::uint_least32_t line() const noexcept
  {
    return lin;
  }
  [[nodiscard]] constexpr std::uint_least32_t column() const noexcept
  {
    return col;
  }
};


/// Return an arbitrary positive number.
std::uint_least32_t make_pos_num(pqxx::test::context &tctx)
{
  return static_cast<std::uint_least32_t>(tctx.make_num(9999) + 1);
}


void test_source_loc_handles_full_location(pqxx::test::context &tctx)
{
  std::string const func{make_function(tctx)};
  fake_sl const loc{
    .fil = make_filename(tctx),
    .fun = func.c_str(),
    .lin = make_pos_num(tctx),
    .col = make_pos_num(tctx),
  };

  PQXX_CHECK_EQUAL(
    pqxx::source_loc(loc),
    std::format("{}:{}:{}: ({})", loc.fil, loc.lin, loc.col, loc.fun));
}


void test_source_loc_handles_missing_column(pqxx::test::context &tctx)
{
  std::string const func{make_function(tctx)};
  fake_sl const loc{
    .fil = make_filename(tctx),
    .fun = func.c_str(),
    .lin = make_pos_num(tctx),
  };

  PQXX_CHECK_EQUAL(
    pqxx::source_loc(loc),
    std::format("{}:{}: ({})", loc.fil, loc.lin, loc.fun));
}


void test_source_loc_handles_missing_line(pqxx::test::context &tctx)
{
  std::string const func{make_function(tctx)};
  fake_sl const loc{
    .fil = make_filename(tctx),
    .fun = func.c_str(),
    .col = make_pos_num(tctx),
  };

  PQXX_CHECK_EQUAL(
    pqxx::source_loc(loc), std::format("{}: ({})", loc.fil, loc.fun));
}


void test_source_loc_handles_missing_function(pqxx::test::context &tctx)
{
  fake_sl const loc{
    .fil = make_filename(tctx),
    .lin = make_pos_num(tctx),
    .col = make_pos_num(tctx),
  };

  PQXX_CHECK_EQUAL(
    pqxx::source_loc(loc),
    std::format("{}:{}:{}:", loc.fil, loc.lin, loc.col));
}


void test_source_loc_handles_line_only(pqxx::test::context &tctx)
{
  fake_sl const loc{
    .fil = make_filename(tctx),
    .lin = make_pos_num(tctx),
  };
  PQXX_CHECK_EQUAL(
    pqxx::source_loc(loc), std::format("{}:{}:", loc.fil, loc.lin));
}


void test_source_loc_handles_column_only(pqxx::test::context &tctx)
{
  fake_sl const loc{.fil = make_filename(tctx), .col = make_pos_num(tctx)};
  // We don't bother printing a column number without a line number.
  PQXX_CHECK_EQUAL(pqxx::source_loc(loc), std::format("{}:", loc.fil));
}


void test_source_loc_handles_func_only(pqxx::test::context &tctx)
{
  std::string const func{make_function(tctx)};
  fake_sl const loc{.fil = make_filename(tctx), .fun = func.c_str()};

  PQXX_CHECK_EQUAL(
    pqxx::source_loc(loc), std::format("{}: ({})", loc.fil, loc.fun));
}


void test_source_loc_handles_minimal_source_location(pqxx::test::context &tctx)
{
  fake_sl const loc{.fil = make_filename(tctx)};
  PQXX_CHECK_EQUAL(pqxx::source_loc(loc), std::format("{}:", loc.fil));
}


PQXX_REGISTER_TEST(test_make_num);
PQXX_REGISTER_TEST(test_binary_cast);
PQXX_REGISTER_TEST(test_check_cast);
PQXX_REGISTER_TEST(test_source_loc_renders_real_source_location);
PQXX_REGISTER_TEST(test_source_loc_handles_full_location);
PQXX_REGISTER_TEST(test_source_loc_handles_missing_column);
PQXX_REGISTER_TEST(test_source_loc_handles_missing_line);
PQXX_REGISTER_TEST(test_source_loc_handles_missing_function);
PQXX_REGISTER_TEST(test_source_loc_handles_line_only);
PQXX_REGISTER_TEST(test_source_loc_handles_column_only);
PQXX_REGISTER_TEST(test_source_loc_handles_func_only);
PQXX_REGISTER_TEST(test_source_loc_handles_minimal_source_location);
} // namespace
