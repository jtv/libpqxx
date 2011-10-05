/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/result.hxx
 *
 *   DESCRIPTION
 *      definitions for the pqxx::result class and support classes.
 *   pqxx::result represents the set of result tuples from a database query
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/tuple instead.
 *
 * Copyright (c) 2001-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_TUPLE
#define PQXX_H_TUPLE

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/except"
#include "pqxx/field"


/* Methods tested in eg. self-test program test001 are marked with "//[t1]"
 */

namespace pqxx
{
class const_tuple_iterator;
class const_reverse_tuple_iterator;
class result;


/// Reference to one row in a result.
/** A tuple represents one row (also called a tuple) in a query result set.
 * It also acts as a container mapping column numbers or names to field
 * values (see below):
 *
 * @code
 *	cout << tuple["date"].c_str() << ": " << tuple["name"].c_str() << endl;
 * @endcode
 *
 * The tuple itself acts like a (non-modifyable) container, complete with its
 * own const_iterator and const_reverse_iterator.
 */
class PQXX_LIBEXPORT tuple
{
public:
  typedef tuple_size_type size_type;
  typedef tuple_difference_type difference_type;
  typedef const_tuple_iterator const_iterator;
  typedef const_iterator iterator;
  typedef field reference;
  typedef const_tuple_iterator pointer;
  typedef const_reverse_tuple_iterator const_reverse_iterator;
  typedef const_reverse_iterator reverse_iterator;

  /// @deprecated Do not use this constructor.  It will become private.
  tuple(const result *r, size_t i) throw ();

  ~tuple() throw () {} // Yes Scott Meyers, you're absolutely right[1]

  /**
   * @name Comparison
   */
  //@{
  bool PQXX_PURE operator==(const tuple &) const throw ();		//[t75]
  bool operator!=(const tuple &rhs) const throw ()			//[t75]
      { return !operator==(rhs); }
  //@}

  const_iterator begin() const throw ();				//[t82]
  const_iterator end() const throw ();					//[t82]

  /**
   * @name Field access
   */
  //@{
  reference front() const throw ();					//[t74]
  reference back() const throw ();					//[t75]

  const_reverse_tuple_iterator rbegin() const;				//[t82]
  const_reverse_tuple_iterator rend() const;				//[t82]

  reference operator[](size_type) const throw ();			//[t11]
  reference operator[](int) const throw ();				//[t2]
  reference operator[](const char[]) const;				//[t11]
  reference operator[](const PGSTD::string &) const;			//[t11]
  reference at(size_type) const throw (range_error);			//[t11]
  reference at(int) const throw (range_error);				//[t11]
  reference at(const char[]) const;					//[t11]
  reference at(const PGSTD::string &) const;				//[t11]
  //@}

  size_type size() const throw ()					//[t11]
						     { return m_End-m_Begin; }

  void swap(tuple &) throw ();						//[t11]

  size_t rownumber() const throw () { return m_Index; }			//[t11]

  /**
   * @name Column information
   */
  //@{
  /// Number of given column (throws exception if it doesn't exist)
  size_type column_number(const PGSTD::string &ColName) const		//[t30]
      { return column_number(ColName.c_str()); }

  /// Number of given column (throws exception if it doesn't exist)
  size_type column_number(const char[]) const;       			//[t30]

  /// Type of given column
  oid column_type(size_type) const;					//[t7]

  /// Type of given column
  oid column_type(int ColNum) const					//[t7]
      { return column_type(size_type(ColNum)); }

  /// Type of given column
  oid column_type(const PGSTD::string &ColName) const			//[t7]
      { return column_type(column_number(ColName)); }

  /// Type of given column
  oid column_type(const char ColName[]) const				//[t7]
      { return column_type(column_number(ColName)); }

  /// What table did this column come from?
  oid column_table(size_type ColNum) const;				//[t2]

  /// What table did this column come from?
  oid column_table(int ColNum) const					//[t2]
      { return column_table(size_type(ColNum)); }
  /// What table did this column come from?
  oid column_table(const PGSTD::string &ColName) const		//[t2]
      { return column_table(column_number(ColName)); }

  /// What column number in its table did this result column come from?
  /** A meaningful answer can be given only if the column in question comes
   * directly from a column in a table.  If the column is computed in any
   * other way, a logic_error will be thrown.
   *
   * @param ColNum a zero-based column number in this result set
   * @return a zero-based column number in originating table
   *
   * Requires libpq from PostgreSQL 7.4 or better, as well as a server version
   * of at least 7.4.
   */
  size_type table_column(size_type) const;				//[t93]

