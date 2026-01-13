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
#ifndef PQXX_H_RESULT
#define PQXX_H_RESULT

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include <format>
#include <functional>
#include <ios>
#include <list>
#include <memory>
#include <stdexcept>

#include "pqxx/except.hxx"
#include "pqxx/types.hxx"
#include "pqxx/util.hxx"
#include "pqxx/zview.hxx"

#include "pqxx/internal/encodings.hxx"


namespace pqxx::internal
{
PQXX_LIBEXPORT void clear_result(pq::PGresult const *) noexcept;
} // namespace pqxx::internal


namespace pqxx::internal::gate
{
class result_connection;
class result_creation;
class result_field_ref;
class result_pipeline;
class result_row;
class result_sql_cursor;
} // namespace pqxx::internal::gate


namespace pqxx::internal
{
// 9.0: Remove this, just use the notice handler in connection/result.
/// Various callbacks waiting for a notice to come in.
struct notice_waiters final
{
  std::function<void(zview)> notice_handler;
  std::list<errorhandler *> errorhandlers;

  notice_waiters() = default;
  notice_waiters(notice_waiters const &) = delete;
  notice_waiters(notice_waiters &&) = delete;
  notice_waiters &operator=(notice_waiters const &) = delete;
  notice_waiters &operator=(notice_waiters &&) = delete;
};
} // namespace pqxx::internal


namespace pqxx
{
class row_ref;
class field_ref;


/// Result set containing data returned by a query or command.
/** This behaves as a container (as defined by the C++ standard library) and
 * provides random access const iterators to iterate over its rows.  You can
 * also access a row by indexing a `result R` by the row's zero-based
 * number:
 *
 * ```cxx
 *     for (result::size_type i=0; i < std::size(R); ++i) Process(R[i]);
 * ```
 *
 * Result sets in libpqxx are lightweight, reference-counted wrapper objects
 * which are relatively small and cheap to copy.  Think of a result object as
 * a "smart pointer" to an underlying result set.
 *
 * @warning The result set that a result object points to is not thread-safe.
 * If you copy a result object, it still refers to the same underlying result
 * set.  So never copy, destroy, query, or otherwise access a result while
 * another thread may be copying, destroying, querying, or otherwise accessing
 * the same result set--even if it is doing so through a different result
 * object!
 */
class PQXX_LIBEXPORT result final
{
public:
  using size_type = result_size_type;
  using difference_type = result_difference_type;
  using reference = row_ref;
  using const_iterator = const_result_iterator;
  using pointer = const_iterator;
  using iterator = const_iterator;
  using const_reverse_iterator = const_reverse_result_iterator;
  using reverse_iterator = const_reverse_iterator;

  result() noexcept = default;
  result(result const &rhs) noexcept = default;
  result(result &&rhs) noexcept = default;

  /// Assign one result to another.
  /** Copying results is cheap: it copies only smart pointers, but the actual
   * data stays in the same place.
   */
  result &operator=(result const &rhs) noexcept = default;

  /// Assign one result to another, invaliding the old one.
  result &operator=(result &&rhs) noexcept = default;

  /**
   * @name Comparisons
   *
   * @warning The meaning of these comparisons has changed in 8.0.  The _old_
   * comparisons went through the results' data (but not metadata!) and looked
   * for differences.  This was fairly arbitrary and a potential performance
   * trap.
   *
   * A `result` is essentially a reference-counted pointer to a data structure
   * that we received from the database.  When you copy a result (through
   * assignment or copy construction) you get a second reference to the same
   * underlying data structure.
   *
   * This can be important because efficient code will use all kinds of direct
   * references to that data: @ref row_ref, @ref field_ref, `std::string_vew`,
   * @ref zview, raw C-style string pointers.  Those all stay valid even when
   * you copy your `result` and destroy the original, because all you really
   * did was replace one smart pointer with another.  The actual data
   * structure underneath stays exactly where it was.
   *
   * (This may also help explain why we have @ref row and @ref field classes on
   * the one hand, and @ref row_ref, @ref field_ref, and iterators on the
   * other.  @ref row and @ref field contain their own copy of the `result`.
   * Those other classes carry just a _pointer_ to the `result`.)
   *
   * The meaning of the `result` comparison operators is: _Do these two
   * `result` objects refer to the same underlying data structure?_
   */
  //@{
  /// Compare two results for equality.
  [[nodiscard]] bool operator==(result const &rhs) const noexcept
  {
    return rhs.m_data == m_data;
  }
  /// Compare two results for inequality.
  [[nodiscard]] bool operator!=(result const &rhs) const noexcept
  {
    return not operator==(rhs);
  }
  //@}

