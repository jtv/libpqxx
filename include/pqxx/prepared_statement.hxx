/* Helper classes for prepared statements and parameterised statements.
 *
 * See the connection class for more about such statements.
 *
 * Copyright (c) 2000-2021, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_PREPARED_STATEMENT
#define PQXX_H_PREPARED_STATEMENT

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include "pqxx/internal/statement_parameters.hxx"
#include "pqxx/types.hxx"

// TODO: This header is all about parameters.  Rename it.
// TODO: Fold into the pqxx namespace?  Or inline?

/// Dedicated namespace for helper types related to prepared statements.
namespace pqxx::prepare
{
/// Pass a number of statement parameters only known at runtime.
/** @deprecated Use @c params instead.
 *
 * When you call any of the @c exec_params functions, the number of arguments
 * is normally known at compile time.  This helper function supports the case
 * where it is not.
 *
 * Use this function to pass a variable number of parameters, based on a
 * sequence ranging from @c begin to @c end exclusively.
 *
 * The technique combines with the regular static parameters.  You can use it
 * to insert dynamic parameter lists in any place, or places, among the call's
 * parameters.  You can even insert multiple dynamic sequences.
 *
 * @param begin A pointer or iterator for iterating parameters.
 * @param end A pointer or iterator for iterating parameters.
 * @return An object representing the parameters.
 */
template<typename IT>
[[deprecated("Use params instead.")]] constexpr inline auto make_dynamic_params(IT begin, IT end)
{
  return pqxx::internal::dynamic_params(begin, end);
}


/// Pass a number of statement parameters only known at runtime.
/** @deprecated Use @c params instead.
 *
 * When you call any of the @c exec_params functions, the number of arguments
 * is normally known at compile time.  This helper function supports the case
 * where it is not.
 *
 * Use this function to pass a variable number of parameters, based on a
 * container of parameter values.
 *
 * The technique combines with the regular static parameters.  You can use it
 * to insert dynamic parameter lists in any place, or places, among the call's
 * parameters.  You can even insert multiple dynamic containers.
 *
 * @param container A container of parameter values.
 * @return An object representing the parameters.
 */
template<typename C>
[[nodiscard]] constexpr inline auto make_dynamic_params(C const &container)
{
  using IT = typename C::const_iterator;
  return pqxx::internal::dynamic_params<IT>{container};
}


/// Pass a number of statement parameters only known at runtime.
/** @deprecated User @c params instead.
 *
 * When you call any of the @c exec_params functions, the number of arguments
 * is normally known at compile time.  This helper function supports the case
 * where it is not.
 *
 * Use this function to pass a variable number of parameters, based on a
 * container of parameter values.
 *
 * The technique combines with the regular static parameters.  You can use it
 * to insert dynamic parameter lists in any place, or places, among the call's
 * parameters.  You can even insert multiple dynamic containers.
 *
 * @param container A container of parameter values.
 * @param accessor For each parameter @c p, pass @c accessor(p).
 * @return An object representing the parameters.
 */
template<typename C, typename ACCESSOR>
[[nodiscard]] constexpr inline auto
make_dynamic_params(C &container, ACCESSOR accessor)
{
  using IT = decltype(std::begin(container));
  return pqxx::internal::dynamic_params<IT, ACCESSOR>{container, accessor};
}
} // namespace pqxx::prepare


namespace pqxx
{
/// Build a parameter list for a parameterised or prepared statement.
/** When calling a parameterised statement or a prepared statement, you can
 * pass parameters into the statement directly in the invocation, as additional
 * arguments to @c exec_prepared or @c exec_params.  But in complex cases,
 * sometimes that's just not convenient.
 *
 * In those situations, you can create a @c params and append your parameters
 * into that, one by one.  Then you pass the @c params to @c exec_prepared or
 * @c exec_params.
 *
 * Combinations also work: if you have a @c params containing a string
 * parameter, and you call @c exec_params with an @c int argument followed by
 * your @c params, you'll be passing the @c int as the first parameter and the
 * string as the second.  You can even insert a @c params in a @c params, or
 * pass two @c params objects to a statement.
 */
class params
{
public:
  params() = default;

  /// Create a @c params pre-populated with args.  Feel free to add more later.
  template<typename... Args> constexpr params(Args &&...args)
  {
    reserve(sizeof...(args));
    append_pack(std::forward<Args>(args)...);
  }

  void reserve(std::size_t n) { m_params.reserve(n); }
  auto size() const { return std::size(m_params); }
  auto ssize() const { return pqxx::internal::ssize(m_params); }