  /// What column number in its table did this result column come from?
  size_type table_column(int ColNum) const				//[t93]
      { return table_column(size_type(ColNum)); }

  /// What column number in its table did this result column come from?
  size_type table_column(const PGSTD::string &ColName) const		//[t93]
      { return table_column(column_number(ColName)); }
  //@}

  size_t num() const { return rownumber(); }				//[t1]

  /** Produce a slice of this tuple, containing the given range of columns.
   *
   * The slice runs from the range's starting column to the range's end
   * column, exclusive.  It looks just like a normal result tuple, except
   * slices can be empty.
   *
   * @warning Slicing is a relatively new feature, and not all software may be
   * prepared to deal with empty slices.  If there is any chance that your
   * program might be creating empty slices and passing them to code that may
   * not be designed with the possibility of empty tuples in mind, be sure to
   * test for that case.
   */
  tuple slice(size_type Begin, size_type End) const;

  // Is this an empty slice?
  bool PQXX_PURE empty() const throw ();

protected:
  friend class field;
  const result *m_Home;
  size_t m_Index;
  size_type m_Begin;
  size_type m_End;

private:
  // Not allowed:
  tuple();
};


/// Iterator for fields in a tuple.  Use as tuple::const_iterator.
class PQXX_LIBEXPORT const_tuple_iterator :
  public PGSTD::iterator<PGSTD::random_access_iterator_tag,
			 const field,
			 tuple::size_type>,
  public field
{
  typedef PGSTD::iterator<PGSTD::random_access_iterator_tag,
				const field,
				tuple::size_type> it;
public:
  using it::pointer;
  typedef tuple::size_type size_type;
  typedef tuple::difference_type difference_type;
  typedef field reference;

  const_tuple_iterator(const tuple &T, tuple::size_type C) throw () :	//[t82]
    field(T, C) {}
  const_tuple_iterator(const field &F) throw () : field(F) {}		//[t82]

  /**
   * @name Dereferencing operators
   */
  //@{
  pointer operator->() const { return this; }				//[t82]
  reference operator*() const { return field(*this); }			//[t82]
  //@}

  /**
   * @name Manipulations
   */
  //@{
  const_tuple_iterator operator++(int);					//[t82]
  const_tuple_iterator &operator++() { ++m_col; return *this; }		//[t82]
  const_tuple_iterator operator--(int);					//[t82]
  const_tuple_iterator &operator--() { --m_col; return *this; }		//[t82]

  const_tuple_iterator &operator+=(difference_type i)			//[t82]
      { m_col = size_type(difference_type(m_col) + i); return *this; }
  const_tuple_iterator &operator-=(difference_type i)			//[t82]
      { m_col = size_type(difference_type(m_col) - i); return *this; }
  //@}

  /**
   * @name Comparisons
   */
  //@{
  bool operator==(const const_tuple_iterator &i) const			//[t82]
      {return col()==i.col();}
  bool operator!=(const const_tuple_iterator &i) const			//[t82]
      {return col()!=i.col();}
  bool operator<(const const_tuple_iterator &i) const			//[t82]
      {return col()<i.col();}
  bool operator<=(const const_tuple_iterator &i) const			//[t82]
      {return col()<=i.col();}
  bool operator>(const const_tuple_iterator &i) const			//[t82]
      {return col()>i.col();}
  bool operator>=(const const_tuple_iterator &i) const			//[t82]
      {return col()>=i.col();}
  //@}

  /**
   * @name Arithmetic operators
   */
  //@{
  inline const_tuple_iterator operator+(difference_type) const;		//[t82]

  friend const_tuple_iterator operator+(				//[t82]
	difference_type,
	const_tuple_iterator);

  inline const_tuple_iterator operator-(difference_type) const;		//[t82]
  inline difference_type operator-(const_tuple_iterator) const;		//[t82]
  //@}
};


/// Reverse iterator for a tuple.  Use as tuple::const_reverse_iterator.
class PQXX_LIBEXPORT const_reverse_tuple_iterator : private const_tuple_iterator
{
public:
  typedef const_tuple_iterator super;
  typedef const_tuple_iterator iterator_type;
  using iterator_type::iterator_category;
  using iterator_type::difference_type;
  using iterator_type::pointer;
#ifndef _MSC_VER
  using iterator_type::value_type;
  using iterator_type::reference;
#else
  // Workaround for Visual C++.NET 2003, which has access problems
  typedef field value_type;
  typedef const field &reference;
#endif

