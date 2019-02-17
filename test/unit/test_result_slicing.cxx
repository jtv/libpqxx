#include "../test_helpers.hxx"

using namespace pqxx;

namespace pqxx
{
template<> struct PQXX_PRIVATE string_traits<row::const_iterator>
{
  static const char *name() { return "row::const_iterator"; }
  static bool has_null() { return false; }
  static bool is_null(row::const_iterator) { return false; }
  static std::string to_string(row::const_iterator)
	{ return "[row::const_iterator]"; }
};
template<>
struct PQXX_PRIVATE string_traits<row::const_reverse_iterator>
{
  static const char *name() { return "row::const_reverse_iterator"; }
  static bool has_null() { return false; }
  static bool is_null(row::const_reverse_iterator) { return false; }
  static std::string to_string(row::const_reverse_iterator)
	{ return "[row::const_reverse_iterator]"; }
};
}

namespace
{
void test_result_slicing()
{
  connection conn;
  work tx{conn};
  result r = tx.exec("SELECT 1");

  PQXX_CHECK(not r[0].empty(), "A plain row shows up as empty.");

  // Empty slice at beginning of row.
  pqxx::row s = r[0].slice(0, 0);
  PQXX_CHECK(s.empty(), "Empty slice does not show up as empty.");
  PQXX_CHECK_EQUAL(s.size(), 0u, "Slicing produces wrong row size.");
  PQXX_CHECK_EQUAL(s.begin(), s.end(), "Slice begin()/end() are broken.");
  PQXX_CHECK_EQUAL(s.rbegin(), s.rend(), "Slice rbegin()/rend() are broken.");

  PQXX_CHECK_THROWS(s.at(0), pqxx::range_error, "at() does not throw.");
  PQXX_CHECK_THROWS(r[0].slice(0, 2), pqxx::range_error, "No range check.");
  PQXX_CHECK_THROWS(r[0].slice(1, 0), pqxx::range_error, "Can reverse-slice.");

  // Empty slice at end of row.
  s = r[0].slice(1, 1);
  PQXX_CHECK(s.empty(), "empty() is broken.");
  PQXX_CHECK_EQUAL(s.size(), 0u, "size() is broken.");
  PQXX_CHECK_EQUAL(s.begin(), s.end(), "begin()/end() are broken.");
  PQXX_CHECK_EQUAL(s.rbegin(), s.rend(), "rbegin()/rend() are broken.");

  PQXX_CHECK_THROWS(s.at(0), pqxx::range_error, "at() is inconsistent.");

  // Slice that matches the entire row.
  s = r[0].slice(0, 1);
  PQXX_CHECK(not s.empty(), "Nonempty slice shows up as empty.");
  PQXX_CHECK_EQUAL(s.size(), 1u, "size() breaks for non-empty slice.");
  PQXX_CHECK_EQUAL(s.begin() + 1, s.end(), "Iteration is broken.");
  PQXX_CHECK_EQUAL(s.rbegin() + 1, s.rend(), "Reverse iteration is broken.");
  PQXX_CHECK_EQUAL(s.at(0).as<int>(), 1, "Accessing a slice is broken.");
  PQXX_CHECK_EQUAL(s[0].as<int>(), 1, "operator[] is broken.");
  PQXX_CHECK_THROWS(s.at(1).as<int>(), pqxx::range_error, "at() is off.");

  // Meaningful slice at beginning of row.
  r = tx.exec("SELECT 1, 2, 3");
  s = r[0].slice(0, 1);
  PQXX_CHECK(not s.empty(), "Slicing confuses empty().");
  PQXX_CHECK_THROWS(
	s.at(1).as<int>(),
	pqxx::range_error,
	"at() does not enforce slice.");

  // Meaningful slice that skips an initial column.
  s = r[0].slice(1, 2);
  PQXX_CHECK(not s.empty(), "Slicing away leading columns confuses empty().");
  PQXX_CHECK_EQUAL(s[0].as<int>(), 2, "Slicing offset is broken.");
  PQXX_CHECK_EQUAL(s.begin()->as<int>(), 2, "Iteration uses wrong offset.");
  PQXX_CHECK_EQUAL(s.begin() + 1, s.end(), "Iteration has wrong range.");
  PQXX_CHECK_EQUAL(
	s.rbegin() + 1,
	s.rend(),
	"Reverse iteration has wrong range.");
  PQXX_CHECK_THROWS(
	s.at(1).as<int>(),
	pqxx::range_error,
	"Offset slicing is broken.");

  // Column names in a slice.
  r = tx.exec("SELECT 1 AS one, 2 AS two, 3 AS three");
  s = r[0].slice(1, 2);
  PQXX_CHECK_EQUAL(s["two"].as<int>(), 2, "Column addressing breaks.");
  PQXX_CHECK_THROWS(
	s.column_number("one"),
	argument_error,
	"Can access column name before slice.");
  PQXX_CHECK_THROWS(
	s.column_number("three"),
	argument_error,
	"Can access column name after slice.");
  PQXX_CHECK_EQUAL(
	s.column_number("Two"),
	0u,
	"Column name is case sensitive.");

  // Identical column names.
  r = tx.exec("SELECT 1 AS x, 2 AS x");
  s = r[0].slice(1, 2);
  PQXX_CHECK_EQUAL(s["x"].as<int>(), 2, "Identical column names break slice.");
}


PQXX_REGISTER_TEST(test_result_slicing);
} // namespace
