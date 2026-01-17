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
class row_ref_result;
class row_ref_row;
} // namespace pqxx::internal::gate


namespace pqxx
{
class field_ref;


/// Lightweight reference to one row in a result.
/** Like @ref row, represents one row in a query result set.  Unlike with
 * @ref row, however, for as long as you're using a `row_ref`, the @ref result
 * object to which it refers must...
 * 1. remain valid, i.e. you can't destroy it;
 * 2. and in the same place in memory, i.e. you can't move it;
 * 3. and keep the same value, i.e. you can't assign to it.
 *
 * When you use `row_ref`, it is your responsibility to ensure all that.
 *
 * A `row_ref` is one way of accessing the fields themselves.
 */
class PQXX_LIBEXPORT row_ref final
{
public:
  using size_type = row_size_type;
  using difference_type = row_difference_type;
  using const_iterator = const_row_iterator;
  using iterator = const_iterator;
  using reference = field_ref;
  using pointer = field_ref const *;
  using const_reverse_iterator = const_reverse_row_iterator;
  using reverse_iterator = const_reverse_iterator;

  row_ref() = default;
  row_ref(row_ref const &) = default;
  row_ref(row_ref &&) = default;
  row_ref(result const &res, result_size_type index) :
          m_result{&res}, m_index{index}
  {}

  row_ref &operator=(row_ref const &) noexcept = default;
  row_ref &operator=(row_ref &&) noexcept = default;

  /**
   * @name Comparison
   *
   * Equality between two @ref row_ref objects means that they both refer to
   * the same row in _the exact same @ref result object._
   *
   * So, if you copy a @ref result, even though the two copies refer to the
   * exact same underlying data structure, a `row_ref` in the one will never be
   * equal to a `row_ref` in the other.
   *
   * ```cxx
   * void test_equality(pqxx::result const &r1)
   * {
   *   pqxx::result const r2{r1};
   *   if (not r1.empty())
   *   {
   *     assert(r1[0] == r1[0]);
   *     assert(r2[0] != r1[0]);
   *   }
   * }
   * ```
   */
  //@{
  [[nodiscard]] PQXX_PURE bool operator==(row_ref const &rhs) const noexcept
  {
    return rhs.m_result == m_result and rhs.m_index == m_index;
  }
  [[nodiscard]] PQXX_PURE bool operator!=(row_ref const &rhs) const noexcept
  {
    return not operator==(rhs);
  }
  //@}

  [[nodiscard]] const_iterator cbegin() const noexcept;
  [[nodiscard]] const_iterator begin() const noexcept;
  [[nodiscard]] const_iterator end() const noexcept;
  [[nodiscard]] const_iterator cend() const noexcept;

  /**
   * @name Field access
   */
  //@{
  [[nodiscard]] reference front() const noexcept;
  [[nodiscard]] reference back() const noexcept;

  [[nodiscard]] const_reverse_row_iterator crbegin() const noexcept;
  [[nodiscard]] const_reverse_row_iterator rbegin() const noexcept;
  [[nodiscard]] const_reverse_row_iterator crend() const noexcept;
  [[nodiscard]] const_reverse_row_iterator rend() const noexcept;

  [[nodiscard]] PQXX_PURE reference operator[](size_type i) const noexcept
  {
    return {home(), row_number(), i};
  }

#if defined(PQXX_HAVE_MULTIDIM)
  // TODO: There's a proposal to permit a default value for loc.
  /** Address field by name.
   *
   * @warning This is much slower than indexing by number, or iterating.
   *
   * @warning This function is marked as "pure."  This means that if it fails,
   * depending on your compiler, the exception may occur in a different place
   * than you expected.  The compiler may even find scenarios where it can
   * avoid calling this operator, meaning that the exception does not happen at
   * all.  If you need more deterministic exception behaviour, use @ref at().
   */
  [[nodiscard]] PQXX_PURE reference operator[](zview col_name, sl) const;
#endif // PQXX_HAVE_MULTIDIM

  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   *
   * @warning This function is marked as "pure."  This means that if it fails,
   * depending on your compiler, the exception may occur in a different place
   * than you expected.  The compiler may even find scenarios where it can
   * avoid calling this operator, meaning that the exception does not happen at
   * all.  If you need more deterministic exception behaviour, use @ref at().
   */
  [[nodiscard]] PQXX_PURE reference operator[](zview col_name) const;

