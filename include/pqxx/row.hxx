/* Definitions for the pqxx::result class and support classes.
 *
 * pqxx::result represents the set of result rows from a database query.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/result instead.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_ROW
#define PQXX_H_ROW

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include "pqxx/except.hxx"
#include "pqxx/field.hxx"
#include "pqxx/result.hxx"


namespace pqxx
{
/// Reference to one row in a result.
/** A row represents one row (also called a row) in a query result set.
 * It also acts as a container mapping column numbers or names to field
 * values (see below):
 *
 * @code
 *	cout << row["date"].c_str() << ": " << row["name"].c_str() << endl;
 * @endcode
 *
 * The row itself acts like a (non-modifyable) container, complete with its
 * own const_iterator and const_reverse_iterator.
 */
class row
{
public:
  using size_type = row_size_type;
  using difference_type = row_difference_type;
  using const_iterator = const_row_iterator;
  using iterator = const_iterator;
  using reference = field;
  using pointer = const_row_iterator;
  using const_reverse_iterator = const_reverse_row_iterator;
  using reverse_iterator = const_reverse_iterator;

  PQXX_LIBEXPORT row() =default;

  /// @deprecated Do not use this constructor.  It will become private.
  PQXX_LIBEXPORT row(result r, size_t i) noexcept;

  /**
   * @name Comparison
   */
  //@{
  PQXX_LIBEXPORT PQXX_PURE bool operator==(const row &) const noexcept;
  bool operator!=(const row &rhs) const noexcept
      { return not operator==(rhs); }
  //@}

  PQXX_LIBEXPORT const_iterator begin() const noexcept;
  PQXX_LIBEXPORT const_iterator cbegin() const noexcept;
  PQXX_LIBEXPORT const_iterator end() const noexcept;
  PQXX_LIBEXPORT const_iterator cend() const noexcept;

  /**
   * @name Field access
   */
  //@{
  PQXX_LIBEXPORT reference front() const noexcept;
  PQXX_LIBEXPORT reference back() const noexcept;

  PQXX_LIBEXPORT const_reverse_row_iterator rbegin() const;
  PQXX_LIBEXPORT const_reverse_row_iterator crbegin() const;
  PQXX_LIBEXPORT const_reverse_row_iterator rend() const;
  PQXX_LIBEXPORT const_reverse_row_iterator crend() const;

  PQXX_LIBEXPORT reference operator[](size_type) const noexcept;
  PQXX_LIBEXPORT reference operator[](int) const noexcept;
  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   */
  PQXX_LIBEXPORT reference operator[](const char[]) const;
  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   */
  reference operator[](const std::string &s) const
  { return (*this)[s.c_str()]; }

  PQXX_LIBEXPORT reference at(size_type) const; 
  PQXX_LIBEXPORT reference at(int) const;
  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   */
  PQXX_LIBEXPORT reference at(const char[]) const;
  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   */
  reference at(const std::string &s) const
  { return at(s.c_str()); }
  //@}

  size_type size() const noexcept
  { return m_end-m_begin; }

  PQXX_LIBEXPORT void swap(row &) noexcept;

  /// Row number, assuming this is a real row and not end()/rend().
  size_t rownumber() const noexcept { return size_t(m_index); }

  /**
   * @name Column information
   */
  //@{
  /// Number of given column (throws exception if it doesn't exist).
  size_type column_number(const std::string &ColName) const
      { return column_number(ColName.c_str()); }

  /// Number of given column (throws exception if it doesn't exist).
  PQXX_LIBEXPORT size_type column_number(const char[]) const;       

  /// Return a column's type.
  PQXX_LIBEXPORT oid column_type(size_type) const;

  /// Return a column's type.
  oid column_type(int ColNum) const
      { return column_type(size_type(ColNum)); }

  /// Return a column's type.
  template<typename STRING>
  oid column_type(STRING ColName) const
      { return column_type(column_number(ColName)); }

