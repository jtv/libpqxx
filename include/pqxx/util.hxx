/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/util.hxx
 *
 *   DESCRIPTION
 *      Various utility definitions for libpqxx
 *      DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/util instead.
 *
 * Copyright (c) 2001-2009, Jeroen T. Vermeulen <jtv@xs4all.nl>
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
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

#ifdef PQXX_TR1_HEADERS
#include <tr1/memory>
#else
#include <memory>
#endif


/** @mainpage
 * @author Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * Welcome to libpqxx, the C++ API to the PostgreSQL database management system.
 *
 * Compiling this package requires PostgreSQL to be installed--including the C
 * headers for client development.  The library builds on top of PostgreSQL's
 * standard C API, libpq.  The libpq headers are not needed to compile client
 * programs, however.
 *
 * For a quick introduction to installing and using libpqxx, see the README
 * file; a more extensive tutorial is available in doc/html/Tutorial/index.html.
 *
 * The latest information, as well as updates, a mailing list, and a bug
 * reporting system can be found at the project's home page.
 *
 * Some links that should help you find your bearings:
 * \li \ref gettingstarted
 * \li \ref threading
 * \li \ref connection
 * \li \ref transaction
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
 * \li Within that connection, you create a transaction object (see
 * \ref transaction).  You'll usually want the pqxx::work variety.  If you don't
 * want transactional behaviour, use pqxx::nontransaction.  Once you're done you
 * call the transaction's @c commit function to make its work final.  If you
 * don't call this, the work will be rolled back when the transaction object is
 * destroyed.
 * \li While you have the transaction object, use its @c exec function to
 * execute a query.  The query is a simple string.  The function returns a
 * pqxx::result object, which acts as a standard container of rows.  Each row in
 * itself acts as a container of fields.  You can use array indexing and/or
 * iterators to access either.
 * \li The field's data is stored as a text string.  You can read it as such
 * using its @c c_str() function, or convert it to other types using its @c as()
 * and @c to() functions.  These are templated on the destination type.
 *
 * Here's a very basic example.  It connects to the default database (you'll
 * need to have one set up), asks the database for a very simple result,
 * converts it to an @c int, and prints it.  It also contains some basic error
 * handling.
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
 * This should print the number 1.  Notice that you can keep the result object
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
 * \ref stringconversion.
 *
 * These examples access the result object and its contents through indexing, as
 * if they were arrays.  But they also define @c const_iterator types:
 *
 * @code
 *     for (pqxx::result::const_iterator i = r.begin();
 *          i != r.end();
 *          ++i)
 *       std::cout << i[0].c_str() << std::endl;
 * @endcode
 *
 * And @c const_reverse_iterator types:
 *
 * @code
 *     for (pqxx::result::const_reverse_iterator i = r.begin();
 *          i != r.rend();
 *          ++i)
 *       std::cout << i[0].c_str() << std::endl;
 * @endcode
 *
 * That goes not just for the rows in the result, but for the fields in a row as
 * well:
 *
 * @code
 *     const result::tuple tup = r[0];
 *     for (result::tuple::const_iterator j = tup.begin();
 *          j != tup.end();
 *          ++j)
 *       std::cout << j->c_str() << std::endl;
 * @endcode
 *
 * If you want to handle exceptions thrown by libpqxx in more detail, for
 * example to print the SQL contents of a query that failed, see \ref exception.
 */

