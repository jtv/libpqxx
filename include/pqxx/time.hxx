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

namespace pqxx
{
using namespace std::literals;

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
template<> struct PQXX_LIBEXPORT string_traits<std::chrono::year_month_day>
{
  [[nodiscard]] static zview
  to_buf(char *begin, char *end, std::chrono::year_month_day const &value)
  {
    return generic_to_buf(begin, end, value);
  }

  static char *
  into_buf(char *begin, char *end, std::chrono::year_month_day const &value);

  [[nodiscard]] static std::chrono::year_month_day
  from_string(std::string_view text);

  [[nodiscard]] static std::size_t
  size_buffer(std::chrono::year_month_day const &) noexcept
  {
    static_assert(int{(std::chrono::year::min)()} >= -99999);
    static_assert(int{(std::chrono::year::max)()} <= 99999);
    return 5 + 1 + 2 + 1 + 2 + std::size(s_bc) + 1;
  }

private:
  /// The "BC" suffix for years before 1 AD.
  static constexpr std::string_view s_bc{" BC"sv};
};
} // namespace pqxx
#endif // PQXX_HAVE_YEAR_MONTH_DAY

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
