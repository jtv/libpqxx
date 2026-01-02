/* Definitions for the pqxx::result class and support classes.
 *
 * pqxx::result represents the set of result rows from a database query.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/result instead.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_RESULT_ITERATOR
#define PQXX_H_RESULT_ITERATOR

#include "pqxx/row.hxx"


/* Result iterator.
 *
 * Don't include this header from your own application; it is included for you
 * by other libpqxx headers.
 */

namespace pqxx
{
/// Iterator for rows in a result.  Use as result::const_iterator.
/** A result, once obtained, cannot be modified.  Therefore all iterators on a
 * result are const iterators.
 *
 * @warning You must not destroy or move a @ref result object while any
 * iterators are still active on it, or on any of its rows.
 */
class PQXX_LIBEXPORT const_result_iterator
{
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = row_ref const;
  using pointer = row_ref const *;
  using reference = row_ref;
  using size_type = result_size_type;
  using difference_type = result_difference_type;

  /// Create an iterator, but in an unusable state.
  const_result_iterator() noexcept = default;
  /// Copy an iterator.
  const_result_iterator(const_result_iterator const &) noexcept = default;
  /// Move an iterator.
  const_result_iterator(const_result_iterator &&) noexcept = default;

  /// Create an iterator pointing at a row.
  const_result_iterator(result const &r, result_size_type i) noexcept :
          m_row{r, i}
  {}

  /// Create an iterator pointing at a row.
  const_result_iterator(row_ref const &r) noexcept :
          m_row{r.home(), r.row_number()}
  {}

  reference operator[](difference_type d) const { return *(*this + d); }

  /**
   * @name Dereferencing operators
   *
   * An iterator "points to" its current row.
   */
  //@{
  /// Dereference the iterator.
  [[nodiscard]] PQXX_RETURNS_NONNULL pointer operator->() const noexcept
  {
    return &m_row;
  }

  /// Dereference the iterator.
  [[nodiscard]] reference operator*() const noexcept { return m_row; }
  //@}


  /**
   * @name Manipulations
   */
  //@{
  const_result_iterator &operator=(const_result_iterator const &) = default;
  const_result_iterator &operator=(const_result_iterator &&) = default;

  const_result_iterator operator++(int) &;
  PQXX_INLINE_ONLY const_result_iterator &operator++()
  {
    pqxx::internal::gate::row_ref_const_result_iterator{m_row}.offset(1);
    return *this;
  }
  const_result_iterator operator--(int) &;
  PQXX_INLINE_ONLY const_result_iterator &operator--()
  {
    pqxx::internal::gate::row_ref_const_result_iterator(m_row).offset(-1);
    return *this;
  }

  const_result_iterator &operator+=(difference_type i)
  {
    pqxx::internal::gate::row_ref_const_result_iterator(m_row).offset(i);
    return *this;
  }
  const_result_iterator &operator-=(difference_type i)
  {
    pqxx::internal::gate::row_ref_const_result_iterator(m_row).offset(-i);
    return *this;
  }

  /// Interchange two iterators in an exception-safe manner.
  void swap(const_result_iterator &other) noexcept
  {
    std::swap(m_row, other.m_row);
  }
  //@}

  /**
   * @name Comparisons
   */
  //@{
  [[nodiscard]] bool operator==(const_result_iterator const &i) const
  {
    return (&m_row.home() == &i.m_row.home()) and
           (m_row.row_number() == i.m_row.row_number());
  }
  [[nodiscard]] bool operator!=(const_result_iterator const &i) const
  {
    return not(*this == i);
  }
  [[nodiscard]] bool operator<(const_result_iterator const &i) const
  {
    // Only needs to work when iterating the same result.
    return m_row.row_number() < i.m_row.row_number();
  }
  [[nodiscard]] bool operator<=(const_result_iterator const &i) const
  {
    // Only needs to work when iterating the same result.
    return m_row.row_number() <= i.m_row.row_number();
  }
  [[nodiscard]] bool operator>(const_result_iterator const &i) const
  {
    return m_row.row_number() > i.m_row.row_number();
  }
  [[nodiscard]] bool operator>=(const_result_iterator const &i) const
  {
    return m_row.row_number() >= i.m_row.row_number();
  }
  //@}