  const_reverse_tuple_iterator(const const_reverse_tuple_iterator &r) :	//[t82]
    const_tuple_iterator(r) {}
  explicit
    const_reverse_tuple_iterator(const super &rhs) throw() :		//[t82]
      const_tuple_iterator(rhs) { super::operator--(); }

  iterator_type PQXX_PURE base() const throw ();			//[t82]

  /**
   * @name Dereferencing operators
   */
  //@{
  using iterator_type::operator->;					//[t82]
  using iterator_type::operator*;					//[t82]
  //@}

  /**
   * @name Manipulations
   */
  //@{
  const_reverse_tuple_iterator &
    operator=(const const_reverse_tuple_iterator &r)			//[t82]
      { iterator_type::operator=(r); return *this; }
  const_reverse_tuple_iterator operator++()				//[t82]
      { iterator_type::operator--(); return *this; }
  const_reverse_tuple_iterator operator++(int);				//[t82]
  const_reverse_tuple_iterator &operator--()				//[t82]
      { iterator_type::operator++(); return *this; }
  const_reverse_tuple_iterator operator--(int);				//[t82]
  const_reverse_tuple_iterator &operator+=(difference_type i)		//[t82]
      { iterator_type::operator-=(i); return *this; }
  const_reverse_tuple_iterator &operator-=(difference_type i)		//[t82]
      { iterator_type::operator+=(i); return *this; }
  //@}

  /**
   * @name Arithmetic operators
   */
  //@{
  const_reverse_tuple_iterator operator+(difference_type i) const	//[t82]
      { return const_reverse_tuple_iterator(base()-i); }
  const_reverse_tuple_iterator operator-(difference_type i)		//[t82]
      { return const_reverse_tuple_iterator(base()+i); }
  difference_type
    operator-(const const_reverse_tuple_iterator &rhs) const		//[t82]
      { return rhs.const_tuple_iterator::operator-(*this); }
  //@}

  /**
   * @name Comparisons
   */
  //@{
  bool
    operator==(const const_reverse_tuple_iterator &rhs) const throw ()	//[t82]
      { return iterator_type::operator==(rhs); }
  bool
    operator!=(const const_reverse_tuple_iterator &rhs) const throw ()	//[t82]
      { return !operator==(rhs); }

  bool operator<(const const_reverse_tuple_iterator &rhs) const		//[t82]
      { return iterator_type::operator>(rhs); }
  bool operator<=(const const_reverse_tuple_iterator &rhs) const	//[t82]
      { return iterator_type::operator>=(rhs); }
  bool operator>(const const_reverse_tuple_iterator &rhs) const		//[t82]
      { return iterator_type::operator<(rhs); }
  bool operator>=(const const_reverse_tuple_iterator &rhs) const	//[t82]
      { return iterator_type::operator<=(rhs); }
  //@}
};


inline const_tuple_iterator
const_tuple_iterator::operator+(difference_type o) const
{
  return const_tuple_iterator(
	tuple(home(), idx()),
	size_type(difference_type(col()) + o));
}

inline const_tuple_iterator
operator+(const_tuple_iterator::difference_type o, const_tuple_iterator i)
	{ return i + o; }

inline const_tuple_iterator
const_tuple_iterator::operator-(difference_type o) const
{
  return const_tuple_iterator(
	tuple(home(), idx()),
	size_type(difference_type(col()) - o));
}

inline const_tuple_iterator::difference_type
const_tuple_iterator::operator-(const_tuple_iterator i) const
	{ return difference_type(num() - i.num()); }


} // namespace pqxx


/*
[1] Scott Meyers, in one of his essential books, "Effective C++" and "More
Effective C++", points out that it is good style to have any class containing
a member of pointer type define a destructor--just to show that it knows what it
is doing with the pointer.  This helps prevent nasty memory leak / double
deletion bugs typically resulting from programmers' omission to deal with such
issues in their destructors.

The @c -Weffc++ option in gcc generates warnings for noncompliance with Scott's
style guidelines, and hence necessitates the definition of this destructor,
trivial as it may be.
*/


#include "pqxx/compiler-internal-post.hxx"

#endif
