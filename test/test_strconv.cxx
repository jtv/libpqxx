#include <memory>
#include <optional>

#include <pqxx/transaction>

#include "test_helpers.hxx"

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
void test_strconv_bool()
{
  PQXX_CHECK_EQUAL(pqxx::to_string(false), "false", "Wrong to_string(false).");
  PQXX_CHECK_EQUAL(pqxx::to_string(true), "true", "Wrong to_string(true).");

  bool result;
  pqxx::from_string("false", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('false').");
  pqxx::from_string("FALSE", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('FALSE').");
  pqxx::from_string("f", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('f').");
  pqxx::from_string("F", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('F').");
  pqxx::from_string("0", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('0').");
  pqxx::from_string("true", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('true').");
  pqxx::from_string("TRUE", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('TRUE').");
  pqxx::from_string("t", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('t').");
  pqxx::from_string("T", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('T').");
  pqxx::from_string("1", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('1').");
}


void test_strconv_enum()
{
  PQXX_CHECK_EQUAL(pqxx::to_string(red), "0", "Enum value did not convert.");
  PQXX_CHECK_EQUAL(pqxx::to_string(green), "1", "Enum value did not convert.");
  PQXX_CHECK_EQUAL(pqxx::to_string(blue), "2", "Enum value did not convert.");

  colour col;
  pqxx::from_string("2", col);
  PQXX_CHECK_EQUAL(col, blue, "Could not recover enum value from string.");
}


void test_strconv_class_enum()
{
  PQXX_CHECK_EQUAL(
    pqxx::to_string(weather::hot), "0", "Class enum value did not convert.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(weather::wet), "2", "Enum value did not convert.");

  weather w;
  pqxx::from_string("2", w);
  PQXX_CHECK_EQUAL(
    w, weather::wet, "Could not recover class enum value from string.");

  PQXX_CHECK_EQUAL(
    pqxx::to_string(many::bottom), "0",
    "Small wide enum did not convert right.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(many::top),
    pqxx::to_string(std::numeric_limits<unsigned long long>::max()),
    "Large wide enum did not convert right.");

  pqxx::connection cx;
  pqxx::work tx{cx};
  std::tuple<weather> out;
  tx.exec("SELECT 0").one_row().to(out);
}


void test_strconv_optional()
{
  PQXX_CHECK_THROWS(
    pqxx::to_string(std::optional<int>{}), pqxx::conversion_error,
    "Converting an empty optional did not throw conversion error.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::optional<int>{std::in_place, 10}), "10",
    "std::optional<int> does not convert right.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::optional<int>{std::in_place, -10000}), "-10000",
    "std::optional<int> does not convert right.");
}


void test_strconv_smart_pointer()
{
  PQXX_CHECK_THROWS(
    pqxx::to_string(std::unique_ptr<int>{}), pqxx::conversion_error,
    "Converting an empty unique_ptr did not throw conversion error.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::make_unique<int>(10)), "10",
    "std::unique_ptr<int> does not convert right.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::make_unique<int>(-10000)), "-10000",
    "std::unique_ptr<int> does not convert right.");

  PQXX_CHECK_THROWS(
    pqxx::to_string(std::shared_ptr<int>{}), pqxx::conversion_error,
    "Converting an empty shared_ptr did not throw conversion error.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::make_shared<int>(10)), "10",
    "std::shared_ptr<int> does not convert right.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::make_shared<int>(-10000)), "-10000",
    "std::shared_ptr<int> does not convert right.");
}


// Cast a char to an unsigned int.
unsigned char_as_unsigned(char n)
{
  return static_cast<unsigned char>(n);
}


// Hash a std::size_t into a char.
constexpr char hash_index(std::size_t index)
{
  return static_cast<char>(static_cast<unsigned char>((index ^ 37) + (index >> 5)));
}


// Extra-thorough test for to_buf() on a given type.
template<typename T>
void test_to_buf_for(T const &value, std::string_view expected)
{
  std::string const name{pqxx::type_name<T>};
  std::array<char, 100> buf;
  for (auto i{0u}; i < std::size(buf); ++i) buf[i] = hash_index(i);

  pqxx::zview const out{pqxx::to_buf(buf, value)};
  PQXX_CHECK_EQUAL(std::size(out), std::size(expected), std::format("to_buf() for {} wrote wrong length.", name));

  auto const sz{std::size(out)};
  PQXX_CHECK_LESS_EQUAL(sz, pqxx::string_traits<T>::size_buffer(value), std::format("Under-budgeted for {}.", name));
  PQXX_CHECK_LESS(sz, std::size(buf), std::format("Too much data for {}.", name));

  PQXX_CHECK_EQUAL(expected, out, std::format("to_buf() for {} wrote wrong value.", name));
  if (sz > 0) PQXX_CHECK_NOT_EQUAL(char_as_unsigned(out.at(sz - 1)), 0u, "Terminating zero is inside result.");
  PQXX_CHECK_EQUAL(std::size(std::string_view{out}), sz, "Did not find terminating zero in the right place.");
  if (std::data(out) == std::data(buf))
    PQXX_CHECK_EQUAL(char_as_unsigned(buf.at(sz + 1)), char_as_unsigned(hash_index(sz + 1)), std::format("to_buf() for {} overwrote byte after terminating zero.", name));
}


void test_to_buf()
{
  test_to_buf_for(false, "false");
  test_to_buf_for(true, "true");

  test_to_buf_for(short{0}, "0");
  test_to_buf_for(short{1}, "1");
  test_to_buf_for(short{10}, "10");
  test_to_buf_for(short{99}, "99");
  test_to_buf_for(short{100}, "100");
  test_to_buf_for(short{999}, "999");
  test_to_buf_for(short{1000}, "1000");
  test_to_buf_for(short{9999}, "9999");
  test_to_buf_for(short{10000}, "10000");
  test_to_buf_for(short{32767}, "32767");
  test_to_buf_for(short{-1}, "-1");
  test_to_buf_for(short{-10}, "-10");
  test_to_buf_for(short{-99}, "-99");
  test_to_buf_for(short{-100}, "-100");
  test_to_buf_for(short{-999}, "-999");
  test_to_buf_for(short{-1000}, "-1000");
  test_to_buf_for(short{-9999}, "-9999");
  test_to_buf_for(short{-10000}, "-10000");
  test_to_buf_for(short{-32767}, "-32767");
  test_to_buf_for(short{-32768}, "-32768");

  test_to_buf_for(static_cast<unsigned short>(0), "0");
  test_to_buf_for(static_cast<unsigned short>(1), "1");
  test_to_buf_for(static_cast<unsigned short>(9), "9");
  test_to_buf_for(static_cast<unsigned short>(10), "10");
  test_to_buf_for(static_cast<unsigned short>(99), "99");
  test_to_buf_for(static_cast<unsigned short>(32767), "32767");
  test_to_buf_for(static_cast<unsigned short>(32768), "32768");
  test_to_buf_for(static_cast<unsigned short>(65535), "65535");

  test_to_buf_for(0, "0");
  test_to_buf_for(1, "1");
  test_to_buf_for(9, "9");
  test_to_buf_for(10, "10");
  test_to_buf_for(99, "99");
  test_to_buf_for(2147483647, "2147483647");
  test_to_buf_for(-1, "-1");
  test_to_buf_for(-9, "-9");
  test_to_buf_for(-10, "-10");
  test_to_buf_for(-99, "-99");
  test_to_buf_for(-2147483647, "-2147483647");
  test_to_buf_for(-2147483648, "-2147483648");

  test_to_buf_for(0u, "0");
  test_to_buf_for(1u, "1");
  test_to_buf_for(2147483647u, "2147483647");
  test_to_buf_for(4294967296u, "4294967296");

  test_to_buf_for(0l, "0");
  test_to_buf_for(1l, "1");
  test_to_buf_for(100000l, "100000");
  test_to_buf_for(2147483647l, "2147483647");
  test_to_buf_for(-1l, "-1");
  test_to_buf_for(-2147483647l, "-2147483647");
  test_to_buf_for(-2147483648l, "-2147483648");

  test_to_buf_for(0ul, "0");
  test_to_buf_for(1ul, "1");
  test_to_buf_for(2147483647ul, "2147483647");
  test_to_buf_for(4294967296ul, "4294967296");

  test_to_buf_for(0ll, "0");
  test_to_buf_for(1ll, "1");
  test_to_buf_for(100000ll, "100000");
  test_to_buf_for(2147483647ll, "2147483647");
  test_to_buf_for(-1ll, "-1");
  test_to_buf_for(-2147483647ll, "-2147483647");
  test_to_buf_for(-2147483648ll, "-2147483648");

  test_to_buf_for(0ull, "0");
  test_to_buf_for(1ull, "1");
  test_to_buf_for(2147483647ull, "2147483647");
  test_to_buf_for(4294967296ull, "4294967296");

  test_to_buf_for(0.0f, "0");
  test_to_buf_for(0.125f, "0.125");
  test_to_buf_for(1.0f, "1");
  test_to_buf_for(10000.0f, "10000");
  test_to_buf_for(-0.0f, "-0");
  test_to_buf_for(-0.125f, "-0.125");
  test_to_buf_for(-1.0f, "-1");
  test_to_buf_for(-10000.0f, "-10000");

  test_to_buf_for(0.0, "0");
  test_to_buf_for(0.125, "0.125");
  test_to_buf_for(1.0, "1");
  test_to_buf_for(10000.0, "10000");
  test_to_buf_for(-0.0, "-0");
  test_to_buf_for(-0.125, "-0.125");
  test_to_buf_for(-1.0, "-1");
  test_to_buf_for(-10000.0, "-10000");

  test_to_buf_for(0.0l, "0");
  test_to_buf_for(0.125l, "0.125");
  test_to_buf_for(1.0l, "1");
  test_to_buf_for(10000.0l, "10000");
  test_to_buf_for(-0.0l, "-0");
  test_to_buf_for(-0.125l, "-0.125");
  test_to_buf_for(-1.0l, "-1");
  test_to_buf_for(-10000.0l, "-10000");

  test_to_buf_for(std::optional<int>{37}, "37");

  test_to_buf_for(std::variant<int, unsigned long>{482}, "482");
  test_to_buf_for(std::variant<int, unsigned long>{777ul}, "777");

  test_to_buf_for(static_cast<char const *>(""), "");
  test_to_buf_for(static_cast<char const *>("Hello"), "Hello");

  std::array<char, 10> chars;
  chars.at(0) = '\0';
  for (auto i{1u}; i < std::size(chars); ++i) chars.at(i) = 'x';
  test_to_buf_for(std::data(chars), "");

  chars.at(0) = 'n';
  chars.at(1) = '\0';
  test_to_buf_for(std::data(chars), "n");

  test_to_buf_for("World", "World");
  test_to_buf_for("", "");

  test_to_buf_for(std::string{}, "");
  test_to_buf_for(std::string{"Blah"}, "Blah");

  test_to_buf_for(std::make_unique<std::string>("Boogie"), "Boogie");
  test_to_buf_for(std::make_shared<std::string>("Woogie"), "Woogie");

  // XXX: binary data
  // XXX: SQL arrays
  // XXX: Conversions outside of conversions.hxx.
}

// XXX: string_view
// XXX: zview

PQXX_REGISTER_TEST(test_strconv_bool);
PQXX_REGISTER_TEST(test_strconv_enum);
PQXX_REGISTER_TEST(test_strconv_class_enum);
PQXX_REGISTER_TEST(test_strconv_optional);
PQXX_REGISTER_TEST(test_strconv_smart_pointer);
PQXX_REGISTER_TEST(test_to_buf);
} // namespace