  /// What table did this column come from?
  PQXX_LIBEXPORT oid column_table(size_type ColNum) const;

  /// What table did this column come from?
  oid column_table(int ColNum) const
      { return column_table(size_type(ColNum)); }
  /// What table did this column come from?
  template<typename STRING>
  oid column_table(STRING ColName) const
      { return column_table(column_number(ColName)); }

  /// What column number in its table did this result column come from?
  /** A meaningful answer can be given only if the column in question comes
   * directly from a column in a table.  If the column is computed in any
   * other way, a logic_error will be thrown.
   *
   * @param ColNum a zero-based column number in this result set
   * @return a zero-based column number in originating table
   */
  PQXX_LIBEXPORT size_type table_column(size_type) const;

  /// What column number in its table did this result column come from?
  size_type table_column(int ColNum) const
      { return table_column(size_type(ColNum)); }

  /// What column number in its table did this result column come from?
  template<typename STRING>
  size_type table_column(STRING ColName) const
      { return table_column(column_number(ColName)); }
  //@}

  size_t num() const { return rownumber(); }

  /** Produce a slice of this row, containing the given range of columns.
   *
   * The slice runs from the range's starting column to the range's end
   * column, exclusive.  It looks just like a normal result row, except
   * slices can be empty.
   *
   * @warning Slicing is a relatively new feature, and not all software may be
   * prepared to deal with empty slices.  If there is any chance that your
   * program might be creating empty slices and passing them to code that may
   * not be designed with the possibility of empty rows in mind, be sure to
   * test for that case.
   */
  PQXX_LIBEXPORT row slice(size_type Begin, size_type End) const;

  // Is this an empty slice?
  PQXX_LIBEXPORT PQXX_PURE bool empty() const noexcept;

protected:
  friend class field;
  /// Result set of which this is one row.
  result m_result;
  /// Row number.
  /**
   * You'd expect this to be a size_t, but due to the way reverse iterators
   * are related to regular iterators, it must be allowed to underflow to -1.
   */
  long m_index = 0;
  /// First column in slice.  This row ignores lower-numbered columns.
  size_type m_begin = 0;
  /// End column in slice.  This row only sees lower-numbered columns.
  size_type m_end = 0;
};


/// Iterator for fields in a row.  Use as row::const_iterator.
class const_row_iterator : public field
{
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = const field;
  using pointer = const field *;
  using size_type = row_size_type;
  using difference_type = row_difference_type;
  using reference = field;

  const_row_iterator(const row &T, row_size_type C) noexcept :
    field{T, C} {}
  const_row_iterator(const field &F) noexcept : field{F} {}

  /**
   * @name Dereferencing operators
   */
  //@{
  pointer operator->() const { return this; }
  reference operator*() const { return field{*this}; }
  //@}

  /**
   * @name Manipulations
   */
  //@{
  PQXX_LIBEXPORT const_row_iterator operator++(int);
  const_row_iterator &operator++() { ++m_col; return *this; }
  PQXX_LIBEXPORT const_row_iterator operator--(int);
  const_row_iterator &operator--() { --m_col; return *this; }

  const_row_iterator &operator+=(difference_type i)
      { m_col = size_type(difference_type(m_col) + i); return *this; }
  const_row_iterator &operator-=(difference_type i)
      { m_col = size_type(difference_type(m_col) - i); return *this; }
  //@}

  /**
   * @name Comparisons
   */
  //@{
  bool operator==(const const_row_iterator &i) const
      {return col()==i.col();}
  bool operator!=(const const_row_iterator &i) const
      {return col()!=i.col();}
  bool operator<(const const_row_iterator &i) const
      {return col()<i.col();}
  bool operator<=(const const_row_iterator &i) const
      {return col()<=i.col();}
  bool operator>(const const_row_iterator &i) const
      {return col()>i.col();}
  bool operator>=(const const_row_iterator &i) const
      {return col()>=i.col();}
  //@}

