#include "../test_helpers.hxx"

namespace pqxx
{
template<> struct nullness<row::const_iterator> : no_null<row::const_iterator>
{
};

template<> struct nullness<row::const_reverse_iterator> :
	no_null<const_reverse_row_iterator>
{
};


template<> struct string_traits<row::const_iterator>
{
  static constexpr zview text{"[row::const_iterator]"};
  static inline constexpr int buffer_budget = 0;
  static zview to_buf(char *, char *, const row::const_iterator &)
  { return text; }
  static char *into_buf(char *begin, char *end, const row::const_iterator &)
  {
    if ((end - begin) <= 30)
      throw conversion_overrun{"Not enough buffer for const row iterator."};
    std::memcpy(begin, text.c_str(), text.size() + 1);
    return begin + text.size();
  }
  static constexpr size_t size_buffer(const row::const_iterator &) noexcept
  { return text.size() + 1; }
};


template<> struct string_traits<row::const_reverse_iterator>
{
  static constexpr zview text{"[row::const_reverse_iterator]"};
  static inline constexpr int buffer_budget = 0;
  static pqxx::zview to_buf(
	char *, char *, const row::const_reverse_iterator &)
  { return text; }
  static char *into_buf(
	char *begin, char *end, const row::const_reverse_iterator &)
  {
    if ((end - begin) <= 30)
      throw conversion_overrun{"Not enough buffer for const row iterator."};
    std::memcpy(begin, text.c_str(), text.size() + 1);
    return begin + text.size();
  }
  static constexpr size_t
  size_buffer(const row::const_reverse_iterator &) noexcept
  { return 100; }
};
} // namespace pqxx

namespace
{
void test_result_slicing()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  auto r = tx.exec("SELECT 1");

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
	pqxx::argument_error,
	"Can access column name before slice.");
  PQXX_CHECK_THROWS(
	s.column_number("three"),
	pqxx::argument_error,
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
