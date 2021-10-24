/** Support for date/time values.
 *
 * At the moment this supports dates, but not times.
 */
#ifndef PQXX_H_TIME
#define PQXX_H_TIME

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include <chrono>

#include "pqxx/internal/concat.hxx"
#include "pqxx/strconv"


namespace pqxx
{
#if defined(PQXX_HAVE_YEAR_MONTH_DAY)
template<> struct nullness<std::chrono::year> : no_null<std::chrono::year>
{};


/// String conversions for a year value.
/** Of course you can also use a regular integral type to represent a year.
 * But if a @c std::chrono::year is what you want, libpqxx supports it.
 *
 * The C++20 @c year class supports a range from -32767 to 32767 inclusive.
 *
 * An invalid or out-of-range year will not convert.
 */
template<> struct string_traits<std::chrono::year>
{
  [[nodiscard]] static zview
  to_buf(char *begin, char *end, std::chrono::year const &value)
  {
    if (not value.ok()) throw conversion_error{"Year out of range."};
    return string_traits<int>::to_buf(
      begin, end, as_short(value));
  }

  static char *into_buf(char *begin, char *end, std::chrono::year const &value)
  {
    if (not value.ok()) throw conversion_error{"Year out of range."};
    return string_traits<int>::into_buf(
      begin, end, as_short(value));
  }

  [[nodiscard]] static std::chrono::year from_string(std::string_view text)
  {
    std::chrono::year const y{string_traits<int>::from_string(text)};
    if (not y.ok())
      throw pqxx::conversion_error{
        internal::concat("Year out of range: '", text, "'.")};
    return y;
  }

  [[nodiscard]] static std::size_t
  size_buffer(std::chrono::year const &value) noexcept
  {
    // This'll ask for a lot more room than it'll ever need, but we can't
    // reuse the int conversions without that buffer space.
    return string_traits<short>::size_buffer(as_short(value));
  }

private:
  /// Cast an "OK" year value to @c short.
  static short as_short(std::chrono::year value) noexcept
  {
    // This conversion is safe without runtime checks.  The C++ standard
    // requires that a year value (that passes the @c year.ok() check) fit in
    // a signed short.
    return static_cast<short>(int(value));
  }
};


template<> struct nullness<std::chrono::month> : no_null<std::chrono::month>
{};


/// String conversions for a month: 1 for January, etc. up to 12 for December.
/** This is not likely to be very useful to most applications, and I don't
 * think there's a direct SQL equivalent.  However, the string conversions for
 * full dates make use of the @c month conversions.
 *
 * An invalid or out-of-range month will not convert.
 */
template<> struct string_traits<std::chrono::month>
{
  [[nodiscard]] static zview
  to_buf(char *begin, char *end, std::chrono::month const &value)
  {
    into_buf(begin, end, value);
    return zview{begin, 2u};
  }

  static char *
  into_buf(char *begin, char *end, std::chrono::month const &value)
  {
    if ((end - begin) < 3)
      throw conversion_overrun{"Not enough buffer space for month."};
    if (not value.ok())
      throw conversion_error{"Month value out of range."};
    unsigned const month{value};
    begin[0] = internal::number_to_digit(static_cast<int>(month / 10));
    begin[1] = internal::number_to_digit(static_cast<int>(month % 10));
    begin[2] = '\0';
    return begin + 3;
  }

  [[nodiscard]] static std::chrono::month from_string(std::string_view text)
  {
    if (
      std::size(text) != 2 or not internal::is_digit(text[0]) or
      not internal::is_digit(text[1]))
      throw conversion_error{make_parse_error(text)};
    std::chrono::month const m{
      unsigned((10 * internal::digit_to_number(text[0])) +
      internal::digit_to_number(text[1]))};
    if (not m.ok())
      throw conversion_error{make_parse_error(text)};
    return m;
  }

  [[nodiscard]] static std::size_t
  size_buffer(std::chrono::month const &) noexcept
  {
    return 3u;
  }

private:
  static std::string make_parse_error(std::string_view text)
  {
    return internal::concat("Invalid month: '", text, "'.");
  }
};


template<> struct nullness<std::chrono::day> : no_null<std::chrono::day>
{};


/// String conversions for day-of-month.  Starts at 1, up to 31 inclusive.
/** This is not likely to be very useful to most applications, and I don't
 * think there's a direct SQL equivalent.  However, the string conversions for
 * full dates make use of the @c day conversions.
 *
 * An invalid or out-of-range day will not convert.  But of course if you want
 * to associate a day of 30 with the month of February, the @c day conversions
 * per se will not notice.  That error only comes to the fore when you convert
 * a full date.
 */
template<> struct string_traits<std::chrono::day>
{
  [[nodiscard]] static zview
  to_buf(char *begin, char *end, std::chrono::day const &value)
  {
    into_buf(begin, end, value);
    return zview{begin, 2u};
  }

  static char *into_buf(char *begin, char *end, std::chrono::day const &value)
  {
    if ((end - begin) < 3)
      throw conversion_overrun{"Not enough buffer space for day."};
    if (not value.ok())
      throw conversion_error{"Day value out of range."};
    unsigned const day{value};
    begin[0] = internal::number_to_digit(static_cast<int>(day / 10));
    begin[1] = internal::number_to_digit(static_cast<int>(day % 10));
    begin[2] = '\0';
    return begin + 3;
  }

  [[nodiscard]] static std::chrono::day from_string(std::string_view text)
  {
    if (
      std::size(text) != 2 or not internal::is_digit(text[0]) or
      not internal::is_digit(text[1]))
      throw conversion_error{make_parse_error(text)};
    std::chrono::day const d{
      unsigned((10 * internal::digit_to_number(text[0])) +
      internal::digit_to_number(text[1]))};
    if (not d.ok())
      throw conversion_error{make_parse_error(text)};
    return d;
  }

  [[nodiscard]] static std::size_t
  size_buffer(std::chrono::day const &) noexcept
  {
    return 3u;
  }

private:
  static std::string make_parse_error(std::string_view text)
  {
    return internal::concat("Invalid day: '", text, "'.");
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
 * @i Gregorian date.
 */
template<> struct string_traits<std::chrono::year_month_day>
{
  [[nodiscard]] static zview
  to_buf(char *begin, char *end, std::chrono::year_month_day const &value)
  {
    return zview{begin, into_buf(begin, end, value) - begin - 1};
  }

  static char *
  into_buf(char *begin, char *end, std::chrono::year_month_day const &value)
  {
    char *here{begin};
    here = string_traits<std::chrono::year>::into_buf(begin, end, value.year());
    if ((end - here) < 6)
      throw pqxx::conversion_overrun{"Not enough buffer space for date."};
    *(here - 1) = '-';
    if (unsigned(value.month()) >= 10)
      *here++ = '1';
    else
      *here++ = '0';
    *here++ = internal::number_to_digit(unsigned(value.month()) % 10);
    *here++ = '-';
    auto const d{unsigned(value.day())};
    *here++ = internal::number_to_digit(d / 10);
    *here++ = internal::number_to_digit(d % 10);
    *here++ = '\0';
    return here;
  }

  [[nodiscard]] static std::chrono::year_month_day
  from_string(std::string_view text)
  {
    if (std::size(text) < 7)
      throw pqxx::conversion_error{make_parse_error(text)};
    auto const ymsep{find_year_month_separator(text)};
    if ((std::size(text) - ymsep) != 6)
      throw pqxx::conversion_error{make_parse_error(text)};
    auto const y{string_traits<std::chrono::year>::from_string(
      std::string_view{std::data(text), ymsep})};
    auto const m{
      string_traits<std::chrono::month>::from_string(text.substr(ymsep + 1, 2))};
    if (text[ymsep + 3] != '-')
      throw pqxx::conversion_error{make_parse_error(text)};
    auto const d{string_traits<std::chrono::day>::from_string(text.substr(ymsep + 4, 2))};
    std::chrono::year_month_day const date{y, m, d};
    if (not date.ok())
      throw conversion_error{make_parse_error(text)};
    return date;
  }

  [[nodiscard]] static std::size_t
  size_buffer(std::chrono::year_month_day const &) noexcept
  {
    return 11;
  }

private:
  /// Look for the dash separating year and month.
  /** Assumes that @c text is nonempty.
   */
  static std::size_t find_year_month_separator(std::string_view text) noexcept
  {
    // We're looking for a dash.  But the year may be negative, so we've got
    // to be careful not to confuse the minus sign for a dash.  Start scanning
    // at offset 1, not at 0.
    std::size_t here;
    for (here = 1; here < std::size(text) and text[here] != '-'; ++here)
      ;
    return here;
  }

  static std::string make_parse_error(std::string_view text)
  {
    return internal::concat("Invalid date: '", text, "'.");
  }
};
#endif // PQXX_HAVE_YEAR_MONTH_DAY
} // namespace pqxx

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
