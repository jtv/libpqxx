/** Support for date/time values.
 *
 * At the moment this supports dates, but not times.
 */
#ifndef PQXX_H_TIME
#define PQXX_H_TIME

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include <chrono>
#include <cstdlib>

#include "pqxx/internal/concat.hxx"
#include "pqxx/strconv"


#if defined(PQXX_HAVE_YEAR_MONTH_DAY)
namespace pqxx::internal
{
/// Render the numeric part of a year value into a buffer.
/** Converts the year from "common era" (with a Year Zero) to "anno domini"
 * (without a Year Zero).
 *
 * Doesn't render the sign.  When you're rendering a date, you indicate a
 * negative year by suffixing "BC" at the very end.
 *
 * Where @c string_traits::into_buf() returns a pointer to the position right
 * after the terminating zero, this function returns a pointer to the character
 * right after the last digit.  (It may or may not write a terminating zero at
 * that position itself.)
 */
inline char *year_into_buf(char *begin, char *end, std::chrono::year const &value)
{
    int const y{value};
    if (y == int{(std::chrono::year::min)()})
    {
      // This is an evil special case: C++ year -32767 translates to 32768 BC,
      // which is a number we can't fit into a short.  At the moment postgres
      // doesn't handle years before 4713 BC, but who knows, right?
      static_assert(int{(std::chrono::year::min)()} == -32767);
      constexpr auto hardcoded{"32768"sv};
      PQXX_UNLIKELY
      if ((end - begin) < std::ssize(hardcoded))
        throw conversion_overrun{"Not enough buffer space for year."};
      begin += hardcoded.copy(begin, std::size(hardcoded));
    }
    else
    {
      // C++ std::chrono::year has a year zero.  PostgreSQL does not.  So, C++
      // year zero is 1 BC in the postgres calendar; C++ 1 BC is postgres 2 BC,
      // and so on.
      auto const absy{static_cast<short>(std::abs(y) + int{y <= 0})};

      // PostgreSQL requires year input to be at least 3 digits long, or it
      // won't be able to deduce the date format correctly.  However on output
      // it always writes years as at least 4 digits, and we'll do the same.
      // Dates and times are a dirty, dirty business.
      if (absy < 1000)
      {
        PQXX_UNLIKELY
        if ((end - begin) < 3)
          throw conversion_overrun{"Not enough room in buffer for year."};
        *begin++ = '0';
        if (absy < 100)
          *begin++ = '0';
        if (absy < 10)
          *begin++ = '0';
      }
      begin = string_traits<short>::into_buf(begin, end, absy) - 1;
    }
    return begin;
}


/// Parse the numeric part of a year value.
inline int year_from_buf(std::string_view text)
{
    if (std::size(text) < 4)
      throw conversion_error{
        internal::concat("Year field is too small: '", text, "'.")};
    // Parse as int, so we can accommodate 32768 BC which won't fit in a short
    // as-is, but equates to 32767 BCE which will.
    int const year{string_traits<int>::from_string(text)};
    if (year <= 0)
      throw conversion_error{internal::concat("Bad year: '", text, "'.")};
    return year;
}


/// Render a valid 1-based month number into a buffer.
/* Where @c string_traits::into_buf() returns a pointer to the position right
 * after the terminating zero, this function returns a pointer to the character
 * right after the last digit.  (It may or may not write a terminating zero at
 * that position itself.)
 */
inline static char *month_into_buf(char *begin, std::chrono::month const &value)
{
  unsigned const m{value};
  if (m >= 10) *begin = '1';
  else *begin = '0';
  ++begin;
  *begin++ = internal::number_to_digit(static_cast<int>(m % 10));
  return begin;
}


/// Parse a 1-based month value.
inline std::chrono::month month_from_string(std::string_view text)
{
  if (not internal::is_digit(text[0]) or not internal::is_digit(text[1]))
    throw conversion_error{internal::concat("Invalid month: '", text, "'.")};
  return std::chrono::month{unsigned(
    (10 * internal::digit_to_number(text[0])) +
    internal::digit_to_number(text[1]))};
}


/// Render a valid 1-based day-of-month value into a buffer.
inline char *day_into_buf(char *begin, std::chrono::day const &value)
{
  unsigned d{value};
  *begin++ = internal::number_to_digit(static_cast<int>(d / 10));
  *begin++ = internal::number_to_digit(static_cast<int>(d % 10));
  return begin;
}


/// Parse a 1-based day-of-month value.
inline std::chrono::day day_from_string(std::string_view text)
{
    if (not internal::is_digit(text[0]) or not internal::is_digit(text[1]))
      throw conversion_error{internal::concat("Bad day in date: '", text, "'.")};
    std::chrono::day const d{unsigned(
      (10 * internal::digit_to_number(text[0])) +
      internal::digit_to_number(text[1]))};
    if (not d.ok())
      throw conversion_error{internal::concat("Bad day in date: '", text, "'.")};
    return d;
}
} // namespace pqxx::internal


