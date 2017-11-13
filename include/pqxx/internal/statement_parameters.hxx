/** Common implementation for statement parameter lists.
 *
 * These are used for both prepared statements and parameterized statements.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY.  Other headers include it for you.
 *
 * Copyright (c) 2009-2017, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_STATEMENT_PARAMETER
#define PQXX_H_STATEMENT_PARAMETER

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <cstring>
#include <string>
#include <vector>

#if defined(PQXX_HAVE_OPTIONAL)
#include <optional>
#endif

#if defined(PQXX_HAVE_EXP_OPTIONAL)
#include <experimental/optional>
#endif

#include "pqxx/binarystring"
#include "pqxx/strconv"
#include "pqxx/util"


namespace pqxx
{
namespace internal
{
class PQXX_LIBEXPORT statement_parameters
{
protected:
  statement_parameters() =default;
  statement_parameters &operator=(const statement_parameters &) =delete;

  void add_param() { this->add_checked_param("", false, false); }
  template<typename T> void add_param(const T &v, bool nonnull)
  {
    nonnull = (nonnull && !pqxx::string_traits<T>::is_null(v));
    this->add_checked_param(
	(nonnull ? pqxx::to_string(v) : ""),
	nonnull,
	false);
  }
  void add_binary_param(const binarystring &b, bool nonnull)
	{ this->add_checked_param(b.str(), nonnull, true); }

  /// Marshall parameter values into C-compatible arrays for passing to libpq.
  int marshall(
	std::vector<const char *> &values,
	std::vector<int> &lengths,
	std::vector<int> &binaries) const;

private:
  void add_checked_param(const std::string &, bool nonnull, bool binary);

  std::vector<std::string> m_values;
  std::vector<bool> m_nonnull;
  std::vector<bool> m_binary;
};


/// Internal type: encode statement parameters.
/** Compiles arguments for prepared statements and parameterised queries into
 * a format that can be passed into libpq.
 *
 * Objects of this type are meant to be short-lived.  If you pass in a non-null
 * pointer as a parameter, it may simply use that pointer as a parameter value.
 */
struct params
{
  /// Construct directly from a series of statement arguments.
  /** The arrays all default to zero, null, and empty strings.
   */
  template<typename ...Args> params(Args... args) :
    values(sizeof...(args), nullptr),
    lengths(sizeof...(args), 0),
    nonnulls(sizeof...(args), 0),
    binaries(sizeof...(args), 0)
  {
    // Pre-allocate room for all strings.  There's no need to construct them
    // now, but we *must* allocate the space here.  Otherwise, re-allocation
    // inside the vector may invalidate the c_str() pointers that we store!
    //
    // How could re-allocation invalidate those pointers?  Possibly because
    // the library implementation uses copy-construction instead of move
    // construction when growing the vector, or possibly because of the
    // Small String Optimisation.
    m_strings.reserve(sizeof...(args));

    // Start recursively storing parameters.
    set_fields(0, args...);
  }

  /// As used by libpq: values, as C string pointers.
  std::vector<const char *> values;
  /// As used by libpq: lengths of non-null arguments, in bytes.
  std::vector<int> lengths;
  /// As used by libpq: boolean "is this parameter non-null?"
  std::vector<int> nonnulls;
  /// As used by libpq: boolean "is this parameter in binary format?"
  std::vector<int> binaries;

private:
  /// Compile one argument (default implementation).
  /** Uses string_traits to represent the argument as a std::string.
   */
  template<typename Arg> void set_field(std::size_t index, Arg arg)
  {
    if (!string_traits<Arg>::is_null(arg))
    {
      nonnulls[index] = 1;
      // Convert to string, store the result in m_strings so that we can pass
      // a C-style pointer to its text.
      m_strings.emplace_back(to_string(arg));
      const auto &storage = m_strings.back();
      values[index] = storage.c_str();
      lengths[index] = int(storage.size());
    }
  }

  /// Compile one argument (specialised for null pointer, a null value).
  void set_field(std::size_t, std::nullptr_t)
  {
    // Nothing to do: using default values.
  }

  /// Compile one argument (specialised for C-style string).
  void set_field(std::size_t index, const char arg[])
  {
    // This argument is already in exactly the right format.  Don't bother
    // turning it into a std::string; just use the pointer we were given.
    // Of course this would be a memory bug if the string's memory were
    // deallocated before we made our actual call.
    values[index] = arg;
    if (arg != nullptr)
    {
      nonnulls[index] = 1;
      lengths[index] = int(std::strlen(arg));
    }
  }

  /// Compile one argument(specialised for C-style string, signed).
  void set_field(std::size_t index, const signed char arg[])
  {
    // The type we got is close enough to the one we want.  Just cast it.
    using target_type = const char *;
    set_field(index, target_type(arg));
  }

  /// Compile one argument (specialised for C-style string, unsigned).
  void set_field(std::size_t index, const unsigned char arg[])
  {
    // The type we got is close enough to the one we want.  Just cast it.
    using target_type = const char *;
    set_field(index, target_type(arg));
  }

  /// Compile one argument (specialised for binarystring).
  void set_field(std::size_t index, const binarystring &arg)
  {
    // Again, assume that the value will stay in memory until we're done.
    values[index] = arg.get();
    nonnulls[index] = 1;
    binaries[index] = 1;
    lengths[index] = int(arg.size());
  }

#if defined(PQXX_HAVE_OPTIONAL)
  /// Compile one argument (specialised for std::optional<type>).
  template<typename Arg> void set_field(
	std::size_t index,
	const std::optional<Arg> &arg)
  {
    if (arg.has_value()) set_field(index, arg.value());
    else set_field(index, nullptr);
  }
#endif

#if defined(PQXX_HAVE_EXP_OPTIONAL)
  /// Compile one argument (specialised for std::experimental::optional<type>).
  template<typename Arg> void set_field(
	std::size_t index,
	const std::experimental::optional<Arg> &arg)
  {
    if (arg) set_field(index, arg.value());
    else set_field(index, nullptr);
  }
#endif

  /// Compile argument list.
  /** This recursively "peels off" the next remaining element, compiles its
   * information into its final form, and calls itself for the rest of the
   * list.
   *
   * @param index Number of this argument, zero-based.
   * @param arg Current argument to be compiled.
   * @param args Optional remaining arguments, to be compiled recursively.
   */
  template<typename Arg, typename ...More>
  void set_fields(std::size_t index, Arg arg, More... args)
  {
    set_field(index, arg);
    // Compile remaining arguments, if any.
    set_fields(index + 1, args...);
  }

  /// Terminating version of set_fields, at the end of the list.
  /** Recurstion in set_fields ends with this call.
   */
  void set_fields(std::size_t) {}

  /// Storage for stringified parameter values.
  std::vector<std::string> m_strings;

};
} // namespace pqxx::internal
} // namespace pqxx

#include "pqxx/compiler-internal-post.hxx"

#endif
