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
 * Copyright (c) 2004-2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#ifdef PQXX_HAVE_LIMITS
#include <limits>
#endif

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

  virtual ~cursor_base() throw () { close(); }

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

  /// Does it make sense to try reading from this cursor again?
  /**
   * @return Null pointer if finished, or some arbitrary non-null pointer
   * otherwise.
   */
  operator void *() const {return m_done ? 0 : m_context;}		//[t81]

  /// Is this cursor finished?
  /** The logical negation of the converstion-to-pointer operator.
   * @return Whether the last attempt to read failed
   */
  bool operator!() const { return m_done; }				//[t81]

  /**
   * @name Special movement distances
   */
  //@{
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
  //@}

  /// Name of underlying SQL cursor
  /**
   * @returns Name of SQL cursor, which may differ from original given name.
   * @warning Don't use this to access the SQL cursor directly without going
   * through the provided wrapper classes!
   */
  const PGSTD::string &name() const throw () { return m_name; }		//[t81]

  /// Fetch up to given number of rows of data
  virtual result fetch(difference_type);				//[]

  /// Fetch result, but also return the number of rows of actual displacement
  /** The relationship between actual displacement and result size gets tricky
   * at the edges of the cursor's range of movement.  As an example, consider a
   * fresh cursor that's been moved forward by 2 rows from its starting
   * position; we can move it backwards from that position by 1 row and get a
   * result set of 1 row, ending up on the first actual row of data.  If instead
   * we move it backwards by 2 or more rows, we end up back at the starting
   * position--but the result is still only 1 row wide!
   *
   * The output parameter compensates for this, returning true displacement
   * (which also signed, so it includes direction).
   */
  virtual result fetch(difference_type, difference_type &);		//[]

  /// Move cursor by given number of rows, returning number of data rows skipped
  /** The number of data rows skipped is equal to the number of rows of data
   * that would have been returned if this were a fetch instead of a move
   * command.
   *
   * @return the number of data rows that would have been returned if this had
   * been a fetch() command.
   */
  virtual difference_type move(difference_type);			//[]

  /// Move cursor, but also return actual displacement in output parameter
  /** As with the @c fetch functions, the actual displacement may differ from
   * the number of data rows skipped by the move.
   */
  virtual difference_type move(difference_type, difference_type &);	//[]

  void close() throw ();						//[]

protected:
  cursor_base(transaction_base *,
      const PGSTD::string &Name,
      bool embellish_name = true);

  void declare(const PGSTD::string &query,
      accesspolicy,
      updatepolicy,
      ownershippolicy,
      bool hold);
  void adopt(ownershippolicy);

  static PGSTD::string stridestring(difference_type);
  transaction_base *m_context;
  bool m_done;

  template<accesspolicy A> void check_displacement(difference_type) const { }

#if defined(_MSC_VER)
  /* Visual C++ won't accept this specialization declaration unless it's in
   * here!  See below for the "standard" alternative.
   */
  template<>
    void check_displacement<cursor_base::forward_only>(difference_type) const;
#endif

private:
  PGSTD::string m_name;
  bool m_adopted;
  ownershippolicy m_ownership;

  struct cachedquery
  {
    difference_type dist;
    PGSTD::string query;

    cachedquery() : dist(0), query() {}
  };
  cachedquery m_lastfetch, m_lastmove;

  /// Not allowed
  cursor_base();
  /// Not allowed
  cursor_base(const cursor_base &);
  /// Not allowed
  cursor_base &operator=(const cursor_base &);
};

/* Visual C++ demands that this specialization be declared inside the class,
 * which gcc claims is illegal.  There seems to be no single universally
 * accepted way to do this.
 */
#if !defined(_MSC_VER)
template<> void
  cursor_base::check_displacement<cursor_base::forward_only>(difference_type)
  	const;
#endif


