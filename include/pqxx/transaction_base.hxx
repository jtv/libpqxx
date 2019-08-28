/* Common code and definitions for the transaction classes.
 *
 * pqxx::transaction_base defines the interface for any abstract class that
 * represents a database transaction.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/transaction_base instead.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_TRANSACTION_BASE
#define PQXX_H_TRANSACTION_BASE

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

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

// Methods tested in eg. test module test01 are marked with "//[t01]".

namespace pqxx::internal
{
class sql_cursor;

class PQXX_LIBEXPORT transactionfocus : public virtual namedclass
{
public:
  explicit transactionfocus(transaction_base &t) :
    namedclass{"transactionfocus"},
    m_trans{t}
  {
  }

  transactionfocus() =delete;
  transactionfocus(const transactionfocus &) =delete;
  transactionfocus &operator=(const transactionfocus &) =delete;

protected:
  void register_me();
  void unregister_me() noexcept;
  void reg_pending_error(const std::string &) noexcept;
  bool registered() const noexcept { return m_registered; }

  transaction_base &m_trans;

private:
  bool m_registered = false;
};
} // namespace pqxx::internal


namespace pqxx::internal::gate
{
class transaction_subtransaction;
class transaction_sql_cursor;
class transaction_stream_from;
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
 * Several implementations of this interface are shipped with libpqxx, including
 * the plain transaction class, the entirely unprotected nontransaction, and the
 * more cautious robusttransaction.
 */

/// Interface definition (and common code) for "transaction" classes.
/**
 * @ingroup transaction
 *
 * Abstract base class for all transaction types.
 */
