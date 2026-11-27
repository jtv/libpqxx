#include <pqxx/types>
#include <pqxx/util>

#include "helpers.hxx"

namespace
{
void test_make_num(pqxx::test::context &tctx)
{
  bool same{true};
  int const last{tctx.make_num()};
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
  PQXX_CHECK(
    std::isinf(
      pqxx::check_cast<TO>(
        std::numeric_limits<FROM>::infinity(), "fail", here())));
  PQXX_CHECK(
    std::isinf(
      pqxx::check_cast<TO>(
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

  PQXX_CHECK_STR_CONTAINS(
    loc_text, "test_source_loc_renders_real_source_location");
  PQXX_CHECK_STR_CONTAINS(loc_text, __FILE__);
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

  // clang-tidy rule bug:
  // NOLINTNEXTLINE(cert-dcl03-c)
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


void test_version_check_requires_identical_abi(pqxx::test::context &)
{
  // (Baseline check)
  PQXX_CHECK_SUCCEEDS(
    pqxx::internal::check_libpqxx_version(
      pqxx::version_major, pqxx::version_minor, pqxx::version_patch,
      pqxx::version));

  PQXX_CHECK_THROWS(
    pqxx::internal::check_libpqxx_version(
      pqxx::version_major + 1, pqxx::version_minor, pqxx::version_patch,
      pqxx::version),
    pqxx::version_mismatch);
  PQXX_CHECK_THROWS(
    pqxx::internal::check_libpqxx_version(
      pqxx::version_major, pqxx::version_minor + 1, pqxx::version_patch,
      pqxx::version),
    pqxx::version_mismatch);
}


void test_version_check_accepts_patch_upgrade(pqxx::test::context &)
{
  // (Baseline check)
  PQXX_CHECK_SUCCEEDS(
    pqxx::internal::check_libpqxx_version(
      pqxx::version_major, pqxx::version_minor, pqxx::version_patch,
      pqxx::version));

  // We don't meddle with the version numbers stored inside the library binary,
  // meaning we can't test both the "special version" scenario and the
  // "regular" scenario.  We'll just check which kind of version we are, and
  // test the corresponding scenario.  In time, all shall be tested.
  if constexpr (pqxx::version_patch >= 0)
  {
    // We're a regular version.

    // Checking an older app build for the same ABI version succeeds.
    PQXX_CHECK_SUCCEEDS(
      pqxx::internal::check_libpqxx_version(
        pqxx::version_major, pqxx::version_minor, pqxx::version_patch - 1,
        pqxx::version));

    // Checking a newer app build for the same ABI version fails.
    //
    // In the special case where our patch level is zero, subtracting one will
    // simulate a special version, which should also fail.
    PQXX_CHECK_THROWS(
      pqxx::internal::check_libpqxx_version(
        pqxx::version_major, pqxx::version_minor, pqxx::version_patch + 1,
        pqxx::version),
      pqxx::version_mismatch);
  }
  else
  {
    // We're a special version.  Even if the application was compiled against
    // the oldest possible patch level, it fails.
    PQXX_CHECK_THROWS(
      pqxx::internal::check_libpqxx_version(
        pqxx::version_major, pqxx::version_minor, 0, pqxx::version),
      pqxx::version_mismatch);
  }
}


/// Verify `pqxx::internal::memmove()`: nonoverlapping, within one allocation.
constexpr bool check_memmove_moves_distinct_within_array()
{
  std::array<int, 10> all{10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
  auto const src{std::span<int>{all}.subspan(1, 3)};

  if ((__cplusplus > 202603L) or not std::is_constant_evaluated())
  {
    // This destination is too small for the copy.
    auto const tight_dst{std::span<int>{all}.subspan(5, 2)};
    PQXX_CHECK_THROWS(
      pqxx::internal::memmove(tight_dst, src), pqxx::internal_error);
    // The aborted memmove() did not make any changes.
    PQXX_CHECK_EQUAL(
      all, (std::array<int, 10>{10, 11, 12, 13, 14, 15, 16, 17, 18, 19}));
  }

  // This destination is just the right size.
  auto const dst{std::span<int>{all}.subspan(5, 3)};
  pqxx::internal::memmove(dst, src);
  // This copies src to dst.
  PQXX_CHECK_EQUAL(
    all, (std::array<int, 10>{10, 11, 12, 13, 14, 11, 12, 13, 18, 19}));

  // This destination is larger than needed.  Nevertheless, memmove() copies
  // only as many elements as are in src.
  auto const wide_dst{std::span<int>{all}.subspan(4, 4)};
  pqxx::internal::memmove(wide_dst, src);
  PQXX_CHECK_EQUAL(
    all, (std::array<int, 10>{10, 11, 12, 13, 11, 12, 13, 13, 18, 19}));

  // Just so we can run this in static_assert().
  return true;
}


/// @ref pqxx::internal::memmove() copies nonoverlapping blocks.
/** This test tests the case where the blocks are within the same allocation,
 * but do not overlap.  This is the easiest case.
 */
void test_memmove_moves_distinct_within_array(pqxx::test::context &)
{
  // We test this both at compile time and at run time; they get different
  // implementations.
  static_assert(check_memmove_moves_distinct_within_array());
  check_memmove_moves_distinct_within_array();
}


constexpr bool check_memmove_handles_overlap()
{
  std::array<int, 10> all{10, 11, 12, 13, 14, 15, 16, 17, 18, 19};

  // Moving in the easy direction: source and destination overlap, but
  // copying in index order will not cause trouble.
  pqxx::internal::memmove(
    std::span<int>{all}.subspan(1), std::span<int>{all}.subspan(2, 2));
  PQXX_CHECK_EQUAL(
    all, (std::array<int, 10>{10, 12, 13, 13, 14, 15, 16, 17, 18, 19}));

  // Moving in the difficult direction: writing to the destination in index
  // order would start overwriting source data that we hadn't copied yet.
  pqxx::internal::memmove(
    std::span<int>{all}.subspan(7), std::span<int>{all}.subspan(6, 2));
  PQXX_CHECK_EQUAL(
    all, (std::array<int, 10>{10, 12, 13, 13, 14, 15, 16, 16, 17, 19}));

  return true;
}


void test_memmove_handles_overlap(pqxx::test::context &)
{
  // We test this both at compile time and at run time; they get different
  // implementations.
  static_assert(check_memmove_handles_overlap());
  check_memmove_handles_overlap();
}


constexpr bool check_memmove_handles_separate_allocations()
{
  // There is a reason to test the case where source and destination are in
  // unrelated chunks of memory: pointer comparisons are crucial to memmove()
  // but the comparison operators are not defined for pointers from separate
  // allocations.
  std::array<int, 10> source{10, 11, 12, 13, 14, 15, 16, 17, 18, 19},
    destination{20, 21, 22, 23, 24, 25, 26, 27, 28, 29};
  pqxx::internal::memmove(
    std::span<int>{destination}.subspan(2),
    std::span<int>{source}.subspan(5, 3));

  // Source is unchanged.
  PQXX_CHECK_EQUAL(
    source, (std::array<int, 10>{10, 11, 12, 13, 14, 15, 16, 17, 18, 19}));
  // But some of its data has been copied into destination.
  PQXX_CHECK_EQUAL(
    destination,
    (std::array<int, 10>{20, 21, 15, 16, 17, 25, 26, 27, 28, 29}));

  return true;
}


void test_memmove_handles_separate_allocations(pqxx::test::context &)
{
  // We test this both at compile time and at run time; they get different
  // implementations.
  static_assert(check_memmove_handles_separate_allocations());
  check_memmove_handles_separate_allocations();
}


/// Compare two pointers, even if they're in separate allocations.
inline int cmp_pointers(char const *lhs, char const *rhs) noexcept
{
  constexpr auto const less{std::less<char const *>{}};
  if (less(lhs, rhs))
    return 1;
  else if (less(rhs, lhs))
    return -1;
  else
    return 0;
}


void test_string_store_stores_empty_strings(pqxx::test::context &)
{
  pqxx::internal::string_store strings;
  auto const empty1{strings.store("")};
  PQXX_CHECK_EQUAL(std::size(empty1), 0u);
  PQXX_CHECK_NOT_EQUAL(empty1.c_str(), nullptr);
  // Storing an empty string does in fact take up space: the terminating zero.
  PQXX_CHECK(*empty1.c_str() == '\0');

  auto const empty2{strings.store("")};
  PQXX_CHECK_EQUAL(std::size(empty2), 0u);
  PQXX_CHECK_NOT_EQUAL(empty1.c_str(), nullptr);
  PQXX_CHECK(*empty2.c_str() == '\0');
  PQXX_CHECK_EQUAL(cmp_pointers(empty1.c_str(), empty2.c_str()), 1);

  // Force the string_store to allocate a new block.  (Assuming it doesn't
  // check for duplicates or anything clever like that.)
  for (int i{0}; i < 1000; ++i)
  {
    auto const e{strings.store("")};
    PQXX_CHECK_EQUAL(std::size(e), 0u);
    PQXX_CHECK_NOT_EQUAL(e.c_str(), nullptr);
    PQXX_CHECK(*e.c_str() == '\0');
    PQXX_CHECK_NOT_EQUAL(cmp_pointers(e.c_str(), empty1.c_str()), 0);
    PQXX_CHECK_NOT_EQUAL(cmp_pointers(e.c_str(), empty2.c_str()), 0);
  }
}


void test_string_store_stores_strings(pqxx::test::context &tctx)
{
  std::vector<std::string> backup;
  pqxx::internal::string_store strings;
  std::vector<pqxx::zview> copies;

  {
    std::vector<std::string> inputs;

    // White-box testing: store more data than string_store can fit into a
    // single block.
    for (int i{0}; i < 500; ++i) inputs.push_back(tctx.make_name());

    // C++23: Use append_range().
    for (auto const &s : inputs) copies.push_back(strings.store(s));

    // The inputs can safely move to somewhere else, because the string_store
    // holds persistent copies.
    // C++23: Use append_range().
    for (auto const &s : inputs) backup.push_back(s);

    // Overwrite the original inputs, before that vector gets destroyed.
    for (std::size_t i{0}; i < std::size(inputs); ++i) inputs[i] = "gone";
  }

  // Without any connection to the original strings, the store still holds
  // the strings we gave it.
  PQXX_CHECK_EQUAL(std::size(backup), std::size(copies));
  for (std::size_t i{0}; i < std::size(backup); ++i)
    PQXX_CHECK_EQUAL(copies.at(i), backup.at(i));
}


void test_string_store_stores_various_sizes(pqxx::test::context &tctx)
{
  std::vector<std::string> inputs;
  pqxx::internal::string_store strings;

  // Some of these will be longer than the default storage block size.
  for (std::size_t i{0}; i < 1000; ++i)
    inputs.push_back(tctx.make_name("", tctx.make_num(1000)));

  std::vector<pqxx::zview> stored;
  for (auto const &s : inputs) stored.push_back(strings.store(s));

  for (std::size_t i{0}; i < 1000; ++i)
    PQXX_CHECK_EQUAL(stored.at(i), inputs.at(i));
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
PQXX_REGISTER_TEST(test_version_check_requires_identical_abi);
PQXX_REGISTER_TEST(test_version_check_accepts_patch_upgrade);
PQXX_REGISTER_TEST(test_memmove_moves_distinct_within_array);
PQXX_REGISTER_TEST(test_memmove_handles_overlap);
PQXX_REGISTER_TEST(test_memmove_handles_separate_allocations);
PQXX_REGISTER_TEST(test_string_store_stores_empty_strings);
PQXX_REGISTER_TEST(test_string_store_stores_strings);
PQXX_REGISTER_TEST(test_string_store_stores_various_sizes);
} // namespace
