/* Definition of the pqxx::stream_from class.
 *
 * pqxx::stream_from enables optimized batch reads from a database table.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/stream_from instead.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_STREAM_FROM
#define PQXX_H_STREAM_FROM

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include <variant>

#include "pqxx/except.hxx"
#include "pqxx/internal/encoding_group.hxx"
#include "pqxx/internal/stream_iterator.hxx"
#include "pqxx/internal/transaction_focus.hxx"
#include "pqxx/separated_list.hxx"


namespace pqxx
{
/// Marker: "stream from table."
struct from_table_t
{
} const from_table;

/// Marker: "stream from query."
struct from_query_t
{
} const from_query;

/// Efficiently pull data directly out of a table.
/** Retrieving data this way has a bit more startup overhead than executing a
 * regular query does, but given enough rows of data, it's likely to be faster.
 *
 * Opening a stream does put the connection in a special state, so you won't be
 * able to do many other things with the connection or the transaction while
 * the stream is open.
 *
 * Usually you'll want the @c stream convenience wrapper in transaction_base,
 * so you don't need to deal with this class directly.
 */
class PQXX_LIBEXPORT stream_from : internal::transactionfocus
{
public:
  /// Execute query, and stream over the results.
  /** The query can be a SELECT query or a VALUES query; or it can be an
   * UPDATE, INSERT, or DELETE with a RETURNING clause.
   *
   * The query is executed as part of a COPY statement, so there are additional
   * restrictions on what kind of query you can use here.  See the PostgreSQL
   * documentation for the COPY command:
   *
   *     https://www.postgresql.org/docs/current/sql-copy.html
   */
  stream_from(transaction_base &, from_query_t, std::string_view query);

  /// Stream all rows in table, all columns.
  stream_from(transaction_base &, from_table_t, std::string_view table);

  /// Stream given columns from all rows in table.
  template<typename Iter>
  stream_from(
    transaction_base &, from_table_t, std::string_view table,
    Iter columns_begin, Iter columns_end);

  /// Stream given columns from all rows in table.
  template<typename Columns>
  stream_from(
    transaction_base &tx, from_table_t, std::string_view table,
    Columns const &columns);

  /// @deprecated Use the @c from_table_t overload instead.
  stream_from(transaction_base &tx, std::string_view table) :
          stream_from{tx, from_table, table}
  {}

  /// @deprecated Use the @c from_table_t overload instead.
  template<typename Columns>
  stream_from(
    transaction_base &tx, std::string_view table, Columns const &columns) :
          stream_from{tx, from_table, table, columns}
  {}

  /// @deprecated Use the @c from_table_t overload instead.
  template<typename Iter>
  stream_from(
    transaction_base &, std::string_view table, Iter columns_begin,
    Iter columns_end);

  ~stream_from() noexcept;

  [[nodiscard]] operator bool() const noexcept { return not m_finished; }
  [[nodiscard]] bool operator!() const noexcept { return m_finished; }

  /// Finish this stream.  Call this before continuing to use the connection.
  /** Consumes all remaining lines, and closes the stream.
   *
   * This may take a while if you're abandoning the stream before it's done, so
   * skip it in error scenarios where you're not planning to use the connection
   * again afterwards.
   */
  void complete();

  bool get_raw_line(std::string &);
  template<typename Tuple> stream_from &operator>>(Tuple &);

  /// Doing this with a @c std::variant is going to be horrifically borked.
  template<typename... Vs>
  stream_from &operator>>(std::variant<Vs...> &) = delete;

  /// Iterate over this stream.  Supports range-based "for" loops.
  /** Produces an input iterator over the stream.
   *
   * Do not call this yourself.  Use it like "for (auto data : stream.iter())".
   */
  template<typename... TYPE>[[nodiscard]] auto iter()
  {
    return pqxx::internal::stream_input_iteration<TYPE...>{*this};
  }

private:
  stream_from(
    transaction_base &tx, std::string_view table, std::string &&columns,
    from_table_t);

  internal::encoding_group m_copy_encoding =
    internal::encoding_group::MONOBYTE;
  std::string m_current_line;
  bool m_finished = false;
  bool m_retry_line = false;

  void close();

  bool extract_field(
    std::string const &, std::string::size_type &, std::string &) const;

  template<typename T>
  void extract_value(
    std::string const &line, T &t, std::string::size_type &here,
    std::string &workspace) const;

  template<typename Tuple, std::size_t... I>
  void do_extract(
    const std::string &line, Tuple &t, std::string &workspace,
    std::index_sequence<I...>)
  {
    std::string::size_type here{0};
    (extract_value(line, std::get<I>(t), here, workspace), ...);
    if (
      here < line.size() and
      not(here == line.size() - 1 and line[here] == '\n'))
      throw usage_error{"Not all fields extracted from stream_from line"};
  }
};


template<typename Columns>
inline stream_from::stream_from(
  transaction_base &tb, from_table_t, std::string_view table_name,
  Columns const &columns) :
        stream_from{tb, from_table, table_name, std::begin(columns),
                    std::end(columns)}
{}


template<typename Iter>
inline stream_from::stream_from(
  transaction_base &tx, from_table_t, std::string_view table,
  Iter columns_begin, Iter columns_end) :
        stream_from{tx, table, separated_list(",", columns_begin, columns_end),
                    from_table}
{}


template<typename Tuple> inline stream_from &stream_from::operator>>(Tuple &t)
{
  if (m_retry_line or get_raw_line(m_current_line))
  {
    // This is just a scratchpad for functions further down to play with.
    // We allocate it here so that we can keep re-using its buffer, rather
    // than always allocating new ones.
    std::string workspace;
    try
    {
      constexpr auto tsize = std::tuple_size_v<Tuple>;
      using indexes = std::make_index_sequence<tsize>;
      do_extract(m_current_line, t, workspace, indexes{});
      m_retry_line = false;
    }
    catch (...)
    {
      m_retry_line = true;
      throw;
    }
  }
  return *this;
}


template<typename T>
inline void stream_from::extract_value(
  std::string const &line, T &t, std::string::size_type &here,
  std::string &workspace) const
{
  if (extract_field(line, here, workspace))
    t = from_string<T>(workspace);
  else if constexpr (nullness<T>::has_null)
    t = nullness<T>::null();
  else
    internal::throw_null_conversion(type_name<T>);
}

template<>
void PQXX_LIBEXPORT stream_from::extract_value<std::nullptr_t>(
  std::string const &line, std::nullptr_t &, std::string::size_type &here,
  std::string &workspace) const;
} // namespace pqxx

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