inline cursor_base::difference_type cursor_base::all() throw ()
{
  // Microsoft Visual C++ sabotages numeric limits by defining min() and max()
  // as preprocessor macros; some other compilers just don't have numeric_limits
#if defined(PQXX_HAVE_LIMITS)
  return PGSTD::numeric_limits<difference_type>::max();
#else
  return INT_MAX;
#endif
}

inline cursor_base::difference_type cursor_base::backward_all() throw ()
{
#if defined(PQXX_HAVE_LIMITS)
  return PGSTD::numeric_limits<difference_type>::min() + 1;
#else
  return INT_MIN + 1;
#endif
}


// TODO: How do we work updates into the scheme?
/// The simplest form of cursor, with no concept of position or stride
template<cursor_base::accesspolicy ACCESS, cursor_base::updatepolicy UPDATE>
class basic_cursor : public cursor_base
{
public:
  /// Create cursor based on given query
  /**
   * @param t transaction this cursor is to live in
   * @param query SQL query whose results this cursor will iterate
   * @param cname name for this cursor, which will be changed to make it unique
   * @param op are we responsible for closing this cursor?
   *
   * @warning If the transaction being used is a nontransaction, or if the
   * ownership policy is "loose," a "WITH HOLD" cursor will be created.  Not all
   * backends versions support this.
   */
  basic_cursor(transaction_base *t,
      const PGSTD::string &query,
      const PGSTD::string &cname,
      ownershippolicy op=owned) :					//[t3]
    cursor_base(t, cname, true)
  {
    declare(query,
	ACCESS,
	UPDATE,
	op,
	op==loose || !dynamic_cast<dbtransaction *>(t));
  }

  /// Adopt existing SQL cursor
  /** Create a cursor object based on an existing SQL cursor.  The name must be
   * the exact name of that cursor (and unlike the name of a newly created
   * cursor, will not be embellished for uniqueness).
   *
   * @param t transaction this cursor is to live in
   * @param cname exact name of this cursor, as declared in SQL
   * @param op are we responsible for closing this cursor?
   */
  basic_cursor(transaction_base *t,
      const PGSTD::string &cname,
      ownershippolicy op=owned) :					//[t45]
    cursor_base(t, cname, false)
  {
    adopt(op);
  }

  /// Fetch a number of rows from cursor
  /** This function can be used to fetch a given number of rows (by passing the
   * desired number of rows as an argument), or all remaining rows (by passing
   * cursor_base::all()), or fetch a given number of rows backwards from the
   * current position (by passing the negative of the desired number), or all
   * rows remaining behind the current position (by using
   * cursor_base::backwards_all()).
   *
   * This function behaves slightly differently from the SQL FETCH command.
   * Most notably, fetching zero rows does not move the cursor, and returns an
   * empty result.
   *
   * @warning When zero rows are fetched, the returned result may not contain
   * any metadata such as the number of columns and their names.
   * @param n number of rows to fetch
   * @return a result set containing at most n rows of data
   */
  virtual result fetch(difference_type n)				//[t3]
  {
    check_displacement<ACCESS>(n);
    return cursor_base::fetch(n);
  }

  virtual result fetch(difference_type n, difference_type &d)		//[]
  {
    check_displacement<ACCESS>(n);
    return cursor_base::fetch(n, d);
  }

  /// Move cursor by given number of rows
  /**
   * @param n number of rows to move
   * @return number of rows actually moved (which cannot exceed n)
   */
  virtual difference_type move(difference_type n)			//[t3]
  {
    check_displacement<ACCESS>(n);
    return cursor_base::move(n);
  }

  virtual difference_type move(difference_type n, difference_type &d)	//[t42]
  {
    check_displacement<ACCESS>(n);
    return cursor_base::move(n, d);
  }

  using cursor_base::close;
};


/// Cursor that knows its position
template<cursor_base::accesspolicy ACCESS, cursor_base::updatepolicy UPDATE>
class absolute_cursor : public basic_cursor<ACCESS,UPDATE>
{
  typedef basic_cursor<ACCESS,UPDATE> super;
public:
  typedef cursor_base::size_type size_type;
  typedef cursor_base::difference_type difference_type;

