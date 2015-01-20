/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/cursor.hxx
 *
 *   DESCRIPTION
 *      definition of the iterator/container-style cursor classes
 *   C++-style wrappers for SQL cursors
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/cursor instead.
 *
 * Copyright (c) 2004-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_CURSOR
#define PQXX_H_CURSOR

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <limits>
#include <stdexcept>

#include "pqxx/result"
#include "pqxx/transaction_base"


namespace pqxx
{
class dbtransaction;


/// Common definitions for cursor types
/** In C++ terms, fetches are always done in pre-increment or pre-decrement
 * fashion--i.e. the result does not include the row the cursor is on at the
 * beginning of the fetch, and the cursor ends up being positioned on the last
 * row in the result.
 *
 * There are singular positions akin to @c end() at both the beginning and the
 * end of the cursor's range of movement, although these fit in so naturally
 * with the semantics that one rarely notices them.  The cursor begins at the
 * first of these, but any fetch in the forward direction will move the cursor
 * off this position and onto the first row before returning anything.
 */
class PQXX_LIBEXPORT cursor_base
{
public:
  typedef result::size_type size_type;
  typedef result::difference_type difference_type;

  /// Cursor access-pattern policy
  /** Allowing a cursor to move forward only can result in better performance,
   * so use this access policy whenever possible.
   */
  enum accesspolicy
  {
    /// Cursor can move forward only
    forward_only,
    /// Cursor can move back and forth
    random_access
  };

  /// Cursor update policy
  /**
   * @warning Not all PostgreSQL versions support updatable cursors.
   */
  enum updatepolicy
  {
    /// Cursor can be used to read data but not to write
    read_only,
    /// Cursor can be used to update data as well as read it
    update
  };

  /// Cursor destruction policy
  /** The normal thing to do is to make a cursor object the owner of the SQL
   * cursor it represents.  There may be cases, however, where a cursor needs to
   * persist beyond the end of the current transaction (and thus also beyond the
   * lifetime of the cursor object that created it!), where it can be "adopted"
   * into a new cursor object.  See the basic_cursor documentation for an
   * explanation of cursor adoption.
   *
   * If a cursor is created with "loose" ownership policy, the object
   * representing the underlying SQL cursor will not take the latter with it
   * when its own lifetime ends, nor will its originating transaction.
   *
   * @warning Use this feature with care and moderation.  Only one cursor object
   * should be responsible for any one underlying SQL cursor at any given time.
   *
   * @warning Don't "leak" cursors!  As long as any "loose" cursor exists,
   * any attempts to deactivate or reactivate the connection, implicitly or
   * explicitly, are quietly ignored.
   */
  enum ownershippolicy
  {
    /// Destroy SQL cursor when cursor object is closed at end of transaction
    owned,
    /// Leave SQL cursor in existence after close of object and transaction
    loose
  };

  /**
   * @name Special movement distances
   */
  //@{
  /// Special value: read until end
  /** @return Maximum value for result::difference_type, so the cursor will
   * attempt to read the largest possible result set.
   */
  static difference_type all() PQXX_NOEXCEPT;				//[t81]
  /// Special value: read one row only
  /** @return Unsurprisingly, 1
   */
  static difference_type next() PQXX_NOEXCEPT { return 1; }		//[t81]
  /// Special value: read backwards, one row only
  /** @return Unsurprisingly, -1
   */
  static difference_type prior() PQXX_NOEXCEPT { return -1; }		//[t0]
  /// Special value: read backwards from current position back to origin
  /** @return Minimum value for result::difference_type
   */
  static difference_type backward_all() PQXX_NOEXCEPT;			//[t0]

  //@}

  /// Name of underlying SQL cursor
  /**
   * @returns Name of SQL cursor, which may differ from original given name.
   * @warning Don't use this to access the SQL cursor directly without going
   * through the provided wrapper classes!
   */
  const std::string &name() const PQXX_NOEXCEPT { return m_name; }	//[t81]

protected:
  cursor_base(connection_base &,
	const std::string &Name,
	bool embellish_name=true);