/** @page threading Thread safety
 *
 * This library does not contain any locking code to protect objects against
 * simultaneous modification in multi-threaded programs.  There are many
 * different threading interfaces and libpqxx does not dictate the choice.
 *
 * Therefore it is up to you, the user of the library, to ensure that your
 * threaded client programs perform no conflicting operations concurrently.
 *
 * The library does try to avoid non-obvious unsafe operations and so does the
 * underlying libpq.  Here's what you should do to keep your threaded libpqxx
 * application safe:
 *
 * \li Treat a connection and all objects related to it as a "world" of its own.
 * With some exceptions (see below), you should make sure that the same "world"
 * is never accessed concurrently by multiple threads.
 *
 * \li Result sets (pqxx::result) and binary data (pqxx::binarystring)
 * are special.  Copying these objects is very cheap, and you can give the copy
 * to another thread.  Just make sure that no other thread accesses the same
 * copy when it's being assigned to, swapped, cleared, or destroyed.
 *
 * @warning Prior to libpqxx 3.1, or in C++ environments without the standard
 * smart pointer type @c "shared_ptr," copying, assigning, or destroying a
 * pqxx::result or pqxx::binarystring could also affect any other other object
 * of the same type referring to the same underlying data.
 */

/// The home of all libpqxx classes, functions, templates, etc.
namespace pqxx {}

/// Alias for the standard namespace "<tt>std</tt>"
/** This alias is used throughout libpqxx to accomodate the use of other
 * standard library implementations than the one native to the compiler.  These
 * alternative implementations may live in separate namespaces to avoid
 * confusion.
 */
namespace PGSTD {}

#include <pqxx/libpq-forward.hxx>


namespace pqxx
{
/// The "null" oid
const oid oid_none = 0;

/// Container of items with easy contents initialization and string rendering
/** Designed as a wrapper around an arbitrary container type, this class lets
 * you easily create a container object and provide its contents in the same
 * line.  Regular addition methods such as push_back() will also still work, but
 * you can now write things like
 * @code
 *  items<int> numbers; numbers(1)(2)(3)(4);
 * @endcode
 *
 * Up to five elements may be specified directly as constructor arguments, e.g.
 * @code
 * items<int> numbers(1,2,3,4);
 * @endcode
 *
 * One thing that cannot be done with this simple class is create const objects
 * with nontrivial contents.  This is because the function invocation operator
 * (which is being used to add items) modifies the container rather than
 * creating a new one.  This was done to keep performance within reasonable
 * bounds.
 *
 * @warning This class may see substantial change in its interface before it
 * stabilizes.  Do not count on it remaining the way it is.
 */
template<typename T=PGSTD::string, typename CONT=PGSTD::vector<T> >
class items : public CONT
{
public:
  /// Create empty items list
  items() : CONT() {}							//[t80]
  /// Create items list with one element
  explicit items(const T &t) : CONT() { push_back(t); }			//[t0]
  items(const T &t1, const T &t2) : CONT()				//[t80]
	{ push_back(t1); push_back(t2); }
  items(const T &t1, const T &t2, const T &t3) : CONT()			//[t0]
	{ push_back(t1); push_back(t2); push_back(t3); }
  items(const T &t1, const T &t2, const T &t3, const T &t4) : CONT()	//[t0]
	{ push_back(t1); push_back(t2); push_back(t3); push_back(t4); }
  items(const T&t1,const T&t2,const T&t3,const T&t4,const T&t5):CONT()	//[t0]
	{push_back(t1);push_back(t2);push_back(t3);push_back(t4);push_back(t5);}
  /// Copy container
  items(const CONT &c) : CONT(c) {}					//[t0]