  /**
   * @name Arithmetic operators
   */
  //@{
  inline const_row_iterator operator+(difference_type) const;

  friend const_row_iterator operator+( difference_type, const_row_iterator);

  inline const_row_iterator operator-(difference_type) const;
  inline difference_type operator-(const_row_iterator) const;
  //@}
};


/// Reverse iterator for a row.  Use as row::const_reverse_iterator.
class const_reverse_row_iterator : private const_row_iterator
{
public:
  using super = const_row_iterator;
  using iterator_type = const_row_iterator;
  using iterator_type::iterator_category;
  using iterator_type::difference_type;
  using iterator_type::pointer;
  using value_type = iterator_type::value_type;
  using reference = iterator_type::reference;

  const_reverse_row_iterator(const const_reverse_row_iterator &r) :
    const_row_iterator{r} {}
  explicit
    const_reverse_row_iterator(const super &rhs) noexcept :
      const_row_iterator{rhs} { super::operator--(); }

  PQXX_LIBEXPORT PQXX_PURE iterator_type base() const noexcept;

  /**
   * @name Dereferencing operators
   */
  //@{
  using iterator_type::operator->;
  using iterator_type::operator*;
  //@}

  /**
   * @name Manipulations
   */
  //@{
  const_reverse_row_iterator &
    operator=(const const_reverse_row_iterator &r)
      { iterator_type::operator=(r); return *this; }
  const_reverse_row_iterator operator++()
      { iterator_type::operator--(); return *this; }
  PQXX_LIBEXPORT const_reverse_row_iterator operator++(int);
  const_reverse_row_iterator &operator--()
      { iterator_type::operator++(); return *this; }
  PQXX_LIBEXPORT const_reverse_row_iterator operator--(int);
  const_reverse_row_iterator &operator+=(difference_type i)
      { iterator_type::operator-=(i); return *this; }
  const_reverse_row_iterator &operator-=(difference_type i)
      { iterator_type::operator+=(i); return *this; }
  //@}

  /**
   * @name Arithmetic operators
   */
  //@{
  const_reverse_row_iterator operator+(difference_type i) const
      { return const_reverse_row_iterator{base()-i}; }
  const_reverse_row_iterator operator-(difference_type i)
      { return const_reverse_row_iterator{base()+i}; }
  difference_type
    operator-(const const_reverse_row_iterator &rhs) const
      { return rhs.const_row_iterator::operator-(*this); }
  //@}

  /**
   * @name Comparisons
   */
  //@{
  bool operator==(const const_reverse_row_iterator &rhs) const noexcept
      { return iterator_type::operator==(rhs); }
  bool operator!=(const const_reverse_row_iterator &rhs) const noexcept
      { return !operator==(rhs); }

  bool operator<(const const_reverse_row_iterator &rhs) const
      { return iterator_type::operator>(rhs); }
  bool operator<=(const const_reverse_row_iterator &rhs) const
      { return iterator_type::operator>=(rhs); }
  bool operator>(const const_reverse_row_iterator &rhs) const
      { return iterator_type::operator<(rhs); }
  bool operator>=(const const_reverse_row_iterator &rhs) const
      { return iterator_type::operator<=(rhs); }
  //@}
};


inline const_row_iterator
const_row_iterator::operator+(difference_type o) const
{
  return const_row_iterator{
	row(home(), idx()),
	size_type(difference_type(col()) + o)};
}

inline const_row_iterator
operator+(const_row_iterator::difference_type o, const_row_iterator i)
	{ return i + o; }

inline const_row_iterator
const_row_iterator::operator-(difference_type o) const
{
  return const_row_iterator{
	row(home(), idx()),
	size_type(difference_type(col()) - o)};
}

inline const_row_iterator::difference_type
const_row_iterator::operator-(const_row_iterator i) const
	{ return difference_type(num() - i.num()); }

} // namespace pqxx

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