  /// Address a field by number, but check that the number is in range.
  reference at(size_type i, sl loc = sl::current()) const
  {
    if (m_result == nullptr)
      throw usage_error{"Indexing uninitialised row.", loc};
    if (std::cmp_less(i, 0))
      throw usage_error{"Negative column index.", loc};
    auto const sz{size()};
    if (std::cmp_greater_equal(i, sz))
      throw range_error{
        std::format(
          "Column index out of range: {} in a result of {} column(s).", i, sz),
        loc};
    return operator[](i);
  }

  /// Address field by name.
  /** @warning This is much slower than indexing by number, or iterating.
   *
   * This function could perhaps be marked as "pure," but in clang and some gcc
   * configurations this may move exceptions out of `try` blocks that are meant
   * to catch them.
   */
  reference at(zview col_name, sl loc = sl::current()) const
  {
    if (m_result == nullptr)
      throw usage_error{"Indexing uninitialised row.", loc};
    return operator[](column_number(col_name, loc));
  }

  [[nodiscard]] PQXX_PURE size_type size() const noexcept
  {
    return home().columns();
  }

  /// Row number, assuming this is a real row and not end()/rend().
  [[nodiscard]] PQXX_PURE constexpr result::size_type
  row_number() const noexcept
  {
    return m_index;
  }

  /**
   * @name Column information
   */
  //@{
  /// Number of given column (throws exception if it doesn't exist).
  /** This function could perhaps be marked as "pure," but in clang and some
   * gcc configurations this may move exceptions out of `try` blocks that are
   * meant to catch them.
   */
  [[nodiscard]] size_type
  column_number(zview col_name, sl = sl::current()) const;

  /// Return a column's type.
  /** This function could perhaps be marked as "pure," but in clang and some
   * gcc configurations this may move exceptions out of `try` blocks that are
   * meant to catch them.
   */
  [[nodiscard]] oid
  column_type(size_type col_num, sl loc = sl::current()) const
  {
    return home().column_type(col_num, loc);
  }

  /// Return a column's type.
  /** This function could perhaps be marked as "pure," but in clang and some
   * gcc configurations this may move exceptions out of `try` blocks that are
   * meant to catch them.
   */
  [[nodiscard]] oid column_type(zview col_name, sl loc = sl::current()) const
  {
    return column_type(column_number(col_name, loc), loc);
  }

  /// What table did this column come from?
  /** This function could perhaps be marked as "pure," but in clang and some
   * gcc configurations this may move exceptions out of `try` blocks that are
   * meant to catch them.
   */
  [[nodiscard]] oid column_table(size_type col_num, sl = sl::current()) const;

  /// What table did this column come from?
  /** This function could perhaps be marked as "pure," but in clang and some
   * gcc configurations this may move exceptions out of `try` blocks that are
   * meant to catch them.
   */
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
  [[nodiscard]] size_type
  table_column(size_type col_num, sl loc = sl::current()) const
  {
    return home().table_column(col_num, loc);
  }

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
   * @throw usage_error If the number of columns in the row does not match
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
    return as_tuple<std::tuple<TYPE...>>(loc);
  }

  /// Convert to a given tuple of values,
  /** Useful in cases where we have a full tuple of field types, but
   * not a parameter pack.
   *
   * `TUPLE` should be a tuple type, with the same number of elements as the
   * result has columns.
   *
   * @throw usage_error If the number of columns in the `row` does not match
   * the number of fields in `TUPLE`.
   */
  template<typename TUPLE>
  TUPLE as_tuple(sl loc = sl::current()) const
    requires(requires(TUPLE t) { std::get<0>(t); })
  {
    check_size(std::tuple_size_v<TUPLE>, loc);
    using seq = std::make_index_sequence<std::tuple_size_v<TUPLE>>;
    return get_tuple<TUPLE>(seq{}, loc);
  }

  /// The @ref result object to which this `row_ref` refers.
  PQXX_PURE result const &home() const noexcept { return *m_result; }

private:
  friend class pqxx::internal::gate::row_ref_const_result_iterator;
  friend class pqxx::internal::gate::row_ref_result;
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

  template<typename... T> friend class pqxx::internal::result_iter;
  /// Convert entire row to tuple fields, without checking row size.
  template<typename Tuple> void convert(Tuple &t, sl loc) const
  {
    extract_fields(
      t, std::make_index_sequence<std::tuple_size_v<Tuple>>{},
      conversion_context{home().get_encoding_group(), loc});
  }

