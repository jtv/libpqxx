/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/util.hxx
 *
 *   DESCRIPTION
 *      Various utility definitions for libpqxx
 *      DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/util instead.
 *
 * Copyright (c) 2001-2017, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_UTIL
#define PQXX_H_UTIL

#include "pqxx/compiler-public.hxx"

#include <cstdio>
#include <cctype>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

#include "pqxx/strconv"


/** @mainpage
 * @author Jeroen T. Vermeulen
 *
 * Welcome to libpqxx, the C++ API to the PostgreSQL database management system.
 *
 * Compiling this package requires PostgreSQL to be installed--including the C
 * headers for client development.  The library builds on top of PostgreSQL's
 * standard C API, libpq.  The libpq headers are not needed to compile client
 * programs, however.
 *
 * For a quick introduction to installing and using libpqxx, see the README.md
 * file; a more extensive tutorial is available in doc/html/Tutorial/index.html.
 *
 * The latest information can be found at http://pqxx.org/
 *
 * Some links that should help you find your bearings:
 * \li \ref gettingstarted
 * \li \ref threading
 * \li \ref connection
 * \li \ref transaction
 * \li \ref performance
 * \li \ref transactor
 *
 * @see http://pqxx.org/
 */

/** @page gettingstarted Getting started
 * The most basic three types in libpqxx are the connection (which inherits its
 * API from pqxx::connection_base and its setup behaviour from
 * pqxx::connectionpolicy), the transaction (derived from
 * pqxx::transaction_base), and the result (pqxx::result).
 *
 * They fit together as follows:
 * \li You connect to the database by creating a
 * connection object (see \ref connection).  The connection type you'll usually
 * want is pqxx::connection.
 * \li You create a transaction object (see \ref transaction) operating on that
 * connection.  You'll usually want the pqxx::work variety.  If you don't want
 * transactional behaviour, use pqxx::nontransaction.  Once you're done you call
 * the transaction's @c commit function to make its work final.  If you don't
 * call this, the work will be rolled back when the transaction object is
 * destroyed.
 * \li Until then, use the transaction's @c exec function to execute queries,
 * which you pass in as simple strings.
 * \li The function returns a pqxx::result object, which acts as a standard
 * container of rows.  Each row in itself acts as a container of fields.  You
 * can use array indexing and/or iterators to access either.
 * \li The field's data is stored as a text string.  You can read it as such
 * using its @c c_str() function, or convert it to other types using its @c as()
 * and @c to() member functions.  These are templated on the destination type:
 * @c myfield.as<int>(); or @c myfield.to(myint);
 * \li After you've closed the transaction, the connection is free to run a next
 * transaction.
 *
 * Here's a very basic example.  It connects to the default database (you'll
 * need to have one set up), queries it for a very simple result, converts it to
 * an @c int, and prints it out.  It also contains some basic error handling.
 *
 * @code
 * #include <iostream>
 * #include <pqxx/pqxx>
 *
 * int main()
 * {
 *   try
 *   {
 *     pqxx::connection c;
 *     pqxx::work w(c);
 *     pqxx::result r = w.exec("SELECT 1");
 *     w.commit();
 *
 *     std::cout << r[0][0].as<int>() << std::endl;
 *   }
 *   catch (const std::exception &e)
 *   {
 *     std::cerr << e.what() << std::endl;
 *     return 1;
 *   }
 * }
 * @endcode
 *
 * This prints the number 1.  Notice that you can keep the result object
 * around after the transaction (or even the connection) has been closed.
 *
 * Here's a slightly more complicated example.  It takes an argument from the
 * command line and retrieves a string with that value.  The interesting part is
 * that it uses the escaping-and-quoting function @c quote() to embed this
 * string value in SQL safely.  It also reads the result field's value as a
 * plain C-style string using its @c c_str() function.
 *
 * @code
 * #include <iostream>
 * #include <pqxx/pqxx>
 *
 * int main(int argc, char *argv[])
 * {
 *   if (!argv[1])
 *   {
 *     std::cerr << "Give me a string!" << std::endl;
 *     return 1;
 *   }
 *   try
 *   {
 *     pqxx::connection c;
 *     pqxx::work w(c);
 *     pqxx::result r = w.exec("SELECT " + w.quote(argv[1]));
 *     w.commit();
 *
 *     std::cout << r[0][0].c_str() << std::endl;
 *   }
 *   catch (const std::exception &e)
 *   {
 *     std::cerr << e.what() << std::endl;
 *     return 1;
 *   }
 * }
 * @endcode
 *
 * You can find more about converting field values to native types, or
 * converting values to strings for use with libpqxx, under
 * \ref stringconversion.  More about getting to the rows and fields of a
 * result is under \ref accessingresults.
 *
 * If you want to handle exceptions thrown by libpqxx in more detail, for
 * example to print the SQL contents of a query that failed, see \ref exception.
 */

