/* Definitions for the pqxx::result class and support classes.
 *
 * pqxx::result represents the set of result rows from a database query.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/result instead.
 *
 * Copyright (c) 2000-2025, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_ROW
#define PQXX_H_ROW

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include "pqxx/except.hxx"
#include "pqxx/field.hxx"
#include "pqxx/result.hxx"


namespace pqxx::internal
{
template<typename... T> class result_iter;
} // namespace pqxx::internal


namespace pqxx::internal::gate
{
class row_ref_const_result_iterator;
} // namespace pqxx::internal::gate


namespace pqxx
{
class field_ref;


/// Lightweight reference to one row in a result.
/** Like @ref row, represents one row (also called a row) in a query result
 * set. Unlike @ref row, however, it requires the @ref result object to which
 * it refers to remain valid and in the same place in memory throughout its
 * lifetime.  If you use this class, it is your responsibility to ensure that.
 */
class PQXX_LIBEXPORT row_ref
{
public:
  // TODO: Some of these types conflict: class is both iterator and container.
  // TODO: Set iterator nested types using std::iterator_traits.
  using size_type = row_size_type;
  using difference_type = row_difference_type;
  using const_iterator = const_row_iterator;
  using iterator = const_iterator;
  // XXX: Does this actually have to be a reference?
  using reference = field_ref;
  using pointer = field_ref const *;
  using const_reverse_iterator = const_reverse_row_iterator;
  using reverse_iterator = const_reverse_iterator;

  row_ref() = default;
  row_ref(row_ref const &) = default;
  row_ref(row_ref &&) = default;
  row_ref(result const &res, result::size_type index) :
          m_result{&res}, m_index{index}
  {}

  row_ref &operator=(row_ref const &) noexcept = default;
  row_ref &operator=(row_ref &&) noexcept = default;

  /**
   * @name Comparison
   */
  //@{
  [[nodiscard]] PQXX_PURE bool operator==(row_ref const &rhs) const noexcept
  {
    return rhs.m_result == m_result and rhs.m_index == m_index;
  }
  [[nodiscard]] bool operator!=(row_ref const &rhs) const noexcept
  {
    return not operator==(rhs);
  }
  //@}

  // XXX: Implement.
  [[nodiscard]] const_iterator cbegin() const noexcept;
  // XXX: Implement.
  [[nodiscard]] const_iterator begin() const noexcept;
  // XXX: Implement.
  [[nodiscard]] const_iterator end() const noexcept;
  // XXX: Implement.
  [[nodiscard]] const_iterator cend() const noexcept;

  /**
   * @name Field access
   */
  //@{
  // XXX: Implement.
  [[nodiscard]] reference front() const noexcept;
  // XXX: Implement.
  [[nodiscard]] reference back() const noexcept;

  // XXX: Implement.
  [[nodiscard]] const_reverse_row_iterator crbegin() const noexcept;
  // XXX: Implement.
  [[nodiscard]] const_reverse_row_iterator rbegin() const noexcept;
  // XXX: Implement.
  [[nodiscard]] const_reverse_row_iterator crend() const noexcept;
  // XXX: Implement.
  [[nodiscard]] const_reverse_row_iterator rend() const noexcept;

  // XXX: Implement.
  [[nodiscard]] reference operator[](size_type) const noexcept;
  // XXX: Implement.
  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   */
  [[nodiscard]] reference operator[](zview col_name) const;

  /// Address a field by number, but check that the number is in range.
  reference at(size_type i, sl loc = sl::current()) const
  {
    if (m_result == nullptr)
      throw usage_error{"Indexing uninitialised row.", loc};
    if (i < 0)
      throw usage_error{"Negative column index.", loc};
    auto const sz{size()};
    if (std::cmp_greater_equal(i, sz))
      throw range_error{
        std::format(
          "Column index out of range: {} in a result of {} column(s).", i, sz),
        loc};
    return operator[](i);
  }

  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   */
  reference at(zview col_name, sl loc = sl::current()) const
  {
    if (m_result == nullptr)
      throw usage_error{"Indexing uninitialised row.", loc};
    return operator[](column_number(col_name, loc));
  }

  [[nodiscard]] constexpr size_type size() const noexcept
  {
    return home().columns();
  }

  /// Row number, assuming this is a real row and not end()/rend().
  [[nodiscard]] constexpr result::size_type row_number() const noexcept
  {
    return m_index;
  }

  /**
   * @name Column information
   */
  //@{
  // XXX: Implement.
  /// Number of given column (throws exception if it doesn't exist).
  [[nodiscard]] size_type
  column_number(zview col_name, sl = sl::current()) const;

