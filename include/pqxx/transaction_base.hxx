/* Common code and definitions for the transaction classes.
 *
 * pqxx::transaction_base defines the interface for any abstract class that
 * represents a database transaction.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/transaction_base instead.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_TRANSACTION_BASE
#define PQXX_H_TRANSACTION_BASE

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include <string_view>

/* End-user programs need not include this file, unless they define their own
 * transaction classes.  This is not something the typical program should want
 * to do.
 *
 * However, reading this file is worthwhile because it defines the public
 * interface for the available transaction classes such as transaction and
 * nontransaction.
 */

#include "pqxx/connection.hxx"
#include "pqxx/internal/encoding_group.hxx"
#include "pqxx/isolation.hxx"
#include "pqxx/result.hxx"
#include "pqxx/row.hxx"
#include "pqxx/stream_from.hxx"

namespace pqxx::internal::gate
{
class transaction_subtransaction;
class transaction_sql_cursor;
class transaction_stream_to;
class transaction_transactionfocus;
} // namespace pqxx::internal::gate


namespace pqxx
{
/**
 * @defgroup transaction Transaction classes
 *
 * All database access goes through instances of these classes.
 * However, not all implementations of this interface need to provide
 * full transactional integrity.
 *
 * Several implementations of this interface are shipped with libpqxx,
 * including the plain transaction class, the entirely unprotected
 * nontransaction, and the more cautious robusttransaction.
 */

/// Interface definition (and common code) for "transaction" classes.
/**
 * @ingroup transaction
 *
 * Abstract base class for all transaction types.
 */
class PQXX_LIBEXPORT PQXX_NOVTABLE transaction_base
        : public virtual internal::namedclass
{
public:
  transaction_base() = delete;
  transaction_base(transaction_base const &) = delete;
  transaction_base &operator=(transaction_base const &) = delete;

  virtual ~transaction_base() = 0;

  /// Commit the transaction
  /** Unless this function is called explicitly, the transaction will not be
   * committed (actually the nontransaction implementation breaks this rule,
   * hence the name).
   *
   * Once this function returns, the whole transaction will typically be
   * irrevocably completed in the database.  There is also, however, a minute
   * risk that the connection to the database may be lost at just the wrong
   * moment.  In that case, libpqxx may be unable to determine whether the
   * transaction was completed or aborted and an in_doubt_error will be thrown
   * to make this fact known to the caller.  The robusttransaction
   * implementation takes some special precautions to reduce this risk.
   */
  void commit();

  /// Abort the transaction
  /** No special effort is required to call this function; it will be called
   * implicitly when the transaction is destructed.
   */
  void abort();

  /**
   * @ingroup escaping-functions
   */
  //@{
  /// Escape string for use as SQL string literal in this transaction.
  [[nodiscard]] std::string esc(char const text[]) const
  {
    return conn().esc(text);
  }
  /// Escape string for use as SQL string literal in this transaction.
  [[nodiscard]] std::string esc(char const text[], std::size_t maxlen) const
  {
    return conn().esc(text, maxlen);
  }
  /// Escape string for use as SQL string literal in this transaction.
  [[nodiscard]] std::string esc(std::string const &text) const
  {
    return conn().esc(text);
  }
  /// Escape string for use as SQL string literal in this transaction.
  [[nodiscard]] std::string esc(std::string_view text) const
  {
    return conn().esc(text);
  }

  /// Escape binary data for use as SQL string literal in this transaction
  /** Raw, binary data is treated differently from regular strings.  Binary
   * strings are never interpreted as text, so they may safely include byte
   * values or byte sequences that don't happen to represent valid characters
   * in the character encoding being used.
   *
   * The binary string does not stop at the first zero byte, as is the case
   * with textual strings.  Instead, they may contain zero bytes anywhere.  If
   * it happens to contain bytes that look like quote characters, or other
   * things that can disrupt their use in SQL queries, they will be replaced
   * with special escape sequences.
   */
  [[nodiscard]] std::string
  esc_raw(unsigned char const data[], std::size_t len) const
  {
    return conn().esc_raw(data, len);
  }
  /// Escape binary data for use as SQL string literal in this transaction
  [[nodiscard]] std::string esc_raw(std::string const &) const;

  /// Unescape binary data, e.g. from a table field or notification payload.
  /** Takes a binary string as escaped by PostgreSQL, and returns a restored
   * copy of the original binary data.
   */
  [[nodiscard]] std::string unesc_raw(std::string const &text) const
  {
    return conn().unesc_raw(text);
  }

  /// Unescape binary data, e.g. from a table field or notification payload.
  /** Takes a binary string as escaped by PostgreSQL, and returns a restored
   * copy of the original binary data.
   */
  [[nodiscard]] std::string unesc_raw(char const *text) const
  {
    return conn().unesc_raw(text);
  }

  /// Represent object as SQL string, including quoting & escaping.
  /** Nulls are recognized and represented as SQL nulls. */
  template<typename T>[[nodiscard]] std::string quote(T const &t) const
  {
    return conn().quote(t);
  }

  /// Binary-escape and quote a binarystring for use as an SQL constant.
  [[nodiscard]] std::string
  quote_raw(unsigned char const bin[], std::size_t len) const
  {
    return conn().quote_raw(bin, len);
  }

  [[nodiscard]] std::string quote_raw(std::string const &bin) const;

  /// Escape an SQL identifier for use in a query.
  [[nodiscard]] std::string quote_name(std::string_view identifier) const
  {
    return conn().quote_name(identifier);
  }

  /// Escape string for literal LIKE match.
  [[nodiscard]] std::string
  esc_like(std::string const &bin, char escape_char = '\\') const
  {
    return conn().esc_like(bin, escape_char);
  }
  //@}

  /// Execute query
  /** Perform a query in this transaction.
   *
   * This is one of the most important functions in libpqxx.
   *
   * Most libpqxx exceptions can be thrown from here, including sql_error,
   * broken_connection, and many sql_error subtypes such as
   * feature_not_supported or insufficient_privilege.  But any exception thrown
   * by the C++ standard library may also occur here.  All exceptions will be
   * derived from std::exception.
   *
   * @param query Query or command to execute
   * @param desc Optional identifier for query, to help pinpoint SQL errors
   * @return A result set describing the query's or command's result
   */
  result exec(std::string_view query, std::string const &desc = std::string{});

  result
  exec(std::stringstream const &query, std::string const &desc = std::string{})
  {
    return exec(query.str(), desc);
  }

  /// Execute query, which should zero rows of data.
  /** Works like exec, but fails if the result contains data.  It still returns
   * a result, however, which may contain useful metadata.
   *
   * @throw unexpected_rows If the query returned the wrong number of rows.
   */
  result
  exec0(std::string const &query, std::string const &desc = std::string{})
  {
    return exec_n(0, query, desc);
  }

  /// Execute query returning a single row of data.
  /** Works like exec, but requires the result to contain exactly one row.
   * The row can be addressed directly, without the need to find the first row
   * in a result set.
   *
   * @throw unexpected_rows If the query returned the wrong number of rows.
   */
  row exec1(std::string const &query, std::string const &desc = std::string{})
  {
    return exec_n(1, query, desc).front();
  }

  /// Execute query, expect given number of rows.
  /** Works like exec, but checks that the number of rows is exactly what's
   * expected.
   *
   * @throw unexpected_rows If the query returned the wrong number of rows.
   */
  result exec_n(
    result::size_type rows, std::string const &query,
    std::string const &desc = std::string{});

  /// Execute query, expecting exactly 1 row with 1 field.
  template<typename TYPE>
  TYPE query_value(
    std::string const &query, std::string const &desc = std::string{})
  {
    row const r{exec1(query, desc)};
    if (r.size() != 1)
      throw usage_error{
        "Queried single value from result with " + to_string(r.size()) +
        " columns."};
    return r[0].as<TYPE>();
  }

  /// Execute a query, and loop over the results row by row.
  /** Converts the rows to @c std::tuple, of the column types you specify.
   *
   * Use this with a range-based "for" loop.  It executes the query, and
   * directly maps the resulting rows onto a @c std::tuple of the types you
   * specify.  It starts before all the data from the server is in, so if your
   * network connection to the server breaks while you're iterating, you'll get
   * an exception partway through.
   *
   * The tuple may contain @c std::string_view fields, but the strings to which
   * they point will only remain valid until you extract the next row.  After
   * that, the memory holding the string may be overwritten or deallocated.
   *
   * If any of the columns can be null, and the C++ type to which it translates
   * does not have a null value, wrap the type in @c std::optional (or if
   * you prefer, @c std::shared_ptr or @c std::unique_ptr).  These templates
   * do recognise null values, and libpqxx will know how to convert to them.
   *
   * The connection is in a special state until the iteration finishes.  So if
   * it does not finish due to a @c break or a @c return or an exception, then
   * the entire connection becomes effectively unusable.
   *
   * Querying in this way is faster than the @c exec() methods, at least for
   * larger results, and you can start processing rows before the full result
   * is in.  Also, @c stream() scales better in terms of memory usage.  Where
   * @c exec() reads the entire result into memory at once, @c stream() will
   * read and process one row at at a time.
   *
   * Your query executes as part of a COPY command, not as a stand-alone query,
   * so there are limitations to what you can do in the query.  It can be
   * either a SELECT or VALUES query; or an INSERT, UPDATE, or DELETE with a
   * RETURNING clause.  See the documentation for PostgreSQL's COPY command for
   * the details:
   *
   *     https://www.postgresql.org/docs/current/sql-copy.html
   *
   * Iterating in this way does require each of the field types you pass to be
   * default-constructible, copy-constructible, and assignable.  These
   * requirements may be loosened once libpqxx moves on to C++20.
   */
  template<typename... TYPE>[[nodiscard]] auto stream(std::string_view query)
  {
    return pqxx::internal::owning_stream_input_iteration<TYPE...>{
      std::make_unique<pqxx::stream_from>(*this, from_query_t{}, query)};
  }

  /**
   * @name Parameterized statements
   *
   * You'll often need parameters in the queries you execute: "select the
   * car with this licence plate."  If the parameter is a string, you need to
   * quote it and escape any special characters inside it, or it may become a
   * target for an SQL injection attack.  If it's an integer (for example),
   * you need to convert it to a string, but in the database's format, without
   * locale-specific niceties like "," separators between the thousands.
   *
   * Parameterised statements are an easier and safer way to do this.  They're
   * like prepared statements, but for a single use.  You don't need to name
   * them, and you don't need to prepare them first.
   *
   * Your query will include placeholders like @c $1 and $2 etc. in the places
   * where you want the arguments to go.  Then, you pass the argument values
   * and the actual query is constructed for you.
   *
   * Pass the exact right number of parameters, and in the right order.  The
   * parameters in the query don't have to be neatly ordered from @c $1 to
   * @c $2 to @c $3 - but you must pass the argument for @c $1 first, the one
   * for @c $2 second, etc.
   *
   * @warning Beware of "nul" bytes.  Any string you pass as a parameter will
   * end at the first char with value zero.  If you pass a @c std::string that
   * contains a zero byte, the last byte in the value will be the one just
   * before the zero.
   */
  //@{
  /// Execute an SQL statement with parameters.
  template<typename... Args>
  result exec_params(std::string const &query, Args &&... args)
  {
    return internal_exec_params(
      query, internal::params(std::forward<Args>(args)...));
  }

  // Execute parameterised statement, expect a single-row result.
  /** @throw unexpected_rows if the result does not consist of exactly one row.
   */
  template<typename... Args>
  row exec_params1(std::string const &query, Args &&... args)
  {
    return exec_params_n(1, query, std::forward<Args>(args)...).front();
  }

  // Execute parameterised statement, expect a result with zero rows.
  /** @throw unexpected_rows if the result contains rows.
   */
  template<typename... Args>
  result exec_params0(std::string const &query, Args &&... args)
  {
    return exec_params_n(0, query, std::forward<Args>(args)...);
  }

  // Execute parameterised statement, expect exactly a given number of rows.
  /** @throw unexpected_rows if the result contains the wrong number of rows.
   */
  template<typename... Args>
  result
  exec_params_n(std::size_t rows, std::string const &query, Args &&... args)
  {
    auto const r{exec_params(query, std::forward<Args>(args)...)};
    check_rowcount_params(rows, r.size());
    return r;
  }
  //@}

  /**
   * @name Prepared statements
   *
   * These are very similar to parameterised statements.  The difference is
   * that you prepare them in advance, giving them identifying names.  You can
   * then call them by these names, passing in the argument values appropriate
   * for that call.
   *
   * You prepare a statement on the connection, using
   * @c pqxx::connection::prepare().  But you then call the statement in a
   * transaction, using the functions you see here.
   *
   * Never try to prepare, execute, or unprepare a prepared statement manually
   * using direct SQL queries when you also use the libpqxx equivalents.  For
   * any given statement, either prepare, manage, and execute it through the
   * dedicated libpqxx functions; or do it all directly in SQL.  Don't mix the
   * two, or the code may get confused.
   *
   * See \ref prepared for a full discussion.
   *
   * @warning Beware of "nul" bytes.  Any string you pass as a parameter will
   * end at the first char with value zero.  If you pass a @c std::string that
   * contains a zero byte, the last byte in the value will be the one just
   * before the zero.  If you need a zero byte, consider using
   * pqxx::binarystring and/or SQL's @c bytea type.
   */
  //@{

  /// Execute a prepared statement, with optional arguments.
  template<typename... Args>
  result exec_prepared(zview statement, Args &&... args)
  {
    return internal_exec_prepared(
      statement, internal::params(std::forward<Args>(args)...));
  }

  /// Execute a prepared statement, and expect a single-row result.
  /** @throw pqxx::unexpected_rows if the result was not exactly 1 row.
   */
  template<typename... Args>
  row exec_prepared1(zview statement, Args &&... args)
  {
    return exec_prepared_n(1, statement, std::forward<Args>(args)...).front();
  }

  /// Execute a prepared statement, and expect a result with zero rows.
  /** @throw pqxx::unexpected_rows if the result contained rows.
   */
  template<typename... Args>
  result exec_prepared0(zview statement, Args &&... args)
  {
    return exec_prepared_n(0, statement, std::forward<Args>(args)...);
  }

  /// Execute a prepared statement, expect a result with given number of rows.
  /** @throw pqxx::unexpected_rows if the result did not contain exactly the
   *  given number of rows.
   */
  template<typename... Args>
  result
  exec_prepared_n(result::size_type rows, zview statement, Args &&... args)
  {
    auto const r{exec_prepared(statement, std::forward<Args>(args)...)};
    check_rowcount_prepared(statement, rows, r.size());
    return r;
  }

  //@}

  /**
   * @name Error/warning output
   */
  //@{
  /// Have connection process a warning message.
  void process_notice(char const msg[]) const { m_conn.process_notice(msg); }
  /// Have connection process a warning message.
  void process_notice(std::string const &msg) const
  {
    m_conn.process_notice(msg);
  }
  //@}

  /// The connection in which this transaction lives.
  [[nodiscard]] connection &conn() const { return m_conn; }

  /// Set session variable using SQL "SET" command.
  /** The new value is typically forgotten if the transaction aborts.
   * Not for nontransaction though: in that case the set value will be kept
   * regardless.
   *
   * @warning This executes SQL.  Do not try to set or get variables while a
   * pipeline or table stream is active.
   *
   * @param var The variable to set.
   * @param value The new value to store in the variable.
   */
  void set_variable(std::string_view var, std::string_view value);

  /// Read session variable using SQL "SHOW" command.
  /** @warning This executes SQL.  Do not try to set or get variables while a
   * pipeline or table stream is active.
   */
  std::string get_variable(std::string_view);

protected:
  /// Create a transaction (to be called by implementation classes only).
  /** The optional name, if nonempty, must begin with a letter and may contain
   * letters and digits only.
   */
  explicit transaction_base(connection &c);

  /// Register this transaction with the connection.
  void register_transaction();

  /// End transaction.  To be called by implementing class' destructor.
  void close() noexcept;

  /// To be implemented by derived implementation class: commit transaction.
  virtual void do_commit() = 0;
  /// To be implemented by derived implementation class: abort transaction.
  virtual void do_abort() = 0;

  // For use by implementing class:

  /// Execute query on connection directly.
  result direct_exec(std::string_view);
  result direct_exec(std::shared_ptr<std::string>);

private:
  enum class status
  {
    nascent,
    active,
    aborted,
    committed,
    in_doubt
  };

  PQXX_PRIVATE void check_pending_error();

  template<typename T> bool parm_is_null(T *p) const noexcept
  {
    return p == nullptr;
  }
  template<typename T> bool parm_is_null(T) const noexcept { return false; }

  result internal_exec_prepared(zview statement, internal::params const &args);

  result
  internal_exec_params(std::string const &query, internal::params const &args);

  /// Throw unexpected_rows if prepared statement returned wrong no. of rows.
  void check_rowcount_prepared(
    zview statement, result::size_type expected_rows,
    result::size_type actual_rows);

  /// Throw unexpected_rows if wrong row count from parameterised statement.
  void
  check_rowcount_params(std::size_t expected_rows, std::size_t actual_rows);

  friend class pqxx::internal::gate::transaction_transactionfocus;
  PQXX_PRIVATE void register_focus(internal::transactionfocus *);
  PQXX_PRIVATE void unregister_focus(internal::transactionfocus *) noexcept;
  PQXX_PRIVATE void register_pending_error(std::string const &) noexcept;

  connection &m_conn;

  internal::unique<internal::transactionfocus> m_focus;
  status m_status = status::active;
  bool m_registered = false;
  std::string m_pending_error;
};
} // namespace pqxx


namespace pqxx::internal
{
/// The SQL command for starting a given type of transaction.
template<pqxx::isolation_level isolation, pqxx::write_policy rw>
extern const zview begin_cmd;

// These are not static members, so "constexpr" does not imply "inline".
template<>
inline constexpr zview begin_cmd<read_committed, write_policy::read_write>{
  "BEGIN"};
template<>
inline constexpr zview begin_cmd<read_committed, write_policy::read_only>{
  "BEGIN READ ONLY"};
template<>
inline constexpr zview begin_cmd<repeatable_read, write_policy::read_write>{
  "BEGIN ISOLATION LEVEL REPEATABLE READ"};
template<>
inline constexpr zview begin_cmd<repeatable_read, write_policy::read_only>{
  "BEGIN ISOLATION LEVEL REPEATABLE READ READ ONLY"};
template<>
inline constexpr zview begin_cmd<serializable, write_policy::read_write>{
  "BEGIN ISOLATION LEVEL SERIALIZABLE"};
template<>
inline constexpr zview begin_cmd<serializable, write_policy::read_only>{
  "BEGIN ISOLATION LEVEL SERIALIZABLE READ ONLY"};
} // namespace pqxx::internal

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