  /**
   * @name Iteration
   *
   * A `result` acts like a container of rows.  Each row in turn acts like a
   * container of fields.
   */
  //@{
  /// Iterate rows, reading them directly into a tuple of "TYPE...".
  /** Converts the fields to values of the given respective types.
   *
   * Use this only with a ranged "for" loop.  The iteration produces
   * std::tuple<TYPE...> which you can "unpack" to a series of `auto`
   * variables.
   */
  template<typename... TYPE> auto iter() const;

  [[nodiscard]] PQXX_PURE const_reverse_iterator rbegin() const noexcept;
  [[nodiscard]] PQXX_PURE const_reverse_iterator crbegin() const noexcept;
  [[nodiscard]] PQXX_PURE const_reverse_iterator rend() const noexcept;
  [[nodiscard]] PQXX_PURE const_reverse_iterator crend() const noexcept;

  [[nodiscard]] PQXX_PURE const_iterator begin() const noexcept;
  [[nodiscard]] PQXX_PURE const_iterator cbegin() const noexcept;
  [[nodiscard]] PQXX_PURE inline const_iterator end() const noexcept;
  [[nodiscard]] PQXX_PURE inline const_iterator cend() const noexcept;

  [[nodiscard]] PQXX_PURE row_ref front() const noexcept;
  [[nodiscard]] PQXX_PURE row_ref back() const noexcept;

  [[nodiscard]] PQXX_PURE size_type size() const noexcept;
  [[nodiscard]] PQXX_PURE bool empty() const noexcept;
  [[nodiscard]] PQXX_PURE size_type capacity() const noexcept
  {
    return size();
  }
  //@}

  /// Exchange two `result` values in an exception-safe manner.
  /** If the swap fails, the two values will be exactly as they were before.
   *
   * The swap is not necessarily thread-safe.
   */
  void swap(result &) noexcept;

  /// Index a result by number to get to a row.
  /** This returns a @ref row object.  Generally you should not keep the row
   * around as a variable, but if you do, make sure that your variable is a
   * `row`, not a `row&`.
   *
   * If you are working in C++23 or better, the two-dimensional indexing
   * operator is likely to be more efficient.  Otherwise, consider @ref at().
   */
  [[nodiscard]] PQXX_PURE row_ref operator[](size_type i) const noexcept;

#if defined(PQXX_HAVE_MULTIDIM)
  /// Index a result by row number and column number to get to a field.
  [[nodiscard]] PQXX_PURE field_ref
  operator[](size_type row_num, row_size_type col_num) const noexcept;
#endif // PQXX_HAVE_MULTIDIM

  /// Index a row by number, but check that the row number is valid.
  row_ref at(size_type, sl = sl::current()) const;

  /// Index a field by row number and column number.
  field_ref at(size_type, row_size_type, sl = sl::current()) const;

  /// Let go of the result's data.
  /** Use this if you need to deallocate the result data earlier than you can
   * destroy the `result` object itself.
   *
   * Multiple `result` objects can refer to the same set of underlying data.
   * The underlying data will be deallocated once all `result` objects that
   * refer to it are cleared or destroyed.
   */
  void clear() noexcept
  {
    m_data.reset();
    m_query = nullptr;
  }

  /**
   * @name Column information
   */
  //@{
  /// Number of columns in result.
  [[nodiscard]] PQXX_PURE row_size_type columns() const noexcept;

  /// Number of given column (throws exception if it doesn't exist).
  [[nodiscard]] row_size_type
  column_number(zview name, sl = sl::current()) const;

  /// Name of column with this number (throws exception if it doesn't exist)
  [[nodiscard]] PQXX_RETURNS_NONNULL char const *
  column_name(row_size_type number, sl = sl::current()) const &;

  /// Server-side storage size for field of column's type, in bytes.
  /** Returns the size of the server's internal representation of the column's
   * data type.  A negative value indicates the data type is variable-length.
   */
  [[nodiscard]] int
  column_storage(row_size_type number, sl = sl::current()) const;

  /// Type modifier of the column with this number.
  /** The meaning of modifier values is type-specific; they typically indicate
   * precision or size limits.
   *
   * _Use this only if you know what you're doing._  Most applications do not
   * need it, and most types do not use modifiers.
   *
   * The value -1 indicates "no information available."
   *
   * @warning There is no check for errors, such as an invalid column number.
   */
  [[nodiscard]] int column_type_modifier(row_size_type number) const noexcept;

  /// Return column's type, as an OID from the system catalogue.
  [[nodiscard]] oid
  column_type(row_size_type col_num, sl = sl::current()) const;

  /// Return column's type, as an OID from the system catalogue.
  [[nodiscard]] oid column_type(zview col_name, sl loc = sl::current()) const
  {
    return column_type(column_number(col_name, loc));
  }

  /// What table did this column come from?
  [[nodiscard]] oid
  column_table(row_size_type col_num, sl = sl::current()) const;