  friend class pqxx::internal::gate::row_ref_row;
  template<typename Tuple, std::size_t... indexes>
  void extract_fields(Tuple &t, std::index_sequence<indexes...>, ctx c) const
  {
    (extract_value<Tuple, indexes>(t, c), ...);
  }

  template<typename Tuple, std::size_t index>
  void extract_value(Tuple &t, ctx) const;

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

  /// Result set of which this is one row.
  result const *m_result = nullptr;

  /// Row number.
  /**
   * You'd expect this to be unsigned, but due to the way reverse iterators
   * are related to regular iterators, it must be allowed to underflow to -1.
   */
  result::size_type m_index = -1;
};
} // namespace pqxx


#include "pqxx/internal/gates/row_ref-const_result_iterator.hxx"
#include "pqxx/internal/gates/row_ref-row.hxx"


namespace pqxx
{
/// Reference to one row in a result.
/** A `row` is a reference to one particular row of data in a query result set
 * (represented by a @ref result object).
 *
 * This is like a @ref row_ref, except (as far as this class is concerned) it
 * is safe to destroy the @ref result object or move it to a different place in
 * memory.  That's because unlike `row_ref`, a `row` contains its own copy of
 * the internal smart pointer referring to the actual data in memory.  You do
 * pay a performance price for this, so prefer @ref row_ref where possible.
 *
 * A row also acts as a container, mapping column numbers or names to field
 * values:
 *
 * ```cxx
 *    cout << row["date"].c_str() << ": " << row["name"].c_str() << endl;
 * ```
 */
class PQXX_LIBEXPORT row final
{
public:
  // TODO: Set iterator nested types using std::iterator_traits.
  using size_type = row_size_type;
  using difference_type = row_difference_type;
  using reference = field_ref;
  using pointer = const_row_iterator;
  using const_iterator = const_row_iterator;
  using iterator = const_iterator;
  using const_reverse_iterator = const_reverse_row_iterator;
  using reverse_iterator = const_reverse_iterator;

  row() noexcept = default;
  row(row &&) noexcept = default;
  row(row const &) noexcept = default;
  row(row_ref const &ref) :
          m_result{ref.home()},
          m_index{ref.row_number()},
          m_end{std::size(ref)}
  {}
  row &operator=(row const &) noexcept = default;
  row &operator=(row &&) noexcept = default;

  /**
   * @name Comparison
   *
   * @warning The meaning of the comparison operators has changed in 8.0.  The
   * _old_ behaviour was to go through all the data in the two rows (but not
   * the metadata) and look for differences.  This was never fully specified.
   *
   * Equality between two @ref row objects means that they refer to the same
   * row in the same data structure that we originally received from the
   * database.  So the two may have been created from different @ref result
   * objects, so long as those are both copies of (or the same as) the same
   * original @ref result that you received from the database.
   *
   * ```cxx
   * void test_row_equality(pqxx::result const &r1)
   * {
   *   pqxx::result r2{r1};
   *   pqxx::row row1{r1[0]}, row2{r2};
   *   assert(row1 == row2);
   * }
   * ```
   */
  //@{
  [[nodiscard]] PQXX_PURE bool operator==(row const &rhs) const noexcept
  {
    return rhs.m_result == m_result and rhs.m_index == m_index;
  }
  [[nodiscard]] PQXX_PURE bool operator!=(row const &rhs) const noexcept
  {
    return not operator==(rhs);
  }
  //@}

  /**
   * @name Iteration
   *
   * A row acts like a container of fields.
   */
  //@{
  [[nodiscard]] PQXX_PURE const_iterator begin() const noexcept;
  [[nodiscard]] PQXX_PURE const_iterator cbegin() const noexcept;
  [[nodiscard]] PQXX_PURE const_iterator end() const noexcept;
  [[nodiscard]] PQXX_PURE const_iterator cend() const noexcept;
  [[nodiscard]] PQXX_PURE const_reverse_row_iterator rbegin() const noexcept;
  [[nodiscard]] PQXX_PURE const_reverse_row_iterator crbegin() const noexcept;
  [[nodiscard]] PQXX_PURE const_reverse_row_iterator rend() const noexcept;
  [[nodiscard]] PQXX_PURE const_reverse_row_iterator crend() const noexcept;
  //@}

  /**
   * @name Field access
   */
  //@{
  [[nodiscard]] field_ref front() const noexcept;
  [[nodiscard]] field_ref back() const noexcept;
  //@}

