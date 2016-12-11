/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/result.hxx
 *
 *   DESCRIPTION
 *      definitions for the pqxx::result class and support classes.
 *   pqxx::result represents the set of result rows from a database query
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/result instead.
 *
 * Copyright (c) 2001-2016, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_RESULT
#define PQXX_H_RESULT

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <ios>
#include <stdexcept>

#include "pqxx/internal/result_data.hxx"

#include "pqxx/except"
#include "pqxx/field"
#include "pqxx/row"
#include "pqxx/util"

/* Methods tested in eg. self-test program test001 are marked with "//[t1]"
 */

// TODO: Support SQL arrays

namespace pqxx
{
namespace internal
{
namespace gate
{
class result_connection;
class result_creation;
class result_sql_cursor;
} // namespace internal::gate
} // namespace internal


class const_result_iterator;
class const_reverse_result_iterator;


/// Result set containing data returned by a query or command.
/** This behaves as a container (as defined by the C++ standard library) and
 * provides random access const iterators to iterate over its rows.  A row
 * can also be accessed by indexing a result R by the row's zero-based
 * number:
 *
 * @code
 *	for (result::size_type i=0; i < R.size(); ++i) Process(R[i]);
 * @endcode
 *
 * Result sets in libpqxx are lightweight, reference-counted wrapper objects
 * (following the Proxy design pattern) that are small and cheap to copy.  Think
 * of a result object as a "smart pointer" to an underlying result set.
 *
 * @warning The result set that a result object points to is not thread-safe.
 * If you copy a result object, it still refers to the same underlying result
 * set.  So never copy, destroy, query, or otherwise access a result while
 * another thread may be copying, destroying, querying, or otherwise accessing
 * the same result set--even if it is doing so through a different result
 * object!
 */
class PQXX_LIBEXPORT result :
  private internal::PQAlloc<
	const internal::result_data, internal::freemem_result_data>
{
  typedef internal::PQAlloc<
	const internal::result_data, internal::freemem_result_data> super;
public:
  typedef unsigned long size_type;
  typedef signed long difference_type;
  typedef row reference;
  typedef const_result_iterator const_iterator;
  typedef const_iterator pointer;
  typedef const_iterator iterator;
  typedef const_reverse_result_iterator const_reverse_iterator;
  typedef const_reverse_iterator reverse_iterator;

  result() PQXX_NOEXCEPT : super(), m_data(0) {}				//[t3]
  result(const result &rhs) PQXX_NOEXCEPT :					//[t1]
	super(rhs), m_data(rhs.m_data) {}

  result &operator=(const result &rhs) PQXX_NOEXCEPT				//[t10]
	{ super::operator=(rhs); m_data=rhs.m_data; return *this; }

  /**
   * @name Comparisons
   */
  //@{
  bool operator==(const result &) const PQXX_NOEXCEPT;			//[t70]
  bool operator!=(const result &rhs) const PQXX_NOEXCEPT		//[t70]
	{ return !operator==(rhs); }
  //@}

  const_reverse_iterator rbegin() const;				//[t75]
  const_reverse_iterator crbegin() const;
  const_reverse_iterator rend() const;					//[t75]
  const_reverse_iterator crend() const;

  const_iterator begin() const PQXX_NOEXCEPT;				//[t1]
  const_iterator cbegin() const PQXX_NOEXCEPT;
  inline const_iterator end() const PQXX_NOEXCEPT;			//[t1]
  inline const_iterator cend() const PQXX_NOEXCEPT;

  reference front() const PQXX_NOEXCEPT { return row(this,0); }		//[t74]
  reference back() const PQXX_NOEXCEPT {return row(this,size()-1);}	//[t75]

  PQXX_PURE size_type size() const PQXX_NOEXCEPT;			//[t2]
  PQXX_PURE bool empty() const PQXX_NOEXCEPT;				//[t11]
  size_type capacity() const PQXX_NOEXCEPT { return size(); }		//[t20]

  void swap(result &) PQXX_NOEXCEPT;					//[t77]

  const row operator[](size_type i) const PQXX_NOEXCEPT			//[t2]
	{ return row(this, i); }
  const row at(size_type) const;					//[t10]

  void clear() PQXX_NOEXCEPT { super::reset(); m_data = 0; }		//[t20]

  /**
   * @name Column information
   */
  //@{
  /// Number of columns in result
  PQXX_PURE row::size_type columns() const PQXX_NOEXCEPT;		//[t11]

  /// Number of given column (throws exception if it doesn't exist)
  row::size_type column_number(const char ColName[]) const;		//[t11]

  /// Number of given column (throws exception if it doesn't exist)
  row::size_type column_number(const std::string &Name) const		//[t11]
	{return column_number(Name.c_str());}

  /// Name of column with this number (throws exception if it doesn't exist)
  const char *column_name(row::size_type Number) const;			//[t11]

  /// Type of given column
  oid column_type(row::size_type ColNum) const;				//[t7]
  /// Type of given column
  oid column_type(int ColNum) const					//[t7]
	{ return column_type(row::size_type(ColNum)); }

  /// Type of given column
  oid column_type(const std::string &ColName) const			//[t7]
	{ return column_type(column_number(ColName)); }

  /// Type of given column
  oid column_type(const char ColName[]) const				//[t7]
	{ return column_type(column_number(ColName)); }

  /// What table did this column come from?
  oid column_table(row::size_type ColNum) const;			//[t2]

  /// What table did this column come from?
  oid column_table(int ColNum) const					//[t2]
	{ return column_table(row::size_type(ColNum)); }

  /// What table did this column come from? 
  oid column_table(const std::string &ColName) const			//[t2]
	{ return column_table(column_number(ColName)); }

  /// What column in its table did this column come from?
  row::size_type table_column(row::size_type ColNum) const;		//[t93]

  /// What column in its table did this column come from?
  row::size_type table_column(int ColNum) const				//[t93]
	{ return table_column(row::size_type(ColNum)); }

  /// What column in its table did this column come from?
  row::size_type table_column(const std::string &ColName) const		//[t93]
	{ return table_column(column_number(ColName)); }
  //@}

  /// Query that produced this result, if available (empty string otherwise)
  PQXX_PURE const std::string &query() const PQXX_NOEXCEPT;		//[t70]

  /// If command was @c INSERT of 1 row, return oid of inserted row
  /** @return Identifier of inserted row if exactly one row was inserted, or
   * oid_none otherwise.
   */
  PQXX_PURE oid inserted_oid() const;					//[t13]

  /// If command was @c INSERT, @c UPDATE, or @c DELETE: number of affected rows
  /** @return Number of affected rows if last command was @c INSERT, @c UPDATE,
   * or @c DELETE; zero for all other commands.
   */
  PQXX_PURE size_type affected_rows() const;				//[t7]


private:
  friend class pqxx::field;
  PQXX_PURE const char *GetValue(size_type Row, row::size_type Col) const;
  PQXX_PURE bool GetIsNull(size_type Row, row::size_type Col) const;
  PQXX_PURE field::size_type GetLength(
	size_type,
	row::size_type) const PQXX_NOEXCEPT;

  friend class pqxx::internal::gate::result_creation;
  result(internal::pq::PGresult *rhs,
	int protocol,
	const std::string &Query,
	int encoding_code);
  PQXX_PRIVATE void CheckStatus() const;

  friend class pqxx::internal::gate::result_connection;
  bool operator!() const PQXX_NOEXCEPT { return !m_data; }
  operator bool() const PQXX_NOEXCEPT { return m_data != 0; }

  PQXX_NORETURN PQXX_PRIVATE void ThrowSQLError(
	const std::string &Err,
	const std::string &Query) const;
  PQXX_PRIVATE PQXX_PURE int errorposition() const PQXX_NOEXCEPT;
  PQXX_PRIVATE std::string StatusError() const;

  friend class pqxx::internal::gate::result_sql_cursor;
  PQXX_PURE const char *CmdStatus() const PQXX_NOEXCEPT;

  /// Shortcut: pointer to result data
  pqxx::internal::pq::PGresult *m_data;

  PQXX_PRIVATE static const std::string s_empty_string;
};


/// Iterator for rows in a result.  Use as result::const_iterator.
/** A result, once obtained, cannot be modified.  Therefore there is no
 * plain iterator type for result.  However its const_iterator type can be
 * used to inspect its rows without changing them.
 */
class PQXX_LIBEXPORT const_result_iterator :
  public std::iterator<
	std::random_access_iterator_tag,
	const row,
	result::difference_type,
	const_result_iterator,
	row>,
  public row
{
public:
  typedef const row *pointer;
  typedef row reference;
  typedef result::size_type size_type;
  typedef result::difference_type difference_type;

  const_result_iterator() PQXX_NOEXCEPT : row(0,0) {}
  const_result_iterator(const row &t) PQXX_NOEXCEPT : row(t) {}

  /**
   * @name Dereferencing operators
   */
  //@{
  /** The iterator "points to" its own row, which is also itself.  This
   * allows a result to be addressed as a two-dimensional container without
   * going through the intermediate step of dereferencing the iterator.  I
   * hope this works out to be similar to C pointer/array semantics in useful
   * cases.
   *
   * IIRC Alex Stepanov, the inventor of the STL, once remarked that having
   * this as standard behaviour for pointers would be useful in some
   * algorithms.  So even if this makes me look foolish, I would seem to be in
   * distinguished company.
   */
  pointer operator->() const { return this; }				//[t12]
  reference operator*() const { return row(*this); }			//[t12]
  //@}

  /**
   * @name Manipulations
   */
  //@{
  const_result_iterator operator++(int);				//[t12]
  const_result_iterator &operator++() { ++m_Index; return *this; }	//[t1]
  const_result_iterator operator--(int);				//[t12]
  const_result_iterator &operator--() { --m_Index; return *this; }	//[t12]

  const_result_iterator &operator+=(difference_type i)			//[t12]
      { m_Index = size_type(difference_type(m_Index) + i); return *this; }
  const_result_iterator &operator-=(difference_type i)			//[t12]
      { m_Index = size_type(difference_type(m_Index) - i); return *this; }
  //@}

  /**
   * @name Comparisons
   */
  //@{
  bool operator==(const const_result_iterator &i) const			//[t12]
      {return m_Index==i.m_Index;}
  bool operator!=(const const_result_iterator &i) const			//[t12]
      {return m_Index!=i.m_Index;}
  bool operator<(const const_result_iterator &i) const			//[t12]
      {return m_Index<i.m_Index;}
  bool operator<=(const const_result_iterator &i) const			//[t12]
      {return m_Index<=i.m_Index;}
  bool operator>(const const_result_iterator &i) const			//[t12]
      {return m_Index>i.m_Index;}
  bool operator>=(const const_result_iterator &i) const			//[t12]
      {return m_Index>=i.m_Index;}
  //@}

  /**
   * @name Arithmetic operators
   */
  //@{
  inline const_result_iterator operator+(difference_type) const;	//[t12]
  friend const_result_iterator operator+(				//[t12]
	difference_type,
	const_result_iterator);
  inline const_result_iterator operator-(difference_type) const;	//[t12]
  inline difference_type operator-(const_result_iterator) const;	//[t12]
  //@}

private:
  friend class pqxx::result;
  const_result_iterator(const pqxx::result *r, result::size_type i)
	PQXX_NOEXCEPT :
    row(r, i) {}
};


/// Reverse iterator for result.  Use as result::const_reverse_iterator.
class PQXX_LIBEXPORT const_reverse_result_iterator :
  private const_result_iterator
{
public:
  typedef const_result_iterator super;
  typedef const_result_iterator iterator_type;
  using iterator_type::iterator_category;
  using iterator_type::difference_type;
  using iterator_type::pointer;
#ifndef _MSC_VER
  using iterator_type::value_type;
  using iterator_type::reference;
#else
  // Workaround for Visual C++.NET 2003, which has access problems
  typedef const row &reference;
  typedef row value_type;
#endif

  const_reverse_result_iterator(					//[t75]
	const const_reverse_result_iterator &rhs) :
    const_result_iterator(rhs) {}
  explicit const_reverse_result_iterator(				//[t75]
	const const_result_iterator &rhs) :
    const_result_iterator(rhs) { super::operator--(); }

  PQXX_PURE const_result_iterator base() const PQXX_NOEXCEPT;		//[t75]

  /**
   * @name Dereferencing operators
   */
  //@{
  using const_result_iterator::operator->;				//[t75]
  using const_result_iterator::operator*;				//[t75]
  //@}

  /**
   * @name Manipulations
   */
  //@{
  const_reverse_result_iterator &operator=(				//[t75]
	const const_reverse_result_iterator &r)
      { iterator_type::operator=(r); return *this; }
  const_reverse_result_iterator operator++()				//[t75]
      { iterator_type::operator--(); return *this; }
  const_reverse_result_iterator operator++(int);			//[t75]
  const_reverse_result_iterator &operator--()				//[t75]
      { iterator_type::operator++(); return *this; }
  const_reverse_result_iterator operator--(int);			//[t75]
  const_reverse_result_iterator &operator+=(difference_type i)		//[t75]
      { iterator_type::operator-=(i); return *this; }
  const_reverse_result_iterator &operator-=(difference_type i)		//[t75]
      { iterator_type::operator+=(i); return *this; }
  //@}

  /**
   * @name Arithmetic operators
   */
  //@{
  const_reverse_result_iterator operator+(difference_type i) const	//[t75]
      { return const_reverse_result_iterator(base() - i); }
  const_reverse_result_iterator operator-(difference_type i)		//[t75]
      { return const_reverse_result_iterator(base() + i); }
  difference_type operator-(						//[t75]
	const const_reverse_result_iterator &rhs) const
      { return rhs.const_result_iterator::operator-(*this); }
  //@}

  /**
   * @name Comparisons
   */
  //@{
  bool operator==(							//[t75]
	const const_reverse_result_iterator &rhs) const PQXX_NOEXCEPT
      { return iterator_type::operator==(rhs); }
  bool operator!=(							//[t75]
	const const_reverse_result_iterator &rhs) const PQXX_NOEXCEPT
      { return !operator==(rhs); }

  bool operator<(const const_reverse_result_iterator &rhs) const	//[t75]
      { return iterator_type::operator>(rhs); }
  bool operator<=(const const_reverse_result_iterator &rhs) const	//[t75]
      { return iterator_type::operator>=(rhs); }
  bool operator>(const const_reverse_result_iterator &rhs) const	//[t75]
      { return iterator_type::operator<(rhs); }
  bool operator>=(const const_reverse_result_iterator &rhs) const	//[t75]
      { return iterator_type::operator<=(rhs); }
  //@}
};



/// Write a result field to any type of stream
/** This can be convenient when writing a field to an output stream.  More
 * importantly, it lets you write a field to e.g. a @c stringstream which you
 * can then use to read, format and convert the field in ways that to() does not
 * support.
 *
 * Example: parse a field into a variable of the nonstandard
 * "<tt>long long</tt>" type.
 *
 * @code
 * extern result R;
 * long long L;
 * stringstream S;
 *
 * // Write field's string into S
 * S << R[0][0];
 *
 * // Parse contents of S into L
 * S >> L;
 * @endcode
 */
template<typename CHAR>
inline std::basic_ostream<CHAR> &operator<<(
	std::basic_ostream<CHAR> &S, const pqxx::field &F)		//[t46]
{
  S.write(F.c_str(), std::streamsize(F.size()));
  return S;
}


/// Convert a field's string contents to another type
template<typename T>
inline void from_string(const field &F, T &Obj)				//[t46]
	{ from_string(F.c_str(), Obj, F.size()); }

/// Convert a field to a string
template<>
inline std::string to_string(const field &Obj)				//[t74]
	{ return std::string(Obj.c_str(), Obj.size()); }


inline const_result_iterator
const_result_iterator::operator+(result::difference_type o) const
{
  return const_result_iterator(
	m_Home, size_type(result::difference_type(m_Index) + o));
}

inline const_result_iterator
operator+(result::difference_type o, const_result_iterator i)
	{ return i + o; }

inline const_result_iterator
const_result_iterator::operator-(result::difference_type o) const
{
  return const_result_iterator(
	m_Home,
	result::size_type(result::difference_type(m_Index) - o));
}

inline result::difference_type
const_result_iterator::operator-(const_result_iterator i) const
	{ return result::difference_type(num() - i.num()); }

inline const_result_iterator result::end() const PQXX_NOEXCEPT
	{ return const_result_iterator(this, size()); }


inline const_result_iterator result::cend() const PQXX_NOEXCEPT
	{ return end(); }


inline const_reverse_result_iterator
operator+(result::difference_type n,
	  const const_reverse_result_iterator &i)
	{ return const_reverse_result_iterator(i.base() - n); }

} // namespace pqxx


#include "pqxx/compiler-internal-post.hxx"

#endif