  const std::string m_name;

private:
  /// Not allowed
  cursor_base();
  /// Not allowed
  cursor_base(const cursor_base &);
  /// Not allowed
  cursor_base &operator=(const cursor_base &);
};


inline cursor_base::difference_type cursor_base::all() PQXX_NOEXCEPT
{
  return std::numeric_limits<int>::max()-1;
}

inline cursor_base::difference_type cursor_base::backward_all() PQXX_NOEXCEPT
{
  return std::numeric_limits<int>::min()+1;
}


namespace internal
{
/// Cursor with SQL positioning semantics
/** Thin wrapper around an SQL cursor, with SQL's ideas of positioning.
 *
 * SQL cursors have pre-increment/pre-decrement semantics, with on either end of
 * the result set a special position that does not repesent a row.  This class
 * models SQL cursors for the purpose of implementing more C++-like semantics on
 * top.
 *
 * Positions of actual rows are numbered starting at 1.  Position 0 exists but
 * does not refer to a row.  There is a similar non-row position at the end of
 * the result set.
 *
 * Don't use this at home.  You deserve better.  Use the stateles_cursor
 * instead.
 */
class PQXX_LIBEXPORT sql_cursor : public cursor_base
{
public:
  sql_cursor(transaction_base &t,
	const std::string &query,
	const std::string &cname,
	cursor_base::accesspolicy ap,
	cursor_base::updatepolicy up,
	cursor_base::ownershippolicy op,
	bool hold);

  sql_cursor(transaction_base &t,
	const std::string &cname,
	cursor_base::ownershippolicy op);

  ~sql_cursor() PQXX_NOEXCEPT { close(); }

  result fetch(difference_type rows, difference_type &displacement);
  result fetch(difference_type rows)
				{ difference_type d=0; return fetch(rows, d); }
  difference_type move(difference_type rows, difference_type &displacement);
  difference_type move(difference_type rows)
				{ difference_type d=0; return move(rows, d); }

  /// Current position, or -1 for unknown
  /**
   * The starting position, just before the first row, counts as position zero.
   *
   * Position may be unknown if (and only if) this cursor was adopted, and has
   * never hit its starting position (position zero).
   */
  difference_type pos() const PQXX_NOEXCEPT { return m_pos; }

  /// End position, or -1 for unknown
  /**
   * Returns the final position, just after the last row in the result set.  The
   * starting position, just before the first row, counts as position zero.
   *
   * End position is unknown until it is encountered during use.
   */
  difference_type endpos() const PQXX_NOEXCEPT { return m_endpos; }

  /// Return zero-row result for this cursor
  const result &empty_result() const PQXX_NOEXCEPT { return m_empty_result; }

  void close() PQXX_NOEXCEPT;

private:
  difference_type adjust(difference_type hoped, difference_type actual);
  static std::string stridestring(difference_type);
  /// Initialize cached empty result.  Call only at beginning or end!
  void init_empty_result(transaction_base &);

  /// Connection this cursor lives in
  connection_base &m_home;

  /// Zero-row result from this cursor (or plain empty one if cursor is adopted)
  result m_empty_result;

  result m_cached_current_row;

  /// Is this cursor adopted (as opposed to created by this cursor object)?
  bool m_adopted;

  /// Will this cursor object destroy its SQL cursor when it dies?
  cursor_base::ownershippolicy m_ownership;

  /// At starting position (-1), somewhere in the middle (0), or past end (1)
  int m_at_end;

  /// Position, or -1 for unknown
  difference_type m_pos;

  /// End position, or -1 for unknown
  difference_type m_endpos;
};


PQXX_LIBEXPORT result::size_type obtain_stateless_cursor_size(sql_cursor &);
PQXX_LIBEXPORT result stateless_cursor_retrieve(
	sql_cursor &,
	result::difference_type size,
	result::difference_type begin_pos,
	result::difference_type end_pos);

} // namespace internal


/// "Stateless cursor" class: easy API for retrieving parts of result sets
/** This is a front-end for SQL cursors, but with a more C++-like API.
 *
 * Actually, stateless_cursor feels entirely different from SQL cursors.  You
 * don't keep track of positions, fetches, and moves; you just say which rows
 * you want.  See the retrieve() member function.
 */
template<cursor_base::updatepolicy up, cursor_base::ownershippolicy op>
class stateless_cursor
{
public:
  typedef result::size_type size_type;
  typedef result::difference_type difference_type;