namespace pqxx
{
template<> struct string_traits<std::chrono::day>
{
  [[nodiscard]] static std::chrono::day from_string(std::string_view text)
  {
    if (
      std::size(text) != 2 or not internal::is_digit(text[0]) or
      not internal::is_digit(text[1]))
      throw conversion_error{internal::concat("Bad day in year: '", text, "'.")};
    std::chrono::day const d{unsigned(
      (10 * internal::digit_to_number(text[0])) +
      internal::digit_to_number(text[1]))};
    if (not d.ok())
      throw conversion_error{internal::concat("Bad day in year: '", text, "'.")};
    return d;
  }
};


template<>
struct nullness<std::chrono::year_month_day>
        : no_null<std::chrono::year_month_day>
{};


/// String representation for a Gregorian date in ISO-8601 format.
/** PostgreSQL supports a choice of date formats, but libpqxx does not.  The
 * other formats in turn support a choice of "month before day" versus "day
 * before month," meaning that it's not necessarily known which format a given
 * date is supposed to be.
 *
 * Invalid dates will not convert.  This includes February 29 on non-leap
 * years, which is why it matters that @c year_month_day represents a
 * @e Gregorian date.
 */
template<> struct string_traits<std::chrono::year_month_day>
{
  [[nodiscard]] static zview
  to_buf(char *begin, char *end, std::chrono::year_month_day const &value)
  {
    return generic_to_buf(begin, end, value);
  }

  static char *
  into_buf(char *begin, char *end, std::chrono::year_month_day const &value)
  {
    if (std::size_t(end - begin) < size_buffer(value))
      throw conversion_overrun{"Not enough room in buffer for date."};
    begin = internal::year_into_buf(begin, end, value.year());
    *begin++ = '-';
    begin = internal::month_into_buf(begin, value.month());
    *begin++ = '-';
    begin = internal::day_into_buf(begin, value.day());
    if (int{value.year()} <= 0)
    {
      PQXX_UNLIKELY
      begin += s_bc.copy(begin, std::size(s_bc));
    }
    *begin++ = '\0';
    return begin;
  }

  [[nodiscard]] static std::chrono::year_month_day
  from_string(std::string_view text)
  {
    // We can't just re-use the std::chrono::year conversions, because the "BC"
    // suffix comes at the very end.
    if (std::size(text) < 9)
      throw pqxx::conversion_error{make_parse_error(text)};
    bool const is_bc{text.ends_with(s_bc)};
    if (is_bc)
      PQXX_UNLIKELY
      text = text.substr(0, std::size(text) - std::size(s_bc));
    auto const ymsep{find_year_month_separator(text)};
    if ((std::size(text) - ymsep) != 6)
      throw pqxx::conversion_error{make_parse_error(text)};
    auto const base_year{string_traits<int>::from_string(
      std::string_view{std::data(text), ymsep})};
    if (base_year == 0)
      throw conversion_error{"Year zero conversion."};
    std::chrono::year const y{is_bc ? (-base_year + 1) : base_year};
    auto const m{internal::month_from_string(text.substr(ymsep + 1, 2))};
    if (text[ymsep + 3] != '-')
      throw pqxx::conversion_error{make_parse_error(text)};
    auto const d{
      string_traits<std::chrono::day>::from_string(text.substr(ymsep + 4, 2))};
    std::chrono::year_month_day const date{y, m, d};
    if (not date.ok())
      throw conversion_error{make_parse_error(text)};
    return date;
  }

  [[nodiscard]] static std::size_t
  size_buffer(std::chrono::year_month_day const &) noexcept
  {
    static_assert(int{(std::chrono::year::min)()} >= -99999);
    static_assert(int{(std::chrono::year::max)()} <= 99999);
    return 5 + 1 + 2 + 1 + 2 + std::size(s_bc) + 1;
  }

private:
  static constexpr std::string_view s_min_year{"32768\0"sv};
  static constexpr std::string_view s_bc{" BC"sv};

  /// Look for the dash separating year and month.
  /** Assumes that @c text is nonempty.
   */
  static std::size_t find_year_month_separator(std::string_view text) noexcept
  {
    // We're looking for a dash.  PostgreSQL won't output a negative year, so
    // no worries about a leading dash.  We could start searching at offset 4,
    // but starting at the beginning produces more helpful error messages for
    // malformed years.
    std::size_t here;
    for (here = 0; here < std::size(text) and text[here] != '-'; ++here)
      ;
    return here;
  }

  static std::string make_parse_error(std::string_view text)
  {
    return internal::concat("Invalid date: '", text, "'.");
  }
};
} // namespace pqxx
#endif // PQXX_HAVE_YEAR_MONTH_DAY

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
