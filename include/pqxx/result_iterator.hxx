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
#ifndef PQXX_H_RESULT_ITERATOR
#define PQXX_H_RESULT_ITERATOR

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include "pqxx/row.hxx"


/* Result iterator.
 *
 * Don't include this header from your own application; it is included for you
 * by other libpqxx headers.
 */

namespace pqxx
{
/// Iterator for rows in a result.  Use as result::const_iterator.
/** A result, once obtained, cannot be modified.  Therefore there is no
 * plain iterator type for result.  However its const_iterator type can be
 * used to inspect its rows without changing them.
 */
class PQXX_LIBEXPORT const_result_iterator : public row
{
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = row const;
  using pointer = row const *;
  using reference = row;
  using size_type = result_size_type;
  using difference_type = result_difference_type;

  const_result_iterator() noexcept = default;
  const_result_iterator(const_result_iterator const &) noexcept = default;
  const_result_iterator(const_result_iterator &&) noexcept = default;
  const_result_iterator(row const &t) noexcept : row{t} {}

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
  [[nodiscard]] pointer operator->() const { return this; }
  [[nodiscard]] reference operator*() const { return row{*this}; }
  //@}

  /**
   * @name Field access
   */
  //@{
  using row::back;
  using row::front;
  using row::operator[];
  using row::at;
  using row::rownumber;
  //@}

  /**
   * @name Manipulations
   */
  //@{
  const_result_iterator &operator=(const_result_iterator const &rhs)
  {
    row::operator=(rhs);
    return *this;
  }

  const_result_iterator &operator=(const_result_iterator &&rhs)
  {
    row::operator=(std::move(rhs));
    return *this;
  }

  const_result_iterator operator++(int);
  const_result_iterator &operator++()
  {
    ++m_index;
    return *this;
  }
  const_result_iterator operator--(int);
  const_result_iterator &operator--()
  {
    --m_index;
    return *this;
  }

  const_result_iterator &operator+=(difference_type i)
  {
    m_index += i;
    return *this;
  }
  const_result_iterator &operator-=(difference_type i)
  {
    m_index -= i;
    return *this;
  }

  void swap(const_result_iterator &other) noexcept { row::swap(other); }
  //@}

  /**
   * @name Comparisons
   */
  //@{
  [[nodiscard]] bool operator==(const_result_iterator const &i) const
  {
    return m_index == i.m_index;
  }
  [[nodiscard]] bool operator!=(const_result_iterator const &i) const
  {
    return m_index != i.m_index;
  }
  [[nodiscard]] bool operator<(const_result_iterator const &i) const
  {
    return m_index < i.m_index;
  }
  [[nodiscard]] bool operator<=(const_result_iterator const &i) const
  {
    return m_index <= i.m_index;
  }
  [[nodiscard]] bool operator>(const_result_iterator const &i) const
  {
    return m_index > i.m_index;
  }
  [[nodiscard]] bool operator>=(const_result_iterator const &i) const
  {
    return m_index >= i.m_index;
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
  friend class pqxx::result;
  const_result_iterator(pqxx::result const *r, result_size_type i) noexcept :
          row{*r, i}
  {}
};


/// Reverse iterator for result.  Use as result::const_reverse_iterator.
class PQXX_LIBEXPORT const_reverse_result_iterator
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

  const_reverse_result_iterator() = default;
  const_reverse_result_iterator(const_reverse_result_iterator const &rhs) =
    default;
  explicit const_reverse_result_iterator(const_result_iterator const &rhs) :
          const_result_iterator{rhs}
  {
    super::operator--();
  }

  explicit const_reverse_result_iterator(const_result_iterator const &&rhs) :
          const_result_iterator{std::move(rhs)}
  {
    super::operator--();
  }

  [[nodiscard]] PQXX_PURE const_result_iterator base() const noexcept;

  /**
   * @name Dereferencing operators
   */
  //@{
  using const_result_iterator::operator->;
  using const_result_iterator::operator*;
  //@}

  /**
   * @name Field access
   */
  //@{
  using const_result_iterator::back;
  using const_result_iterator::front;
  using const_result_iterator::operator[];
  using const_result_iterator::at;
  using const_result_iterator::rownumber;
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
  const_reverse_result_iterator operator++(int);
  const_reverse_result_iterator &operator--()
  {
    iterator_type::operator++();
    return *this;
  }
  const_reverse_result_iterator operator--(int);
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
  return const_result_iterator{
    &m_result, size_type(result::difference_type(m_index) + o)};
}

inline const_result_iterator
operator+(result::difference_type o, const_result_iterator const &i)
{
  return i + o;
}

inline const_result_iterator
const_result_iterator::operator-(result::difference_type o) const
{
  return const_result_iterator{
    &m_result, result_size_type(result::difference_type(m_index) - o)};
}

inline result::difference_type
const_result_iterator::operator-(const const_result_iterator &i) const
{
  return result::difference_type(num() - i.num());
}

inline const_result_iterator result::end() const noexcept
{
  return const_result_iterator{this, size()};
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

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
