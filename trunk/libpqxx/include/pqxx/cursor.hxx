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
class PQXX_LIBEXPORT cursor_base
{
  enum { off_prior=-1, off_next=1 };
public:
  typedef result::size_type size_type;

  operator void *() const { return m_done ? 0 : &s_dummy; }
  bool operator!() const { return m_done; }

  static size_type ALL() throw ();
  static size_type NEXT() throw () { return off_next; }
  static size_type PRIOR() throw () { return off_prior; }
  static size_type BACKWARD_ALL() throw ();

  const PGSTD::string &name() const throw () { return m_name; }

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


inline cursor_base::size_type cursor_base::ALL() throw ()
{
#ifdef _WIN32
  // Microsoft's compiler defines max() and min() macros!  Others may as well
  return INT_MAX;
#else
  return PGSTD::numeric_limits<size_type>::max();
#endif
}

inline cursor_base::size_type cursor_base::BACKWARD_ALL() throw ()
{
#ifdef _WIN32
  return INT_MIN + 1;
#else
  return PGSTD::numeric_limits<size_type>::min() + 1;
#endif
}

/// Simple read-only cursor represented as a stream of results
/** Data is fetched from the cursor as a sequence of result objects.  Each of
 * these will contain the number of rows defined as the stream's stride, except
 * of course the last block of data which may contain fewer rows.
 */
class PQXX_LIBEXPORT icursorstream : public cursor_base
{
public:
  icursorstream(transaction_base &context,
      const PGSTD::string &query,
      const PGSTD::string &basename,
      size_type stride=1);

  icursorstream &get(result &res) { res = fetch(); return *this; }
  icursorstream &operator>>(result &res) { return get(res); }
  icursorstream &ignore(PGSTD::streamsize n=1);

  void set_stride(size_type);

private:
  void declare(const PGSTD::string &query);
  result fetch();

  size_type m_stride;
};


/// Approximate istream_iterator for icursorstream
class PQXX_LIBEXPORT icursor_iterator : 
  public PGSTD::iterator<PGSTD::input_iterator_tag, 
  	result,
	cursor_base::size_type,
	const result *,
	const result &>
{
public:
  typedef icursorstream istream_type;

  icursor_iterator() : m_stream(0), m_here(), m_fresh(true) {}
  icursor_iterator(istream_type &s) :
    m_stream(&s), m_here(), m_fresh(false) {}
  icursor_iterator(const icursor_iterator &rhs) : 
    m_stream(rhs.m_stream), m_here(rhs.m_here), m_fresh(rhs.m_fresh) {}

  const result &operator*() const { refresh(); return m_here; }
  const result *operator->() const { refresh(); return &m_here; }

  icursor_iterator &operator++()
  	{ read(); return *this; }

  icursor_iterator operator++(int)
  	{ icursor_iterator old(*this); read(); return old; }

  // TODO: operator +=

  icursor_iterator &operator=(const icursor_iterator &rhs)
  {
    m_here = rhs.m_here;	// (Already protected against self-assignment)
    m_stream = rhs.m_stream;	// (Does not throw, so we're exception-safe)
    m_fresh = rhs.m_fresh;
    return *this;
  }

  bool operator==(const icursor_iterator &rhs) const throw ()
  	{ return m_here.empty() && rhs.m_here.empty(); }
  bool operator!=(const icursor_iterator &rhs) const throw ()
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