/** @page accessingresults Accessing results and result rows
 *
 * Let's say you have a result object.  For example, your program may have done:
 *
 * @code
 * pqxx::result r = w.exec("SELECT * FROM mytable");
 * @endcode
 *
 * Now how do you access the data inside @c r?
 *
 * The simplest way is array indexing.  A result acts as an array of rows,
 * and a row acts as an array of fields.
 *
 * @code
 * const int num_rows = r.size();
 * for (int rownum=0; rownum < num_rows; ++rownum)
 * {
 *   const pqxx::row row = r[rownum];
 *   const int num_cols = row.size();
 *   for (int colnum=0; colnum < num_cols; ++colnum)
 *   {
 *     const pqxx::field field = row[colnum];
 *     std::cout << field.c_str() << '\t';
 *   }
 *
 *   std::cout << std::endl;
 * }
 * @endcode
 *
 * But results and rows also define @c const_iterator types:
 *
 * @code
 * for (const auto &row: r)
 *  {
 *    for (const auto &field: row) std::cout << field.c_str() << '\t';
 *    std::cout << std::endl;
 *  }
 * @endcode
 *
 * They also have @c const_reverse_iterator types, which iterate backwards from
 * @c rbegin() to @c rend() exclusive.
 *
 * All these iterator types provide one extra bit of convenience that you won't
 * normally find in C++ iterators: referential transparency.  You don't need to
 * dereference them to get to the row or field they refer to.  That is, instead
 * of @c row->end() you can also choose to say @c row.end().  Similarly, you
 * may prefer @c field.c_str() over @c field->c_str().
 *
 * This becomes really helpful with the array-indexing operator.  With regular
 * C++ iterators you would need ugly expressions like @c (*row)[0] or
 * @c row->operator[](0).  With the iterator types defined by the result and
 * row classes you can simply say @c row[0].
 */

/** @page threading Thread safety
 *
 * This library does not contain any locking code to protect objects against
 * simultaneous modification in multi-threaded programs.  Therefore it is up
 * to you, the user of the library, to ensure that your threaded client
 * programs perform no conflicting operations concurrently.
 *
 * Most of the time this isn't hard.  Result sets are immutable, so you can
 * share them between threads without problem.  The main rule is:
 *
 * \li Treat a connection, together with any and all objects related to it, as
 * a "world" of its own.  You should generally make sure that the same "world"
 * is never accessed by another thread while you're doing anything non-const
 * in there.
 *
 * That means: don't issue a query on a transaction while you're also opening
 * a subtransaction, don't access a cursor while you may also be committing,
 * and so on.
 *
 * In particular, cursors are tricky.  It's easy to perform a non-const
 * operation without noticing.  So, if you're going to share cursors or
 * cursor-related objects between threads, lock very conservatively!
 *
 * Use @c pqxx::describe_thread_safety to find out at runtime what level of
 * thread safety is implemented in your build and version of libpqxx.  It
 * returns a pqxx::thread_safety_model describing what you can and cannot rely
 * on.  A command-line utility @c tools/pqxxthreadsafety prints out the same
 * information.
 */

/// The home of all libpqxx classes, functions, templates, etc.
namespace pqxx {}

#include <pqxx/internal/libpq-forward.hxx>


namespace pqxx
{
/// Descriptor of library's thread-safety model.
/** This describes what the library knows about various risks to thread-safety.
 */
struct PQXX_LIBEXPORT thread_safety_model
{
  /// Does standard C library have a thread-safe alternative to @c strerror?
  /** If not, its @c strerror implementation may still be thread-safe.  Check
   * your compiler's manual.
   *
   * Without @c strerror_r (@c strerror_s in Visual C++) or a thread-safe @c
   * strerror, the library can't safely obtain descriptions of certain run-time
   * errors.  In that case, your application must serialize all use of libpqxx.
   */
  bool have_safe_strerror;

  /// Is the underlying libpq build thread-safe?
  /** A @c "false" here may mean one of two things: either the libpq build is
   * not thread-safe, or it is a thread-safe build of an older version that did
   * not offer thread-safety information.
   *
   * In that case, the best fix is to rebuild libpqxx against (a thread-safe
   * build of) a newer libpq version.
   */
  bool safe_libpq;

