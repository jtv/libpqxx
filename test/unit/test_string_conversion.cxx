#include <cstdint>

#include "../test_helpers.hxx"

using namespace pqxx;

// Some enums with string conversions.
enum EnumA { ea0, ea1, ea2 };
enum EnumB { eb0, eb1, eb2 };
namespace pqxx
{
PQXX_DECLARE_ENUM_CONVERSION(EnumA);
PQXX_DECLARE_ENUM_CONVERSION(EnumB);
}


namespace
{
void test_string_conversion()
{
  PQXX_CHECK_EQUAL(
	"C string array",
	to_string("C string array"),
	"C-style string constant does not convert to string properly.");

  char text_array[] = "C char array";
  PQXX_CHECK_EQUAL(
	"C char array",
	to_string(text_array),
	"C-style non-const char array does not convert to string properly.");

  const char *text_ptr = "C string pointer";
  PQXX_CHECK_EQUAL(
	"C string pointer",
	to_string(text_ptr),
	"C-style string pointer does not convert to string properly.");

  const std::string cxx_string = "C++ string";
  PQXX_CHECK_EQUAL(
	"C++ string",
	to_string(cxx_string),
	"C++-style string object does not convert to string properly.");

  PQXX_CHECK_EQUAL("0", to_string(0), "Zero does not convert right.");
  PQXX_CHECK_EQUAL("1", to_string(1), "Basic integer does not convert right.");
  PQXX_CHECK_EQUAL("-1", to_string(-1), "Negative numbers don't work.");
  PQXX_CHECK_EQUAL("9999", to_string(9999), "Larger numbers don't work.");
  PQXX_CHECK_EQUAL(
	"-9999",
	to_string(-9999),
	"Larger negative numbers don't work.");

  int x;
  from_string("0", x);
  PQXX_CHECK_EQUAL(0, x, "Zero does not parse right.");
  from_string("1", x);
  PQXX_CHECK_EQUAL(1, x, "Basic integer does not parse right.");
  from_string("-1", x);
  PQXX_CHECK_EQUAL(-1, x, "Negative numbers don't work.");
  from_string("9999", x);
  PQXX_CHECK_EQUAL(9999, x, "Larger numbers don't work.");
  from_string("-9999", x);
  PQXX_CHECK_EQUAL(-9999, x, "Larger negative numbers don't work.");

  // Bug #263 describes a case where this kind of overflow went undetected.
  if (sizeof(unsigned int) == 4)
  {
    std::uint32_t u;
    PQXX_CHECK_THROWS(
	from_string("4772185884", u),
	pqxx::conversion_error,
	"Overflow not detected.");
  }

  // We can convert to and from long double.  The implementation may fall
  // back on a thread-local std::stringstream.  Each call does its own
  // cleanup, so the conversion works multiple times.
  constexpr long double ld1 = 123456789.25, ld2 = 9876543210.5;
  constexpr char lds1[] = "123456789.25", lds2[] = "9876543210.5";
  PQXX_CHECK_EQUAL(
        to_string(ld1).substr(0, 12),
        lds1,
        "Wrong conversion from long double.");
  PQXX_CHECK_EQUAL(
        to_string(ld2).substr(0, 12),
        lds2,
        "Wrong value on repeated conversion from long double.");
  long double ldi1, ldi2;
  from_string(lds1, ldi1);
  PQXX_CHECK_BOUNDS(
        ldi1,
        ld1 - 0.00001,
        ld1 + 0.00001,
        "Wrong conversion to long double.");
  from_string(lds2, ldi2);
  PQXX_CHECK_BOUNDS(
        ldi2,
        ld2 - 0.00001,
        ld2 + 0.00001,
        "Wrong repeated conversion to long double.");

  // We can define string conversions for enums.
  PQXX_CHECK_EQUAL(
	to_string(ea0),
	"0",
	"Enum-to-string conversion is broken.");
  PQXX_CHECK_EQUAL(
	to_string(eb0),
	"0",
	"Enum-to-string conversion is inconsistent between enum types.");
  PQXX_CHECK_EQUAL(
	to_string(ea1),
	"1",
	"Enum-to-string conversion breaks for nonzero value.");

  EnumA ea;
  from_string("2", ea);
  PQXX_CHECK_EQUAL(ea, ea2, "String-to-enum conversion is broken.");
}


PQXX_REGISTER_TEST(test_string_conversion);
} // namespace