  // XXX: Implement.
  /// Return a column's type.
  [[nodiscard]] oid column_type(size_type, sl = sl::current()) const;

  // XXX: Implement.
  /// Return a column's type.
  [[nodiscard]] oid column_type(zview col_name, sl loc = sl::current()) const
  {
    return column_type(column_number(col_name, loc), loc);
  }

  // XXX: Implement.
  /// What table did this column come from?
  [[nodiscard]] oid column_table(size_type col_num, sl = sl::current()) const;

  // XXX: Implement.
  /// What table did this column come from?
  [[nodiscard]] oid column_table(zview col_name, sl loc = sl::current()) const
  {
    return column_table(column_number(col_name, loc), loc);
  }

  // XXX: Implement.
  /// What column number in its table did this result column come from?
  /** A meaningful answer can be given only if the column in question comes
   * directly from a column in a table.  If the column is computed in any
   * other way, a logic_error will be thrown.
   *
   * @param col_num a zero-based column number in this result set
   * @return a zero-based column number in originating table
   */
  [[nodiscard]] size_type table_column(size_type, sl = sl::current()) const;

  /// What column number in its table did this result column come from?
  [[nodiscard]] size_type
  table_column(zview col_name, sl loc = sl::current()) const
  {
    return table_column(column_number(col_name, loc), loc);
  }
  //@}

  /// Extract entire row's values into a tuple.
  /** Converts to the types of the tuple's respective fields.
   *
   * The types in the tuple must have conversions from PostgreSQL's text format
   * defined; see @ref datatypes.
   *
   * @throw usage_error If the number of columns in the `row` does not match
   * the number of fields in `t`.
   */
  template<typename Tuple> void to(Tuple &t, sl loc = sl::current()) const
  {
    check_size(std::tuple_size_v<Tuple>, loc);
    convert(t, loc);
  }

  /// Extract entire row's values into a tuple.
  /** Converts to the types of the tuple's respective fields.
   *
   * The types must have conversions from PostgreSQL's text format defined;
   * see @ref datatypes.
   *
   * @throw usage_error If the number of columns in the row does not match
   * the number of fields in `t`.
   */
  template<typename... TYPE>
  std::tuple<TYPE...> as(sl loc = sl::current()) const
  {
    check_size(sizeof...(TYPE), loc);
    using seq = std::make_index_sequence<sizeof...(TYPE)>;
    return get_tuple<std::tuple<TYPE...>>(seq{}, loc);
  }

  /// The @ref result object to which this `row_ref` refers.
  result const &home() const noexcept { return *m_result; }

private:
  friend class pqxx::internal::gate::row_ref_const_result_iterator;
  /// Move to another row (positive for forwards, negative for backwards).
  void offset(difference_type d) noexcept { m_index += d; }

  /// Throw @ref usage_error if row size is not `expected`.
  void check_size(size_type expected, sl loc) const
  {
    if (size() != expected)
      throw usage_error{
        std::format(
          "Tried to extract {} field(s) from a row of {}.", expected, size()),
        loc};
  }

  /// Convert to a given tuple of values, don't check sizes.
  /** We need this for cases where we have a full tuple of field types, but
   * not a parameter pack.
   */
  template<typename TUPLE> TUPLE as_tuple(sl loc) const
  {
    using seq = std::make_index_sequence<std::tuple_size_v<TUPLE>>;
    return get_tuple<TUPLE>(seq{}, loc);
  }

  // XXX: Make result_iter use row_ref instead of row.
  // XXX: Create gate.
  template<typename... T> friend class pqxx::internal::result_iter;
  /// Convert entire row to tuple fields, without checking row size.
  template<typename Tuple> void convert(Tuple &t, sl loc) const
  {
    extract_fields(
      t, std::make_index_sequence<std::tuple_size_v<Tuple>>{}, loc);
  }

  /// Result set of which this is one row.
  result const *m_result = nullptr;

  /// Row number.
  /**
   * You'd expect this to be unsigned, but due to the way reverse iterators
   * are related to regular iterators, it must be allowed to underflow to -1.
   */
  result::size_type m_index = -1;

  template<typename Tuple, std::size_t... indexes>
  void extract_fields(Tuple &t, std::index_sequence<indexes...>, sl loc) const
  {
    (extract_value<Tuple, indexes>(t, loc), ...);
  }

  template<typename Tuple, std::size_t index>
  void extract_value(Tuple &t, sl loc) const;

  /// Convert row's values as a new tuple.
  template<typename TUPLE, std::size_t... indexes>
  auto get_tuple(std::index_sequence<indexes...>, sl) const
  {
    return std::make_tuple(get_field<TUPLE, indexes>()...);
  }

