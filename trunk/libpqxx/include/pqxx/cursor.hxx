/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/cursor.hxx
 *
 *   DESCRIPTION
 *      definition of the iterator/container-style cursor classes
 *   C++-style wrappers for SQL cursors
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/pipeline instead.
 *
 * Copyright (c) 2004-2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/libcompiler.h"

#ifdef PQXX_HAVE_LIMITS
#include <limits>
#endif

#include "pqxx/result"


namespace pqxx
{
class transaction_base;

/// Common definitions for cursor types
class PQXX_LIBEXPORT cursor_base
{
public:
  typedef result::size_type size_type;
  typedef result::difference_type difference_type;

  /// Does it make sense to try reading from this cursor again?
  /** @return Null pointer if finished, non-null if not */
  operator void *() const { return m_done ? 0 : &s_dummy; }		//[t81]
  /// Is this cursor finished?
  /** The logical negation of the converstion-to-pointer operator.
   * @return Whether the last attempt to read failed
   */
  bool operator!() const { return m_done; }				//[t81]

  /// Special value: read until end
  /** @return Maximum value for result::difference_type, so the cursor will
   * attempt to read the largest possible result set.
   */
  static difference_type all() throw ();				//[t81]
  /// Special value: read one row only
  /** @return Unsurprisingly, 1
   */
  static difference_type next() throw () { return 1; }			//[t81]
  /// Special value: read backwards, one row only
  /** @return Unsurprisingly, -1
   */
  static difference_type prior() throw () { return -1; }		//[t0]
  /// Special value: read backwards from current position back to origin
  /** @return Minimum value for result::difference_type
   */
  static difference_type backward_all() throw ();			//[t0]

  /// Name of underlying SQL cursor
  /**
   * @returns Name of SQL cursor, which may differ from the given name.
   * @warning Don't use this to access the SQL cursor directly without going
   * through the provided wrapper classes!
   */
  const PGSTD::string &name() const throw () { return m_name; }		//[t81]

protected:
  cursor_base(transaction_base *,
      const PGSTD::string &cname,
      bool embellish_name = true);

  static PGSTD::string stridestring(difference_type);
  transaction_base *m_context;
  bool m_done;

private:
  int get_unique_cursor_num();

  PGSTD::string m_name;

  /// Purely to give us a non-null pointer to return
  static unsigned char s_dummy;

  /// Not allowed
  cursor_base();
  /// Not allowed
  cursor_base(const cursor_base &);
  /// Not allowed
  cursor_base &operator=(const cursor_base &);
};


inline cursor_base::difference_type cursor_base::all() throw ()
{
  // Microsoft Visual C++ sabotages numeric limits by defining min() and max()
  // as preprocessor macros; some other compilers just don't have numeric_limits
#if defined(PQXX_HAVE_LIMITS) && !defined(_MSC_VER)
  return PGSTD::numeric_limits<difference_type>::max();
#else
  return INT_MAX;
#endif
}

inline cursor_base::difference_type cursor_base::backward_all() throw ()
{
#if defined(PQXX_HAVE_LIMITS) && !defined(_MSC_VER)
  return PGSTD::numeric_limits<difference_type>::min() + 1;
#else
  return INT_MIN + 1;
#endif
}

class icursor_iterator;

/// Simple read-only cursor represented as a stream of results
/** Data is fetched from the cursor as a sequence of result objects.  Each of
 * these will contain the number of rows defined as the stream's stride, except
 * of course the last block of data which may contain fewer rows.
 *
 * This class can create or adopt cursors that live in nontransactions, i.e.
 * outside any backend transaction, which your backend version may not support.
 */
class PQXX_LIBEXPORT icursorstream : public cursor_base
{
public:
  /// Set up a read-only, forward-only cursor
  /** Roughly equivalent to a C++ Standard Library istream, this cursor type
   * supports only two operations: reading a block of rows while moving forward,
   * and moving forward without reading any data.
   *
   * @param Context Transaction context that this cursor will be active in
   * @param Query SQL query whose results this cursor shall iterate
   * @param Basename Suggested name for the SQL cursor; a unique code will be
   * appended by the library to ensure its uniqueness
   * @param Stride Number of rows to fetch per read operation; must be a
   * positive number
   */
  icursorstream(transaction_base &Context,
      const PGSTD::string &Query,
      const PGSTD::string &Basename,
      difference_type Stride=1);					//[t81]

  /// Adopt existing SQL cursor.  Use with care.
  /** Forms a cursor stream around an existing SQL cursor, as returned by e.g. a
   * server-side function.  The SQL cursor will be cleaned up by the stream's
   * destructor as if it had been created by the stream; cleaning it up by hand
   * or adopting the same cursor twice is an error.
   *
   * Passing the name of the cursor as a string is not allowed, both to avoid
   * confusion with the other constructor and to discourage unnecessary use of
   * adopted cursors.
   *
   * @warning It is technically possible to adopt a "WITH HOLD" cursor, i.e. a
   * cursor that stays alive outside its creating transaction.  However, any
   * cursor stream (including the underlying SQL cursor, naturally) must be
   * destroyed before its transaction context object is destroyed.  Therefore
   * the only way to use SQL's WITH HOLD feature is to adopt the cursor, but
   * defer doing so until after entering the transaction context that will
   * eventually destroy it.
   *
   * @param Context Transaction context that this cursor will be active in
   * @param Name Result field containing the name of the SQL cursor to adopt
   * @param Stride Number of rows to fetch per read operation; must be a
   * positive number
   */
  icursorstream(transaction_base &Context,
      const result::field &Name,
      difference_type Stride=1); 					//[t84]

