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
    return generic_to_buf(begin, end, value);
  }

  static char *into_buf(char *begin, char *end, std::chrono::year const &value)
  {
    if (not value.ok())
      throw conversion_error{"Year out of range."};
    int const y{value};
    if (y == int{std::chrono::year::min()})
    {
      // This is an evil special case: C++ year -32767 translates to 32768 BC,
      // which is a number we can't fit into a short.  At the moment postgres
      // doesn't handle years before 4713 BC, but who knows, right?
      static_assert(int{std::chrono::year::min()} == -32767);
      constexpr auto hardcoded{"32768 BC\0"sv};
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

      // PostgreSQL requires years to be at least 3 digits long, or it won't be
      // able to deduce the date format correctly.  Dates and times are a
      // dirty, dirty business.
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
      begin = string_traits<short>::into_buf(begin, end, absy);
      if (y <= 0)
      {
        // Backspace the terminating zero, and append the BC suffix.
        PQXX_UNLIKELY
        --begin;
        if ((end - begin) < std::ssize(s_bc))
          throw conversion_overrun{"Not enough room in buffer for BC year."};
        begin += s_bc.copy(begin, std::size(s_bc));
      }
    }
    return begin;
  }

  [[nodiscard]] static std::chrono::year from_string(std::string_view text)
  {
    auto const bc_len{std::size(s_bc) - 1};
    // std::string_view::ends_with() is C++20, but so is std::chrono::year.
    bool const is_bc{text.ends_with(s_bc.substr(0, bc_len))};
    // Parse the year number separately.
    if (is_bc)
      PQXX_UNLIKELY text = text.substr(0, std::size(text) - bc_len);
    if (std::size(text) < 4)
      throw conversion_error{
        internal::concat("Year field is too small: '", text, "'.")};
    // Parse as int, so we can accommodate 32768 BC which won't fit in a short
    // as-is, but equates to 32767 BCE which will.
    int const base_year{string_traits<int>::from_string(text)};
    if (base_year <= 0)
      throw conversion_error{internal::concat("Bad year: '", text, "'.")};
    int const y{is_bc ? (-base_year + 1) : base_year};
    std::chrono::year year{y};
    if (not year.ok())
      throw conversion_error{
        internal::concat("Year out of range: '", text, "'.")};
    return year;
  }

  [[nodiscard]] static std::size_t
  size_buffer(std::chrono::year const &value) noexcept
  {
    auto base{string_traits<short>::size_buffer(as_short(value))};
    return base + (int{value} <= 0) * std::size(s_bc);
  }

private:
  static constexpr std::string_view s_bc{" BC\0"sv};

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
    char *here{begin};
    if (month >= 10)
      *here = '1';
    else
      *here = '0';
    ++here;
    *here++ = internal::number_to_digit(static_cast<int>(month % 10));
    *here++ = '\0';
    return here;
  }

  [[nodiscard]] static std::chrono::month from_string(std::string_view text)
  {
    if (
      std::size(text) != 2 or not internal::is_digit(text[0]) or
      not internal::is_digit(text[1]))
      throw conversion_error{make_parse_error(text)};
    std::chrono::month const m{unsigned(
      (10 * internal::digit_to_number(text[0])) +
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
    std::chrono::day const d{unsigned(
      (10 * internal::digit_to_number(text[0])) +
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
    // We can't just re-use the std::chrono::year conversions, because the "BC"
    // suffix comes at the very end.
    bool const is_bc{int{value.year()} <= 0};
    char *here{begin};
    if (value.year() == std::chrono::year::min())
    {
      // Special case, because year-zero compensation wouldn't fit a short.
      PQXX_UNLIKELY
      here += s_min_year.copy(begin, std::size(s_min_year));
    }
    else if (is_bc)
    {
      PQXX_UNLIKELY
      here = string_traits<std::chrono::year>::into_buf(
        begin, end, std::chrono::year{-int{value.year()} + 1});
    }
    else
    {
      here =
        string_traits<std::chrono::year>::into_buf(begin, end, value.year());
    }
    *(here - 1) = '-';
    here =
      string_traits<std::chrono::month>::into_buf(here, end, value.month());
    *(here - 1) = '-';
    here = string_traits<std::chrono::day>::into_buf(here, end, value.day());
    if (is_bc)
    {
      PQXX_UNLIKELY
      --here;
      here += s_bc.copy(here, std::size(s_bc));
    }
    return here;
  }

  [[nodiscard]] static std::chrono::year_month_day
  from_string(std::string_view text)
  {
    // We can't just re-use the std::chrono::year conversions, because the "BC"
    // suffix comes at the very end.
    if (std::size(text) < 9)
      throw pqxx::conversion_error{make_parse_error(text)};
    constexpr auto suffix_len{std::size(s_bc) - 1};
    bool const is_bc{text.ends_with(s_bc.substr(0, suffix_len))};
    if (is_bc)
      PQXX_UNLIKELY
      text = text.substr(0, std::size(text) - suffix_len);
    auto const ymsep{find_year_month_separator(text)};
    if ((std::size(text) - ymsep) != 6)
      throw pqxx::conversion_error{make_parse_error(text)};
    auto const base_year{string_traits<int>::from_string(
      std::string_view{std::data(text), ymsep})};
    if (base_year == 0)
      throw conversion_error{"Year zero conversion."};
    std::chrono::year const y{is_bc ? (-base_year + 1) : base_year};
    auto const m{string_traits<std::chrono::month>::from_string(
      text.substr(ymsep + 1, 2))};
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
  size_buffer(std::chrono::year_month_day const &value) noexcept
  {
    return string_traits<std::chrono::year>::size_buffer(value.year()) +
           string_traits<std::chrono::month>::size_buffer(value.month()) +
           string_traits<std::chrono::day>::size_buffer(value.day());
  }

private:
  static constexpr std::string_view s_min_year{"32768\0"sv};
  static constexpr std::string_view s_bc{" BC\0"sv};

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
#endif // PQXX_HAVE_YEAR_MONTH_DAY
} // namespace pqxx

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