  /// Create cursor.
  stateless_cursor(
	transaction_base &trans,
	const std::string &query,
	const std::string &cname,
	bool hold) :
    m_cur(trans, query, cname, cursor_base::random_access, up, op, hold)
  {
  }

  /// Adopt existing scrolling SQL cursor.
  stateless_cursor(
	transaction_base &trans,
	const std::string adopted_cursor) :
    m_cur(trans, adopted_cursor, op)
  {
    // Put cursor in known position
    m_cur.move(cursor_base::backward_all());
  }

  void close() PQXX_NOEXCEPT { m_cur.close(); }

  /// Number of rows in cursor's result set
  /** @note This function is not const; it may need to scroll to find the size
   * of the result set.
   */
  size_type size() { return internal::obtain_stateless_cursor_size(m_cur); }

  /// Retrieve rows from begin_pos (inclusive) to end_pos (exclusive)
  /** Rows are numbered starting from 0 to size()-1.
   *
   * @param begin_pos First row to retrieve.  May be one row beyond the end of
   * the result set, to avoid errors for empty result sets.  Otherwise, must be
   * a valid row number in the result set.
   * @param end_pos Row up to which to fetch.  Rows are returned ordered from
   * begin_pos to end_pos, i.e. in ascending order if begin_pos < end_pos but
   * in descending order if begin_pos > end_pos.  The end_pos may be arbitrarily
   * inside or outside the result set; only existing rows are included in the
   * result.
   */
  result retrieve(difference_type begin_pos, difference_type end_pos)
  {
    return internal::stateless_cursor_retrieve(
	m_cur,
	result::difference_type(size()),
	begin_pos,
	end_pos);
  }

  const std::string &name() const PQXX_NOEXCEPT { return m_cur.name(); }

private:
  internal::sql_cursor m_cur;
};


class icursor_iterator;


namespace internal
{
namespace gate
{
class icursor_iterator_icursorstream;
class icursorstream_icursor_iterator;
} // namespace internal::gate
} // namespace internal


/// Simple read-only cursor represented as a stream of results
/** SQL cursors can be tricky, especially in C++ since the two languages seem to
 * have been designed on different planets.  An SQL cursor has two singular
 * positions akin to @c end() on either side of the underlying result set.
 *
 * These cultural differences are hidden from view somewhat by libpqxx, which
 * tries to make SQL cursors behave more like familiar C++ entities such as
 * iterators, sequences, streams, and containers.
 *
 * Data is fetched from the cursor as a sequence of result objects.  Each of
 * these will contain the number of rows defined as the stream's stride, except
 * of course the last block of data which may contain fewer rows.
 *
 * This class can create or adopt cursors that live outside any backend
 * transaction, which your backend version may not support.
 */
class PQXX_LIBEXPORT icursorstream
{
public:
  typedef cursor_base::size_type size_type;
  typedef cursor_base::difference_type difference_type;

  /// Set up a read-only, forward-only cursor
  /** Roughly equivalent to a C++ Standard Library istream, this cursor type
   * supports only two operations: reading a block of rows while moving forward,
   * and moving forward without reading any data.
   *
   * @param context Transaction context that this cursor will be active in
   * @param query SQL query whose results this cursor shall iterate
   * @param basename Suggested name for the SQL cursor; a unique code will be
   * appended by the library to ensure its uniqueness
   * @param sstride Number of rows to fetch per read operation; must be a
   * positive number
   */
  icursorstream(transaction_base &context,
      const std::string &query,
      const std::string &basename,
      difference_type sstride=1);					//[t81]

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
   * @param context Transaction context that this cursor will be active in.
   * @param cname Result field containing the name of the SQL cursor to adopt.
   * @param sstride Number of rows to fetch per read operation; must be a
   * positive number.
   * @param op Ownership policy.  Determines whether the cursor underlying this
   * stream will be destroyed when the stream is closed.
   */
  icursorstream(transaction_base &context,
      const field &cname,
      difference_type sstride=1,
      cursor_base::ownershippolicy op=cursor_base::owned);		//[t84]

