/* Various utility definitions for libpqxx.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/util instead.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_UTIL
#define PQXX_H_UTIL

#include "pqxx/compiler-public.hxx"

#include <cstdio>
#include <cctype>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <vector>

#include "pqxx/strconv.hxx"
#include "pqxx/version.hxx"


/// The home of all libpqxx classes, functions, templates, etc.
namespace pqxx {}

#include <pqxx/internal/libpq-forward.hxx>


namespace pqxx
{
/// Suppress compiler warning about an unused item.
template<typename T> inline void ignore_unused(T) {}


/** Check library version at link time.
 *
 * Ensures a failure when linking an application against a radically
 * different libpqxx version than the one against which it was compiled.
 *
 * Sometimes application builds fail in unclear ways because they compile
 * using headers from libpqxx version X, but then link against libpqxx
 * binary version Y.  A typical scenario would be one where you're building
 * against a libpqxx which you have built yourself, but a different version
 * is installed on the system.
 *
 * The check_library_version template is declared for any library version,
 * but only actually defined for the version of the libpqxx binary against
 * which the code is linked.
 *
 * If the library binary is a different version than the one declared in
 * these headers, then this call will fail to link: there will be no
 * definition for the function with these exact template parameter values.
 * There will be a definition, but the version in the parameter values will
 * be different.
 */
inline PQXX_PRIVATE void check_version()
{
  // There is no particular reason to do this here in @c connection, except
  // to ensure that every meaningful libpqxx client will execute it.  The call
  // must be in the execution path somewhere or the compiler won't try to link
  // it.  We can't use it to initialise a global or class-static variable,
  // because a smart compiler might resolve it at compile time.
  //
  // On the other hand, we don't want to make a useless function call too
  // often for performance reasons.  A local static variable is initialised
  // only on the definition's first execution.  Compilers will be well
  // optimised for this behaviour, so there's a minimal one-time cost.
  static const auto version_ok =
    internal::check_library_version<PQXX_VERSION_MAJOR, PQXX_VERSION_MINOR>();
  ignore_unused(version_ok);
}


/// Descriptor of library's thread-safety model.
/** This describes what the library knows about various risks to thread-safety.
 */
struct PQXX_LIBEXPORT thread_safety_model
{
  /// @deprecated Is error reporting thread-safe?  Now always true.
  bool have_safe_strerror = true;

  /// Is the underlying libpq build thread-safe?
  bool safe_libpq;

  /// @deprecated Query cancel is always thread-safe now.
  bool safe_query_cancel = true;

  /// @deprecated Always thread-safe to copy a 'result' or 'binarystring' now.
  bool safe_result_copy = true;

  /// Is Kerberos thread-safe?
  /** @warning Is currently always @c false.
   *
   * If your application uses Kerberos, all accesses to libpqxx or Kerberos must
   * be serialized.  Confine their use to a single thread, or protect it with a
   * global lock.
   */
  bool safe_kerberos;

  /// A human-readable description of any thread-safety issues.
  std::string description;
};


/// Describe thread safety available in this build.
PQXX_LIBEXPORT thread_safety_model describe_thread_safety() noexcept;


/// The "null" oid.
constexpr oid oid_none = 0;


/**
 * @defgroup utility Utility functions
 */
//@{

/// Represent sequence of values as a string, joined by a given separator.
/**
 * Use this to turn e.g. the numbers 1, 2, and 3 into a string "1, 2, 3".
 *
 * @param sep separator string (to be placed between items)
 * @param begin beginning of items sequence
 * @param end end of items sequence
 * @param access functor defining how to dereference sequence elements
 */
template<typename ITER, typename ACCESS> inline std::string
separated_list(								//[t00]
	std::string_view sep,
	ITER begin,
	ITER end,
	ACCESS access)
{
  std::string result;
// TODO: Can we pre-compute optimal length for result?
  if (begin != end)
  {
    result = to_string(access(begin));
    for (++begin; begin != end; ++begin)
    {
      result += sep;
      result += to_string(access(begin));
    }
  }
  return result;
}


/// Render sequence as a string, using given separator between items.
template<typename ITER> inline std::string
separated_list(std::string_view sep, ITER begin, ITER end)		//[t00]
	{ return separated_list(sep, begin, end, [](ITER i){ return *i; }); }


/// Render items in a container as a string, using given separator.
template<typename CONTAINER> inline auto
separated_list(std::string_view sep, const CONTAINER &c)		//[t10]
	/*
	Always std::string; necessary because SFINAE doesn't work with the
	contents of function bodies, so the check for iterability has to be in
	the signature.
	*/
	-> typename std::enable_if<
		(
			not std::is_void<decltype(std::begin(c))>::value
			and not std::is_void<decltype(std::end(c))>::value
		),
		std::string
	>::type
{
  return separated_list(sep, std::begin(c), std::end(c));
}


