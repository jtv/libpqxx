/** Common implementation for statement parameter lists.
 *
 * These are used for both prepared statements and parameterized statements.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY.  Other headers include it for you.
 *
 * Copyright (c) 2000-2022, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_STATEMENT_PARAMETER
#define PQXX_H_STATEMENT_PARAMETER

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include <cstring>
#include <functional>
#include <iterator>
#include <string>
#include <vector>

#include "pqxx/binarystring"
#include "pqxx/strconv"
#include "pqxx/util"


namespace pqxx::internal
{
template<typename ITERATOR>
constexpr inline auto const iterator_identity{
  [](decltype(*std::declval<ITERATOR>()) x) { return x; }};


// TODO: C++20 "ranges" alternative.

/// Marker type: pass a dynamically-determined number of statement parameters.
/** Normally when invoking a prepared or parameterised statement, the number
 * of parameters is known at compile time.  For instance,
 * @c t.exec_prepared("foo", 1, "x"); executes statement @c foo with two
 * parameters, an @c int and a C string.
 *
 * But sometimes you may want to pass a number of parameters known only at run
 * time.  In those cases, a @c dynamic_params encodes a dynamically
 * determined number of parameters.  You can mix these with regular, static
 * parameter lists, and you can re-use them for multiple statement invocations.
 *
 * A dynamic_params object does not store copies of its parameters, so make
 * sure they remain accessible until you've executed the statement.
 *
 * The ACCESSOR is an optional callable (such as a lambda).  If you pass an
 * accessor @c a, then each parameter @c p goes into your statement as @c a(p).
 */
template<typename IT, typename ACCESSOR = decltype(iterator_identity<IT>)>
class dynamic_params
{
public:
  /// Wrap a sequence of pointers or iterators.
  constexpr dynamic_params(IT begin, IT end) :
          m_begin(begin), m_end(end), m_accessor(iterator_identity<IT>)
  {}

  /// Wrap a sequence of pointers or iterators.
  /** This version takes an accessor callable.  If you pass an accessor @c acc,
   * then any parameter @c p will go into the statement's parameter list as
   * @c acc(p).
   */
  constexpr dynamic_params(IT begin, IT end, ACCESSOR &acc) :
          m_begin(begin), m_end(end), m_accessor(acc)
  {}

  /// Wrap a container.
  template<typename C>
  explicit constexpr dynamic_params(C &container) :
          dynamic_params(std::begin(container), std::end(container))
  {}

  /// Wrap a container.
  /** This version takes an accessor callable.  If you pass an accessor @c acc,
   * then any parameter @c p will go into the statement's parameter list as
   * @c acc(p).
   */
  template<typename C>
  explicit constexpr dynamic_params(C &container, ACCESSOR &acc) :
          dynamic_params(std::begin(container), std::end(container), acc)
  {}

  constexpr IT begin() const { return m_begin; }
  constexpr IT end() const { return m_end; }

  constexpr auto access(decltype(*std::declval<IT>()) value) const
    -> decltype(std::declval<ACCESSOR>()(value))
  {
    return m_accessor(value);
  }

private:
  IT const m_begin, m_end;
  ACCESSOR m_accessor = iterator_identity<IT>;
};


/// Internal type: encode statement parameters.
/** Compiles arguments for prepared statements and parameterised queries into
 * a format that can be passed into libpq.
 *
 * Objects of this type are meant to be short-lived.  For example, if you pass
 * in a non-null pointer as a parameter, it may simply use that pointer as a
 * parameter value.  All values referenced by parameters must remain "live"
 * until the call is complete.
 */
struct params
{
  /// Construct directly from a series of statement arguments.
  /** The arrays all default to zero, null, and empty strings.
   */
  template<typename... Args> constexpr params(Args &&...args)
  {
    strings.reserve(sizeof...(args));
    lengths.reserve(sizeof...(args));
    nonnulls.reserve(sizeof...(args));
    binaries.reserve(sizeof...(args));

    // Start recursively storing parameters.
    add_fields(std::forward<Args>(args)...);
  }

  /// Compose a vector of pointers to parameter string values.
  std::vector<char const *> get_pointers() const
  {
    std::size_t const num_fields{std::size(lengths)};
    std::size_t cur_string{0}, cur_bin_string{0};
    std::vector<char const *> pointers(num_fields);
    for (std::size_t index{0}; index < num_fields; index++)
    {
      char const *value;
      if (binaries[index] != 0)
      {
        value =
          reinterpret_cast<char const *>(bin_strings[cur_bin_string].data());
        cur_bin_string++;
      }
      else if (nonnulls[index] != 0)
      {
        value = strings[cur_string].c_str();
        cur_string++;
      }
      else
      {
        value = nullptr;
      }
      pointers[index] = value;
    }
    return pointers;
  }