  /// Is canceling queries thread-safe?
  /** If not, avoid use of the pqxx::pipeline class in threaded programs.  Or
   * better, rebuild libpqxx against a newer libpq version.
   */
  bool safe_query_cancel;

  /// Are copies of pqxx::result and pqxx::binarystring objects thread-safe?
  /** If @c true, it's safe to copy an object of either of these types (copying
   * these is done very efficiently, so don't worry about data size) and hand
   * the copy off to another thread.  The other thread may access the copy
   * freely without any concurrency concerns.
   */
  bool safe_result_copy;

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


namespace internal
{
// TODO: Does standard library provide a ready-made version of this?
/// Functor: dereference iterator
template<typename ITER> struct dereference
{
  typename ITER::value_type operator()(ITER i) const { return *i; }
};
template<typename T> struct deref_ptr { T operator()(T *i) const {return *i;} };
} // namespace internal


/// Access iterators using ACCESS functor, returning separator-separated list
/**
 * @param sep separator string (to be placed between items)
 * @param begin beginning of items sequence
 * @param end end of items sequence
 * @param access functor defining how to dereference sequence elements
 */
template<typename ITER, typename ACCESS> inline
std::string separated_list(						//[t0]
	const std::string &sep,
	ITER begin,
	ITER end,
	ACCESS access)
{
  std::string result;
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


/**
 * @defgroup utility Utility functions
 */
//@{

/// Render sequence as a string, using given separator between items
template<typename ITER> inline std::string
separated_list(const std::string &sep, ITER begin, ITER end)		//[t0]
	{ return separated_list(sep,begin,end,internal::dereference<ITER>()); }


/// Render array as a string, using given separator between items
template<typename OBJ> inline std::string
separated_list(const std::string &sep, OBJ *begin, OBJ *end)		//[t0]
	{ return separated_list(sep,begin,end,internal::deref_ptr<OBJ>()); }


/// Render items in a container as a string, using given separator
template<typename CONTAINER> inline std::string
separated_list(const std::string &sep, const CONTAINER &c)		//[t10]
	{ return separated_list(sep, c.begin(), c.end()); }
//@}

/// Private namespace for libpqxx's internal use; do not access
/** This namespace hides definitions internal to libpqxx.  These are not
 * supposed to be used by client programs, and they may change at any time
 * without notice.
 *
 * Conversely, if you find something in this namespace tremendously useful, by
 * all means do lodge a request for its publication.
 *
 * @warning Here be dragons!
 */
namespace internal
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
    m_classname(Classname),
    m_name()
  {
  }

  namedclass(const std::string &Classname, const std::string &Name) :
    m_classname(Classname),
    m_name(Name)
  {
  }

  /// Object name, or the empty string if no name was given.
  const std::string &name() const noexcept { return m_name; }		//[t1]

  /// Class name.
  const std::string &classname() const noexcept				//[t73]
	{ return m_classname; }

  /// Combination of class name and object name; or just class name.
  std::string description() const;

private:
  std::string m_classname, m_name;
};


void CheckUniqueRegistration(const namedclass *New, const namedclass *Old);
void CheckUniqueUnregistration(const namedclass *New, const namedclass *Old);


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
  unique &operator=(const unique &) =delete;

  GUEST *get() const noexcept { return m_guest; }

  void register_guest(GUEST *G)
  {
    CheckUniqueRegistration(G, m_guest);
    m_guest = G;
  }

  void unregister_guest(GUEST *G)
  {
    CheckUniqueUnregistration(G, m_guest);
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

/// Work around problem with library export directives and pointers.
using cstring = const char *;


/// Human-readable description for error code, possibly using given buffer
/** Wrapper for @c strerror() or thread-safe variant, as available.  The
 * default code copies the string to the provided buffer, but this may not
 * always be necessary.  The result is guaranteed to remain usable for as long
 * as the given buffer does.
 * @param err system error code as copied from errno
 * @param buf caller-provided buffer for message to be stored in, if needed
 * @param len usable size (in bytes) of provided buffer
 * @return human-readable error string, which may or may not reside in buf
 */
PQXX_LIBEXPORT cstring strerror_wrapper(int err, char buf[], std::size_t len)
  noexcept;


/// Commonly used SQL commands
extern const char sql_begin_work[], sql_commit_work[], sql_rollback_work[];

} // namespace internal
} // namespace pqxx

#endif
