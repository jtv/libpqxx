/* Helpers for prepared statements and parameterised statements.
 *
 * See @ref connection and @ref transaction_base for more.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
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
#include <format>

#include "pqxx/internal/statement_parameters.hxx"
#include "pqxx/types.hxx"


namespace pqxx::internal
{
/// Identity function for @ref encoding_group, for regularity.
PQXX_PURE [[nodiscard]] inline constexpr pqxx::encoding_group
get_encoding_group(encoding_group const &enc, sl = sl::current()) noexcept
{
  return enc;
}


/// Return client encoding from @ref conversion_context.
/** Also accepts a `source_location` argument, but ignores it in favour of the
 * one embedded in the `conversion_context`.
 */
PQXX_PURE [[nodiscard]] inline constexpr pqxx::encoding_group
get_encoding_group(ctx c, sl = sl::current()) noexcept
{
  return c.enc;
}


/// Return connection's current client encoding.
[[nodiscard]] PQXX_LIBEXPORT pqxx::encoding_group
get_encoding_group(connection const &, sl = sl::current());


/// Return transaction's connection's current client encoding.
[[nodiscard]] PQXX_LIBEXPORT pqxx::encoding_group
get_encoding_group(transaction_base const &, sl = sl::current());
} // namespace pqxx::internal


namespace pqxx
{
/// Build a parameter list for a parameterised or prepared statement.
/** When calling a parameterised statement or a prepared statement, in many
 * cases you can pass parameters into the statement in the form of a
 * `pqxx::params` object.
 */
class PQXX_LIBEXPORT params final
{
public:
  params() = default;

  /// Pre-populate a `params` with `args`.  Feel free to add more later.
  /** @note As a first parameter, we recommend passing an @ref encoding_group,
   * or a @ref connection, or a @ref transaction_base (or a more specific
   * transaction type derived from it), or a @ref conversion_context.  The
   * `params` will use this to obtain the client encoding.  (It will not be
   * passed as a parameter; it's just there as a source of the encoding
   * information).  In most cases you won't need this, but there are exceptions
   * where a complex object can't be passed otherwise.  To keep things clear,
   * we recommend passing it in the general case so that you never run into
   * exceptions about encoding being unknown.
   */
  template<typename First, typename... Args>
  params(First &&first, Args &&...args)
  {
    sl loc;
    if constexpr (std::is_same_v<std::remove_cvref<First>, conversion_context>)
      loc = first.loc;
    else
      loc = sl::current();

    if constexpr (requires(encoding_group eg) {
                    eg = pqxx::internal::get_encoding_group(first);
                  })
    {
      // First argument is a source of an encoding group, not a parameter.
      m_enc = pqxx::internal::get_encoding_group(first);
      append_pack(loc, std::forward<Args>(args)...);
    }
    else
    {
      // The first argument is just a regular parameter.  Append it first.
      append_pack(
        loc, std::forward<First>(first), std::forward<Args>(args)...);
    }
  }

  /// Pre-allocate room for at least `n` parameters.
  /** This is not needed, but it may improve efficiency.
   *
   * Reserve space if you're going to add parameters individually, and you've
   * got some idea of how many there are going to be.  It may save some
   * memory re-allocations.
   */
  void reserve(std::size_t n) &;

  /// Get the number of parameters currently in this `params`.
  [[nodiscard]] constexpr auto size() const noexcept
  {
    return m_params.size();
  }

  /// Get the number of parameters (signed).
  /** Unlike `size()`, this is not yet `noexcept`.  That's because C++17's
   * `std::vector` does not have a `ssize()` member function.  These member
   * functions are `noexcept`, but `std::size()` and `std::ssize()` are
   * not.
   */
  [[nodiscard]] constexpr auto ssize() const { return std::ssize(m_params); }

  /// Append a null value.
  void append(sl = sl::current()) &;

  /// Append a non-null zview parameter.
  /** The underlying data must stay valid for as long as the `params`
   * remains active.
   */
  void append(zview, sl = sl::current()) &;

  /// Append a non-null string parameter.
  /** Copies the underlying data into internal storage.  For best efficiency,
   * use the @ref zview variant if you can, or `std::move()`
   */
  void append(std::string const &, sl = sl::current()) &;

  /// Append a non-null string parameter.
  void append(std::string &&, sl = sl::current()) &;

  /// Append a non-null binary parameter.
  /** The underlying data must stay valid for as long as the `params`
   * remains active.
   */
  void append(bytes_view, sl = sl::current()) &;

  /// Append a non-null binary parameter.
  /** The `data` object must stay in place and unchanged, for as long as the
   * `params` remains active.
   */
  template<binary DATA> void append(DATA const &data, sl loc = sl::current()) &
  {
    append(binary_cast(data), loc);
  }

  /// Append a non-null binary parameter.
  void append(bytes &&, sl = sl::current()) &;

  /// Append all parameters in `value`.
  void append(params const &value, sl = sl::current()) &;

  /// Append all parameters in `value`.
  void append(params &&value, sl = sl::current()) &;

  /// Append a non-null parameter, converting it to its string
  /// representation.
  template<typename TYPE>
  void append([[maybe_unused]] TYPE const &value, sl loc = sl::current()) &
  {
    // TODO: Pool storage for multiple string conversions in one buffer?
    if constexpr (pqxx::always_null<TYPE>())
    {
      m_params.emplace_back();
    }
    else if (is_null(value))
    {
      m_params.emplace_back();
    }
    else
    {
      // TODO: Block-allocate storage for parameters.
      m_params.emplace_back(
        entry{to_string(value, conversion_context{m_enc, loc})});
    }
  }

  /// Append all elements of `range` as parameters.
  template<std::ranges::range RANGE>
  void append_multi(RANGE const &range, sl loc = sl::current()) &
  {
    if constexpr (std::ranges::sized_range<RANGE>)
      reserve(std::size(*this) + std::size(range));
    for (auto &value : range) append(value, loc);
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
  pqxx::internal::c_params make_c_params(sl loc) const;

private:
  /// Append a pack of params.
  template<typename... Args> void append_pack(sl loc, Args &&...args)
  {
    reserve(size() + sizeof...(args));
    ((this->append(std::forward<Args>(args), loc)), ...);
  }

  /// Append a pack of params: trivial case.
  /** (Only here to silence annoying warnings about unused parameters.)
   */
  void append_pack(sl) const noexcept {}

  // The way we store a parameter depends on whether it's binary or text
  // (most types are text), and whether we're responsible for storing the
  // contents.
  using entry =
    std::variant<std::nullptr_t, zview, std::string, bytes_view, bytes>;
  std::vector<entry> m_params;

  encoding_group m_enc{encoding_group::unknown};

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
template<typename COUNTER = unsigned int> class placeholders final
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
  void next(sl loc = sl::current()) &
  {
    conversion_context const c{{}, loc};
    if (m_current >= max_params)
      throw range_error{
        std::format(
          "Too many parameters in one statement: limit is {}.", max_params),
        loc};
    PQXX_ASSUME(m_current > 0);
    ++m_current;
    if (m_current % 10 == 0)
    {
      // Carry the 1.  Don't get too clever for this relatively rare
      // case, just rewrite the entire number.  Leave the $ in place
      // though.
      char *const data{std::data(m_buf)};

      auto const written{pqxx::into_buf<COUNTER>(
        {data + 1, data + std::size(m_buf) - 1}, m_current, c)};
      std::size_t end{1 + written};
      assert(end < std::size(m_buf));
      data[end] = '\0';
      m_len = check_cast<COUNTER>(end, "placeholders counter", loc);
    }
    else [[likely]]
    {
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
#endif