  /**
   * @name Arithmetic operators
   */
  //@{
  [[nodiscard]] inline const_result_iterator operator+(difference_type) const;
  friend const_result_iterator
  operator+(difference_type, const_result_iterator const &);
  [[nodiscard]] inline const_result_iterator operator-(difference_type) const;
  [[nodiscard]] inline difference_type
  operator-(const_result_iterator const &) const;
  //@}

private:
  row_ref m_row;
};


/// Reverse iterator for result.  Use as result::const_reverse_iterator.
class PQXX_LIBEXPORT const_reverse_result_iterator final
        : private const_result_iterator
{
public:
  using super = const_result_iterator;
  using iterator_type = const_result_iterator;
  using iterator_type::difference_type;
  using iterator_type::iterator_category;
  using iterator_type::pointer;
  using value_type = iterator_type::value_type;
  using reference = iterator_type::reference;

  /// Create an iterator, but in an unusable state.
  const_reverse_result_iterator() noexcept = default;
  /// Copy an iterator.
  const_reverse_result_iterator(
    const_reverse_result_iterator const &rhs) noexcept = default;
  /// Copy a reverse iterator from a regular iterator.
  PQXX_INLINE_ONLY explicit const_reverse_result_iterator(
    const_result_iterator const &rhs) :
          const_result_iterator{rhs}
  {
    super::operator--();
  }

  /// Return the underlying "regular" iterator (as per standard library).
  [[nodiscard]] const_result_iterator base() const noexcept;

  /**
   * @name Dereferencing operators
   */
  //@{
  /// Dereference iterator.
  using const_result_iterator::operator->;
  /// Dereference iterator.
  using const_result_iterator::operator*;
  //@}

  /**
   * @name Field access
   */
  //@{
  using const_result_iterator::operator[];
  //@}

  /**
   * @name Manipulations
   */
  //@{
  const_reverse_result_iterator &
  operator=(const_reverse_result_iterator const &r)
  {
    iterator_type::operator=(r);
    return *this;
  }
  const_reverse_result_iterator &operator=(const_reverse_result_iterator &&r)
  {
    iterator_type::operator=(std::move(r));
    return *this;
  }
  const_reverse_result_iterator &operator++()
  {
    iterator_type::operator--();
    return *this;
  }
  const_reverse_result_iterator operator++(int) &;
  const_reverse_result_iterator &operator--()
  {
    iterator_type::operator++();
    return *this;
  }
  const_reverse_result_iterator operator--(int) &;
  const_reverse_result_iterator &operator+=(difference_type i)
  {
    iterator_type::operator-=(i);
    return *this;
  }
  const_reverse_result_iterator &operator-=(difference_type i)
  {
    iterator_type::operator+=(i);
    return *this;
  }

  void swap(const_reverse_result_iterator &other) noexcept
  {
    const_result_iterator::swap(other);
  }
  //@}

  /**
   * @name Arithmetic operators
   */
  //@{
  [[nodiscard]] const_reverse_result_iterator
  operator+(difference_type i) const
  {
    return const_reverse_result_iterator(base() - i);
  }
  [[nodiscard]] const_reverse_result_iterator operator-(difference_type i)
  {
    return const_reverse_result_iterator(base() + i);
  }
  [[nodiscard]] difference_type
  operator-(const_reverse_result_iterator const &rhs) const
  {
    return rhs.const_result_iterator::operator-(*this);
  }
  //@}

  /**
   * @name Comparisons
   */
  //@{
  [[nodiscard]] bool
  operator==(const_reverse_result_iterator const &rhs) const noexcept
  {
    return iterator_type::operator==(rhs);
  }
  [[nodiscard]] bool
  operator!=(const_reverse_result_iterator const &rhs) const noexcept
  {
    return not operator==(rhs);
  }

  [[nodiscard]] bool operator<(const_reverse_result_iterator const &rhs) const
  {
    return iterator_type::operator>(rhs);
  }
  [[nodiscard]] bool operator<=(const_reverse_result_iterator const &rhs) const
  {
    return iterator_type::operator>=(rhs);
  }
  [[nodiscard]] bool operator>(const_reverse_result_iterator const &rhs) const
  {
    return iterator_type::operator<(rhs);
  }
  [[nodiscard]] bool operator>=(const_reverse_result_iterator const &rhs) const
  {
    return iterator_type::operator<=(rhs);
  }
  //@}
};


inline const_result_iterator
const_result_iterator::operator+(result::difference_type o) const
{
  return {
    m_row.home(), size_type(result::difference_type(m_row.row_number()) + o)};
}

inline const_result_iterator
operator+(result::difference_type o, const_result_iterator const &i)
{
  return i + o;
}

inline const_result_iterator
const_result_iterator::operator-(result::difference_type o) const
{
  return {
    m_row.home(),
    result_size_type(result::difference_type(m_row.row_number()) - o)};
}

PQXX_INLINE_ONLY inline result::difference_type
const_result_iterator::operator-(const const_result_iterator &i) const
{
  return result::difference_type(m_row.row_number() - i->row_number());
}

PQXX_INLINE_ONLY inline const_result_iterator result::end() const noexcept
{
  return {*this, size()};
}


inline const_result_iterator result::cend() const noexcept
{
  return end();
}


inline const_reverse_result_iterator
operator+(result::difference_type n, const_reverse_result_iterator const &i)
{
  return const_reverse_result_iterator{i.base() - n};
}

} // namespace pqxx
#endif
