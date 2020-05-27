/* Definitions for the pqxx::result class and support classes.
 *
 * pqxx::result represents the set of result rows from a database query.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/result instead.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
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

namespace pqxx::internal
{
template<typename... T> class result_iter;
} // namespace pqxx::internal


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
class PQXX_LIBEXPORT row
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

  row() = default;
  row(row &&) = default;
  row(row const &) = default;
  row &operator=(row const &) = default;
  row &operator=(row &&) = default;

  /**
   * @name Comparison
   */
  //@{
  [[nodiscard]] PQXX_PURE bool operator==(row const &) const noexcept;
  [[nodiscard]] bool operator!=(row const &rhs) const noexcept
  {
    return not operator==(rhs);
  }
  //@}

  [[nodiscard]] const_iterator begin() const noexcept;
  [[nodiscard]] const_iterator cbegin() const noexcept;
  [[nodiscard]] const_iterator end() const noexcept;
  [[nodiscard]] const_iterator cend() const noexcept;

  /**
   * @name Field access
   */
  //@{
  [[nodiscard]] reference front() const noexcept;
  [[nodiscard]] reference back() const noexcept;

  [[nodiscard]] const_reverse_row_iterator rbegin() const;
  [[nodiscard]] const_reverse_row_iterator crbegin() const;
  [[nodiscard]] const_reverse_row_iterator rend() const;
  [[nodiscard]] const_reverse_row_iterator crend() const;

  [[nodiscard]] reference operator[](size_type) const noexcept;
  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   */
  [[nodiscard]] reference operator[](char const[]) const;
  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   */
  [[nodiscard]] reference operator[](std::string const &s) const
  {
    return (*this)[s.c_str()];
  }

  reference at(size_type) const;
  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   */
  reference at(char const[]) const;
  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   */
  reference at(std::string const &s) const { return at(s.c_str()); }
  //@}

  [[nodiscard]] size_type size() const noexcept { return m_end - m_begin; }

  /// @deprecated Do this sort of thing on iterators instead.
  void swap(row &) noexcept;

  /// Row number, assuming this is a real row and not end()/rend().
  [[nodiscard]] result::size_type rownumber() const noexcept
  {
    return m_index;
  }

  /**
   * @name Column information
   */
  //@{
  /// Number of given column (throws exception if it doesn't exist).
  [[nodiscard]] size_type column_number(std::string const &col_name) const
  {
    return column_number(col_name.c_str());
  }

  /// Number of given column (throws exception if it doesn't exist).
  size_type column_number(char const[]) const;

  /// Return a column's type.
  [[nodiscard]] oid column_type(size_type) const;

  /// Return a column's type.
  template<typename STRING> oid column_type(STRING col_name) const
  {
    return column_type(column_number(col_name));
  }

  /// What table did this column come from?
  [[nodiscard]] oid column_table(size_type col_num) const;

  /// What table did this column come from?
  template<typename STRING> oid column_table(STRING col_name) const
  {
    return column_table(column_number(col_name));
  }

  /// What column number in its table did this result column come from?
  /** A meaningful answer can be given only if the column in question comes
   * directly from a column in a table.  If the column is computed in any
   * other way, a logic_error will be thrown.
   *
   * @param col_num a zero-based column number in this result set
   * @return a zero-based column number in originating table
   */
  [[nodiscard]] size_type table_column(size_type) const;

  /// What column number in its table did this result column come from?
  template<typename STRING> size_type table_column(STRING col_name) const
  {
    return table_column(column_number(col_name));
  }
  //@}

  [[nodiscard]] result::size_type num() const { return rownumber(); }

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
  [[nodiscard]] row slice(size_type sbegin, size_type send) const;

  // Is this an empty slice?
  [[nodiscard]] PQXX_PURE bool empty() const noexcept;

  /// Extract entire row's values into a tuple.
  /** Converts to the types of the tuple's respective fields.
   */
  template<typename Tuple> void to(Tuple &t) const
  {
    check_size(std::tuple_size_v<Tuple>);
    convert(t);
  }

