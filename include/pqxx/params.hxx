/* Helpers for prepared statements and parameterised statements.
 *
 * See @ref connection and @ref transaction_base for more.
 *
 * Copyright (c) 2000-2024, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_PARAMS
#define PQXX_H_PARAMS

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include <array>

#include "pqxx/internal/concat.hxx"
#include "pqxx/internal/statement_parameters.hxx"
#include "pqxx/types.hxx"


namespace pqxx
{
/// Build a parameter list for a parameterised or prepared statement.
/** When calling a parameterised statement or a prepared statement, in many
 * cases you can pass parameters into the statement in the form of a
 * `pqxx::params` object.
 */
class PQXX_LIBEXPORT params
{
public:
  params() = default;

  /// Pre-populate a `params` with `args`.  Feel free to add more later.
  template<typename... Args> constexpr params(Args &&...args)
  {
    reserve(sizeof...(args));
    append_pack(std::forward<Args>(args)...);
  }

  /// Pre-allocate room for at least `n` parameters.
  /** This is not needed, but it may improve efficiency.
   *
   * Reserve space if you're going to add parameters individually, and you've
   * got some idea of how many there are going to be.  It may save some
   * memory re-allocations.
   */
  void reserve(std::size_t n) &;

  // C++20: constexpr.
  /// Get the number of parameters currently in this `params`.
  [[nodiscard]] auto size() const noexcept { return m_params.size(); }

  // C++20: Use the vector's ssize() directly and go noexcept+constexpr.
  /// Get the number of parameters (signed).
  /** Unlike `size()`, this is not yet `noexcept`.  That's because C++17's
   * `std::vector` does not have a `ssize()` member function.  These member
   * functions are `noexcept`, but `std::size()` and `std::ssize()` are
   * not.
   */
  [[nodiscard]] auto ssize() const { return pqxx::internal::ssize(m_params); }

  /// Append a null value.
  void append() &;

  /// Append a non-null zview parameter.
  /** The underlying data must stay valid for as long as the `params`
   * remains active.
   */
  void append(zview) &;

  /// Append a non-null string parameter.
  /** Copies the underlying data into internal storage.  For best efficiency,
   * use the @ref zview variant if you can, or `std::move()`
   */
  void append(std::string const &) &;

  /// Append a non-null string parameter.
  void append(std::string &&) &;

  /// Append a non-null binary parameter.
  /** The underlying data must stay valid for as long as the `params`
   * remains active.
   */
  void append(bytes_view) &;

  /// Append a non-null binary parameter.
  /** Copies the underlying data into internal storage.  For best efficiency,
   * use the `pqxx::bytes_view` variant if you can, or `std::move()`.
   */
  void append(bytes const &) &;

#if defined(PQXX_HAVE_CONCEPTS)
  /// Append a non-null binary parameter.
  /** The `data` object must stay in place and unchanged, for as long as the
   * `params` remains active.
   */
  template<binary DATA> void append(DATA const &data) &
  {
    append(bytes_view{std::data(data), std::size(data)});
  }
#endif // PQXX_HAVE_CONCEPTS

  /// Append a non-null binary parameter.
  void append(bytes &&) &;

  /// @deprecated Append binarystring parameter.
  /** The binarystring must stay valid for as long as the `params` remains
   * active.
   */
  void append(binarystring const &value) &;

  /// Append all parameters from value.
  template<typename IT, typename ACCESSOR>
  void append(pqxx::internal::dynamic_params<IT, ACCESSOR> const &value) &
  {
    for (auto &param : value) append(value.access(param));
  }

  void append(params const &value) &;

  void append(params &&value) &;

  /// Append a non-null parameter, converting it to its string
  /// representation.
  template<typename TYPE> void append(TYPE const &value) &
  {
    // TODO: Pool storage for multiple string conversions in one buffer?
    if constexpr (nullness<strip_t<TYPE>>::always_null)
    {
      ignore_unused(value);
      m_params.emplace_back();
    }
    else if (is_null(value))
    {
      m_params.emplace_back();
    }
    else
    {
      m_params.emplace_back(entry{to_string(value)});
    }
  }

  /// Append all elements of `range` as parameters.
  template<PQXX_RANGE_ARG RANGE> void append_multi(RANGE const &range) &
  {
#if defined(PQXX_HAVE_CONCEPTS)
    if constexpr (std::ranges::sized_range<RANGE>)
      reserve(std::size(*this) + std::size(range));
#endif
    for (auto &value : range) append(value);
  }

  /// For internal use: Generate a `params` object for use in calls.
  /** The params object encapsulates the pointers which we will need to pass
   * to libpq when calling a parameterised or prepared statement.
   *
   * The pointers in the params will refer to storage owned by either the
   * params object, or the caller.  This is not a problem because a
   * `c_params` object is guaranteed to live only while the call is going on.
   * As soon as we climb back out of that call tree, we're done with that
   * data.
   */
  pqxx::internal::c_params make_c_params() const;

private:
  /// Recursively append a pack of params.
  template<typename Arg, typename... More>
  void append_pack(Arg &&arg, More &&...args)
  {
    this->append(std::forward<Arg>(arg));
    // Recurse for remaining args.
    append_pack(std::forward<More>(args)...);
  }

