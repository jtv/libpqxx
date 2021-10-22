#include <pqxx/time>

#include "../test_helpers.hxx"

namespace
{
#if defined(PQXX_HAVE_YEAR_MONTH_DAY)
using namespace std::literals;


void test_year_string_conversion()
{
  // C++20: Use y/m/d literals.
  auto const conversions{
    {-543, "-543"sv}, {-1, "-1"sv},     {0, "0"sv},
    {1, "1"sv},       {1971, "1971"sv}, {10191, "10191"sv},
  };
  for (auto const [num, text] : conversions)
  {
    std::chrono::year const y{num};
    PQXX_CHECK_EQUAL(pqxx::to_string(y), text, "Year did not convert right.");
    PQXX_CHECK_EQUAL(
      pqxx::from_string<std::chrono::year>(text), y,
      "Year did not parse right.");
  }

  auto const invalid{
    ""sv,
    "-"sv,
    "+"sv,
    "1929-"sv,
  };
  for (auto const text : invalid)
    PQXX_CHECK_THROWS(
      ignore_unused(from_string<std::chrono::year>(text)),
      pqxx::conversion_error, "Invalid year parsed as if valid.");
}


// XXX: Similar for month.
// XXX: Similar for day.
// XXX: Similar for year_month_day.


PQXX_REGISTER_TEST(test_year_string_conversion);
#endif // PQXX_HAVE_YEAR_MONTH_DAY
} // namespace