  /** @warning This function is marked as "pure."  This means that if it
   * fails, depending on your compiler, the exception may occur in a different
   * place than you expected.  The compiler may even find scenarios where it
   * can avoid calling this operator, meaning that the exception does not
   * happen at all.  If you need more deterministic exception behaviour, use
   * @ref at().
   */
  [[nodiscard]] PQXX_PURE field_ref operator[](size_type) const noexcept;

#if defined(PQXX_HAVE_MULTIDIM)
  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   *
   * @warning This function is marked as "pure."  This means that if it fails,
   * depending on your compiler, the exception may occur in a different place
   * than you expected.  The compiler may even find scenarios where it can
   * avoid calling this operator, meaning that the exception does not happen at
   * all.  If you need more deterministic exception behaviour, use @ref at().
   */
  [[nodiscard]] PQXX_PURE field_ref operator[](zview col_name, sl loc) const
  {
    // TODO: There's a proposal to permit a default value for loc.
    // Visual Studio 2022 says it has multi-dimensional indexing, but issues a
    // bogus warning that this comma is a sequence operator and the first value
    // is meaningless.  If that were true, well, row_ref has no operator[] that
    // takes just a source_location anyway so the call would have failed.
    // Clearly the warning is a lie.
#  if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4548)
#  endif
    return as_row_ref()[col_name, loc];
#  if defined(_MSC_VER)
#    pragma warning(pop)
#  endif
  }
#endif

  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   *
   * @warning This function is marked as "pure."  This means that if it fails,
   * depending on your compiler, the exception may occur in a different place
   * than you expected.  The compiler may even find scenarios where it can
   * avoid calling this operator, meaning that the exception does not happen at
   * all.  If you need more deterministic exception behaviour, use @ref at().
   */
  [[nodiscard]] PQXX_PURE field_ref operator[](zview col_name) const
  {
    return as_row_ref()[col_name];
  }

  /// Address a field by number, but check that the number is in range.
  field_ref at(size_type, sl = sl::current()) const;

  /** Address field by name.
   * @warning This is much slower than indexing by number, or iterating.
   */
  field_ref at(zview col_name, sl = sl::current()) const;

  [[nodiscard]] PQXX_PURE constexpr size_type size() const noexcept
  {
    return m_end;
  }

  /// Row number, assuming this is a real row and not end()/rend().
  [[nodiscard, deprecated("Use row_number().")]] constexpr result::size_type
  rownumber() const noexcept
  {
    return m_index;
  }

  /// Row number, assuming this is a real row and not end()/rend().
  [[nodiscard]] PQXX_PURE constexpr result::size_type
  row_number() const noexcept
  {
    return m_index;
  }

  /**
   * @name Column information
   */
  //@{
  /// Number of given column (throws exception if it doesn't exist).
  [[nodiscard]] size_type
  column_number(zview col_name, sl loc = sl::current()) const
  {
    return as_row_ref().column_number(col_name, loc);
  }

  /// Return a column's type.
  [[nodiscard]] oid
  column_type(size_type col_num, sl loc = sl::current()) const
  {
    return as_row_ref().column_type(col_num, loc);
  }

  /// Return a column's type.
  [[nodiscard]] oid column_type(zview col_name, sl loc = sl::current()) const
  {
    return column_type(column_number(col_name, loc), loc);
  }

  /// What table did this column come from?
  [[nodiscard]] oid
  column_table(size_type col_num, sl loc = sl::current()) const
  {
    return as_row_ref().column_table(col_num, loc);
  }

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
  [[nodiscard]] size_type
  table_column(size_type col, sl loc = sl::current()) const
  {
    return as_row_ref().table_column(col, loc);
  }

  /// What column number in its table did this result column come from?
  [[nodiscard]] size_type
  table_column(zview col_name, sl loc = sl::current()) const
  {
    return as_row_ref().table_column(col_name, loc);
  }
  //@}

  [[nodiscard]] PQXX_PURE constexpr result::size_type num() const noexcept
  {
    return row_number();
  }