  /// Extract and convert a field.
  template<typename TUPLE, std::size_t index> auto get_field() const
  {
    return (*this)[index].as<std::tuple_element_t<index, TUPLE>>();
  }
};
} // namespace pqxx


#include "pqxx/internal/gates/row_ref-const_result_iterator.hxx"


namespace pqxx
{
/// Reference to one row in a result.
/** This is like a @ref row_ref, except it's safe to destroy the @ref result
 * object or move it to a different place in memory.  The price is performance.
 *
 * A row represents one row (also called a row) in a query result set.
 * It also acts as a container mapping column numbers or names to field
 * values (see below):
 *
 * ```cxx
 *    cout << row["date"].c_str() << ": " << row["name"].c_str() << endl;
 * ```
 *
 * The row itself acts like a (non-modifyable) container, complete with its
 * own const_iterator and const_reverse_iterator.
 */
class PQXX_LIBEXPORT row
{
public:
  // TODO: Some of these types conflict: class is both iterator and container.
  // TODO: Set iterator nested types using std::iterator_traits.
  using size_type = row_size_type;
  using difference_type = row_difference_type;
  using const_iterator = const_row_iterator;
  using iterator = const_iterator;
  using reference = field;
  using pointer = const_row_iterator;
  using const_reverse_iterator = const_reverse_row_iterator;
  using reverse_iterator = const_reverse_iterator;

  row() noexcept = default;
  row(row &&) noexcept = default;
  row(row const &) noexcept = default;
  row &operator=(row const &) noexcept = default;
  row &operator=(row &&) noexcept = default;

  // XXX: Can probably rewrite a bunch of functions in terms of row_ref.
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

  [[nodiscard]] const_reverse_row_iterator rbegin() const noexcept;
  [[nodiscard]] const_reverse_row_iterator crbegin() const noexcept;
  [[nodiscard]] const_reverse_row_iterator rend() const noexcept;
  [[nodiscard]] const_reverse_row_iterator crend() const noexcept;

  [[nodiscard]] reference operator[](size_type) const noexcept;
  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   */
  [[nodiscard]] reference operator[](zview col_name) const;

  /// Address a field by number, but check that the number is in range.
  reference at(size_type, sl = sl::current()) const;

  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   */
  reference at(zview col_name, sl = sl::current()) const;

  [[nodiscard]] constexpr size_type size() const noexcept { return m_end; }

  /// Row number, assuming this is a real row and not end()/rend().
  [[nodiscard]] constexpr result::size_type rownumber() const noexcept
  {
    return m_index;
  }

  /**
   * @name Column information
   */
  //@{
  /// Number of given column (throws exception if it doesn't exist).
  [[nodiscard]] size_type
  column_number(zview col_name, sl = sl::current()) const;

  /// Return a column's type.
  [[nodiscard]] oid column_type(size_type, sl = sl::current()) const;

  /// Return a column's type.
  [[nodiscard]] oid column_type(zview col_name, sl loc = sl::current()) const
  {
    return column_type(column_number(col_name, loc), loc);
  }

  /// What table did this column come from?
  [[nodiscard]] oid column_table(size_type col_num, sl = sl::current()) const;

  /// What table did this column come from?
  [[nodiscard]] oid column_table(zview col_name, sl loc = sl::current()) const
  {
    return column_table(column_number(col_name, loc), loc);
  }

  /// What column number in its table did this result column come from?
  /** A meaningful answer can be given only if the column in question comes
   * directly from a column in a table.  If the column is computed in any
   * other way, a logic_error will be thrown.
   *
   * @param col_num a zero-based column number in this result set
   * @return a zero-based column number in originating table
   */
  [[nodiscard]] size_type table_column(size_type, sl = sl::current()) const;

  /// What column number in its table did this result column come from?
  [[nodiscard]] size_type
  table_column(zview col_name, sl loc = sl::current()) const
  {
    return table_column(column_number(col_name, loc), loc);
  }
  //@}

  [[nodiscard]] constexpr result::size_type num() const noexcept
  {
    return rownumber();
  }

  /// Extract entire row's values into a tuple.
  /** Converts to the types of the tuple's respective fields.
   *
   * The types in the tuple must have conversions from PostgreSQL's text format
   * defined; see @ref datatypes.
   *
   * @throw usage_error If the number of columns in the `row` does not match
   * the number of fields in `t`.
   */
  template<typename Tuple> void to(Tuple &t, sl loc = sl::current()) const
  {
    check_size(std::tuple_size_v<Tuple>, loc);
    convert(t, loc);
  }

