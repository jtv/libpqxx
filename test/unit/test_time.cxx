#include <pqxx/time>
#include <pqxx/transaction>

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
    {-543, "0544 BC"sv},
    {-1, "0002 BC"sv},
    {0, "0001 BC"sv},
    {1, "0001"sv},
    {1971, "1971"sv},
    {10191, "10191"sv},
    {int(std::chrono::year::min()), "32768 BC"},
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
    ""sv,      "-"sv,  "+"sv, "1929-"sv, "-32768"sv, "32768"sv, "x"sv,
    "2001y"sv, "10"sv, "3"sv, "-1999"sv, "0"sv,      "0000"sv,
  };
  for (auto const text : invalid)
    PQXX_CHECK_THROWS(
      pqxx::ignore_unused(pqxx::from_string<std::chrono::year>(text)),
      pqxx::conversion_error, "Invalid year parsed as if valid.");
}


void test_month_string_conversion()
{
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::chrono::month{1}), "01",
    "Month did not convert right.");
  PQXX_CHECK_EQUAL(
    pqxx::from_string<std::chrono::month>("01"), std::chrono::month{1u},
    "Month did not parse right.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::chrono::month{12}), "12",
    "December did not convert right.");
  PQXX_CHECK_EQUAL(
    pqxx::from_string<std::chrono::month>("12"), std::chrono::month{12u},
    "December did not parse right.");

  std::string_view const invalid[]{
    ""sv,   "-1"sv,      "+1"sv, "+"sv,  "0"sv,
    "13"sv, "January"sv, "5"sv,  "5m"sv, "08-1"sv,
  };
  for (auto const text : invalid)
    PQXX_CHECK_THROWS(
      pqxx::ignore_unused(pqxx::from_string<std::chrono::month>(text)),
      pqxx::conversion_error, "Invalid month parsed as if valid.");
}


void test_day_string_conversion()
{
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::chrono::day{1}), "01", "Day did not convert right.");
  PQXX_CHECK_EQUAL(
    pqxx::from_string<std::chrono::day>("01"), std::chrono::day{1u},
    "Day did not parse right.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::chrono::day{31}), "31",
    "Day 31 did not convert right.");
  PQXX_CHECK_EQUAL(
    pqxx::from_string<std::chrono::day>("31"), std::chrono::day{31u},
    "Day 31 did not parse right.");

  std::string_view const invalid[]{
    ""sv, "-1"sv, "+1"sv, "0"sv, "32"sv, "inf"sv, "3"sv, "24-3"sv,
  };
  for (auto const text : invalid)
    PQXX_CHECK_THROWS(
      pqxx::ignore_unused(pqxx::from_string<std::chrono::day>(text)),
      pqxx::conversion_error, "Invalid day parsed as if valid.");
}


void test_date_string_conversion()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  std::tuple<int, unsigned, unsigned, std::string_view> const conversions[]{
    {-543, 1, 1, "0544-01-01 BC"sv},
    {-1, 2, 3, "0002-02-03 BC"sv},
    {0, 9, 14, "0001-09-14 BC"sv},
    {1, 12, 8, "0001-12-08"sv},
    {2021, 10, 24, "2021-10-24"sv},
    {10191, 8, 30, "10191-08-30"sv},
    {-4712, 1, 1, "4713-01-01 BC"sv},
    {32767, 12, 31, "32767-12-31"sv},
    {2000, 2, 29, "2000-02-29"sv},
    {2004, 2, 29, "2004-02-29"sv},
    // This one won't work in postgres, but we can test the conversions.
    {-32767, 11, 3, "32768-11-03 BC"sv},
  };
  for (auto const &[y, m, d, text] : conversions)
  {
    std::chrono::year_month_day const date{
      std::chrono::year{y}, std::chrono::month{m}, std::chrono::day{d}};
    PQXX_CHECK_EQUAL(
      pqxx::to_string(date), text, "Date did not convert right.");
    PQXX_CHECK_EQUAL(
      pqxx::from_string<std::chrono::year_month_day>(text), date,
      "Date did not parse right.");
    if (int{date.year()} > -4712)
    {
      // We can't test this for years before 4713 BC (4712 BCE), because
      // postgres doesn't handle earlier years.
      PQXX_CHECK_EQUAL(
        tx.query_value<std::string>(
          "SELECT '" + pqxx::to_string(date) + "'::date"),
        text, "Backend interpreted date differently.");
    }
  }

  std::string_view const invalid[]{
    ""sv,           "yesterday"sv,  "1981-01"sv,    "2010"sv,
    "2010-8-9"sv,   "1900-02-29"sv, "2021-02-29"sv, "2000-11-29-3"sv,
    "1900-02-29"sv, "2003-02-29"sv, "12-12-12"sv,   "0000-09-16"sv,
  };
  for (auto const text : invalid)
    PQXX_CHECK_THROWS(
      pqxx::ignore_unused(
        pqxx::from_string<std::chrono::year_month_day>(text)),
      pqxx::conversion_error,
      pqxx::internal::concat("Invalid date '", text, "' parsed as if valid."));
}


PQXX_REGISTER_TEST(test_year_string_conversion);
PQXX_REGISTER_TEST(test_month_string_conversion);
PQXX_REGISTER_TEST(test_day_string_conversion);
PQXX_REGISTER_TEST(test_date_string_conversion);
#endif // PQXX_HAVE_YEAR_MONTH_DAY
} // namespace
