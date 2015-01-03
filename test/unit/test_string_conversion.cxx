#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;

namespace
{
void test_string_conversion(transaction_base &)
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

  const string cxx_string = "C++ string";
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
    unsigned int u;
    PQXX_CHECK_THROWS(
	from_string("4772185884", u),
	pqxx::failure,
	"Overflow not detected.");
  }
}
}

PQXX_REGISTER_TEST_NODB(test_string_conversion)