protected:
  friend class const_row_iterator;
  friend class result;
  row(result const &r, result_size_type i) noexcept;

  /// Throw @c usage_error if row size is not @c expected.
  void check_size(size_type expected) const
  {
    if (size() != expected)
      throw usage_error{
        "Tried to extract " + to_string(expected) +
        " field(s) from a row of " + to_string(size()) + "."};
  }

  template<typename... T> friend class pqxx::internal::result_iter;
  /// Convert entire row to tuple fields, without checking row size.
  template<typename Tuple> void convert(Tuple &t) const
  {
    constexpr auto tup_size{std::tuple_size_v<Tuple>};
    extract_fields(t, std::make_index_sequence<tup_size>{});
  }

  friend class field;
  /// Result set of which this is one row.
  result m_result;
  /// Row number.
  /**
   * You'd expect this to be unsigned, but due to the way reverse iterators
   * are related to regular iterators, it must be allowed to underflow to -1.
   */
  result::size_type m_index = 0;
  /// First column in slice.  This row ignores lower-numbered columns.
  size_type m_begin = 0;
  /// End column in slice.  This row only sees lower-numbered columns.
  size_type m_end = 0;

private:
  template<typename Tuple, std::size_t... indexes>
  void extract_fields(Tuple &t, std::index_sequence<indexes...>) const
  {
    (extract_value<Tuple, indexes>(t), ...);
  }

  template<typename Tuple, std::size_t index>
  void extract_value(Tuple &t) const;
};


/// Iterator for fields in a row.  Use as row::const_iterator.
class PQXX_LIBEXPORT const_row_iterator : public field
{
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = field const;
  using pointer = field const *;
  using size_type = row_size_type;
  using difference_type = row_difference_type;
  using reference = field;

  const_row_iterator() = default;
  const_row_iterator(row const &T, row_size_type C) noexcept : field{T, C} {}
  const_row_iterator(field const &F) noexcept : field{F} {}
  const_row_iterator(const_row_iterator const &) = default;
  const_row_iterator(const_row_iterator &&) = default;

  /**
   * @name Dereferencing operators
   */
  //@{
  [[nodiscard]] pointer operator->() const { return this; }
  [[nodiscard]] reference operator*() const { return field{*this}; }
  //@}

  /**
   * @name Manipulations
   */
  //@{
  const_row_iterator &operator=(const_row_iterator const &) = default;
  const_row_iterator &operator=(const_row_iterator &&) = default;

  const_row_iterator operator++(int);
  const_row_iterator &operator++()
  {
    ++m_col;
    return *this;
  }
  const_row_iterator operator--(int);
  const_row_iterator &operator--()
  {
    --m_col;
    return *this;
  }

  const_row_iterator &operator+=(difference_type i)
  {
    m_col = size_type(difference_type(m_col) + i);
    return *this;
  }
  const_row_iterator &operator-=(difference_type i)
  {
    m_col = size_type(difference_type(m_col) - i);
    return *this;
  }
  //@}

  /**
   * @name Comparisons
   */
  //@{
  [[nodiscard]] bool operator==(const_row_iterator const &i) const
  {
    return col() == i.col();
  }
  [[nodiscard]] bool operator!=(const_row_iterator const &i) const
  {
    return col() != i.col();
  }
  [[nodiscard]] bool operator<(const_row_iterator const &i) const
  {
    return col() < i.col();
  }
  [[nodiscard]] bool operator<=(const_row_iterator const &i) const
  {
    return col() <= i.col();
  }
  [[nodiscard]] bool operator>(const_row_iterator const &i) const
  {
    return col() > i.col();
  }
  [[nodiscard]] bool operator>=(const_row_iterator const &i) const
  {
    return col() >= i.col();
  }
  //@}

  /**
   * @name Arithmetic operators
   */
  //@{
  [[nodiscard]] inline const_row_iterator operator+(difference_type) const;

  friend const_row_iterator
  operator+(difference_type, const_row_iterator const &);

  [[nodiscard]] inline const_row_iterator operator-(difference_type) const;
  [[nodiscard]] inline difference_type
  operator-(const_row_iterator const &) const;
  //@}
};