  /// Extract entire row's values into a tuple.
  /** Converts to the types of the tuple's respective fields.
   *
   * The types in the tuple must have conversions from PostgreSQL's text format
   * defined; see @ref datatypes.
   *
   * @throw usage_error If the number of columns in the row does not match
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
    return as_row_ref().as<TYPE...>(loc);
  }

  /// Convert to a given tuple of values,
  /** Useful in cases where we have a full tuple of field types, but
   * not a parameter pack.
   *
   * `TUPLE` should be a tuple type, with the same number of elements as the
   * result has columns.
   *
   * @throw usage_error If the number of columns in the `row` does not match
   * the number of fields in `TUPLE`.
   */
  template<typename TUPLE>
  TUPLE as_tuple(sl loc = sl::current()) const
    requires(requires(TUPLE t) { std::get<0>(t); })
  {
    return as_row_ref().as_tuple<TUPLE>(loc);
  }

  [[deprecated("Swap iterators, not rows.")]] void swap(row &) noexcept;

private:
  /// Return @ref row_ref for this row.
  /** @warning The @ref row_ref holds a reference to the @ref result object
   * _inside this `row` object._  So if you change that, the @ref row_ref
   * becomes invalid.
   */
  row_ref as_row_ref() const noexcept { return {m_result, row_number()}; }

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

  /// The @ref pqxx::result of which this is a row.
  result const &home() const { return m_result; }

  /// Convert entire row to tuple fields, without checking row size.
  template<typename Tuple> void convert(Tuple &t, sl loc) const
  {
    auto ref{as_row_ref()};
    pqxx::internal::gate::row_ref_row{ref}.extract_fields(
      t, std::make_index_sequence<std::tuple_size_v<Tuple>>{},
      conversion_context{home().get_encoding_group(), loc});
  }

  // TODO: Define gate.
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
};


/// Iterator for fields in a row.  Use as row::const_iterator.
/** @warning Do not destroy or move the @ref result object while you're
 * iterating it.  It will invalidate all iterators on the entire result.  They
 * will no longer refer to valid memory, and your application may crash, or
 * worse, read the wrong data.
 */
class PQXX_LIBEXPORT const_row_iterator
{
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = field_ref const;
  using pointer = field_ref const *;
  using size_type = row_size_type;
  using difference_type = row_difference_type;
  using reference = field_ref;

  const_row_iterator() noexcept = default;
  const_row_iterator(row_ref const &t, row_size_type c) noexcept :
          m_field{t.home(), t.row_number(), c}
  {}
  const_row_iterator(field_ref const &f) noexcept : m_field{f} {}
  const_row_iterator(const_row_iterator const &) noexcept = default;
  const_row_iterator(const_row_iterator &&) noexcept = default;

  /// Current column number, if the iterator is pointing at a valid field.
  PQXX_PURE size_type col() const noexcept { return m_field.column_number(); }

  /**
   * @name Dereferencing operators
   */
  //@{
  [[nodiscard]] PQXX_PURE PQXX_RETURNS_NONNULL constexpr pointer
  operator->() const noexcept
  {
    return &m_field;
  }
  [[nodiscard]] PQXX_PURE reference operator*() const noexcept
  {
    return m_field;
  }
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
    pqxx::internal::gate::field_ref_const_row_iterator(m_field).offset(1);
    return *this;
  }
  const_row_iterator operator--(int) & noexcept;
  const_row_iterator &operator--() noexcept
  {
    pqxx::internal::gate::field_ref_const_row_iterator(m_field).offset(-1);
    return *this;
  }

  const_row_iterator &operator+=(difference_type n) noexcept
  {
    pqxx::internal::gate::field_ref_const_row_iterator(m_field).offset(n);
    return *this;
  }
  const_row_iterator &operator-=(difference_type n) noexcept
  {
    pqxx::internal::gate::field_ref_const_row_iterator(m_field).offset(-n);
    return *this;
  }
  //@}

  /**
   * @name Comparisons
   */
  //@{
  [[nodiscard]] bool operator==(const_row_iterator const &i) const noexcept
  {
    return col() == i.col();
  }
  [[nodiscard]] bool operator!=(const_row_iterator const &i) const noexcept
  {
    return col() != i.col();
  }
  [[nodiscard]] bool operator<(const_row_iterator const &i) const noexcept
  {
    return col() < i.col();
  }
  [[nodiscard]] bool operator<=(const_row_iterator const &i) const noexcept
  {
    return col() <= i.col();
  }
  [[nodiscard]] bool operator>(const_row_iterator const &i) const noexcept
  {
    return col() > i.col();
  }
  [[nodiscard]] bool operator>=(const_row_iterator const &i) const noexcept
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

