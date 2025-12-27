/** Implementation of date/time support.
 */
#include "pqxx-source.hxx"

#include <cstdlib>

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/time.hxx"

#include "pqxx/internal/header-post.hxx"

#if defined(PQXX_HAVE_YEAR_MONTH_DAY)
namespace
{
using namespace std::literals;


/// Because the C++ Core Guidelines don't like numeric constants...
constexpr int ten{10};


/// Render the numeric part of a year value into a buffer.
/** Converts the year from "common era" (with a Year Zero) to "anno domini"
 * (without a Year Zero).
 *
 * Doesn't render the sign.  When you're rendering a date, you indicate a
 * negative year by suffixing "BC" at the very end.
 *
 * @return A pointer to the character right after the last digit.
 */
inline char *
year_into_buf(char *begin, std::chrono::year const &value, pqxx::ctx c = {})
{
  int const y{value};
  if (y == int{(std::chrono::year::min)()}) [[unlikely]]
  {
    // This is an evil special case: C++ year -32767 translates to 32768 BC,
    // which is a number we can't fit into a short.  At the moment postgres
    // doesn't handle years before 4713 BC, but who knows, right?
    constexpr int oldest{-32767};
    static_assert(int{(std::chrono::year::min)()} == oldest);
    constexpr auto hardcoded{"32768"sv};
    begin += hardcoded.copy(begin, std::size(hardcoded));
  }
  else
  {
    // C++ std::chrono::year has a year zero.  PostgreSQL does not.  So, C++
    // year zero is 1 BC in the postgres calendar; C++ 1 BC is postgres 2 BC,
    // and so on.
    auto const absy{static_cast<short>(std::abs(y) + int{y <= 0})};

    // Keep C++ Code Guidelines happy.
    constexpr int hundred{100}, thousand{1000};

    // PostgreSQL requires year input to be at least 3 digits long, or it
    // won't be able to deduce the date format correctly.  However on output
    // it always writes years as at least 4 digits, and we'll do the same.
    // Dates and times are a dirty, dirty business.
    if (absy < thousand) [[unlikely]]
    {
      *begin++ = '0';
      if (absy < hundred)
        *begin++ = '0';
      if (absy < ten)
        *begin++ = '0';
    }

    // Maximum text length for a year.
    static constexpr int max_year{std::chrono::year::max()};

    // This isn't the actual buffer size, but it's a conservative
    // approximation so should be fine.
    begin +=
      pqxx::into_buf({begin, begin + pqxx::size_buffer(max_year)}, absy, c);
  }
  return begin;
}


/// Parse the numeric part of a year value.
inline int year_from_buf(std::string_view text, pqxx::sl loc)
{
  if (std::size(text) < 4)
    throw pqxx::conversion_error{
      std::format("Year field is too small: '{}'.", text), loc};
  // Parse as int, so we can accommodate 32768 BC which won't fit in a short
  // as-is, but equates to 32767 BCE which will.
  int const year{pqxx::from_string<int>(text)};
  if (year <= 0)
    throw pqxx::conversion_error{std::format("Bad year: '{}'.", text), loc};
  return year;
}


/// Render a valid 1-based month number into a buffer.
/* @return A pointer to the byte right after the last digit.
 */
inline char *month_into_buf(char *begin, std::chrono::month const &value)
{
  unsigned const m{value};
  if (m >= ten)
    *begin = '1';
  else
    *begin = '0';
  ++begin;
  *begin++ = pqxx::internal::number_to_digit(static_cast<int>(m % ten));
  return begin;
}


/// Parse a 1-based month value.
inline std::chrono::month
month_from_string(std::string_view text, pqxx::sl loc)
{
  if (
    not pqxx::internal::is_digit(text[0]) or
    not pqxx::internal::is_digit(text[1]))
    throw pqxx::conversion_error{
      std::format("Invalid month: '{}'.", text), loc};
  return std::chrono::month{unsigned(
    (ten * pqxx::internal::digit_to_number(text[0])) +
    pqxx::internal::digit_to_number(text[1]))};
}


/// Render a valid 1-based day-of-month value into a buffer.
inline char *day_into_buf(char *begin, std::chrono::day const &value)
{
  unsigned const d{value};
  *begin++ = pqxx::internal::number_to_digit(static_cast<int>(d / ten));
  *begin++ = pqxx::internal::number_to_digit(static_cast<int>(d % ten));
  return begin;
}


/// Parse a 1-based day-of-month value.
inline std::chrono::day day_from_string(std::string_view text, pqxx::sl loc)
{
  if (
    not pqxx::internal::is_digit(text[0]) or
    not pqxx::internal::is_digit(text[1]))
    throw pqxx::conversion_error{
      std::format("Bad day in date: '{}'.", text), loc};
  std::chrono::day const d{unsigned(
    (ten * pqxx::internal::digit_to_number(text[0])) +
    pqxx::internal::digit_to_number(text[1]))};
  if (not d.ok())
    throw pqxx::conversion_error{
      std::format("Bad day in date: '{}'.", text), loc};
  return d;
}


/// Look for the dash separating year and month.
/** Assumes that @c text is nonempty.
 */
inline std::size_t find_year_month_separator(std::string_view text) noexcept
{
  // We're looking for a dash.  PostgreSQL won't output a negative year, so
  // no worries about a leading dash.  We could start searching at offset 4,
  // but starting at the beginning produces more helpful error messages for
  // malformed years.
  std::size_t here{0};
  while (here < std::size(text) and text[here] != '-') ++here;
  return here;
}


/// Componse generic "invalid date" message for given (invalid) date text.
std::string make_parse_error(std::string_view text)
{
  return std::format("Invalid date: '{}'.", text);
}
} // namespace


