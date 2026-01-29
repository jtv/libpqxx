#include <cstdint>
#include <variant>

#include <pqxx/connection>
#include <pqxx/transaction>

#include "helpers.hxx"

using namespace std::literals;


// Some enums with string conversions.
enum EnumA
{
  ea0,
  ea1,
  ea2
};
enum EnumB
{
  eb0,
  eb1,
  eb2
};
namespace pqxx
{
PQXX_DECLARE_ENUM_CONVERSION(EnumA);
PQXX_DECLARE_ENUM_CONVERSION(EnumB);
} // namespace pqxx


namespace
{
// "A minimal difference."
constexpr double thres{0.00001};


void test_string_conversion(pqxx::test::context &tctx)
{
  PQXX_CHECK_EQUAL("C string array", pqxx::to_string("C string array"));

  char const text_array[]{"C char array"};
  PQXX_CHECK_EQUAL("C char array", pqxx::to_string(text_array));

  char const *text_ptr{"C string pointer"};
  PQXX_CHECK_EQUAL("C string pointer", pqxx::to_string(text_ptr));

  std::string const cxx_string{"C++ string"};
  PQXX_CHECK_EQUAL("C++ string", pqxx::to_string(cxx_string));

  PQXX_CHECK_EQUAL("0", pqxx::to_string(0));
  PQXX_CHECK_EQUAL("1", pqxx::to_string(1));
  PQXX_CHECK_EQUAL("-1", pqxx::to_string(-1));
  PQXX_CHECK_EQUAL("9999", pqxx::to_string(9999));
  PQXX_CHECK_EQUAL("-9999", pqxx::to_string(-9999));

  int x{};
  pqxx::from_string("0", x);
  PQXX_CHECK_EQUAL(0, x);
  pqxx::from_string("1", x);
  PQXX_CHECK_EQUAL(1, x);
  pqxx::from_string("-1", x);
  PQXX_CHECK_EQUAL(-1, x);
  pqxx::from_string("9999", x);
  PQXX_CHECK_EQUAL(9999, x);
  pqxx::from_string("-9999", x);
  PQXX_CHECK_EQUAL(-9999, x);

  auto const num{tctx.make_num()};
  PQXX_CHECK_EQUAL(pqxx::to_string(num), std::format("{}", num));
  auto const numstr{std::format("{}", num)};
  PQXX_CHECK_EQUAL(pqxx::to_string(num), pqxx::to_string(numstr));
  PQXX_CHECK_EQUAL(
    pqxx::to_string(num), pqxx::to_string(std::string_view{numstr}));
  PQXX_CHECK_EQUAL(pqxx::from_string<int>(numstr), num);

  // Bug #263 describes a case where this kind of overflow went undetected.
  if constexpr (sizeof(unsigned int) == 4)
  {
    std::uint32_t u{};
    PQXX_CHECK_THROWS(
      pqxx::from_string("4772185884", u), pqxx::conversion_error,
      "Overflow not detected.");
  }

  // Valgrind doesn't seem to support long double, so skip "long double"
  // tests when compiling for a Valgrind run.  :-/
#if !defined(PQXX_VALGRIND)
  // We can convert to and from long double.  The implementation may fall
  // back on a thread-local std::stringstream.  Each call does its own
  // cleanup, so the conversion works multiple times.
  constexpr long double ld1{123456789.25}, ld2{9876543210.5};
  constexpr char lds1[]{"123456789.25"}, lds2[]{"9876543210.5"};
  PQXX_CHECK_EQUAL(pqxx::to_string(ld1).substr(0, 12), lds1);
  PQXX_CHECK_EQUAL(pqxx::to_string(ld2).substr(0, 12), lds2);
  long double ldi1{}, ldi2{};
  pqxx::from_string(std::data(lds1), ldi1);
  PQXX_CHECK_BOUNDS(ldi1, ld1 - thres, ld1 + thres);
  pqxx::from_string(std::data(lds2), ldi2);
  PQXX_CHECK_BOUNDS(ldi2, ld2 - thres, ld2 + thres);
#endif

  // We can define string conversions for enums.
  PQXX_CHECK_EQUAL(pqxx::to_string(ea0), "0");
  PQXX_CHECK_EQUAL(pqxx::to_string(eb0), "0");
  PQXX_CHECK_EQUAL(pqxx::to_string(ea1), "1");

  EnumA ea{};
  pqxx::from_string("2", ea);
  PQXX_CHECK_EQUAL(ea, ea2);
}


void test_convert_variant_to_string(pqxx::test::context &)
{
  PQXX_CHECK_EQUAL(pqxx::to_string(std::variant<int, std::string>{99}), "99");

  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::variant<int, std::string>{"Text"}), "Text");
}


void test_integer_conversion(pqxx::test::context &)
{
  PQXX_CHECK_EQUAL(pqxx::from_string<int>("12"), 12);
  PQXX_CHECK_EQUAL(pqxx::from_string<int>(" 12"), 12);
  PQXX_CHECK_THROWS(
    std::ignore = pqxx::from_string<int>(""), pqxx::conversion_error);
  PQXX_CHECK_THROWS(
    std::ignore = pqxx::from_string<int>(" "), pqxx::conversion_error);
  PQXX_CHECK_EQUAL(pqxx::from_string<int>("-6"), -6);
  PQXX_CHECK_THROWS(
    std::ignore = pqxx::from_string<int>("- 3"), pqxx::conversion_error);
  PQXX_CHECK_THROWS(
    std::ignore = pqxx::from_string<int>("-"), pqxx::conversion_error);
}