  /// What table did this column come from?
  [[nodiscard]] oid column_table(zview col_name, sl loc = sl::current()) const
  {
    return column_table(column_number(col_name, loc), loc);
  }

  /// What column in its table did this column come from?
  [[nodiscard]] row_size_type
  table_column(row_size_type col_num, sl = sl::current()) const;

  /// What column in its table did this column come from?
  [[nodiscard]] row_size_type
  table_column(zview col_name, sl loc = sl::current()) const
  {
    return table_column(column_number(col_name), loc);
  }
  //@}

  /// Query that produced this result, if available (empty string otherwise)
  [[nodiscard]] PQXX_PURE std::string const &query() const & noexcept;

  /// If command was an `INSERT` of 1 row, return oid of the inserted row.
  /** @return Identifier of inserted row if exactly one row was inserted, or
   * @ref oid_none otherwise.
   */
  [[nodiscard]] PQXX_PURE oid inserted_oid(sl = sl::current()) const;

  /// Number of rows affected by the SQL command whose result this is.
  /** Returns the number of rows affected _if_ the command was a `SELECT`,
   * `CREATE TABLE AS`, `INSERT`, `UPDATE`, `DELETE`, `MERGE`, `MOVE`, `FETCH`,
   * or `COPY`; or an `EXECUTE` or a parameterised or prepared statement that
   * did an `INSERT`, `UPDATE`, `DELETE`, or `MERGE`.
   *
   * Otherwise, returns zero.
   */
  [[nodiscard]] PQXX_PURE size_type affected_rows() const;

  /// Run `func` on each row, passing the row's fields as parameters.
  /** Goes through the rows from first to last.  You provide a callable `func`.
   *
   * For each row in the `result`, `for_each` will call `func`.  It converts
   * the row's fields to the types of `func`'s parameters, and pass them to
   * `func`.
   *
   * (Therefore `func` must have a _single_ signature.  It can't be a generic
   * lambda, or an object of a class with multiple overloaded function call
   * operators.  Otherwise, `for_each` will have no way to detect a parameter
   * list without ambiguity.)
   *
   * If any of your parameter types is `std::string_view`, it refers to the
   * underlying storage of this `result`.
   *
   * If any of your parameter types is a reference type, its argument will
   * refer to a temporary value which only lives for the duration of that
   * single invocation to `func`.  If the reference is an lvalue reference, it
   * must be `const`.
   *
   * For example, this queries employee names and salaries from the database
   * and prints how much each would like to earn instead:
   * ```cxx
   *   tx.exec("SELECT name, salary FROM employee").for_each(
   *       [](std::string_view name, float salary){
   *           std::cout << name << " would like " << salary * 2 << ".\n";
   *       })
   * ```
   *
   * If `func` throws an exception, processing stops at that point and
   * propagates the exception.
   *
   * @throws pqxx::usage_error if `func`'s number of parameters does not match
   * the number of columns in this result.
   *
   * The parameter types must have conversions from PostgreSQL's string format
   * defined; see @ref datatypes.
   */
  template<typename CALLABLE>
  inline void for_each(CALLABLE &&func, sl = sl::current()) const;

  /// Check that result contains exactly `n` rows.
  /** @return The result itself, for convenience.
   * @throw @ref unexpected_rows if the actual count is not equal to `n`.
   */
  result const &expect_rows(size_type n, sl loc = sl::current()) const
  {
    auto const sz{size()};
    if (sz != n)
    {
      // TODO: See whether result contains a generated statement.
      if (not m_query or m_query->empty())
        throw unexpected_rows{
          std::format("Expected {} row(s) from query, got {}.", n, sz), loc};
      else
        throw unexpected_rows{
          std::format(
            "Expected {} row(s) from query '{}', got {}.", n, *m_query, sz),
          loc};
    }
    return *this;
  }

  /// Check that result contains exactly 1 row, and return that row.
  /** A @ref row is less efficient than a @ref row_ref, but will ensure that
   * the underlying result data stays valid for as long as the @ref row object
   * exists.
   *
   * @throw @ref unexpected_rows if the actual count is not equal to `n`.
   */
  row one_row(sl = sl::current()) const;

  /// Check that result contains exactly 1 row, and return a reference to it.
  /** You must ensure that the @ref result object stays valid, and does not
   * move, whenever you access the row.
   *
   * @throw @ref unexpected_rows if the actual count is not equal to `n`.
   */
  row_ref one_row_ref(sl = sl::current()) const;

  /// Expect that result contains at most one row, and return as optional.
  /** Returns an empty `std::optional` if the result is empty, or if it has
   * exactly one row, a `std::optional` containing the row.
   *
   * @throw @ref unexpected_rows is the row count is not 0 or 1.
   */
  std::optional<row> opt_row(sl = sl::current()) const;