  /// Add element to items list
  items &operator()(const T &t)						//[t80]
  {
    push_back(t);
    return *this;
  }
};


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
PGSTD::string separated_list(const PGSTD::string &sep,			//[t0]
    ITER begin,
    ITER end,
    ACCESS access)
{
  PGSTD::string result;
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
template<typename ITER> inline PGSTD::string
separated_list(const PGSTD::string &sep, ITER begin, ITER end)		//[t8]
	{ return separated_list(sep,begin,end,internal::dereference<ITER>()); }


/// Render array as a string, using given separator between items
template<typename OBJ> inline PGSTD::string
separated_list(const PGSTD::string &sep, OBJ *begin, OBJ *end)		//[t9]
	{ return separated_list(sep,begin,end,internal::deref_ptr<OBJ>()); }


/// Render items in a container as a string, using given separator
template<typename CONTAINER> inline PGSTD::string
separated_list(const PGSTD::string &sep, const CONTAINER &c)		//[t10]
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
typedef unsigned long result_size_type;
typedef long result_difference_type;
} // namespace internal


namespace internal
{
void PQXX_LIBEXPORT freepqmem(const void *);


#ifdef PQXX_HAVE_SHARED_PTR

/// Reference-counted smart pointer to libpq-allocated object
template<typename T> class PQAlloc : protected PGSTD::tr1::shared_ptr<T>
{
  typedef PGSTD::tr1::shared_ptr<T> super;
public:
  typedef T content_type;
  PQAlloc() : super() {}
  explicit PQAlloc(T *t) : super(t, PQAlloc::freemem) {}

  using super::get;
  using super::operator=;
  using super::operator->;
  using super::operator*;
  using super::reset;
  using super::swap;

private:
  static void freemem(T *t) throw () { freepqmem(t); }
};

#else // !PQXX_HAVE_SHARED_PTR

/// Helper class used in reference counting (doubly-linked circular list)
/// Reference-counted smart-pointer for libpq-allocated resources.
class PQXX_LIBEXPORT refcount
{
  refcount *volatile m_l, *volatile m_r;

public:
  refcount();
  ~refcount();

  /// Create additional reference based on existing refcount object
  void makeref(refcount &) throw ();

  /// Drop this reference; return whether we were the last reference
  bool loseref() throw ();

private:
  /// Not allowed
  refcount(const refcount &);
  /// Not allowed
  refcount &operator=(const refcount &);
};


/// Reference-counted smart pointer to libpq-allocated object
/** Keep track of a libpq-allocated object, and free it once all references to
 * it have died.
 *
 * If the available PostgreSQL development files supply @c PQfreemem() or
 * @c PQfreeNotify(), this is used to free the memory.  If not, free() is used
 * instead.  This matters on Windows, where memory allocated by a DLL must be
 * freed by the same DLL.
 *
 * @warning Copying, swapping, and destroying PQAlloc objects that refer to the
 * same underlying libpq-allocated block is <em>not thread-safe</em>.  If you
 * wish to pass reference-counted objects around between threads, make sure that
 * each of these operations is protected against concurrency with similar
 * operations on the same object--or other copies of the same object.
 */
template<typename T> class PQAlloc
{
  T *m_Obj;
  mutable refcount m_rc;
public:
  typedef T content_type;

  PQAlloc() throw () : m_Obj(0), m_rc() {}
  PQAlloc(const PQAlloc &rhs) throw () : m_Obj(0), m_rc() { makeref(rhs); }
  ~PQAlloc() throw () { loseref(); }

  PQAlloc &operator=(const PQAlloc &rhs) throw () {redoref(rhs); return *this;}

  /// Assume ownership of a pointer
  /** @warning Don't to this more than once for a given object!
   */
  explicit PQAlloc(T *obj) throw () : m_Obj(obj), m_rc() {}

  void swap(PQAlloc &rhs) throw ()
  {
    PQAlloc tmp(*this);
    *this = rhs;
    rhs = tmp;
  }

  //PQAlloc &operator=(T *obj) throw () { redoref(obj); return *this; }

  /// Is this pointer non-null?
  operator bool() const throw () { return m_Obj != 0; }

  /// Is this pointer null?
  bool operator!() const throw () { return !m_Obj; }

  /// Dereference pointer
  /** Throws a logic_error if the pointer is null.
   */
  T *operator->() const throw (PGSTD::logic_error)
  {
    if (!m_Obj) throw PGSTD::logic_error("Null pointer dereferenced");
    return m_Obj;
  }

  /// Dereference pointer
  /** Throws a logic_error if the pointer is null.
   */
  T &operator*() const throw (PGSTD::logic_error) { return *operator->(); }

  /// Obtain underlying pointer
  /** Ownership of the pointer's memory remains with the PQAlloc object
   */
  T *get() const throw () { return m_Obj; }

  void reset() throw () { loseref(); }

private:
  void makeref(T *p) throw () { m_Obj = p; }

  void makeref(const PQAlloc &rhs) throw ()
  {
    m_Obj = rhs.m_Obj;
    m_rc.makeref(rhs.m_rc);
  }

  /// Free and reset current pointer (if any)
  void loseref() throw ()
  {
    if (m_rc.loseref() && m_Obj) freemem(m_Obj);
    m_Obj = 0;
  }

  void redoref(const PQAlloc &rhs) throw ()
	{ if (rhs.m_Obj != m_Obj) { loseref(); makeref(rhs); } }
  void redoref(T *obj) throw ()
	{ if (obj != m_Obj) { loseref(); makeref(obj); } }

  void freemem(T *t) throw () { freepqmem(t); }
};

#endif // PQXX_HAVE_SHARED_PTR


void PQXX_LIBEXPORT freemem_notif(const pq::PGnotify *) throw ();
template<>
inline void PQAlloc<const pq::PGnotify>::freemem(const pq::PGnotify *t) throw ()
	{ freemem_notif(t); }


template<typename T> class scoped_array
{
  T *m_ptr;
public:
  typedef size_t size_type;
  typedef long difference_type;

  scoped_array() : m_ptr(0) {}
  explicit scoped_array(size_type n) : m_ptr(new T[n]) {}
  explicit scoped_array(T *t) : m_ptr(t) {}
  ~scoped_array() { delete [] m_ptr; }

  T *get() const throw () { return m_ptr; }
  T &operator*() const throw () { return *m_ptr; }
  template<typename INDEX> T &operator[](INDEX i) const throw ()
	{ return m_ptr[i]; }

  scoped_array &operator=(T *t) throw ()
  {
    if (t != m_ptr)
    {
      delete [] m_ptr;
      m_ptr = t;
    }
    return *this;
  }

private:
  /// Not allowed
  scoped_array(const scoped_array &);
  scoped_array &operator=(const scoped_array &);
};


class PQXX_LIBEXPORT namedclass
{
public:
  namedclass(const PGSTD::string &Classname, const PGSTD::string &Name="") :
    m_Classname(Classname),
    m_Name(Name)
  {
  }

