#include "test_helpers.hxx"

using namespace PGSTD;
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
}
}

PQXX_REGISTER_TEST_NODB(test_string_conversion)