  /// Expect that result contains at most one row, and return as optional.
  /** Returns an empty `std::optional` if the result is empty, or if it has
   * exactly one row, a `std::optional` containing the row.
   *
   * @throw @ref unexpected_rows is the row count is not 0 or 1.
   */
  std::optional<row_ref> opt_row_ref(sl = sl::current()) const;

  /// Expect that result contains no rows.  Return result for convenience.
  result const &no_rows(sl loc = sl::current()) const
  {
    expect_rows(0, loc);
    return *this;
  }

  /// Expect that result consists of exactly `cols` columns.
  /** @return The result itself, for convenience.
   * @throw @ref usage_error otherwise.
   */
  result const &
  expect_columns(row_size_type cols, sl loc = sl::current()) const
  {
    auto const actual{columns()};
    if (actual != cols)
    {
      // TODO: See whether result contains a generated statement.
      if (not m_query or m_query->empty())
        throw usage_error{
          std::format(
            "Expected {} column(s) from query, got {}.", cols, actual),
          loc};
      else
        throw usage_error{
          std::format(
            "Expected {} column(s) from query '{}', got {}.", cols, *m_query,
            actual),
          loc};
    }
    return *this;
  }

  /// Expect that result consists of exactly 1 row and 1 column; return it.
  /** A @ref field is less efficient than a @ref field_ref, but will ensure
   * that the underlying result data stays valid for as long as the @ref field
   * object exists.
   *
   * @throw @ref usage_error otherwise.
   */
  field one_field(sl = sl::current()) const;

  /// Expect that result consists of exactly 1 row and 1 column; return it.
  /** You must ensure that the @ref result object stays valid, and does not
   * move, whenever you access the row.
   *
   * @throw @ref usage_error otherwise.
   */
  field_ref one_field_ref(sl = sl::current()) const;

  /// Retrieve encoding group for this result's client encoding.
  encoding_group get_encoding_group() const noexcept { return m_encoding; }

private:
  using data_pointer = std::shared_ptr<internal::pq::PGresult const>;

  /// Check that there's exactly row of data, or throw @ref unexpected_rows.
  void check_one_row(sl loc) const
  {
    auto const sz{size()};
    if (sz != 1)
    {
      if (not m_query or m_query->empty())
        throw unexpected_rows{
          std::format("Expected 1 row from query, got {}.", sz), loc};
      else
        throw unexpected_rows{
          std::format("Expected 1 row from query '{}', got {}.", *m_query, sz),
          loc};
    }
  }

  /// Underlying libpq result set.
  data_pointer m_data;

  friend class pqxx::internal::gate::result_pipeline;
  PQXX_PURE std::shared_ptr<std::string const> query_ptr() const noexcept
  {
    return m_query;
  }

  // TODO: Could we colocate some members in a single struct?
  /// Query string.
  std::shared_ptr<std::string const> m_query;

  /// The connection's notice handler.
  /** We're not actually using this, but we need a copy here so that the
   * actual function does not get deallocated if the connection is destroyed
   * while this result still exists.
   */
  std::shared_ptr<pqxx::internal::notice_waiters> m_notice_waiters;

  encoding_group m_encoding = encoding_group::unknown;

  static std::string const s_empty_string;

  friend class pqxx::internal::gate::result_field_ref;
  PQXX_PURE PQXX_RETURNS_NONNULL char const *
  get_value(size_type row, row_size_type col) const noexcept;
  PQXX_PURE bool get_is_null(size_type row, row_size_type col) const noexcept;
  PQXX_PURE
  field_size_type get_length(size_type, row_size_type) const noexcept;

  friend class pqxx::internal::gate::result_creation;
  result(
    std::shared_ptr<internal::pq::PGresult> const &rhs,
    std::shared_ptr<std::string> const &query,
    std::shared_ptr<pqxx::internal::notice_waiters> const &waiters,
    encoding_group enc);

  PQXX_PRIVATE void check_status(std::string_view desc, sl loc) const;

  friend class pqxx::internal::gate::result_connection;
  friend class pqxx::internal::gate::result_row;
  bool operator!() const noexcept { return m_data.get() == nullptr; }
  operator bool() const noexcept { return m_data.get() != nullptr; }

  [[noreturn]] PQXX_COLD PQXX_PRIVATE void
  throw_sql_error(std::string const &Err, std::string const &Query, sl) const;
  PQXX_PURE PQXX_PRIVATE int errorposition() const;
  PQXX_PRIVATE std::string status_error(sl) const;

  friend class pqxx::internal::gate::result_sql_cursor;
  PQXX_PURE char const *cmd_status() const noexcept;
};
} // namespace pqxx
#endif
