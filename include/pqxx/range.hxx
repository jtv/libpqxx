#ifndef PQXX_H_RANGE
#define PQXX_H_RANGE

#include "pqxx/internal/compiler-internal-pre.hxx"

#include <variant>

#include "pqxx/internal/array-composite.hxx"
#include "pqxx/internal/concat.hxx"

namespace pqxx
{
/// An @e unlimited boundary value to a @c pqxx::range.
/** Use this as a lower or upper bound for a range if the range should extend
 * to infinity on that side.
 *
 * An unlimited boundary is always inclusive of "infinity" values, if the
 * range's value type supports them.
 */
struct no_bound
{
  template<typename TYPE>
  constexpr bool extends_down_to(TYPE const &value) const noexcept
  {
    return true;
  }
  template<typename TYPE>
  constexpr bool extends_up_to(TYPE const &value) const noexcept
  {
    return true;
  }
};


/// An @e inclusive boundary value to a @c pqxx::range.
/** Use this as a lower or upper bound for a range if the range should include
 * the value.
 */
template<typename TYPE> class inclusive_bound
{
public:
  inclusive_bound() =delete;
  explicit inclusive_bound(TYPE const &value) : m_value{value}
  {
    if (is_null(value))
      throw argument_error{"Got null value as an inclusive range bound."};
  }

  TYPE const &get() const noexcept { return m_value; }

  /// Would this bound, as a lower bound, include value?
  bool extends_down_to(TYPE const &value) const
  {
    return not(value < m_value);
  }

  /// Would this bound, as an upper bound, include value?
  bool extends_up_to(TYPE const &value) const { return not(m_value < value); }

private:
  TYPE m_value;
};


/// An @e exclusive boundary value to a @c pqxx::range.
/** Use this as a lower or upper bound for a range if the range should @e not
 * include the value.
 */
template<typename TYPE> class exclusive_bound
{
public:
  exclusive_bound() =delete;
  explicit exclusive_bound(TYPE const &value) : m_value{value}
  {
    if (is_null(value))
      throw argument_error{"Got null value as an exclusive range bound."};
  }

  TYPE const &get() const noexcept { return m_value; }

  /// Would this bound, as a lower bound, include value?
  bool extends_down_to(TYPE const &value) const { return m_value < value; }

  /// Would this bound, as an upper bound, include value?
  bool extends_up_to(TYPE const &value) const { return value < m_value; }

private:
  TYPE m_value;
};


/// A range boundary value.
/** A range bound is either no bound at all; or an inclusive bound; or an
 * exclusive bound.  Pass one of the three to the constructor.
 */
template<typename TYPE> class range_bound
{
public:
  range_bound() = delete;
  range_bound(no_bound) : m_bound{} {}
  range_bound(inclusive_bound<TYPE> const &bound) : m_bound{bound} {}
  range_bound(exclusive_bound<TYPE> const &bound) : m_bound{bound} {}

  /// Is this a finite bound?
  bool is_limited() const noexcept
  {
    return not std::holds_alternative<no_bound>(m_bound);
  }

  /// Is this boundary an inclusive one?
  bool is_inclusive() const noexcept
  {
    return std::holds_alternative<inclusive_bound<TYPE>>(m_bound);
  }

  /// Is this boundary an exclusive one?
  bool is_exclusive() const noexcept
  {
    return std::holds_alternative<exclusive_bound<TYPE>>(m_bound);
  }

  /// Would this bound, as a lower bound, include @c value?
  bool extends_down_to(TYPE const &value) const
  {
    return std::visit(
      [&value](auto const &bound) { return bound.extends_down_to(value); },
      m_bound);
  }

  /// Would this bound, as an upper bound, include @c value?
  bool extends_up_to(TYPE const &value) const
  {
    return std::visit(
      [&value](auto const &bound) { return bound.extends_up_to(value); },
      m_bound);
  }

  /// Return bound value, or @c nullptr if it's not limited.
  TYPE const *value() const noexcept
  {
    return std::visit(
      [](auto const &bound) {
        using bound_t = std::decay_t<decltype(bound)>;
        if constexpr (std::is_same_v<bound_t, no_bound>)
          return static_cast<TYPE const *>(nullptr);
        else
          return &bound.get();
      },
      m_bound);
  }

private:
  std::variant<no_bound, inclusive_bound<TYPE>, exclusive_bound<TYPE>> m_bound;
};


// C++20: Concepts for comparisons, construction, etc.
/// A C++ equivalent to PostgreSQL's range types.
/** You can use this as a client-side representation of a "range" in SQL.
 *
 * PostgreSQL defines several range types, differing in the data type over
 * which they range.  You can also define your own range types.
 *
 * Usually you'll want the server to deal with ranges.  But on occasions where
 * you need to work with them client-side, you may want to use @c pqxx::range.
 * (In cases where all you do is pass them along to the server though, it's not
 * worth the complexity.  In that case you might as well treat ranges as just
 * strings.)
 *
 * For documentation on PostgreSQL's range types, see:
 * https://www.postgresql.org/docs/current/rangetypes.html
 *
 * The value type must be copyable and default-constructible, and support the
 * less-than @c (<) and equals @c (==) comparisons.  Value initialisation must
 * produce a consistent value.
 */
template<typename TYPE> class range
{
public:
  /// Create a range.
  /** For each of the two bounds, pass a @c no_bound, @c inclusive_bound, or
   * @c exclusive_bound.
   */
  range(range_bound<TYPE> lower, range_bound<TYPE> upper) :
          m_lower{lower}, m_upper{upper}
  {
    if (
      lower.is_limited() and upper.is_limited() and
      (*upper.value() < *lower.value()))
      throw range_error{internal::concat(
        "Range's lower bound (", *lower.value(),
        ") is greater than its upper bound (", *upper.value(), ").")};
  }

  /// Create an empty range.
  /** SQL has a separate literal to denote an empty range, but any range which
   * encompasses no values is an empty range.
   */
  range() :
    m_lower{exclusive_bound<TYPE>{TYPE{}}},
    m_upper{exclusive_bound<TYPE>{TYPE{}}}
  {}

  bool operator==(range const &rhs) const
  {
    if (rhs.empty() and this->empty()) return true;
    bool const llimited{m_lower.is_limited()}, ulimited{m_upper.is_limited()};
    if (
      rhs.m_lower.is_limited() != llimited or
      rhs.m_lower.is_inclusive() != m_lower.is_inclusive() or
      rhs.m_lower.is_exclusive() != m_lower.is_exclusive() or
      rhs.m_upper.is_limited() != ulimited or
      rhs.m_upper.is_inclusive() != m_upper.is_inclusive() or
      rhs.m_upper.is_exclusive() != m_upper.is_exclusive()
    ) return false;

    if (llimited and *rhs.m_lower.value() != *m_lower.value()) return false;
    if (ulimited and *rhs.m_upper.value() != *m_upper.value()) return false;
    return true;
  }

  bool operator!=(range const &rhs) const { return !(*this == rhs); }

  /// Is this range clearly empty?
  /** An empty range encompasses no values.
   *
   * It is possible to "fool" this.  For example, if your range is of an
   * integer type and has exclusive bounds of 0 and 1, it encompasses no values
   * but its @c empty() will return false.  The PostgreSQL implementation, by
   * contrast, will notice that it is empty.  Similar things can happen for
   * floating-point types, but with more subtleties and edge cases.
   */
  bool empty() const
  {
    return (m_lower.is_exclusive() or m_upper.is_exclusive()) and
           m_lower.is_limited() and m_upper.is_limited() and
           not(*m_lower.value() < *m_upper.value());
  }

  /// Does this range encompass @c value?
  bool contains(TYPE value) const
  {
    return m_lower.extends_down_to(value) and m_upper.extends_up_to(value);
  }

  range_bound<TYPE> const &lower_bound() const noexcept { return m_lower; }
  range_bound<TYPE> const &upper_bound() const noexcept { return m_upper; }

  // TODO: Intersection.
  // TODO: Iteration?
  // TODO: Casts to other value types.

private:
  range_bound<TYPE> m_lower, m_upper;
};


/// String conversions for a @c range type.
/** Conversion assumes that either your client encoding is UTF-8, or the values
 * are pure ASCII.
 */
template<typename TYPE> struct string_traits<range<TYPE>>
{
  [[nodiscard]] static inline zview
  to_buf(char *begin, char *end, range<TYPE> const &value)
  {
    return zview{begin, into_buf(begin, end, value) - begin};
  }

  static inline char *
  into_buf(char *begin, char *end, range<TYPE> const &value)
  {
    if (value.empty())
    {
      if ((end - begin) <= std::size(s_empty))
        throw conversion_overrun{s_overrun.c_str()};
      char *here = begin + s_empty.copy(begin, std::size(s_empty));
      *here++ = '\0';
      return here;
    }
    else
    {
      if (end - begin < 4)
        throw conversion_overrun{s_overrun.c_str()};
      char *here = begin;
      *here++ = (
        static_cast<char>(value.lower_bound().is_inclusive() ? '[' : '(')
      );
      TYPE const *lower{value.lower_bound().value()};
      // Convert bound (but go back to overwrite that trailing zero).
      if (lower != nullptr)
        here = string_traits<TYPE>::into_buf(here, end, *lower) - 1;
      *here++ = ',';
      TYPE const *upper{value.upper_bound().value()};
      // Convert bound (but go back to overwrite that trailing zero).
      if (upper != nullptr)
        here = string_traits<TYPE>::into_buf(here, end, *upper) - 1;
      if ((end - here) < 2)
        throw conversion_overrun{s_overrun.c_str()};
      *here++ =
        static_cast<char>(value.upper_bound().is_inclusive() ? ']' : ')');
      *here++ = '\0';
      return here;
    }
  }

  [[nodiscard]] static inline range<TYPE> from_string(std::string_view text)
  {
    if (std::size(text) < 3) throw pqxx::conversion_error{err_bad_input(text)};
    bool left_inc{false};
    switch (text[0])
    {
    case '[':
      left_inc = true;
      break;

    case '(':
      break;

    case 'e':
    case 'E':
      if (
        (std::size(text) != std::size(s_empty)) or
        (text[1] != 'm' and text[1] != 'M') or
	(text[2] != 'p' and text[2] != 'P') or
	(text[3] != 't' and text[3] != 'T') or
	(text[4] != 'y' and text[4] != 'Y')
      )
        throw pqxx::conversion_error{err_bad_input(text)};
      return range<TYPE>{};
      break;

    default:
      throw pqxx::conversion_error{err_bad_input(text)};
    }
    return range<TYPE>{}; // XXX: Parse!
  }

  [[nodiscard]] static inline std::size_t
  size_buffer(range<TYPE> const &value) noexcept
  {
    using value_traits = string_traits<TYPE>;
    TYPE const *lower{value.lower_bound().value()},
      *upper{value.upper_bound().value()};
    std::size_t const lsz{
      lower == nullptr ? 0 : string_traits<TYPE>::size_buffer(*lower) - 1},
      usz{upper == nullptr ? 0 : string_traits<TYPE>::size_buffer(*upper) - 1};

    if (value.empty())
      return std::size(s_empty) + 1;
    else
      return 1 + lsz + 1 + usz + 2;
  }

private:
  static constexpr zview s_empty{"empty"_zv};
  static constexpr auto s_overrun{"Not enough space in buffer for range."_zv};

  /// Compose error message for invalid range input.
  static std::string err_bad_input(std::string_view text)
  {
    return internal::concat("Invalid range input: '", text, "'");
  }
};


/// A range type does not have an innate null value.
template<typename TYPE> struct nullness<range<TYPE>> : no_null<range<TYPE>>
{};
} // namespace pqxx
#include "pqxx/internal/compiler-internal-post.hxx"
#endif