/// Reverse iterator for a row.  Use as row::const_reverse_iterator.
class PQXX_LIBEXPORT const_reverse_row_iterator : private const_row_iterator
{
public:
  using super = const_row_iterator;
  using iterator_type = const_row_iterator;
  using iterator_type::difference_type;
  using iterator_type::iterator_category;
  using iterator_type::pointer;
  using value_type = iterator_type::value_type;
  using reference = iterator_type::reference;

  const_reverse_row_iterator() = default;
  const_reverse_row_iterator(const_reverse_row_iterator const &) = default;
  const_reverse_row_iterator(const_reverse_row_iterator &&) = default;

  explicit const_reverse_row_iterator(super const &rhs) noexcept :
          const_row_iterator{rhs}
  {
    super::operator--();
  }

  [[nodiscard]] PQXX_PURE iterator_type base() const noexcept;

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
  const_reverse_row_iterator &operator=(const_reverse_row_iterator const &r)
  {
    iterator_type::operator=(r);
    return *this;
  }
  const_reverse_row_iterator operator++()
  {
    iterator_type::operator--();
    return *this;
  }
  const_reverse_row_iterator operator++(int);
  const_reverse_row_iterator &operator--()
  {
    iterator_type::operator++();
    return *this;
  }
  const_reverse_row_iterator operator--(int);
  const_reverse_row_iterator &operator+=(difference_type i)
  {
    iterator_type::operator-=(i);
    return *this;
  }
  const_reverse_row_iterator &operator-=(difference_type i)
  {
    iterator_type::operator+=(i);
    return *this;
  }
  //@}

  /**
   * @name Arithmetic operators
   */
  //@{
  [[nodiscard]] const_reverse_row_iterator operator+(difference_type i) const
  {
    return const_reverse_row_iterator{base() - i};
  }
  [[nodiscard]] const_reverse_row_iterator operator-(difference_type i)
  {
    return const_reverse_row_iterator{base() + i};
  }
  [[nodiscard]] difference_type
  operator-(const_reverse_row_iterator const &rhs) const
  {
    return rhs.const_row_iterator::operator-(*this);
  }
  //@}

  /**
   * @name Comparisons
   */
  //@{
  [[nodiscard]] bool
  operator==(const_reverse_row_iterator const &rhs) const noexcept
  {
    return iterator_type::operator==(rhs);
  }
  [[nodiscard]] bool
  operator!=(const_reverse_row_iterator const &rhs) const noexcept
  {
    return !operator==(rhs);
  }

  [[nodiscard]] bool operator<(const_reverse_row_iterator const &rhs) const
  {
    return iterator_type::operator>(rhs);
  }
  [[nodiscard]] bool operator<=(const_reverse_row_iterator const &rhs) const
  {
    return iterator_type::operator>=(rhs);
  }
  [[nodiscard]] bool operator>(const_reverse_row_iterator const &rhs) const
  {
    return iterator_type::operator<(rhs);
  }
  [[nodiscard]] bool operator>=(const_reverse_row_iterator const &rhs) const
  {
    return iterator_type::operator<=(rhs);
  }
  //@}
};


const_row_iterator const_row_iterator::operator+(difference_type o) const
{
  return const_row_iterator{
    row(home(), idx()), size_type(difference_type(col()) + o)};
}

inline const_row_iterator
operator+(const_row_iterator::difference_type o, const_row_iterator const &i)
{
  return i + o;
}

inline const_row_iterator
const_row_iterator::operator-(difference_type o) const
{
  return const_row_iterator{
    row(home(), idx()), size_type(difference_type(col()) - o)};
}

inline const_row_iterator::difference_type
const_row_iterator::operator-(const_row_iterator const &i) const
{
  return difference_type(num() - i.num());
}


template<typename Tuple, std::size_t index>
inline void row::extract_value(Tuple &t) const
{
  using field_type = strip_t<decltype(std::get<index>(t))>;
  field const f{*this, index};
  std::get<index>(t) = from_string<field_type>(f);
}
} // namespace pqxx

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