void test_convert_null(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work const tx{cx};
  PQXX_CHECK_EQUAL(tx.quote(nullptr), "NULL");
  PQXX_CHECK_EQUAL(tx.quote(std::nullopt), "NULL");
  PQXX_CHECK_EQUAL(tx.quote(std::monostate{}), "NULL");
}


void test_string_view_conversion(pqxx::test::context &)
{
  using traits = pqxx::string_traits<std::string_view>;

  PQXX_CHECK_EQUAL(pqxx::to_string("view here"sv), "view here"s);

  std::array<char, 200> buf{};

  std::size_t const stop{pqxx::into_buf(buf, "more view"sv)};
  PQXX_CHECK_LESS(stop, std::size(buf));
  assert(stop > 0);
  PQXX_CHECK_EQUAL(
    (std::string{std::data(buf), static_cast<std::size_t>(stop)}),
    "more view"s);
  PQXX_CHECK(buf.at(stop - 1) == 'w');

  std::string_view const org{"another!"sv}, out{traits::to_buf(buf, org)};
  PQXX_CHECK_EQUAL(std::string{out}, "another!"s);
}


void test_binary_converts_to_string(pqxx::test::context &)
{
  std::array<std::byte, 3> const bin_data{
    std::byte{0x41}, std::byte{0x42}, std::byte{0x43}};
  std::string_view const text_data{"\\x414243"};
  PQXX_CHECK_EQUAL(pqxx::to_string(bin_data), text_data);

  std::array<std::byte, 1> x{std::byte{0x78}};
  PQXX_CHECK_EQUAL(std::size(x), 1u);
  std::span<std::byte> const span{x};
  PQXX_CHECK_EQUAL(std::size(span), 1u);
  PQXX_CHECK_EQUAL(pqxx::to_string(span), "\\x78");
}


void test_string_converts_to_binary(pqxx::test::context &)
{
  std::array<std::byte, 3> const bin_data{
    std::byte{0x41}, std::byte{0x42}, std::byte{0x43}};
  std::string_view const text_data{"\\x414243"};

  // We can convert a bytea SQL string to a vector of bytes.
  auto const vec{pqxx::from_string<std::vector<std::byte>>(text_data)};
  PQXX_CHECK_EQUAL(std::size(vec), std::size(bin_data));
  for (std::size_t i{0}; i < std::size(vec); ++i)
    PQXX_CHECK(
      vec.at(i) == bin_data.at(i),
      std::format("Difference in binary byte #{}.", i));

  // We can also convert a bytea SQL string to an array of bytes of the right
  // size.
  auto const arr{pqxx::from_string<std::array<std::byte, 3>>(text_data)};
  for (std::size_t i{0}; i < std::size(arr); ++i)
    PQXX_CHECK(
      arr.at(i) == bin_data.at(i),
      std::format("Difference in binary byte #{}.", i));

  // However we can _not_ convert a bytea SQL string to an array of bytes of a
  // different size.
  PQXX_CHECK_THROWS(
    (std::ignore = pqxx::from_string<std::array<std::byte, 4>>(text_data)),
    pqxx::conversion_error);
}


struct legacy_item final
{
  constexpr explicit legacy_item(int i) noexcept : val{i} {}
  [[nodiscard]] constexpr int get_val() const noexcept { return val; }

private:
  int val;
};
} // namespace


namespace pqxx
{
template<> struct nullness<legacy_item> final : no_null<legacy_item>
{};

template<> struct string_traits<legacy_item> final
{
  static inline zview
  to_buf(char *begin, char const *end, legacy_item const &value)
  {
    auto const bufsz{static_cast<std::size_t>(end - begin)},
      need{size_buffer(value)};
    if (std::cmp_less(bufsz, need))
      throw conversion_overrun{std::format(
        "Needed {} bytes to convert '{}', got {}.", need, value.get_val(),
        bufsz)};

    auto const outsz{pqxx::into_buf({begin, bufsz}, value.get_val())};
    if (outsz >= bufsz)
      throw conversion_error{"No room for legacy terminating zero."};
    begin[outsz] = '\0';
    return zview{begin, outsz};
  }

  static inline legacy_item from_string(std::string_view text)
  {
    return legacy_item{pqxx::from_string<int>(text)};
  }

  static inline std::size_t size_buffer(legacy_item const &value) noexcept
  {
    return pqxx::size_buffer(value.get_val()) + 1;
  }
};
} // namespace pqxx


namespace
{
void test_legacy_7_conversion_support(pqxx::test::context &tctx)
{
  legacy_item const leg{tctx.make_num()};
  auto const as_string{pqxx::to_string(leg)};
  PQXX_CHECK_EQUAL(as_string, pqxx::to_string(leg.get_val()));
  PQXX_CHECK_EQUAL(
    pqxx::from_string<legacy_item>(as_string).get_val(), leg.get_val());
}


PQXX_REGISTER_TEST(test_string_conversion);
PQXX_REGISTER_TEST(test_convert_variant_to_string);
PQXX_REGISTER_TEST(test_integer_conversion);
PQXX_REGISTER_TEST(test_convert_null);
PQXX_REGISTER_TEST(test_string_view_conversion);
PQXX_REGISTER_TEST(test_binary_converts_to_string);
PQXX_REGISTER_TEST(test_string_converts_to_binary);
PQXX_REGISTER_TEST(test_legacy_7_conversion_support);
} // namespace
