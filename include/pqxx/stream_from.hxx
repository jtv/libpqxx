/* Definition of the pqxx::stream_from class.
 *
 * pqxx::stream_from enables optimized batch reads from a database table.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/stream_from instead.
 *
 * Copyright (c) 2000-2021, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_STREAM_FROM
#define PQXX_H_STREAM_FROM

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include <cassert>
#include <functional>
#include <variant>

#include "pqxx/except.hxx"
#include "pqxx/internal/concat.hxx"
#include "pqxx/internal/encoding_group.hxx"
#include "pqxx/internal/stream_iterator.hxx"
#include "pqxx/internal/transaction_focus.hxx"
#include "pqxx/separated_list.hxx"
#include "pqxx/transaction_base.hxx"


namespace pqxx
{
/// Pass this to a @c stream_from constructor to stream table contents.
constexpr from_table_t from_table;
/// Pass this to a @c stream_from constructor to stream query results.
constexpr from_query_t from_query;


/// Stream data from the database.
/** Retrieving data this way is likely to be faster than executing a query and
 * then iterating and converting the rows fields.  You will also be able to
 * start processing before all of the data has come in.
 *
 * There are also downsides.  If there's an error, it may leave the entire
 * connection in an unusable state, so you'll have to give the whole thing up.
 * Also, your connection to the database may break before you've received all
 * the data, so you may end up processing only part of the data.  Finally,
 * opening a stream puts the connection in a special state, so you won't be
 * able to do many other things with the connection or the transaction while
 * the stream is open.
 *
 * There are two ways of starting a stream: you stream either all rows in a
 * table (in which case, use a constructor which accepts @c pqxx::from_table),
 * or the results of a query (in which case, use a concstructor which accepts
 * @c pqxx::from_query).
 *
 * Usually you'll want the @c stream convenience wrapper in transaction_base,
 * so you don't need to deal with this class directly.
 *
 * @warning While a stream is active, you cannot execute queries, open a
 * pipeline, etc. on the same transaction.  A transaction can have at most one
 * object of a type derived from @c pqxx::internal::transactionfocus active on
 * it at a time.

 */
class PQXX_LIBEXPORT stream_from : internal::transactionfocus
{
public:
  using raw_line =
    std::pair<std::unique_ptr<char, std::function<void(char *)>>, std::size_t>;

  /**
   * @name Streaming queries
   *
   * You can use @c stream_from to execute a query, and stream its results.
   *
   * The query can be a SELECT query or a VALUES query; or it can be an
   * UPDATE, INSERT, or DELETE with a RETURNING clause.
   *
   * The query is executed as part of a COPY statement, so there are additional
   * restrictions on what kind of query you can use here.  See the PostgreSQL
   * documentation for the COPY command:
   *
   *     https://www.postgresql.org/docs/current/sql-copy.html
   */
  //@{
  /// Factory: Execute query, and stream the results.
  static stream_from query(transaction_base &tx, std::string_view q)
  {
    return stream_from{tx, from_query, q};
  }

  /// Execute query, and stream over the results.
  /** This is the awkward way to construct a @c stream_from.  It uses a marker
   * argument type to disambiguate overloads.
   *
   * Where possible, use factory function @c query() instead.
   */
  stream_from(transaction_base &, from_query_t, std::string_view query);
  //@}

  /**
   * @name Streaming tables
   *
   * You can use @c stream_from to read a table's contents.
   *
   * Streaming does not work from a view, and the table name cannot include a
   * schema name, and there are no guarantees about ordering.  If you need any
   * of those things, consider streaming from a query instead.
   */
  //@{
  // TODO: Replace constructors with factories.  Use Concepts.

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
  PQXX_DEPRECATED("Use the from_table_t overload instead.") stream_from(
    transaction_base &tx, std::string_view table) :
          stream_from{tx, from_table, table}
  {}

  /// @deprecated Use the @c from_table_t overload instead.
  template<typename Columns>
  PQXX_DEPRECATED("Use the from_table_t overload instead.") stream_from(
    transaction_base &tx, std::string_view table, Columns const &columns) :
          stream_from{tx, from_table, table, columns}
  {}

  /// @deprecated Use the @c from_table_t overload instead.
  template<typename Iter>
  PQXX_DEPRECATED("Use the from_table_t overload instead.") stream_from(
    transaction_base &, std::string_view table, Iter columns_begin,
    Iter columns_end);
  //@}

  ~stream_from() noexcept;

  /// May this stream still produce more data?
  [[nodiscard]] operator bool() const noexcept { return not m_finished; }
  /// Has this stream produced all the data it is going to produce?
  [[nodiscard]] bool operator!() const noexcept { return m_finished; }

