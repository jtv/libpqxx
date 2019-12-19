/* Definitions for the pqxx::result class and support classes.
 *
 * pqxx::result represents the set of result rows from a database query.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/result instead.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_RESULT
#define PQXX_H_RESULT

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include <ios>
#include <memory>
#include <stdexcept>

#include "pqxx/except.hxx"
#include "pqxx/types.hxx"
#include "pqxx/util.hxx"

#include "pqxx/internal/encodings.hxx"


namespace pqxx::internal
{
PQXX_LIBEXPORT void clear_result(const pq::PGresult *);
}


namespace pqxx::internal::gate
{
class result_connection;
class result_creation;
class result_row;
class result_sql_cursor;
} // namespace internal::gate


namespace pqxx
{
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
class result
{
public:
  using size_type = result_size_type;
  using difference_type = result_difference_type;
  using reference = row;
  using const_iterator = const_result_iterator;
  using pointer = const_iterator;
  using iterator = const_iterator;
  using const_reverse_iterator = const_reverse_result_iterator;
  using reverse_iterator = const_reverse_iterator;

  result() noexcept :
      m_data(make_data_pointer()),
      m_query(),
      m_encoding(internal::encoding_group::MONOBYTE)
    {}
  result(const result &rhs) noexcept =default;

  PQXX_LIBEXPORT result &operator=(const result &rhs) noexcept =default;

  /**
   * @name Comparisons
   */
  //@{
  PQXX_LIBEXPORT bool operator==(const result &) const noexcept;
  bool operator!=(const result &rhs) const noexcept
	{ return not operator==(rhs); }
  //@}

  PQXX_LIBEXPORT const_reverse_iterator rbegin() const;
  PQXX_LIBEXPORT const_reverse_iterator crbegin() const;
  PQXX_LIBEXPORT const_reverse_iterator rend() const;
  PQXX_LIBEXPORT const_reverse_iterator crend() const;

  PQXX_LIBEXPORT const_iterator begin() const noexcept;
  PQXX_LIBEXPORT const_iterator cbegin() const noexcept;
  inline const_iterator end() const noexcept;
  inline const_iterator cend() const noexcept;

  PQXX_LIBEXPORT reference front() const noexcept;
  PQXX_LIBEXPORT reference back() const noexcept;

  PQXX_LIBEXPORT PQXX_PURE size_type size() const noexcept;
  PQXX_LIBEXPORT PQXX_PURE bool empty() const noexcept;
  size_type capacity() const noexcept { return size(); }

  PQXX_LIBEXPORT void swap(result &) noexcept;

  PQXX_LIBEXPORT row operator[](size_type i) const noexcept;
  PQXX_LIBEXPORT row at(size_type) const;

  void clear() noexcept { m_data.reset(); m_query = nullptr; }

  /**
   * @name Column information
   */
  //@{
  /// Number of columns in result.
  PQXX_LIBEXPORT PQXX_PURE row_size_type columns() const noexcept;

  /// Number of given column (throws exception if it doesn't exist).
  PQXX_LIBEXPORT row_size_type column_number(const char ColName[]) const;

  /// Number of given column (throws exception if it doesn't exist).
  row_size_type column_number(const std::string &Name) const
	{return column_number(Name.c_str());}

  /// Name of column with this number (throws exception if it doesn't exist)
  PQXX_LIBEXPORT const char *column_name(row_size_type Number) const;

  /// Return column's type, as an OID from the system catalogue.
  PQXX_LIBEXPORT oid column_type(row_size_type ColNum) const;

  /// Return column's type, as an OID from the system catalogue.
  template<typename STRING>
  oid column_type(STRING ColName) const
	{ return column_type(column_number(ColName)); }

  /// What table did this column come from?
  PQXX_LIBEXPORT oid column_table(row_size_type ColNum) const;

  /// What table did this column come from?
  template<typename STRING>
  oid column_table(STRING ColName) const
	{ return column_table(column_number(ColName)); }

  /// What column in its table did this column come from?
  PQXX_LIBEXPORT row_size_type table_column(row_size_type ColNum) const;

  /// What column in its table did this column come from?
  template<typename STRING>
  row_size_type table_column(STRING ColName) const
	{ return table_column(column_number(ColName)); }
  //@}

  /// Query that produced this result, if available (empty string otherwise)
  PQXX_LIBEXPORT PQXX_PURE const std::string &query() const noexcept;

  /// If command was @c INSERT of 1 row, return oid of inserted row
  /** @return Identifier of inserted row if exactly one row was inserted, or
   * oid_none otherwise.
   */
  PQXX_LIBEXPORT PQXX_PURE oid inserted_oid() const;

  /// If command was @c INSERT, @c UPDATE, or @c DELETE: number of affected rows
  /** @return Number of affected rows if last command was @c INSERT, @c UPDATE,
   * or @c DELETE; zero for all other commands.
   */
  PQXX_LIBEXPORT PQXX_PURE size_type affected_rows() const;


private:
  using data_pointer = std::shared_ptr<const internal::pq::PGresult>;

  /// Underlying libpq result set.
   data_pointer m_data;

  /// Factory for data_pointer.
  static data_pointer make_data_pointer(
	const internal::pq::PGresult *res=nullptr)
	{ return data_pointer{res, internal::clear_result}; }

  /// Query string.
  std::shared_ptr<std::string> m_query;

  internal::encoding_group m_encoding;

  static const std::string s_empty_string;

  friend class pqxx::field;
  PQXX_LIBEXPORT PQXX_PURE const char *
  GetValue(size_type Row, row_size_type Col) const;
  PQXX_LIBEXPORT PQXX_PURE bool
  get_is_null(size_type Row, row_size_type Col) const;
  PQXX_LIBEXPORT PQXX_PURE field_size_type get_length(
	size_type,
	row_size_type) const noexcept;

  friend class pqxx::internal::gate::result_creation;
  PQXX_LIBEXPORT result(
        internal::pq::PGresult *rhs,
        const std::string &Query,
        internal::encoding_group enc);

  PQXX_PRIVATE void check_status() const;

  friend class pqxx::internal::gate::result_connection;
  friend class pqxx::internal::gate::result_row;
  bool operator!() const noexcept { return not m_data.get(); }
  operator bool() const noexcept { return m_data.get() != nullptr; }

  [[noreturn]] PQXX_PRIVATE void ThrowSQLError(
	const std::string &Err,
	const std::string &Query) const;
  PQXX_PRIVATE PQXX_PURE int errorposition() const;
  PQXX_PRIVATE std::string StatusError() const;

  friend class pqxx::internal::gate::result_sql_cursor;
  PQXX_LIBEXPORT PQXX_PURE const char *cmd_status() const noexcept;
};
} // namespace pqxx

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