  const PGSTD::string &name() const throw () { return m_Name; }		//[t1]
  const PGSTD::string &classname() const throw () {return m_Classname;}	//[t73]
  PGSTD::string description() const;

private:
  PGSTD::string m_Classname, m_Name;
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
  unique() : m_Guest(0) {}

  GUEST *get() const throw () { return m_Guest; }

  void Register(GUEST *G)
  {
    CheckUniqueRegistration(G, m_Guest);
    m_Guest = G;
  }

  void Unregister(GUEST *G)
  {
    CheckUniqueUnregistration(G, m_Guest);
    m_Guest = 0;
  }

private:
  GUEST *m_Guest;

  /// Not allowed
  unique(const unique &);
  /// Not allowed
  unique &operator=(const unique &);
};

/// Sleep for the given number of seconds
/** May return early, e.g. when interrupted by a signal.  Completes instantly if
 * a zero or negative sleep time is requested.
 */
void PQXX_LIBEXPORT sleep_seconds(int);

/// Work around problem with library export directives and pointers
typedef const char *cstring;

/// Human-readable description for error code, possibly using given buffer
/** Wrapper for @c strerror() or @c strerror_r(), as available.  The normal case
 * is to copy the string to the provided buffer, but this may not always be the
 * case.  The result is guaranteed to remain usable for as long as the given
 * buffer does.
 * @param err system error code as copied from errno
 * @param buf caller-provided buffer for message to be stored in, if needed
 * @param len usable size (in bytes) of provided buffer
 * @return human-readable error string, which may or may not reside in buf
 */
cstring PQXX_LIBEXPORT strerror_wrapper(int err, char buf[], PGSTD::size_t len)
  throw ();


/// Commonly used SQL commands
extern const char sql_begin_work[], sql_commit_work[], sql_rollback_work[];

} // namespace internal
} // namespace pqxx

#endif

