#include <memory>
#include <optional>

#include <pqxx/range>
#include <pqxx/time>
#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
enum colour
{
  red,
  green,
  blue
};
enum class weather : short
{
  hot,
  cold,
  wet
};
enum class many : unsigned long long
{
  bottom = 0,
  top = std::numeric_limits<unsigned long long>::max(),
};
} // namespace

namespace pqxx
{
PQXX_DECLARE_ENUM_CONVERSION(colour);
PQXX_DECLARE_ENUM_CONVERSION(weather);
PQXX_DECLARE_ENUM_CONVERSION(many);
} // namespace pqxx


namespace
{
using namespace std::string_view_literals;


void test_strconv_bool()
{
  PQXX_CHECK_EQUAL(pqxx::to_string(false), "false", "Wrong to_string(false).");
  PQXX_CHECK_EQUAL(pqxx::to_string(true), "true", "Wrong to_string(true).");

  bool result{};
  pqxx::from_string("false", result);
  PQXX_CHECK_EQUAL(result, false);
  pqxx::from_string("FALSE", result);
  PQXX_CHECK_EQUAL(result, false);
  pqxx::from_string("f", result);
  PQXX_CHECK_EQUAL(result, false);
  pqxx::from_string("F", result);
  PQXX_CHECK_EQUAL(result, false);
  pqxx::from_string("0", result);
  PQXX_CHECK_EQUAL(result, false);
  pqxx::from_string("true", result);
  PQXX_CHECK_EQUAL(result, true);
  pqxx::from_string("TRUE", result);
  PQXX_CHECK_EQUAL(result, true);
  pqxx::from_string("t", result);
  PQXX_CHECK_EQUAL(result, true);
  pqxx::from_string("T", result);
  PQXX_CHECK_EQUAL(result, true);
  pqxx::from_string("1", result);
  PQXX_CHECK_EQUAL(result, true);

  // Nasty little corner case: to_buf() for bool will return a view on a
  // string constant, and not use the buffer you give it.  But into_buf() will
  // copy that into the buffer, and this requires a separate overrun check.
  std::array<char, 3> small_buf{};
  PQXX_CHECK_THROWS(
    std::ignore = pqxx::into_buf(small_buf, true), pqxx::conversion_overrun);
}


void test_strconv_enum()
{
  PQXX_CHECK_EQUAL(pqxx::to_string(red), "0");
  PQXX_CHECK_EQUAL(pqxx::to_string(green), "1");
  PQXX_CHECK_EQUAL(pqxx::to_string(blue), "2");

  colour col{};
  pqxx::from_string("2", col);
  PQXX_CHECK_EQUAL(col, blue);
}


void test_strconv_class_enum()
{
  PQXX_CHECK_EQUAL(pqxx::to_string(weather::hot), "0");
  PQXX_CHECK_EQUAL(pqxx::to_string(weather::wet), "2");

  weather w{};
  pqxx::from_string("2", w);
  PQXX_CHECK_EQUAL(w, weather::wet);

  PQXX_CHECK_EQUAL(pqxx::to_string(many::bottom), "0");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(many::top),
    pqxx::to_string(std::numeric_limits<unsigned long long>::max()));

  pqxx::connection cx;
  pqxx::work tx{cx};
  std::tuple<weather> out;
  tx.exec("SELECT 0").one_row().to(out);
}


void test_strconv_optional()
{
  PQXX_CHECK_THROWS(
    pqxx::to_string(std::optional<int>{}), pqxx::conversion_error);
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::optional<int>{std::in_place, 10}), "10");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::optional<int>{std::in_place, -10000}), "-10000");
}


void test_strconv_smart_pointer()
{
  PQXX_CHECK_THROWS(
    pqxx::to_string(std::unique_ptr<int>{}), pqxx::conversion_error);
  PQXX_CHECK_EQUAL(pqxx::to_string(std::make_unique<int>(10)), "10");
  PQXX_CHECK_EQUAL(pqxx::to_string(std::make_unique<int>(-10000)), "-10000");

  PQXX_CHECK_THROWS(
    pqxx::to_string(std::shared_ptr<int>{}), pqxx::conversion_error);
  PQXX_CHECK_EQUAL(pqxx::to_string(std::make_shared<int>(10)), "10");
  PQXX_CHECK_EQUAL(pqxx::to_string(std::make_shared<int>(-10000)), "-10000");
}


// Cast a char to an unsigned int.
unsigned char_as_unsigned(char n)
{
  return static_cast<unsigned char>(n);
}


// Hash a std::size_t into a char.
constexpr char hash_index(std::size_t index)
{
  return static_cast<char>(
    static_cast<unsigned char>((index ^ 37) + (index >> 5)));
}