  /// String values, for string parameters.
  std::vector<std::string> strings;
  /// As used by libpq: lengths of non-null arguments, in bytes.
  std::vector<int> lengths;
  /// As used by libpq: boolean "is this parameter non-null?"
  std::vector<int> nonnulls;
  /// As used by libpq: boolean "is this parameter in binary format?"
  std::vector<int> binaries;
  /// Binary string values, for binary parameters.
  std::vector<std::basic_string_view<std::byte>> bin_strings;

private:
  static constexpr std::string_view s_overflow{
    "Statement parameter length overflow."sv};

  /// Add a non-null string field.
  void add_field(std::string_view text)
  {
    lengths.push_back(check_cast<int>(internal::ssize(text), s_overflow));
    nonnulls.push_back(1);
    binaries.push_back(0);
    strings.emplace_back(std::string{text});
  }

  /// Add a non-null string field.
  void add_field(std::string &&text)
  {
    lengths.push_back(check_cast<int>(internal::ssize(text), s_overflow));
    nonnulls.push_back(1);
    binaries.push_back(0);
    strings.emplace_back(std::move(text));
  }

  /// Compile one argument (specialised for null pointer, a null value).
  void add_field(std::nullptr_t)
  {
    lengths.push_back(0);
    nonnulls.push_back(0);
    binaries.push_back(0);
  }

  /// Compile one argument (specialised for binarystring).
  void add_field(binarystring const &arg)
  {
    lengths.push_back(check_cast<int>(internal::ssize(arg), s_overflow));
    nonnulls.push_back(1);
    binaries.push_back(1);
    bin_strings.push_back(std::basic_string_view<std::byte>{
      reinterpret_cast<std::byte const *>(arg.data()), std::size(arg)});
  }

  /// Compile one binary argument.
  void add_field(std::basic_string_view<std::byte> arg)
  {
    lengths.push_back(check_cast<int>(internal::ssize(arg), s_overflow));
    nonnulls.push_back(1);
    binaries.push_back(1);
    bin_strings.push_back(arg);
  }

  /// Compile one argument (default, generic implementation).
  /** Uses string_traits to represent the argument as a std::string.
   */
  template<typename Arg> void add_field(Arg const &arg)
  {
    if (is_null(arg))
      add_field(nullptr);
    else
      add_field(to_string(arg));
  }

  /// Special treatment for smart pointers, std::optional, etc.
  /** These overload allow parameters to "see through" some standard wrapper
   * types into the underlying type.
   *
   * It's not strictly needed, but consider what happens when you pass e.g.
   * @c std::optional<std::basic_string<std::byte>>.  It would otherwise go in
   * as a normal parameter, meaning libpqxx converts the value to a string and
   * passes it as a text parameter.  But with these overloads, it will end up
   * in the @c std::basic_string<std::byte> overload, which gets passed in a
   * binary format.  This saves us an encoding pass, transfers only half the
   * number of bytes, and saves the backend a decoding pass.
   */
  template<typename Arg> void add_field(std::shared_ptr<Arg> arg)
  {
    if (arg)
      add_field(*arg);
    else
      add_field(nullptr);
  }
  /// Special treatment for smart pointers, std::optional, etc.
  template<typename Arg> void add_field(std::unique_ptr<Arg> arg)
  {
    if (arg)
      add_field(*arg);
    else
      add_field(nullptr);
  }
  /// Special treatment for smart pointers, std::optional, etc.
  template<typename Arg> void add_field(std::optional<Arg> arg)
  {
    if (arg.has_value())
      add_field(arg.value());
    else
      add_field(nullptr);
  }

  /// Compile a dynamic_params object into a dynamic number of parameters.
  template<typename IT, typename ACCESSOR>
  void add_field(dynamic_params<IT, ACCESSOR> const &parameters)
  {
    for (auto param : parameters) add_field(parameters.access(param));
  }

  /// Compile argument list.
  /** This recursively "peels off" the next remaining element, compiles its
   * information into its final form, and calls itself for the rest of the
   * list.
   *
   * @param arg Current argument to be compiled.
   * @param args Optional remaining arguments, to be compiled recursively.
   */
  template<typename Arg, typename... More>
  void add_fields(Arg &&arg, More &&...args)
  {
    add_field(std::forward<Arg>(arg));
    // Compile remaining arguments, if any.
    add_fields(std::forward<More>(args)...);
  }

  /// Terminating version of add_fields, at the end of the list.
  /** Recursion in add_fields ends with this call.
   */
  void add_fields() {}
};
} // namespace pqxx::internal

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
