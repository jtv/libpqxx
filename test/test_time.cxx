#include <pqxx/time>
#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
#if defined(PQXX_HAVE_YEAR_MONTH_DAY)
using namespace std::literals;


using date_tup =
  std::tuple<int, std::chrono::month, std::chrono::day, std::string_view>;


/// Build a `date_tup`.
constexpr date_tup
make_date(int y, unsigned m, unsigned d, std::string_view text)
{
  return date_tup{
    y,
    std::chrono::month{m},
    std::chrono::day{d},
    text,
  };
}


void test_date_string_conversion(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  date_tup const conversions[]{
    make_date(-542, 1, 1, "0543-01-01 BC"sv),
    make_date(-1, 2, 3, "0002-02-03 BC"sv),
    make_date(0, 9, 14, "0001-09-14 BC"sv),
    make_date(1, 12, 8, "0001-12-08"sv),
    make_date(2021, 10, 24, "2021-10-24"sv),
    make_date(10191, 8, 30, "10191-08-30"sv),
    make_date(-4712, 1, 1, "4713-01-01 BC"sv),
    make_date(32767, 12, 31, "32767-12-31"sv),
    make_date(2000, 2, 29, "2000-02-29"sv),
    make_date(2004, 2, 29, "2004-02-29"sv),
    // This one won't work in postgres, but we can test the conversions.
    make_date(-32767, 11, 3, "32768-11-03 BC"sv),
  };
  for (auto const &[y, m, d, text] : std::span{conversions})
  {
    std::chrono::year_month_day const date{
      std::chrono::year{y}, std::chrono::month{m}, std::chrono::day{d}};
    PQXX_CHECK_EQUAL(pqxx::to_string(date), text);
    PQXX_CHECK_EQUAL(
      pqxx::from_string<std::chrono::year_month_day>(text), date);
    if (int{date.year()} > -4712)
    {
      // We can't test this for years before 4713 BC (4712 BCE), because
      // postgres doesn't handle earlier years.
      PQXX_CHECK_EQUAL(
        tx.query_value<std::string>(
          "SELECT '" + pqxx::to_string(date) + "'::date"),
        text);
    }
  }

  std::string_view const invalid[]{
    ""sv,
    "yesterday"sv,
    "1981-01"sv,
    "2010"sv,
    "2010-8-9"sv,
    "1900-02-29"sv,
    "2021-02-29"sv,
    "2000-11-29-3"sv,
    "1900-02-29"sv,
    "2003-02-29"sv,
    "12-12-12"sv,
    "0000-09-16"sv,
    "-01-01"sv,
    "-1000-01-01"sv,
    "1000-00-01"sv,
    "1000-01-00"sv,
    "2001y-01-01"sv,
    "10-09-08"sv,
    "0-01-01"sv,
    "0000-01-01"sv,
    "2021-13-01"sv,
    "2021-+02-01"sv,
    "2021-12-32"sv,
  };
  for (auto const text : invalid)
    PQXX_CHECK_THROWS(
      std::ignore = pqxx::from_string<std::chrono::year_month_day>(text),
      pqxx::conversion_error,
      std::format("Invalid date '{}' parsed as if valid.", text));
}


PQXX_REGISTER_TEST(test_date_string_conversion);
#endif // PQXX_HAVE_YEAR_MONTH_DAY
} // namespace