  /// Extract entire row's values into a tuple.
  /** Converts to the types of the tuple's respective fields.
   *
   * The types must have conversions from PostgreSQL's text format defined;
   * see @ref datatypes.
   *
   * @throw usage_error If the number of columns in the `row` does not match
   * the number of fields in `t`.
   */
  template<typename... TYPE>
  std::tuple<TYPE...> as(sl loc = sl::current()) const
  {
    check_size(sizeof...(TYPE), loc);
    using seq = std::make_index_sequence<sizeof...(TYPE)>;
    return get_tuple<std::tuple<TYPE...>>(seq{}, loc);
  }

  [[deprecated("Swap iterators, not rows.")]] void swap(row &) noexcept;

protected:
  friend class const_row_iterator;
  friend class result;
  row(result r, result_size_type index, size_type cols) noexcept;

  /// Throw @ref usage_error if row size is not `expected`.
  void check_size(size_type expected, sl loc) const
  {
    if (size() != expected)
      throw usage_error{
        std::format(
          "Tried to extract {} field(s) from a row of {}.", expected, size()),
        loc};
  }

  /// Convert to a given tuple of values, don't check sizes.
  /** We need this for cases where we have a full tuple of field types, but
   * not a parameter pack.
   */
  template<typename TUPLE> TUPLE as_tuple(sl loc) const
  {
    using seq = std::make_index_sequence<std::tuple_size_v<TUPLE>>;
    return get_tuple<TUPLE>(seq{}, loc);
  }