class PQXX_LIBEXPORT PQXX_NOVTABLE transaction_base :
  public virtual internal::namedclass
{
public:
  transaction_base() =delete;
  transaction_base(const transaction_base &) =delete;
  transaction_base &operator=(const transaction_base &) =delete;

  virtual ~transaction_base() =0;					//[t01]

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
  void commit();							//[t01]

  /// Abort the transaction
  /** No special effort is required to call this function; it will be called
   * implicitly when the transaction is destructed.
   */
  void abort();								//[t10]

  /**
   * @ingroup escaping-functions
   */
  //@{
  /// Escape string for use as SQL string literal in this transaction
  std::string esc(const char str[]) const            { return conn().esc(str); }
  /// Escape string for use as SQL string literal in this transaction
  std::string esc(const char str[], size_t maxlen) const
                                             { return conn().esc(str, maxlen); }
  /// Escape string for use as SQL string literal in this transaction
  std::string esc(const std::string &str) const      { return conn().esc(str); }

  /// Escape binary data for use as SQL string literal in this transaction
  /** Raw, binary data is treated differently from regular strings.  Binary
   * strings are never interpreted as text, so they may safely include byte
   * values or byte sequences that don't happen to represent valid characters in
   * the character encoding being used.
   *
   * The binary string does not stop at the first zero byte, as is the case with
   * textual strings.  Instead, they may contain zero bytes anywhere.  If it
   * happens to contain bytes that look like quote characters, or other things
   * that can disrupt their use in SQL queries, they will be replaced with
   * special escape sequences.
   */
  std::string esc_raw(const unsigned char data[], size_t len) const	//[t62]
                                           { return conn().esc_raw(data, len); }
  /// Escape binary data for use as SQL string literal in this transaction
  std::string esc_raw(const std::string &) const;			//[t62]

  /// Unescape binary data, e.g. from a table field or notification payload.
  /** Takes a binary string as escaped by PostgreSQL, and returns a restored
   * copy of the original binary data.
   */
  std::string unesc_raw(const std::string &text) const
					      { return conn().unesc_raw(text); }

  /// Unescape binary data, e.g. from a table field or notification payload.
  /** Takes a binary string as escaped by PostgreSQL, and returns a restored
   * copy of the original binary data.
   */
  std::string unesc_raw(const char *text) const
					      { return conn().unesc_raw(text); }

  /// Represent object as SQL string, including quoting & escaping.
  /** Nulls are recognized and represented as SQL nulls. */
  template<typename T> std::string quote(const T &t) const
                                                     { return conn().quote(t); }

  /// Binary-escape and quote a binarystring for use as an SQL constant.
  std::string quote_raw(const unsigned char str[], size_t len) const
					  { return conn().quote_raw(str, len); }

  std::string quote_raw(const std::string &str) const;

  /// Escape an SQL identifier for use in a query.
  std::string quote_name(const std::string &identifier) const
				       { return conn().quote_name(identifier); }

  /// Escape string for literal LIKE match.
  std::string esc_like(const std::string &str, char escape_char='\\') const
				   { return conn().esc_like(str, escape_char); }
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
   * @param Query Query or command to execute
   * @param Desc Optional identifier for query, to help pinpoint SQL errors
   * @return A result set describing the query's or command's result
   */
  result exec(
	const std::string &Query,
	const std::string &Desc=std::string{});				//[t01]

  result exec(
	const std::stringstream &Query,
	const std::string &Desc=std::string{})
	{ return exec(Query.str(), Desc); }

  /// Execute query, which should zero rows of data.
  /** Works like exec, but fails if the result contains data.  It still returns
   * a result, however, which may contain useful metadata.
   *
   * @throw unexpected_rows If the query returned the wrong number of rows.
   */
  result exec0(
	const std::string &Query,
	const std::string &Desc=std::string{})
	{ return exec_n(0, Query, Desc); }

  /// Execute query returning a single row of data.
  /** Works like exec, but requires the result to contain exactly one row.
   * The row can be addressed directly, without the need to find the first row
   * in a result set.
   *
   * @throw unexpected_rows If the query returned the wrong number of rows.
   */
  row exec1(const std::string &Query, const std::string &Desc=std::string{})
	{ return exec_n(1, Query, Desc).front(); }

  /// Execute query, expect given number of rows.
  /** Works like exec, but checks that the number of rows is exactly what's
   * expected.
   *
   * @throw unexpected_rows If the query returned the wrong number of rows.
   */
  result exec_n(
        size_t rows,
	const std::string &Query,
	const std::string &Desc=std::string{});

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
  template<typename ...Args>
  result exec_params(const std::string &query, Args &&...args)
  {
    return internal_exec_params(
      query, internal::params(std::forward<Args>(args)...));
  }

  // Execute parameterised statement, expect a single-row result.
  /** @throw unexpected_rows if the result does not consist of exactly one row.
   */
  template<typename ...Args>
  row exec_params1(const std::string &query, Args&&... args)
  {
    return exec_params_n(1, query, std::forward<Args>(args)...).front();
  }

  // Execute parameterised statement, expect a result with zero rows.
  /** @throw unexpected_rows if the result contains rows.
   */
  template<typename ...Args>
  result exec_params0(const std::string &query, Args &&...args)
  {
    return exec_params_n(0, query, std::forward<Args>(args)...);
  }

  // Execute parameterised statement, expect exactly a given number of rows.
  /** @throw unexpected_rows if the result contains the wrong number of rows.
   */
  template<typename ...Args>
  result exec_params_n(size_t rows, const std::string &query, Args &&...args)
  {
    const auto r = exec_params(query, std::forward<Args>(args)...);
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
  template<typename ...Args>
  result exec_prepared(const std::string &statement, Args&&... args)
  {
    return internal_exec_prepared(
      statement, internal::params(std::forward<Args>(args)...));
  }

  /// Execute a prepared statement, and expect a single-row result.
  /** @throw pqxx::unexpected_rows if the result was not exactly 1 row.
   */
  template<typename ...Args>
  row exec_prepared1(const std::string &statement, Args&&... args)
  {
    return exec_prepared_n(1, statement, std::forward<Args>(args)...).front();
  }

  /// Execute a prepared statement, and expect a result with zero rows.
  /** @throw pqxx::unexpected_rows if the result contained rows.
   */
  template<typename ...Args>
  result exec_prepared0(const std::string &statement, Args&&... args)
  {
    return exec_prepared_n(0, statement, std::forward<Args>(args)...);
  }

  /// Execute a prepared statement, expect a result with given number of rows.
  /** @throw pqxx::unexpected_rows if the result did not contain exactly the
   *  given number of rows.
   */
  template<typename ...Args>
  result exec_prepared_n(
	size_t rows,
	const std::string &statement,
	Args&&... args)
  {
    const auto r = exec_prepared(statement, std::forward<Args>(args)...);
    check_rowcount_prepared(statement, rows, r.size());
    return r;
  }

  //@}

  /**
   * @name Error/warning output
   */
  //@{
  /// Have connection process a warning message.
  void process_notice(const char Msg[]) const				//[t14]
	{ m_conn.process_notice(Msg); }
  /// Have connection process a warning message.
  void process_notice(const std::string &Msg) const			//[t14]
	{ m_conn.process_notice(Msg); }
  //@}

  /// The connection in which this transaction lives.
  connection &conn() const { return m_conn; }				//[t04]

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
  void set_variable(std::string_view var, std::string_view value);	//[t61]

  /// Read session variable using SQL "SHOW" command.
  /** @warning This executes SQL.  Do not try to set or get variables while a
   * pipeline or table stream is active.
   */
  std::string get_variable(std::string_view);   			//[t61]

protected:
  /// Create a transaction (to be called by implementation classes only)
  /** The optional name, if nonempty, must begin with a letter and may contain
   * letters and digits only.
   */
  explicit transaction_base(connection &c);

  /// Register this transaction with the connection.
  void register_transaction();

  /// Begin transaction (to be called by implementing class)
  /** Will typically be called from implementing class' constructor.
   */
  void Begin();

  /// End transaction.  To be called by implementing class' destructor
  void close() noexcept;

  /// To be implemented by derived implementation class: commit transaction
  virtual void do_commit() =0;
  /// To be implemented by derived implementation class: abort transaction
  virtual void do_abort() =0;

  // For use by implementing class:

  /// Execute query on connection directly.
  result direct_exec(const char C[]);

private:
  enum class status
  {
    nascent,
    active,
    aborted,
    committed,
    in_doubt
  };

  PQXX_PRIVATE void CheckPendingError();

  template<typename T> bool parm_is_null(T *p) const noexcept
	{ return p == nullptr; }
  template<typename T> bool parm_is_null(T) const noexcept
	{ return false; }

  result internal_exec_prepared(
	const std::string &statement,
	const internal::params &args);

  result internal_exec_params(
	const std::string &query,
	const internal::params &args);

  /// Throw unexpected_rows if prepared statement returned wrong no. of rows.
  void check_rowcount_prepared(
	const std::string &statement,
	size_t expected_rows,
	size_t actual_rows);

  /// Throw unexpected_rows if wrong row count from parameterised statement.
  void check_rowcount_params(
	size_t expected_rows, size_t actual_rows);

  friend class pqxx::internal::gate::transaction_transactionfocus;
  PQXX_PRIVATE void register_focus(internal::transactionfocus *);
  PQXX_PRIVATE void unregister_focus(internal::transactionfocus *) noexcept;
  PQXX_PRIVATE void register_pending_error(const std::string &) noexcept;

  friend class pqxx::internal::gate::transaction_stream_from;
  PQXX_PRIVATE void BeginCopyRead(const std::string &, const std::string &);
  bool read_copy_line(std::string &);

  friend class pqxx::internal::gate::transaction_stream_to;
  PQXX_PRIVATE void BeginCopyWrite(
	const std::string &Table,
	const std::string &Columns);
  void write_copy_line(std::string_view);
  void end_copy_write();

  friend class pqxx::internal::gate::transaction_subtransaction;

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
extern const char *begin_cmd;

// These are not static members, so "constexpr" does not imply "inline".
template<> inline constexpr const char *
begin_cmd<read_committed, write_policy::read_write> =
	"BEGIN";
template<> inline constexpr const char *
begin_cmd<read_committed, write_policy::read_only> =
	"BEGIN READ ONLY";
template<> inline constexpr const char *
begin_cmd<repeatable_read, write_policy::read_write> =
	"BEGIN ISOLATION LEVEL REPEATABLE READ";
template<> inline constexpr const char *
begin_cmd<repeatable_read, write_policy::read_only> =
	"BEGIN ISOLATION LEVEL REPEATABLE READ READ ONLY";
template<> inline constexpr const char *
begin_cmd<serializable, write_policy::read_write> =
	"BEGIN ISOLATION LEVEL SERIALIZABLE";
template<> inline constexpr const char *
begin_cmd<serializable, write_policy::read_only> =
	"BEGIN ISOLATION LEVEL SERIALIZABLE READ ONLY";
} // namespace pqxx::internal

#include "pqxx/compiler-internal-post.hxx"
#endif