  /// Create cursor based on given query
  /**
   * @param t transaction this cursor is to live in
   * @param query SQL query whose results this cursor will iterate
   * @param cname name for this cursor, which will be changed to make it unique
   *
   * @warning If the transaction being used is a nontransaction, a "WITH HOLD"
   * cursor will be created.  Not all backends versions support this.
   */
  absolute_cursor(transaction_base *t,
      const PGSTD::string &query,
      const PGSTD::string &cname) :					//[]
    super(t, query, cname, cursor_base::owned),
    m_pos(0),
    m_size(0),
    m_size_known(false)
  {
  }

  virtual result fetch(difference_type n)				//[]
  {
    difference_type m;
    return fetch(n, m);
  }

  virtual difference_type move(difference_type n)			//[]
  {
    difference_type m;
    return move(n, m);
  }

  virtual difference_type move(difference_type d, difference_type &m)	//[]
  {
    const difference_type r(super::move(d, m));
    digest(d, m);
    return r;
  }

  virtual result fetch(difference_type d, difference_type &m)		//[]
  {
    const result r(super::fetch(d, m));
    digest(d, m);
    return r;
  }

  size_type pos() const throw () { return m_pos; }			//[]

  difference_type move_to(cursor_base::size_type);			//[]

private:
  /// Set result size if appropriate, given requested and actual displacement
  void digest(cursor_base::difference_type req,
      cursor_base::difference_type got) throw ()
  {
    m_pos += got;

    // This assumes that got < req can only happen if req < 0
    if (got < req && !m_size_known)
    {
      m_size = m_pos;
      m_size_known = true;
    }
  }

  cursor_base::size_type m_pos;
  cursor_base::size_type m_size;
  bool m_size_known;
};


/// Convenience typedef: the most common cursor type (read-only, random access)
typedef basic_cursor<cursor_base::random_access, cursor_base::read_only> cursor;


class icursor_iterator;

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
class PQXX_LIBEXPORT icursorstream :
  public basic_cursor<cursor_base::forward_only, cursor_base::read_only>
{
  typedef basic_cursor<cursor_base::forward_only, cursor_base::read_only> super;
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
   * (<tt>C.get(r1).get(r2);</tt>)
   */
  icursorstream &get(result &res) { res = fetchblock(); return *this; }	//[t81]
  /// Read new value into given result object; same as get(result &)
  /** The result set may continue any number of rows from zero to the chosen
   * stride, inclusive.  An empty result will only be returned if there are no
   * more rows to retrieve.
   * @return Reference to this very stream, to facilitate "chained" invocations
   * (<tt>C >> r1 >> r2;</tt>)
   */
  icursorstream &operator>>(result &res) { return get(res); }		//[t81]
  /// Move given number of rows forward (ignoring stride) without reading data
  /**
   * @return Reference to this very stream, to facilitate "chained" invocations
   * (<tt>C.ignore(2).get(r).ignore(4);</tt>)
   */
  icursorstream &ignore(PGSTD::streamsize n=1);				//[t81]

  /// Change stride, i.e. the number of rows to fetch per read operation
  /**
   * @param stride Must be a positive number
   */
  void set_stride(difference_type stride);				//[t81]
  difference_type stride() const throw () { return m_stride; }		//[t81]

private:
  result fetchblock();

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
 * access operations, i.e. "<tt>ici += 1</tt>" advances the stream by one
 * stride's worth of tuples, and "<tt>*ici++</tt>" reads one stride's worth of
 * tuples from the stream.
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
  void fill(const result &);

  icursorstream *m_stream;
  result m_here;
  size_type m_pos;
  icursor_iterator *m_prev, *m_next;
};


} // namespace pqxx

#include "pqxx/compiler-internal-post.hxx"