  template<typename... T> friend class pqxx::internal::result_iter;
  /// Convert entire row to tuple fields, without checking row size.
  template<typename Tuple> void convert(Tuple &t, sl loc) const
  {
    extract_fields(
      t, std::make_index_sequence<std::tuple_size_v<Tuple>>{}, loc);
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

  /// Number of columns in the row.
  size_type m_end = 0;

private:
  template<typename Tuple, std::size_t... indexes>
  void extract_fields(Tuple &t, std::index_sequence<indexes...>, sl loc) const
  {
    (extract_value<Tuple, indexes>(t, loc), ...);
  }

  template<typename Tuple, std::size_t index>
  void extract_value(Tuple &t, sl loc) const;

  /// Convert row's values as a new tuple.
  template<typename TUPLE, std::size_t... indexes>
  auto get_tuple(std::index_sequence<indexes...>, sl) const
  {
    return std::make_tuple(get_field<TUPLE, indexes>()...);
  }

  /// Extract and convert a field.
  template<typename TUPLE, std::size_t index> auto get_field() const
  {
    return (*this)[index].as<std::tuple_element_t<index, TUPLE>>();
  }
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
  using reference = field_ref;

  const_row_iterator() noexcept = default;
  const_row_iterator(row const &t, row_size_type c) noexcept :
          field{t.m_result, t.m_index, c}
  {}
  const_row_iterator(field const &F) noexcept : field{F} {}
  const_row_iterator(const_row_iterator const &) noexcept = default;
  const_row_iterator(const_row_iterator &&) noexcept = default;

  /**
   * @name Dereferencing operators
   */
  //@{
  [[nodiscard]] constexpr pointer operator->() const noexcept { return this; }
  [[nodiscard]] reference operator*() const noexcept { return {*this}; }
  //@}

  /**
   * @name Manipulations
   */
  //@{
  const_row_iterator &operator=(const_row_iterator const &) noexcept = default;
  const_row_iterator &operator=(const_row_iterator &&) noexcept = default;

  const_row_iterator operator++(int) & noexcept;
  const_row_iterator &operator++() noexcept
  {
    ++m_col;
    return *this;
  }
  const_row_iterator operator--(int) & noexcept;
  const_row_iterator &operator--() noexcept
  {
    --m_col;
    return *this;
  }

  const_row_iterator &operator+=(difference_type i) noexcept
  {
    m_col = size_type(difference_type(m_col) + i);
    return *this;
  }
  const_row_iterator &operator-=(difference_type i) noexcept
  {
    m_col = size_type(difference_type(m_col) - i);
    return *this;
  }
  //@}

  /**
   * @name Comparisons
   */
  //@{
  [[nodiscard]] constexpr bool
  operator==(const_row_iterator const &i) const noexcept
  {
    return col() == i.col();
  }
  [[nodiscard]] constexpr bool
  operator!=(const_row_iterator const &i) const noexcept
  {
    return col() != i.col();
  }
  [[nodiscard]] constexpr bool
  operator<(const_row_iterator const &i) const noexcept
  {
    return col() < i.col();
  }
  [[nodiscard]] constexpr bool
  operator<=(const_row_iterator const &i) const noexcept
  {
    return col() <= i.col();
  }
  [[nodiscard]] constexpr bool
  operator>(const_row_iterator const &i) const noexcept
  {
    return col() > i.col();
  }
  [[nodiscard]] constexpr bool
  operator>=(const_row_iterator const &i) const noexcept
  {
    return col() >= i.col();
  }
  //@}

  /**
   * @name Arithmetic operators
   */
  //@{
  [[nodiscard]] inline const_row_iterator
  operator+(difference_type) const noexcept;

  friend const_row_iterator
  operator+(difference_type, const_row_iterator const &) noexcept;

  [[nodiscard]] inline const_row_iterator
  operator-(difference_type) const noexcept;
  [[nodiscard]] inline difference_type
  operator-(const_row_iterator const &) const noexcept;

  [[nodiscard]] inline field operator[](difference_type offset) const noexcept
  {
    return *(*this + offset);
  }
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

  const_reverse_row_iterator() noexcept = default;
  const_reverse_row_iterator(const_reverse_row_iterator const &) noexcept =
    default;
  const_reverse_row_iterator(const_reverse_row_iterator &&) noexcept = default;

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
  const_reverse_row_iterator &
  operator=(const_reverse_row_iterator const &r) noexcept
  {
    iterator_type::operator=(r);
    return *this;
  }
  const_reverse_row_iterator operator++() noexcept
  {
    iterator_type::operator--();
    return *this;
  }
  const_reverse_row_iterator operator++(int) & noexcept;
  const_reverse_row_iterator &operator--() noexcept
  {
    iterator_type::operator++();
    return *this;
  }
  const_reverse_row_iterator operator--(int) &;
  const_reverse_row_iterator &operator+=(difference_type i) noexcept
  {
    iterator_type::operator-=(i);
    return *this;
  }
  const_reverse_row_iterator &operator-=(difference_type i) noexcept
  {
    iterator_type::operator+=(i);
    return *this;
  }
  //@}

  /**
   * @name Arithmetic operators
   */
  //@{
  [[nodiscard]] const_reverse_row_iterator
  operator+(difference_type i) const noexcept
  {
    return const_reverse_row_iterator{base() - i};
  }
  [[nodiscard]] const_reverse_row_iterator
  operator-(difference_type i) noexcept
  {
    return const_reverse_row_iterator{base() + i};
  }
  [[nodiscard]] difference_type
  operator-(const_reverse_row_iterator const &rhs) const noexcept
  {
    return rhs.const_row_iterator::operator-(*this);
  }
  [[nodiscard]] inline field operator[](difference_type offset) const noexcept
  {
    return *(*this + offset);
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

  [[nodiscard]] constexpr bool
  operator<(const_reverse_row_iterator const &rhs) const noexcept
  {
    return iterator_type::operator>(rhs);
  }
  [[nodiscard]] constexpr bool
  operator<=(const_reverse_row_iterator const &rhs) const noexcept
  {
    return iterator_type::operator>=(rhs);
  }
  [[nodiscard]] constexpr bool
  operator>(const_reverse_row_iterator const &rhs) const noexcept
  {
    return iterator_type::operator<(rhs);
  }
  [[nodiscard]] constexpr bool
  operator>=(const_reverse_row_iterator const &rhs) const noexcept
  {
    return iterator_type::operator<=(rhs);
  }
  //@}
};


const_row_iterator
const_row_iterator::operator+(difference_type o) const noexcept
{
  // TODO:: More direct route to home().columns()?
  return {
    row{home(), idx(), home().columns()},
    size_type(difference_type(col()) + o)};
}

inline const_row_iterator operator+(
  const_row_iterator::difference_type o, const_row_iterator const &i) noexcept
{
  return i + o;
}

inline const_row_iterator
const_row_iterator::operator-(difference_type o) const noexcept
{
  // TODO:: More direct route to home().columns()?
  return {
    row{home(), idx(), home().columns()},
    size_type(difference_type(col()) - o)};
}

inline const_row_iterator::difference_type
const_row_iterator::operator-(const_row_iterator const &i) const noexcept
{
  return difference_type(num() - i.num());
}


template<typename Tuple, std::size_t index>
inline void row::extract_value(Tuple &t, sl) const
{
  using field_type = std::remove_cvref_t<decltype(std::get<index>(t))>;
  field const f{m_result, m_index, index};
  std::get<index>(t) = from_string<field_type>(f);
}
} // namespace pqxx
#endif