  operator bool() const PQXX_NOEXCEPT { return !m_done; }

  /// Read new value into given result object; same as operator >>
  /** The result set may continue any number of rows from zero to the chosen
   * stride, inclusive.  An empty result will only be returned if there are no
   * more rows to retrieve.
   * @return Reference to this very stream, to facilitate "chained" invocations
   * ("C.get(r1).get(r2);")
   */
  icursorstream &get(result &res) { res = fetchblock(); return *this; }	//[t81]
  /// Read new value into given result object; same as get(result &)
  /** The result set may continue any number of rows from zero to the chosen
   * stride, inclusive.  An empty result will only be returned if there are no
   * more rows to retrieve.
   * @return Reference to this very stream, to facilitate "chained" invocations
   * ("C >> r1 >> r2;")
   */
  icursorstream &operator>>(result &res) { return get(res); }		//[t81]

  /// Move given number of rows forward (ignoring stride) without reading data
  /**
   * @return Reference to this very stream, to facilitate "chained" invocations
   * ("C.ignore(2).get(r).ignore(4);")
   */
  icursorstream &ignore(std::streamsize n=1);				//[t81]

  /// Change stride, i.e. the number of rows to fetch per read operation
  /**
   * @param stride Must be a positive number
   */
  void set_stride(difference_type stride);				//[t81]
  difference_type stride() const PQXX_NOEXCEPT { return m_stride; }	//[t81]

private:
  result fetchblock();

  friend class internal::gate::icursorstream_icursor_iterator;
  size_type forward(size_type n=1);
  void insert_iterator(icursor_iterator *) PQXX_NOEXCEPT;
  void remove_iterator(icursor_iterator *) const PQXX_NOEXCEPT;

  void service_iterators(difference_type);

  internal::sql_cursor m_cur;

  difference_type m_stride;
  difference_type m_realpos, m_reqpos;

  mutable icursor_iterator *m_iterators;

  bool m_done;
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
 * of rows, and "*ici++" reads one stride's worth of rows from the stream.
 *
 * @warning Do not read from the underlying stream or its cursor, move its read
 * position, or change its stride, between the time the first icursor_iterator
 * on it is created and the time its last icursor_iterator is destroyed.
 *
 * @warning Manipulating these iterators within the context of a single cursor
 * stream is <em>not thread-safe</em>.  Creating a new iterator, copying one, or
 * destroying one affects the stream as a whole.
 */
class PQXX_LIBEXPORT icursor_iterator :
  public std::iterator<std::input_iterator_tag,
	result,
	cursor_base::size_type,
	const result *,
	const result &>
{
public:
  typedef icursorstream istream_type;
  typedef istream_type::size_type size_type;
  typedef istream_type::difference_type difference_type;

  icursor_iterator() PQXX_NOEXCEPT;					//[t84]
  explicit icursor_iterator(istream_type &) PQXX_NOEXCEPT;		//[t84]
  icursor_iterator(const icursor_iterator &) PQXX_NOEXCEPT;		//[t84]
  ~icursor_iterator() PQXX_NOEXCEPT;

  const result &operator*() const { refresh(); return m_here; }		//[t84]
  const result *operator->() const { refresh(); return &m_here; }	//[t84]
  icursor_iterator &operator++();					//[t84]
  icursor_iterator operator++(int);					//[t84]
  icursor_iterator &operator+=(difference_type);			//[t84]
  icursor_iterator &operator=(const icursor_iterator &) PQXX_NOEXCEPT;	//[t84]

  bool operator==(const icursor_iterator &rhs) const;			//[t84]
  bool operator!=(const icursor_iterator &rhs) const PQXX_NOEXCEPT	//[t84]
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

  friend class internal::gate::icursor_iterator_icursorstream;
  difference_type pos() const PQXX_NOEXCEPT { return m_pos; }
  void fill(const result &);

  icursorstream *m_stream;
  result m_here;
  difference_type m_pos;
  icursor_iterator *m_prev, *m_next;
};


} // namespace pqxx

#include "pqxx/compiler-internal-post.hxx"

#endif