  [[nodiscard]] inline field_ref
  operator[](difference_type offset) const noexcept
  {
    return *(*this + offset);
  }
  //@}

private:
  field_ref m_field;
};


/// Reverse iterator for a row.  Use as row::const_reverse_iterator.
class PQXX_LIBEXPORT const_reverse_row_iterator final
        : private const_row_iterator
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
  [[nodiscard]] inline field_ref
  operator[](difference_type offset) const noexcept
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

  [[nodiscard]] bool
  operator<(const_reverse_row_iterator const &rhs) const noexcept
  {
    return iterator_type::operator>(rhs);
  }
  [[nodiscard]] bool
  operator<=(const_reverse_row_iterator const &rhs) const noexcept
  {
    return iterator_type::operator>=(rhs);
  }
  [[nodiscard]] bool
  operator>(const_reverse_row_iterator const &rhs) const noexcept
  {
    return iterator_type::operator<(rhs);
  }
  [[nodiscard]] bool
  operator>=(const_reverse_row_iterator const &rhs) const noexcept
  {
    return iterator_type::operator<=(rhs);
  }
  //@}
};


const_row_iterator
const_row_iterator::operator+(difference_type o) const noexcept
{
  return {
    {m_field.home(), m_field.row_number()},
    static_cast<row_size_type>(
      static_cast<difference_type>(m_field.column_number()) + o)};
}

inline const_row_iterator operator+(
  const_row_iterator::difference_type o, const_row_iterator const &i) noexcept
{
  return i + o;
}

inline const_row_iterator
const_row_iterator::operator-(difference_type o) const noexcept
{
  return {
    {m_field.home(), m_field.row_number()},
    static_cast<row_size_type>(
      static_cast<difference_type>(m_field.column_number()) - o)};
}

inline const_row_iterator::difference_type
const_row_iterator::operator-(const_row_iterator const &i) const noexcept
{
  return difference_type(m_field.column_number() - i.m_field.column_number());
}


template<typename Tuple, std::size_t index>
inline void row_ref::extract_value(Tuple &t, ctx c) const
{
  using field_type = std::remove_cvref_t<decltype(std::get<index>(t))>;
  field_ref const f{*m_result, m_index, index};
  std::get<index>(t) = from_string<field_type>(f, c);
}


inline row_ref::const_iterator row_ref::cbegin() const noexcept
{
  return {{home(), row_number()}, 0};
}

inline row_ref::const_iterator row_ref::begin() const noexcept
{
  return cbegin();
}

inline row_ref::const_iterator row_ref::cend() const noexcept
{
  return {{home(), row_number()}, home().columns()};
}

inline row_ref::const_iterator row_ref::end() const noexcept
{
  return cend();
}

inline row_ref::const_reverse_iterator row_ref::crbegin() const noexcept
{
  return const_reverse_iterator{end()};
}

inline row_ref::const_reverse_iterator row_ref::rbegin() const noexcept
{
  return crbegin();
}

inline row_ref::const_reverse_iterator row_ref::crend() const noexcept
{
  return const_reverse_iterator{begin()};
}

inline row_ref::const_reverse_iterator row_ref::rend() const noexcept
{
  return crend();
}

inline field_ref row_ref::front() const noexcept
{
  return {home(), row_number(), 0};
}

inline field_ref row_ref::back() const noexcept
{
  return {home(), row_number(), home().columns() - 1};
}

inline row_size_type row_ref::column_number(zview col_name, sl loc) const
{
  return home().column_number(col_name, loc);
}

inline oid row_ref::column_table(row_size_type col_num, sl loc) const
{
  return home().column_table(col_num, loc);
}

inline row::const_iterator row::cbegin() const noexcept
{
  return as_row_ref().cbegin();
}

inline row::const_iterator row::begin() const noexcept
{
  return cbegin();
}

inline row::const_iterator row::cend() const noexcept
{
  return {{m_result, row_number()}, m_end};
}

inline row::const_iterator row::end() const noexcept
{
  return cend();
}

inline row::const_reverse_iterator row::crbegin() const noexcept
{
  return const_reverse_iterator{end()};
}

inline row::const_reverse_iterator row::rbegin() const noexcept
{
  return crbegin();
}

inline row::const_reverse_iterator row::crend() const noexcept
{
  return const_reverse_iterator{begin()};
}

inline row::const_reverse_iterator row::rend() const noexcept
{
  return crend();
}
} // namespace pqxx
#endif