  /// Terminating case: append an empty parameter pack.  It's not hard BTW.
  constexpr void append_pack() noexcept {}

  // The way we store a parameter depends on whether it's binary or text
  // (most types are text), and whether we're responsible for storing the
  // contents.
  using entry =
    std::variant<std::nullptr_t, zview, std::string, bytes_view, bytes>;
  std::vector<entry> m_params;

  static constexpr std::string_view s_overflow{
    "Statement parameter length overflow."sv};
};


/// Generate parameter placeholders for use in an SQL statement.
/** When you want to pass parameters to a prepared statement or a parameterised
 * statement, you insert placeholders into the SQL.  During invocation, the
 * database replaces those with the respective parameter values you passed.
 *
 * The placeholders look like `$1` (for the first parameter value), `$2` (for
 * the second), and so on.  You can just write those directly in your
 * statement.  But for those rare cases where it becomes difficult to track
 * which number a placeholder should have, you can use a `placeholders` object
 * to count and generate them in order.
 */
template<typename COUNTER = unsigned int> class placeholders
{
public:
  /// Maximum number of parameters we support.
  static inline constexpr unsigned int max_params{
    (std::numeric_limits<COUNTER>::max)()};

  placeholders()
  {
    static constexpr auto initial{"$1\0"sv};
    initial.copy(std::data(m_buf), std::size(initial));
  }

  /// Read an ephemeral version of the current placeholder text.
  /** @warning Changing the current placeholder number will overwrite this.
   * Use the view immediately, or lose it.
   */
  constexpr zview view() const & noexcept
  {
    return zview{std::data(m_buf), m_len};
  }

  /// Read the current placeholder text, as a `std::string`.
  /** This will be slightly slower than converting to a `zview`.  With most
   * C++ implementations however, until you get into ridiculous numbers of
   * parameters, the string will benefit from the Short String Optimization, or
   * SSO.
   */
  std::string get() const { return std::string(std::data(m_buf), m_len); }

  /// Move on to the next parameter.
  void next() &
  {
    if (m_current >= max_params)
      throw range_error{pqxx::internal::concat(
        "Too many parameters in one statement: limit is ", max_params, ".")};
    PQXX_ASSUME(m_current > 0);
    ++m_current;
    if (m_current % 10 == 0)
    {
      // Carry the 1.  Don't get too clever for this relatively rare
      // case, just rewrite the entire number.  Leave the $ in place
      // though.
      char *const data{std::data(m_buf)};
      char *const end{string_traits<COUNTER>::into_buf(
        data + 1, data + std::size(m_buf), m_current)};
      // (Subtract because we don't include the trailing zero.)
      m_len = check_cast<COUNTER>(end - data, "placeholders counter") - 1;
    }
    else
    {
      PQXX_LIKELY
      // Shortcut for the common case: just increment that last digit.
      ++m_buf[m_len - 1];
    }
  }

  /// Return the current placeholder number.  The initial placeholder is 1.
  COUNTER count() const noexcept { return m_current; }

private:
  /// Current placeholder number.  Starts at 1.
  COUNTER m_current = 1;

  /// Length of the current placeholder string, not including trailing zero.
  COUNTER m_len = 2;

  /// Text buffer where we render the placeholders, with a trailing zero.
  /** We keep reusing this for every subsequent placeholder, just because we
   * don't like string allocations.
   *
   * Maximum length is the maximum base-10 digits that COUNTER can fully
   * represent, plus 1 more for the extra digit that it can only partially
   * fill up, plus room for the dollar sign and the trailing zero.
   */
  std::array<char, std::numeric_limits<COUNTER>::digits10 + 3> m_buf;
};
} // namespace pqxx


/// @deprecated The new @ref params class replaces all of this.
namespace pqxx::prepare
{
/// @deprecated Use @ref params instead.
template<typename IT>
[[deprecated("Use the params class instead.")]] constexpr inline auto
make_dynamic_params(IT begin, IT end)
{
  return pqxx::internal::dynamic_params(begin, end);
}


/// @deprecated Use @ref params instead.
template<typename C>
[[deprecated("Use the params class instead.")]] constexpr inline auto
make_dynamic_params(C const &container)
{
  using IT = typename C::const_iterator;
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  return pqxx::internal::dynamic_params<IT>{container};
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


/// @deprecated Use @ref params instead.
template<typename C, typename ACCESSOR>
[[deprecated("Use the params class instead.")]] constexpr inline auto
make_dynamic_params(C &container, ACCESSOR accessor)
{
  using IT = decltype(std::begin(container));
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  return pqxx::internal::dynamic_params<IT, ACCESSOR>{container, accessor};
#include "pqxx/internal/ignore-deprecated-post.hxx"
}
} // namespace pqxx::prepare
#endif