// Extra-thorough test for to_buf() & into_buf() on a given type.
template<typename T>
void check_write(
  T const &value, std::string_view expected,
  pqxx::sl loc = pqxx::sl::current())
{
  std::string_view const name{pqxx::name_type<T>()};
  std::array<char, 1000> buf{};
  for (auto i{0u}; i < std::size(buf); ++i) buf.at(i) = hash_index(i);

  pqxx::conversion_context const c{pqxx::encoding_group::monobyte};

  // Test to_buf().
  pqxx::zview const out{pqxx::to_buf(buf, value, c)};
  PQXX_CHECK_EQUAL(
    std::size(out), std::size(expected),
    std::format("to_buf() for {} wrote wrong length.", name), loc);

  auto const sz{std::size(out)};
  PQXX_CHECK_LESS_EQUAL(
    sz, pqxx::string_traits<T>::size_buffer(value),
    std::format("Under-budgeted for to_buf on {}.", name), loc);
  PQXX_CHECK_LESS(
    sz, std::size(buf), std::format("Too much to_buf() data for {}.", name),
    loc);

  PQXX_CHECK_EQUAL(
    expected, out, std::format("to_buf() for {} wrote wrong value.", name),
    loc);
  if (sz > 0)
    PQXX_CHECK_NOT_EQUAL(
      char_as_unsigned(out.at(sz - 1)), 0u,
      std::format("to_buf() for {} put terminating zero inside result.", name),
      loc);

  // Test into_buf().
  for (auto i{0u}; i < std::size(buf); ++i) buf.at(i) = hash_index(i);
  std::size_t const end{pqxx::into_buf(buf, value, c)};
  PQXX_CHECK_LESS_EQUAL(
    end, std::size(buf),
    std::format("into_buf() for {} overran buffer.", name), loc);
  PQXX_CHECK_LESS_EQUAL(
    end, pqxx::string_traits<T>::size_buffer(value),
    std::format("Under-budgeted for into_buf() on {}.", name), loc);
  PQXX_CHECK_EQUAL(
    (std::string_view{std::data(buf), end}), expected,
    std::format("Wrong result from into_buf() on {}.", name), loc);
}