namespace pqxx
{
std::string_view string_traits<std::chrono::year_month_day>::to_buf(
  std::span<char> buf, std::chrono::year_month_day const &value, ctx c)
{
  if (std::size(buf) < size_buffer(value))
    throw conversion_overrun{"Not enough room in buffer for date.", c.loc};
  auto here{std::data(buf)};
  here = year_into_buf(here, value.year());
  *here++ = '-';
  here = month_into_buf(here, value.month());
  *here++ = '-';
  here = day_into_buf(here, value.day());
  if (int{value.year()} <= 0) [[unlikely]]
    here += s_bc.copy(here, std::size(s_bc));
  return {std::data(buf), static_cast<std::size_t>(here - std::data(buf))};
}


std::chrono::year_month_day
string_traits<std::chrono::year_month_day>::from_string(
  std::string_view text, sl loc)
{
  // We can't just re-use the std::chrono::year conversions, because the "BC"
  // suffix comes at the very end.
  if (std::size(text) < 9)
    throw conversion_error{make_parse_error(text), loc};
  bool const is_bc{text.ends_with(s_bc)};
  if (is_bc) [[unlikely]]
    text = text.substr(0, std::size(text) - std::size(s_bc));
  auto const ymsep{find_year_month_separator(text)};
  if ((std::size(text) - ymsep) != 6)
    throw conversion_error{make_parse_error(text), loc};
  auto const base_year{
    year_from_buf(std::string_view{std::data(text), ymsep}, loc)};
  if (base_year == 0)
    throw conversion_error{"Year zero conversion.", loc};
  std::chrono::year const y{is_bc ? (-base_year + 1) : base_year};
  auto const m{month_from_string(text.substr(ymsep + 1, 2), loc)};
  if (text[ymsep + 3] != '-')
    throw conversion_error{make_parse_error(text), loc};
  auto const d{day_from_string(text.substr(ymsep + 4, 2), loc)};
  std::chrono::year_month_day const date{y, m, d};
  if (not date.ok())
    throw conversion_error{make_parse_error(text), loc};
  return date;
}
} // namespace pqxx
#endif