  /// Finish this stream.  Call this before continuing to use the connection.
  /** Consumes all remaining lines, and closes the stream.
   *
   * This may take a while if you're abandoning the stream before it's done, so
   * skip it in error scenarios where you're not planning to use the connection
   * again afterwards.
   */
  void complete();

  /// Read one row into a tuple.
  /** Converts the row's fields into the fields making up the tuple.
   *
   * For a column which can contain nulls, be sure to give the corresponding
   * tuple field a type which can be null.  For example, to read a field as
   * @c int when it may contain nulls, read it as @c std::optional<int>.
   * Using @c std::shared_ptr or @c std::unique_ptr will also work.
   */
  template<typename Tuple> stream_from &operator>>(Tuple &);

  /// Doing this with a @c std::variant is going to be horrifically borked.
  template<typename... Vs>
  stream_from &operator>>(std::variant<Vs...> &) = delete;

  // TODO: Hide this from the public.
  /// Iterate over this stream.  Supports range-based "for" loops.
  /** Produces an input iterator over the stream.
   *
   * Do not call this yourself.  Use it like "for (auto data : stream.iter())".
   */
  template<typename... TYPE> [[nodiscard]] auto iter()
  {
    return pqxx::internal::stream_input_iteration<TYPE...>{*this};
  }

  /// Read a row.  Return fields as views, valid until you read the next row.
  /** Returns @c nullptr when there are no more rows to read.  Do not attempt
   * to read any further rows after that.
   *
   * Do not access the vector, or the storage referenced by the views, after
   * closing or completing the stream, or after attempting to read a next row.
   *
   * A @c pqxx::zview is like a @c std::string_view, but with the added
   * guarantee that if its data pointer is non-null, the string is followed by
   * a terminating zero (which falls just outside the view itself).
   *
   * If any of the views' data pointer is null, that means that the
   * corresponding SQL field is null.
   *
   * @warning The return type may change in the future, to support C++20
   * coroutine-based usage.
   */
  std::vector<zview> const *read_row();

  /// Read a raw line of text from the COPY command.
  /** @warning Do not use this unless you really know what you're doing. */
  raw_line get_raw_line();

private:
  stream_from(
    transaction_base &tx, std::string_view table, std::string &&columns,
    from_table_t);

  template<typename Tuple, std::size_t... indexes>
  void extract_fields(Tuple &t, std::index_sequence<indexes...>) const
  {
    (extract_value<Tuple, indexes>(t), ...);
  }

  pqxx::internal::glyph_scanner_func *m_glyph_scanner;

  /// Current row's fields' text, combined into one reusable string.
  std::string m_row;

  /// The current row's fields.
  std::vector<zview> m_fields;

  bool m_finished = false;

  void close();

  template<typename Tuple, std::size_t index>
  void extract_value(Tuple &) const;

  /// Read a line of COPY data, write @c m_row and @c m_fields.
  void parse_line();
};


template<typename Columns>
inline stream_from::stream_from(
  transaction_base &tb, from_table_t, std::string_view table_name,
  Columns const &columns) :
        stream_from{
          tb, from_table, table_name, std::begin(columns), std::end(columns)}
{}


template<typename Iter>
inline stream_from::stream_from(
  transaction_base &tx, from_table_t, std::string_view table,
  Iter columns_begin, Iter columns_end) :
        stream_from{
          tx, table, separated_list(",", columns_begin, columns_end),
          from_table}
{}


template<typename Tuple> inline stream_from &stream_from::operator>>(Tuple &t)
{
  if (m_finished)
    return *this;
  constexpr auto tup_size{std::tuple_size_v<Tuple>};
  m_fields.reserve(tup_size);
  parse_line();
  if (m_finished)
    return *this;

  if (std::size(m_fields) != tup_size)
    throw usage_error{internal::concat(
      "Tried to extract ", tup_size, " field(s) from a stream of ",
      std::size(m_fields), ".")};

  extract_fields(t, std::make_index_sequence<tup_size>{});
  return *this;
}


template<typename Tuple, std::size_t index>
inline void stream_from::extract_value(Tuple &t) const
{
  using field_type = strip_t<decltype(std::get<index>(t))>;
  using nullity = nullness<field_type>;
  assert(index < std::size(m_fields));
  if constexpr (nullity::always_null)
  {
    if (m_fields[index].data() != nullptr)
      throw conversion_error{"Streaming non-null value into null field."};
  }
  else if (m_fields[index].data() == nullptr)
  {
    if constexpr (nullity::has_null)
      std::get<index>(t) = nullity::null();
    else
      internal::throw_null_conversion(type_name<field_type>);
  }
  else
  {
    // Don't ever try to convert a non-null value to nullptr_t!
    std::get<index>(t) = from_string<field_type>(m_fields[index]);
  }
}
} // namespace pqxx

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