void test_to_buf_into_buf()
{
  check_write(false, "false");
  check_write(true, "true");

  check_write(short{0}, "0");
  check_write(short{1}, "1");
  check_write(short{10}, "10");
  check_write(short{99}, "99");
  check_write(short{100}, "100");
  check_write(short{999}, "999");
  check_write(short{1000}, "1000");
  check_write(short{9999}, "9999");
  check_write(short{10000}, "10000");
  check_write(short{32767}, "32767");
  check_write(short{-1}, "-1");
  check_write(short{-10}, "-10");
  check_write(short{-99}, "-99");
  check_write(short{-100}, "-100");
  check_write(short{-999}, "-999");
  check_write(short{-1000}, "-1000");
  check_write(short{-9999}, "-9999");
  check_write(short{-10000}, "-10000");
  check_write(short{-32767}, "-32767");
  check_write(short{-32768}, "-32768");

  check_write(static_cast<unsigned short>(0), "0");
  check_write(static_cast<unsigned short>(1), "1");
  check_write(static_cast<unsigned short>(9), "9");
  check_write(static_cast<unsigned short>(10), "10");
  check_write(static_cast<unsigned short>(99), "99");
  check_write(static_cast<unsigned short>(32767), "32767");
  check_write(static_cast<unsigned short>(32768), "32768");
  check_write(static_cast<unsigned short>(65535), "65535");

  check_write(0, "0");
  check_write(1, "1");
  check_write(9, "9");
  check_write(10, "10");
  check_write(99, "99");
  check_write(2147483647, "2147483647");
  check_write(-1, "-1");
  check_write(-9, "-9");
  check_write(-10, "-10");
  check_write(-99, "-99");
  check_write(-2147483647, "-2147483647");
  check_write(-2147483648, "-2147483648");

  check_write(0u, "0");
  check_write(1u, "1");
  check_write(2147483647u, "2147483647");
  check_write(4294967296u, "4294967296");

  check_write(0L, "0");
  check_write(1L, "1");
  check_write(100000L, "100000");
  check_write(2147483647L, "2147483647");
  check_write(-1L, "-1");
  check_write(-2147483647L, "-2147483647");
  check_write(-2147483648L, "-2147483648");

  check_write(0uL, "0");
  check_write(1uL, "1");
  check_write(2147483647uL, "2147483647");
  check_write(4294967296uL, "4294967296");

  check_write(0LL, "0");
  check_write(1LL, "1");
  check_write(100000LL, "100000");
  check_write(2147483647LL, "2147483647");
  check_write(-1LL, "-1");
  check_write(-2147483647LL, "-2147483647");
  check_write(-2147483648LL, "-2147483648");

  check_write(0ULL, "0");
  check_write(1ULL, "1");
  check_write(2147483647ULL, "2147483647");
  check_write(4294967296ULL, "4294967296");

  check_write(0.0f, "0");
  check_write(0.125f, "0.125");
  check_write(1.0f, "1");
  check_write(10000.0f, "10000");
  check_write(-0.0f, "-0");
  check_write(-0.125f, "-0.125");
  check_write(-1.0f, "-1");
  check_write(-10000.0f, "-10000");

  check_write(0.0, "0");
  check_write(0.125, "0.125");
  check_write(1.0, "1");
  check_write(10000.0, "10000");
  check_write(-0.0, "-0");
  check_write(-0.125, "-0.125");
  check_write(-1.0, "-1");
  check_write(-10000.0, "-10000");

  check_write(0.0L, "0");
  check_write(0.125L, "0.125");
  check_write(1.0L, "1");
  check_write(10000.0L, "10000");
  check_write(-0.0L, "-0");
  check_write(-0.125L, "-0.125");
  check_write(-1.0L, "-1");
  check_write(-10000.0L, "-10000");

  check_write(std::optional<int>{37}, "37");

  check_write(std::variant<int, unsigned long>{482}, "482");
  check_write(std::variant<int, unsigned long>{777UL}, "777");

  check_write(static_cast<char const *>(""), "");
  check_write(static_cast<char const *>("Hello"), "Hello");

  std::array<char, 10> chars{};
  chars.at(0) = '\0';
  for (auto i{1u}; i < std::size(chars); ++i) chars.at(i) = 'x';
  check_write(std::data(chars), "");

  chars.at(0) = 'n';
  chars.at(1) = '\0';
  check_write(std::data(chars), "n");

  check_write("World", "World");
  check_write("", "");

  check_write(std::string{}, "");
  check_write(std::string{"Blah"}, "Blah");

  check_write(""sv, "");
  // NOLINTNEXTLINE(bugprone-string-constructor)
  check_write(std::string_view{"abc", 0u}, "");
  check_write("view"sv, "view");
  check_write(std::string_view{"viewport", 4u}, "view");

  check_write(""sv, "");
  check_write(pqxx::zview{"xyz", 0u}, "");

  check_write(std::make_unique<std::string>("Boogie"), "Boogie");
  check_write(std::make_shared<std::string>("Woogie"), "Woogie");

  check_write(std::vector<int>{}, "{}");
  check_write(std::array<int, 3>{10, 9, 8}, "{10,9,8}");
  check_write(std::vector<int>{3, 2, 1}, "{3,2,1}");
  check_write(std::vector<std::string>{}, "{}");

  // TODO: Use raw strings once Visual Studio copes with backslashes there.

  // NOLINTBEGIN(modernize-raw-string-literal)
  check_write(std::vector<std::string>{"eins", "zwo"}, "{\"eins\",\"zwo\"}");
  check_write(std::vector<std::string>{"x,y", "z"}, "{\"x,y\",\"z\"}");
  // NOLINTEND(modernize-raw-string-literal)
  check_write(std::list<std::string_view>{"foo"}, "{\"foo\"}");

  check_write(
    std::chrono::year_month_day{
      std::chrono::year{2025}, std::chrono::month{03}, std::chrono::day{01}},
    "2025-03-01");

  check_write(
    pqxx::range<int>{
      pqxx::inclusive_bound<int>{9}, pqxx::inclusive_bound<int>{17}},
    "[9,17]");
  check_write(
    pqxx::range<int>{
      pqxx::exclusive_bound<int>{0}, pqxx::exclusive_bound<int>{10}},
    "(0,10)");
  check_write(pqxx::range<int>{pqxx::no_bound{}, pqxx::no_bound{}}, "(,)");

  check_write(std::vector<std::byte>{}, "\\x");
  check_write(std::vector<std::byte>{std::byte{0x61}}, "\\x61");
  check_write(
    std::array<std::byte, 2>{std::byte{'a'}, std::byte{'b'}}, "\\x6162");
}


void test_to_buf_multi()
{
  std::vector<char> buf{};
  buf.resize(50);
  auto strings{pqxx::to_buf_multi(buf, "foo", -1025, "bar", 3ul, "zarg")};
  PQXX_CHECK_EQUAL(std::size(strings), 5u);
  PQXX_CHECK_EQUAL(strings.at(0), "foo");
  PQXX_CHECK_EQUAL(strings.at(1), "-1025");
  PQXX_CHECK_EQUAL(strings.at(2), "bar");
  PQXX_CHECK_EQUAL(strings.at(3), "3");
  PQXX_CHECK_EQUAL(strings.at(4), "zarg");

  // The strings start right at the beginning of buf.
  PQXX_CHECK(std::data(strings.at(0)) == std::data(buf));

  // The strings are packed tightly together.
  for (std::size_t i{1}; i < std::size(strings); ++i)
    PQXX_CHECK(
      std::data(strings.at(i)) ==
      std::data(strings.at(i - 1)) + std::size(strings.at(i - 1)));
}


PQXX_REGISTER_TEST(test_strconv_bool);
PQXX_REGISTER_TEST(test_strconv_enum);
PQXX_REGISTER_TEST(test_strconv_class_enum);
PQXX_REGISTER_TEST(test_strconv_optional);
PQXX_REGISTER_TEST(test_strconv_smart_pointer);
PQXX_REGISTER_TEST(test_to_buf_into_buf);
PQXX_REGISTER_TEST(test_to_buf_multi);
} // namespace
