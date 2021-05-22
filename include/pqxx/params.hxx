/* Helpers for prepared statements and parameterised statements.
 *
 * See the connection class for more about such statements.
 *
 * Copyright (c) 2000-2021, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_PARAMS
#define PQXX_H_PARAMS

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include "pqxx/internal/statement_parameters.hxx"
#include "pqxx/types.hxx"


/// @deprecated The new @c params class replaces all of this.
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
[[deprecated("Use the params class instead.")]] constexpr inline auto
make_dynamic_params(IT begin, IT end)
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
[[deprecated("Use the params class instead.")]] constexpr inline auto
make_dynamic_params(C const &container)
{
  using IT = typename C::const_iterator;
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  return pqxx::internal::dynamic_params<IT>{container};
#include "pqxx/internal/ignore-deprecated-post.hxx"
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
 * @param accessor For each parameter @c p, pass @c accessor(p).
 * @return An object representing the parameters.
 */
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
class PQXX_LIBEXPORT params
{
public:
  params() = default;

  /// Create a @c params pre-populated with args.  Feel free to add more later.
  template<typename... Args> constexpr params(Args &&...args)
  {
    reserve(sizeof...(args));
    append_pack(std::forward<Args>(args)...);
  }

  /// Pre-allocate room for at least @c n parameters.
  /** This is not needed, but it may improve efficiency.
   *
   * Reserve space if you're going to add parameters individually, and you've
   * got some idea of how many there are going to be.  It may save some memory
   * re-allocations.
   */
  void reserve(std::size_t n);

  /// Get the number of parameters currently in this @c params.
  auto size() const noexcept { return m_params.size(); }

  // TODO: In C++20, use the vector's ssize() directly and go noexcept.
  /// Get the number of parameters (signed).
  /** Unlike @c size(), this is not yet @c noexcept.  That's because C++17's
   * @c std::vector does not have a @c ssize() member function.  These member
   * functions are @c noexcept, but @c std::size() and @c std::ssize() are not.
   */
  auto ssize() const { return pqxx::internal::ssize(m_params); }

  /// Append a null value.
  void append();

  /// Append a non-null zview parameter.
  /** The underlying data must stay valid for as long as the @c params remains
   * active.
   */
  void append(zview);

  /// Append a non-null string parameter.
  /** Copies the underlying data into internal storage.  For best efficiency,
   * use the @c zview variant if you can, or @c std::move().
   */
  void append(std::string const &);

  /// Append a non-null string parameter.
  void append(std::string &&);

  /// Append a non-null binary parameter.
  /** The underlying data must stay valid for as long as the @c params remains
   * active.
   */
  void append(std::basic_string_view<std::byte>);

  /// Append a non-null binary parameter.
  /** Copies the underlying data into internal storage.  For best efficiency,
   * use the @c std::basic_string_view<std::byte> variant if you can, or
   * @c std::move().
   */
  void append(std::basic_string<std::byte> const &);

#if defined(PQXX_HAVE_CONCEPTS)
  /// Append a non-null binary parameter.
  /** The @c data object must stay in place and unchanged, for as long as the
   * @c params remains active.
   */
  template<binary DATA> void append(DATA const &data)
  {
    append(
      std::basic_string_view<std::byte>{std::data(data), std::size(data)});
  }
#endif // PQXX_HAVE_CONCEPTS

  /// Append a non-null binary parameter.
  void append(std::basic_string<std::byte> &&);

  /// @deprecated Append binarystring parameter.
  /** The binarystring must stay valid for as long as the @c params remains
   * active.
   */
  void append(binarystring const &value);

  /// Append all parameters from value.
  template<typename IT, typename ACCESSOR>
  void append(pqxx::internal::dynamic_params<IT, ACCESSOR> const &value)
  {
    for (auto &param : value) append(value.access(param));
  }

  void append(params const &value);

  void append(params &&value);

  /// Append a non-null parameter, converting it to its string representation.
  template<typename TYPE> void append(TYPE const &value)
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

  /// Append all elements of @c range as parameters.
  template<PQXX_RANGE_ARG RANGE> void append_multi(RANGE const &range)
  {
#if defined(PQXX_HAVE_CONCEPTS)
    if constexpr (std::ranges::sized_range<RANGE>)
      reserve(std::size(*this) + std::size(range));
#endif
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
  void append_pack() {}

  // The way we store a parameter depends on whether it's binary or text (most
  // types are text), and whether we're responsible for storing the contents.
  using entry = std::variant<
    std::nullptr_t, zview, std::string, std::basic_string_view<std::byte>,
    std::basic_string<std::byte>>;
  std::vector<entry> m_params;

  static constexpr std::string_view s_overflow{
    "Statement parameter length overflow."sv};
};
} // namespace pqxx

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