/// Render items in a tuple as a string, using given separator.
template<
	typename TUPLE,
	std::size_t INDEX=0,
	typename ACCESS,
	typename std::enable_if<
		(INDEX == std::tuple_size<TUPLE>::value-1),
		int
	>::type=0
>
inline std::string
separated_list(
	std::string_view /* sep */,
	const TUPLE &t,
	const ACCESS& access
)
{
  return to_string(access(&std::get<INDEX>(t)));
}

template<
	typename TUPLE,
	std::size_t INDEX=0,
	typename ACCESS,
	typename std::enable_if<
		(INDEX < std::tuple_size<TUPLE>::value-1),
		int
	>::type=0
>
inline std::string
separated_list(std::string_view sep, const TUPLE &t, const ACCESS& access)
{
  std::string out{to_string(access(&std::get<INDEX>(t)))};
  out.append(sep);
  out.append(separated_list<TUPLE, INDEX+1>(sep, t, access));
  return out;
}

template<
	typename TUPLE,
	std::size_t INDEX=0,
	typename std::enable_if<
		(INDEX <= std::tuple_size<TUPLE>::value),
		int
	>::type=0
>
inline std::string
separated_list(std::string_view sep, const TUPLE &t)
{
  return separated_list(sep, t, [](const TUPLE &tup){return *tup;});
}
//@}
} // namespace pqxx


/// Private namespace for libpqxx's internal use; do not access.
/** This namespace hides definitions internal to libpqxx.  These are not
 * supposed to be used by client programs, and they may change at any time
 * without notice.
 *
 * Conversely, if you find something in this namespace tremendously useful, by
 * all means do lodge a request for its publication.
 *
 * @warning Here be dragons!
 */
namespace pqxx::internal
{
PQXX_LIBEXPORT void freepqmem(const void *) noexcept;
template<typename P> inline void freepqmem_templated(P *p) noexcept
{
  freepqmem(p);
}

PQXX_LIBEXPORT void freemallocmem(const void *) noexcept;
template<typename P> inline void freemallocmem_templated(P *p) noexcept
{
  freemallocmem(p);
}


/// Helper base class: object descriptions for error messages and such.
/**
 * Classes derived from namedclass have a class name (such as "transaction"),
 * an optional object name (such as "delete-old-logs"), and a description
 * generated from the two names (such as "transaction delete-old-logs").
 *
 * The class name is dynamic here, in order to support inheritance hierarchies
 * where the exact class name may not be known statically.
 *
 * In inheritance hierarchies, make namedclass a virtual base class so that
 * each class in the hierarchy can specify its own class name in its
 * constructors.
 */
class PQXX_LIBEXPORT namedclass
{
public:
  explicit namedclass(const std::string &Classname) :
    m_classname{Classname},
    m_name{}
  {
  }

  namedclass(const std::string &Classname, const std::string &Name) :
    m_classname{Classname},
    m_name{Name}
  {
  }

  /// Object name, or the empty string if no name was given.
  const std::string &name() const noexcept { return m_name; }		//[t01]

  /// Class name.
  const std::string &classname() const noexcept
	{ return m_classname; }

  /// Combination of class name and object name; or just class name.
  std::string description() const;

private:
  std::string m_classname, m_name;
};


PQXX_PRIVATE void check_unique_registration(
        const namedclass *new_ptr, const namedclass *old_ptr);
PQXX_PRIVATE void check_unique_unregistration(
        const namedclass *new_ptr, const namedclass *old_ptr);


/// Ensure proper opening/closing of GUEST objects related to a "host" object
/** Only a single GUEST may exist for a single host at any given time.  GUEST
 * must be derived from namedclass.
 */
template<typename GUEST>
class unique
{
public:
  unique() =default;
  unique(const unique &) =delete;
  unique(unique &&rhs) : m_guest(rhs.m_guest) { rhs.m_guest = nullptr; }
  unique &operator=(const unique &) =delete;
  unique &operator=(unique &&rhs)
  {
    m_guest = rhs.m_guest;
    rhs.m_guest = nullptr;
    return *this;
  }

  GUEST *get() const noexcept { return m_guest; }

  void register_guest(GUEST *G)
  {
    check_unique_registration(G, m_guest);
    m_guest = G;
  }

  void unregister_guest(GUEST *G)
  {
    check_unique_unregistration(G, m_guest);
    m_guest = nullptr;
  }

private:
  GUEST *m_guest = nullptr;
};


/// Sleep for the given number of seconds
/** May return early, e.g. when interrupted by a signal.  Completes instantly if
 * a zero or negative sleep time is requested.
 */
PQXX_LIBEXPORT void sleep_seconds(int);

} // namespace pqxx::internal

#endif
