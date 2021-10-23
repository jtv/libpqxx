#include <pqxx/time>

#include "../test_helpers.hxx"

namespace
{
#if defined(PQXX_HAVE_YEAR_MONTH_DAY)
using namespace std::literals;


void test_year_string_conversion()
{
  // C++20: Use y/m/d literals.
  // The check for min/max representable years is odd, but there's one big
  // advantage: if the range ever expands beyond a 16-bit signed integer, this
  // test will fail and tell us that our assumed range is no longer valid.
  std::tuple<int, std::string_view> const conversions[]{
    {-543, "-543"sv}, {-1, "-1"sv},     {0, "0"sv},
    {1, "1"sv},       {1971, "1971"sv}, {10191, "10191"sv},
    {int(std::chrono::year::min()), "-32767"},
    {int(std::chrono::year::max()), "32767"},
  };
  for (auto const &[num, text] : conversions)
  {
    std::chrono::year const y{num};
    PQXX_CHECK_EQUAL(pqxx::to_string(y), text, "Year did not convert right.");
    PQXX_CHECK_EQUAL(
      pqxx::from_string<std::chrono::year>(text), y,
      "Year did not parse right.");
  }

  std::string_view const invalid[]{
    ""sv,
    "-"sv,
    "+"sv,
    "1929-"sv,
    // According to cppreference.com, years are limited to 16-bit signed.
    "-32768"sv,
    "32768"sv,
  };
  for (auto const text : invalid)
    PQXX_CHECK_THROWS(
      pqxx::ignore_unused(pqxx::from_string<std::chrono::year>(text)),
      pqxx::conversion_error, "Invalid year parsed as if valid.");
}


// XXX: Similar for month.
// XXX: Similar for day.
// XXX: Similar for year_month_day.


PQXX_REGISTER_TEST(test_year_string_conversion);
#endif // PQXX_HAVE_YEAR_MONTH_DAY
} // namespace