  /// Read new value into given result object; same as operator >>
  /** The result set may continue any number of rows from zero to the chosen
   * stride, inclusive.  An empty result will only be returned if there are no
   * more rows to retrieve.
   * @return Reference to this very stream, to facilitate "chained" invocations
   * (@code C.get(r1).get(r2); @endcode)
   */
  icursorstream &get(result &res) { res = fetch(); return *this; }	//[t81]
  /// Read new value into given result object; same as get(result &)
  /** The result set may continue any number of rows from zero to the chosen
   * stride, inclusive.  An empty result will only be returned if there are no
   * more rows to retrieve.
   * @return Reference to this very stream, to facilitate "chained" invocations
   * (@code C >> r1 >> r2; @endcode)
   */
  icursorstream &operator>>(result &res) { return get(res); }		//[t81]
  /// Move given number of rows forward (ignoring stride) without reading data
  /**
   * @return Reference to this very stream, to facilitate "chained" invocations
   * (@code C.ignore(2).get(r).ignore(4); @endcode)
   */
  icursorstream &ignore(PGSTD::streamsize n=1);				//[t81]

  /// Change stride, i.e. the number of rows to fetch per read operation
  /**
   * @param stride Must be a positive number
   */
  void set_stride(difference_type stride);				//[t81]
  difference_type stride() const throw () { return m_stride; }		//[t81]

private:
  void declare(const PGSTD::string &query);
  result fetch();

  friend class icursor_iterator;
  size_type forward(size_type n=1);
  void insert_iterator(icursor_iterator *) throw ();
  void remove_iterator(icursor_iterator *) const throw ();

  void service_iterators(size_type);

  difference_type m_stride;
  size_type m_realpos, m_reqpos;

  mutable icursor_iterator *m_iterators;
};


/// Approximate istream_iterator for icursorstream
/** Intended as an implementation of an input_iterator (as defined by the C++
 * Standard Library), this class supports only two basic operations: reading the
 * current element, and moving forward.  In addition to the minimal guarantees
 * for istream_iterators, this class supports multiple successive reads of the
 * same position (the current result set is cached in the iterator) even after
 * copying and even after new data have been read from the stream.  This appears
 * to be a requirement for input_iterators.  Comparisons are also supported in
 * the general case.
 *
 * The iterator does not care about its own position, however.  Moving an
 * iterator forward moves the underlying stream forward and reads the data from
 * the new stream position, regardless of the iterator's old position in the
 * stream.
 *
 * The stream's stride defines the granularity for all iterator movement or
 * access operations, i.e. "ici += 1" advances the stream by one stride's worth
 * of tuples, and "*ici++" reads one stride's worth of tuples from the stream.
 *
 * @warning Do not read from the underlying stream or its cursor, move its read
 * position, or change its stride, between the time the first icursor_iterator
 * on it is created and the time its last icursor_iterator is destroyed.
 */
class PQXX_LIBEXPORT icursor_iterator :
  public PGSTD::iterator<PGSTD::input_iterator_tag,
  	result,
	cursor_base::size_type,
	const result *,
	const result &>
{
public:
  typedef icursorstream istream_type;
  typedef istream_type::size_type size_type;
  typedef istream_type::difference_type difference_type;

  icursor_iterator() throw ();						//[t84]
  explicit icursor_iterator(istream_type &) throw ();			//[t84]
  icursor_iterator(const icursor_iterator &) throw ();			//[t84]
  ~icursor_iterator() throw ();

  const result &operator*() const { refresh(); return m_here; }		//[t84]
  const result *operator->() const { refresh(); return &m_here; }	//[t84]
  icursor_iterator &operator++();					//[t84]
  icursor_iterator operator++(int);					//[t84]
  icursor_iterator &operator+=(difference_type);			//[t84]
  icursor_iterator &operator=(const icursor_iterator &) throw ();	//[t84]

  bool operator==(const icursor_iterator &rhs) const;			//[t84]
  bool operator!=(const icursor_iterator &rhs) const throw ()		//[t84]
  	{ return !operator==(rhs); }
  bool operator<(const icursor_iterator &rhs) const;			//[t84]
  bool operator>(const icursor_iterator &rhs) const			//[t84]
	{ return rhs < *this; }
  bool operator<=(const icursor_iterator &rhs) const			//[t84]
	{ return !(*this > rhs); }
  bool operator>=(const icursor_iterator &rhs) const			//[t84]
	{ return !(*this < rhs); }

private:
  void refresh() const;

  friend class icursorstream;
  size_type pos() const throw () { return m_pos; }
  void fill(const result &) const;

  icursorstream *m_stream;
  mutable result m_here;
  size_type m_pos;
  icursor_iterator *m_prev, *m_next;
};


} // namespace pqxx

