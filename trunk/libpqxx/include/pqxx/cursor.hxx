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
 * Copyright (c) 2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/libcompiler.h"

#include <limits>
#include <string>

#include "pqxx/result"


namespace pqxx
{
class transaction_base;

/// Common definitions for cursor types
/** @warning This code is experimental.  It is not fully covered by libpqxx'
 * regression tests, and may see considerable change before becoming part of a
 * stable release.  Do not use except to test it.
 */
class PQXX_LIBEXPORT cursor_base
{
public:
  typedef result::size_type size_type;

  operator void *() const { return m_done ? 0 : &s_dummy; }		//[t81]
  bool operator!() const { return m_done; }				//[t81]

  static size_type all() throw ();					//[t81]
  static size_type next() throw () { return 1; }			//[t81]
  static size_type prior() throw () { return -1; }			//[]
  static size_type backward_all() throw ();				//[]

  const PGSTD::string &name() const throw () { return m_name; }		//[t81]

protected:
  cursor_base(transaction_base *context, const PGSTD::string &cname) :
  	m_context(context), m_done(false), m_name(cname)
  {
    m_name += "_";
    m_name += to_string(get_unique_cursor_num());
  }

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


inline cursor_base::size_type cursor_base::all() throw ()
{
#ifdef _MSC_VER
  // Microsoft's compiler defines max() and min() macros!  Others may as well
  return INT_MAX;
#else
  return PGSTD::numeric_limits<size_type>::max();
#endif
}

inline cursor_base::size_type cursor_base::backward_all() throw ()
{
#ifdef _MSC_VER
  // Microsoft's compiler defines max() and min() macros!  Others may as well
  return INT_MIN + 1;
#else
  return PGSTD::numeric_limits<size_type>::min() + 1;
#endif
}

/// Simple read-only cursor represented as a stream of results
/** Data is fetched from the cursor as a sequence of result objects.  Each of
 * these will contain the number of rows defined as the stream's stride, except
 * of course the last block of data which may contain fewer rows.
 *
 * @warning This code is experimental.  It is not fully covered by libpqxx'
 * regression tests, and may see considerable change before becoming part of a
 * stable release.  Do not use except to test it.
 */
class PQXX_LIBEXPORT icursorstream : public cursor_base
{
public:
  /// Set up a read-only, forward-only cursor
  /** Roughly equivalent to a C++ Standard Library istream, this cursor type
   * supports only two operations: reading a block of rows while moving forward,
   * and moving forward without reading any data.
   *
   * @param context transaction context that this cursor will be active in
   * @param query SQL query whose results this cursor shall iterate
   * @param basename suggested name for the SQL cursor; a unique code will be
   * appended by the library to ensure its uniqueness
   * @param stride the number of rows to fetch per read operation; must be a
   * positive number
   */
  icursorstream(transaction_base &context,
      const PGSTD::string &query,
      const PGSTD::string &basename,
      size_type stride=1);						//[t81]

  /// Read new value into given result object; same as operator >>
  /** The result set may continue any number of rows from zero to the chosen
   * stride, inclusive.  An empty result will only be returned if there are no
   * more rows to retrieve.
   */
  icursorstream &get(result &res) { res = fetch(); return *this; }	//[]
  /// Read new value into given result object; same as get(result &)
  /** The result set may continue any number of rows from zero to the chosen
   * stride, inclusive.  An empty result will only be returned if there are no
   * more rows to retrieve.
   */
  icursorstream &operator>>(result &res) { return get(res); }		//[t81]
  /// Move given number of rows forward (ignoring stride) without reading data
  icursorstream &ignore(PGSTD::streamsize n=1);				//[]

  /// Change stride, i.e. the number of rows to fetch per read operation
  /**
   * @param stride must be a positive number
   */
  void set_stride(size_type stride);					//[t81]

private:
  void declare(const PGSTD::string &query);
  result fetch();

  size_type m_stride;
};


/// Approximate istream_iterator for icursorstream
/** A rough equivalent of the C++ Standard Library's istream_iterator or
 * input_iterator, this class supports only two basic operations: reading the
 * current element, and moving forward.  In addition to the minimal guarantees
 * for istream_iterators, this class supports multiple successive reads of the
 * same position (the current result set is cached in the iterator) even after
 * copying and even after new data have been read from the stream.
 *
 * The iterator has no concept of its own position, however.  Moving an iterator
 * forward moves the underlying stream forward and reads the data from the new
 * position, regardless of "where the iterator was" in the stream.  Comparison
 * of iterators is only supported for detecting the end of a stream.
 *
 * @warning This code is experimental.  It is not fully covered by libpqxx'
 * regression tests, and may see considerable change before becoming part of a
 * stable release.  Do not use except to test it.
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

  icursor_iterator() : m_stream(0), m_here(), m_fresh(true) {}		//[]
  icursor_iterator(istream_type &s) :					//[]
    m_stream(&s), m_here(), m_fresh(false) {}
  icursor_iterator(const icursor_iterator &rhs) : 			//[]
    m_stream(rhs.m_stream), m_here(rhs.m_here), m_fresh(rhs.m_fresh) {}

  const result &operator*() const { refresh(); return m_here; }		//[]
  const result *operator->() const { refresh(); return &m_here; }	//[]

  icursor_iterator &operator++() { read(); return *this; }		//[]

  icursor_iterator operator++(int)					//[]
  	{ icursor_iterator old(*this); read(); return old; }

  icursor_iterator &operator+=(size_type n)				//[]
  {
    m_stream->ignore(n);
    m_fresh = false;
    return *this;
  }

  icursor_iterator &operator=(const icursor_iterator &rhs)		//[]
  {
    m_here = rhs.m_here;	// (Already protected against self-assignment)
    m_stream = rhs.m_stream;	// (Does not throw, so we're exception-safe)
    m_fresh = rhs.m_fresh;
    return *this;
  }

  bool operator==(const icursor_iterator &rhs) const throw ()		//[]
  	{ return m_here.empty() && rhs.m_here.empty(); }
  bool operator!=(const icursor_iterator &rhs) const throw ()		//[]
  	{ return !operator==(rhs); }

private:
  void read() const
  {
    m_stream->get(m_here);
    m_fresh = true;
  }
  void refresh() const { if (!m_fresh) read(); }
  icursorstream *m_stream;
  mutable result m_here;
  mutable bool m_fresh;
};


} // namespace pqxx