  /// Append a non-null zview parameter.push
  /** The underlying data must stay valid for as long as the @c params remains
   * active.
   */
  void append(zview value) { m_params.push_back(entry{value}); }
  /// Append a non-null string parameter.
  /** Copies the underlying data into internal storage.  For best efficiency,
   * use the @c zview variant if you can, or @c std::move().
   */
  void append(std::string const &value) { m_params.push_back(entry{value}); }
  /// Append a non-null string parameter.
  void append(std::string &&value)
  {
    m_params.push_back(entry{std::move(value)});
  }

  /// Append a non-null binary parameter.
  /** The underlying data must stay valid for as long as the @c params remains
   * active.
   */
  void append(std::basic_string_view<std::byte> value)
  {
    m_params.push_back(entry{value});
  }
  /// Append a non-null binary parameter.
  /** Copies the underlying data into internal storage.  For best efficiency,
   * use the @c std::basic_string_view<std::byte> variant if you can, or
   * @c std::move().
   */
  void append(std::basic_string<std::byte> const &value)
  {
    m_params.push_back(entry{value});
  }
  /// Append a non-null binary parameter.
  void append(std::basic_string<std::byte> &&value)
  {
    m_params.push_back(entry{std::move(value)});
  }

  /// @deprecated Append binarystring parameter.
  /** The binarystring must stay valid for as long as the @c params remains
   * active.
   */
  void append(binarystring const &value)
  {
    m_params.push_back(entry{std::basic_string_view<std::byte>{
      reinterpret_cast<std::byte const *>(value.data()), std::size(value)}});
  }

  /// Append all parameters from value.
  template<typename IT, typename ACCESSOR>
  void append(pqxx::internal::dynamic_params<IT, ACCESSOR> const &value)
  {
    // TODO: If supported: reserve(std::size(value)).
    for (auto &param : value) append(value.access(param));
  }

  void append(params const &value)
  {
    this->reserve(std::size(value.m_params) + std::size(this->m_params));
    for (auto const &param : value.m_params) m_params.emplace_back(param);
  }

  void append(params &&value)
  {
    this->reserve(std::size(value.m_params) + std::size(this->m_params));
    for (auto const &param : value.m_params)
      m_params.emplace_back(std::move(param));
    value.m_params.clear();
  }

  /// Append a non-null parameter, converting it to its string representation.
  template<typename TYPE> void append(TYPE const &value)
  {
    // TODO: Pool storage for multiple string conversions in one buffer?
    // XXX: Doesn't is_null() include the always_null check?
    if constexpr (nullness<TYPE>::always_null)
      m_params.emplace_back();
    else if (is_null(value))
      m_params.emplace_back();
    else
      m_params.emplace_back(entry{to_string(value)});
  }

  /// Append all elements of @c range as parameters.
  template<typename RANGE> void append_multi(RANGE &range)
  {
    // TODO: If supported, reserve(std::size(m_params) + std::size(c)).
    for (auto &value : range) append(value);
  }

  /// For internal use: Generate a @c params object for use in calls.
  /** The params object encapsulates the pointers which we will need to pass to
   * libpq when calling a parameterised or prepared statement.
   *
   * The pointers in the params will refer to storage owned by either the
   * params object, or the caller.  This is not a problem because a @c c_params
   * object is guaranteed to live only while the call is going on.  As soon as
   * we climb back out of that call tree, we're done with that data.
   */
  pqxx::internal::c_params make_pointers() const
  {
    pqxx::internal::c_params p;
    p.reserve(std::size(m_params));
    for (auto const &param : m_params)
      std::visit(
        [&p](auto const &value) {
          using T = strip_t<decltype(value)>;

          if constexpr (std::is_same_v<T, nullptr_t>)
          {
            p.values.push_back(nullptr);
            p.lengths.push_back(0);
          }
          else
          {
            p.values.push_back(reinterpret_cast<char const *>(value.data()));
            p.lengths.push_back(
              check_cast<int>(internal::ssize(value), s_overflow));
          }

          p.formats.push_back(param_format(value));
        },
        param);

    return p;
  }

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
  void append_pack() {}

  // The way we store a parameter depends on whether it's binary or text (most
  // types are text), and whether we're responsible for storing the contents.
  using entry = std::variant<
    nullptr_t, zview, std::string, std::basic_string_view<std::byte>,
    std::basic_string<std::byte>>;
  std::vector<entry> m_params;

  static constexpr std::string_view s_overflow{
    "Statement parameter length overflow."sv};
};
} // namespace pqxx::prepare

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
