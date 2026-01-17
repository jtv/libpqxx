/* Definition of libpqxx exception classes.
 *
 * pqxx::sql_error, pqxx::broken_connection, pqxx::in_doubt_error, ...
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/except instead.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_EXCEPT
#define PQXX_H_EXCEPT

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include <memory>
#include <stdexcept>
#include <string>

#include "pqxx/types.hxx"


namespace pqxx
{
// XXX: Rewrite heading.
// XXX: Automatically report tx as poisoned if cx is poisoned.
// XXX: Explain how poisoning works.
/**
 * @addtogroup exception Exception classes
 *
 * These exception classes follow, roughly, the two-level hierarchy defined by
 * the PostgreSQL SQLSTATE error codes (see Appendix A of the PostgreSQL
 * documentation corresponding to your server version).  This is not a complete
 * mapping though.  There are other differences as well, e.g. the error code
 * for `statement_completion_unknown` has a separate status in libpqxx as
 * @ref in_doubt_error, and `too_many_connections` is classified as a
 * `broken_connection` rather than a subtype of `insufficient_resources`.
 *
 * @see http://www.postgresql.org/docs/9.4/interactive/errcodes-appendix.html
 *
 * @{
 */

/// Base class for all exceptions specific to libpqxx.
struct PQXX_LIBEXPORT failure : std::exception
{
  failure(failure const &) = delete;
  failure(failure &&) = default;
  explicit failure(sl loc = sl::current()) :
          m_block{std::make_shared<block>(loc)}
  {}
  explicit failure(std::string whatarg, sl loc = sl::current()) :
          m_block{std::make_shared<block>(std::move(whatarg), loc)}
  {}

  virtual ~failure() noexcept;

  failure &operator=(failure const &) = default;
  failure &operator=(failure &&) = default;

  /// Error message.
  [[nodiscard]] PQXX_PURE virtual char const *what() const noexcept override
  {
    return m_block->message.c_str();
  }

  /// Best known `std::source_location` for where the error occurred.
  /** Generally there's no one single source location, but the exception only
   * stores one.  This is generally either one that the caller passed to a
   * libpqxx call, or the place where the caller called libpqxx.
   */
  [[nodiscard]] PQXX_PURE sl const &location() const noexcept
  {
    return m_block->location;
  }

  /// SQLSTATE error code, or empty string if unavailable.
  /** The PostgreSQL error codes are documented here:
   *
   * https://www.postgresql.org/docs/current/errcodes-appendix.html
   */
  [[nodiscard]] PQXX_PURE std::string const &sqlstate() const noexcept
  {
    return m_block->sqlstate;
  }

  /// SQL statement that encountered the error, if applicable; or empty string.
  [[nodiscard]] PQXX_PURE std::string const &query() const noexcept
  {
    return m_block->statement;
  }

  /// Does this type of error make the current @ref connection unusable?
  virtual bool poisons_connection() const noexcept { return false; }

  /// Does this type of error make an ongoing @ref dbtransaction unusable?
  /** Even when this returns `true`, it does not generally apply to
   * @ref nontransaction (which is not derived from @ref dbtransaction).
   */
  virtual bool poisons_transaction() const noexcept { return false; }

  /// The name of this exception type: "failure", "sql_error", etc.
  /** Do not count on this being exact, or never changing.  There can be
   * mistakes, or the names may change.  If you need to check for an exact
   * exception type, use `dynamic_cast`.
   */
  virtual std::string_view name() const noexcept { return "failure"; }

protected:
  /// For constructing derived exception types with the additional
  /// fields.
  failure(
    std::string whatarg, std::string stat, std::string sqls,
    sl loc = sl::current()) :
          m_block{std::make_shared<block>(
            std::move(whatarg), std::move(stat), std::move(sqls), loc)}
  {}

  /// All the data this exception or its descendants might need.
  struct block final
  {
    /// Message string.
    std::string message;
    /// SQL statement, if applicable (empty string otherwise).
    std::string statement;
    /// SQLSTATE variable, if applicable (empty string otherwise).
    std::string sqlstate;
    /// Source location.
    sl location;

    block(sl loc) : location{loc} {}
    block(std::string &&msg, sl loc) : message{std::move(msg)}, location{loc}
    {}
    block(std::string &&msg, std::string &&stat, std::string &&sqls, sl loc) :
            message{std::move(msg)},
            statement{std::move(stat)},
            sqlstate{std::move(sqls)},
            location{loc}
    {}
  };

private:
  /// The data for this exception, as a shared pointer for easy copying.
  std::shared_ptr<block const> m_block;
};


/// Exception class for lost or failed backend connection.
/**
 * @warning When this happens on Unix-like systems, you may also get a SIGPIPE
 * signal.  That signal aborts the program by default, so if you wish to be
 * able to continue after a connection breaks, be sure to disarm this signal.
 *
 * If you're working on a Unix-like system, see the manual page for
 * `signal` (2) on how to deal with SIGPIPE.  The easiest way to make this
 * signal harmless is to make your program ignore it:
 *
 * ```cxx
 * #include <csignal>
 *
 * int main()
 * {
 *   std::signal(SIGPIPE, SIG_IGN);
 *   // ...
 * }
 * ```
 */
struct PQXX_LIBEXPORT broken_connection : failure
{
  explicit broken_connection(sl loc = sl::current()) :
          failure{"Connection to database failed.", loc}
  {}

  explicit broken_connection(
    std::string const &whatarg, sl loc = sl::current()) :
          failure{whatarg, loc}
  {}

  virtual ~broken_connection() noexcept;

  /// By its nature, this type of error makes the connection unusable.
  virtual bool poisons_connection() const noexcept override { return true; }

  /// When the connection breaks, so will an ongoing transaction.
  virtual bool poisons_transaction() const noexcept override { return true; }

  virtual std::string_view name() const noexcept override
  {
    return "broken_connection";
  }
};


/// Could not establish connection due to version mismatch.
struct PQXX_LIBEXPORT version_mismatch : broken_connection
{
  explicit version_mismatch(
    std::string const &whatarg, sl loc = sl::current()) :
          broken_connection{whatarg, loc}
  {}

  virtual ~version_mismatch() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "version_mismatch";
  }
};


// XXX: New exception type for version mismatch.
// XXX: Make protocol_violation an sql_error.
/// Exception class for mis-communication with the server.
/** This happens when the conversation between libpq and the server gets messed
 * up.  There aren't many situations where this happens, but one known instance
 * is when you call a parameterised or prepared statement with the wrong number
 * of parameters.
 *
 * Retrying is not likely to make this problem go away.
 */
struct PQXX_LIBEXPORT protocol_violation : failure
{
  explicit protocol_violation(
    std::string const &whatarg, sl loc = sl::current()) :
          failure{whatarg, loc}
  {}

  virtual ~protocol_violation() noexcept;

  /// When this happens, the connection is in a confused state.
  virtual bool poisons_connection() const noexcept override { return true; }

  /// Since the connection is broken, so is a transaction.
  virtual bool poisons_transaction() const noexcept override { return true; }

  virtual std::string_view name() const noexcept override
  {
    return "protocol_violation";
  }
};


/// The caller attempted to set a variable to null, which is not allowed.
struct PQXX_LIBEXPORT variable_set_to_null : failure
{
  explicit variable_set_to_null(
    std::string const &whatarg, sl loc = sl::current()) :
          failure{whatarg, loc}
  {}

  virtual ~variable_set_to_null() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "variable_set_to_null";
  }
};


/// Exception class for failed queries.
/** Carries, in addition to a regular error message, a copy of the failed query
 * and (if available) the SQLSTATE value accompanying the error.
 */
struct PQXX_LIBEXPORT sql_error : public failure
{
  PQXX_ZARGS explicit sql_error(
    std::string const &whatarg = {}, std::string const &stmt = {},
    std::string const &sqls = {}, sl loc = sl::current()) :
          failure{whatarg, stmt, sqls, loc}
  {}
  sql_error(sql_error const &other) = default;
  sql_error(sql_error &&other) = default;

  virtual ~sql_error() noexcept;

  /// If a transaction was ongoing, an SQL error will break it.
  virtual bool poisons_transaction() const noexcept override { return true; }

  virtual std::string_view name() const noexcept override
  {
    return "sql_error";
  }
};


/// "Help, I don't know whether transaction was committed successfully!"
/** Exception that might be thrown in rare cases where the connection to the
 * database is lost while finishing a database transaction, and there's no way
 * of telling whether it was actually executed by the backend.  In this case
 * the database is left in an indeterminate (but consistent) state, and only
 * manual inspection will tell which is the case.
 */
struct PQXX_LIBEXPORT in_doubt_error : failure
{
  explicit in_doubt_error(std::string const &whatarg, sl loc = sl::current()) :
          failure{whatarg, loc}
  {}

  virtual ~in_doubt_error() noexcept;

  /// This kind of error can only happen when the connection breaks.
  virtual bool poisons_connection() const noexcept override { return true; }

  /// The transaction is already closed, and the connection is broken.
  virtual bool poisons_transaction() const noexcept override { return true; }

  virtual std::string_view name() const noexcept override
  {
    return "in_doubt_error";
  }
};


/// The backend saw itself forced to roll back the ongoing transaction.
struct PQXX_LIBEXPORT transaction_rollback : sql_error
{
  PQXX_ZARGS explicit transaction_rollback(
    std::string const &whatarg, std::string const &q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{whatarg, q, sqlstate, loc}
  {}

  virtual ~transaction_rollback() noexcept;

  /// Some earlier failure broke the transaction.
  virtual bool poisons_transaction() const noexcept override { return true; }

  virtual std::string_view name() const noexcept override
  {
    return "transaction_rollback";
  }
};


/// Transaction failed to serialize.  Please retry the whole thing.
/** Can only happen at transaction isolation levels REPEATABLE READ and
 * SERIALIZABLE.
 *
 * The current transaction cannot be committed without violating the guarantees
 * made by its isolation level.  This is the effect of a conflict with another
 * ongoing transaction.  The transaction may still succeed if you try to
 * perform it again.
 */
struct PQXX_LIBEXPORT serialization_failure : transaction_rollback
{
  PQXX_ZARGS explicit serialization_failure(
    std::string const &whatarg, std::string const &q,
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          transaction_rollback{whatarg, q, sqlstate, loc}
  {}

  virtual ~serialization_failure() noexcept;

  /// To retry the transaction, you'll need to start a fresh one.
  virtual bool poisons_transaction() const noexcept override { return true; }

  virtual std::string_view name() const noexcept override
  {
    return "serialization_failure";
  }
};

// XXX:

/// We can't tell whether our last statement succeeded.
struct PQXX_LIBEXPORT statement_completion_unknown : transaction_rollback
{
  PQXX_ZARGS explicit statement_completion_unknown(
    std::string const &whatarg, std::string const &q,
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          transaction_rollback{whatarg, q, sqlstate, loc}
  {}

  virtual ~statement_completion_unknown() noexcept;

  /// It's not advisable to continue using the connection after this.
  virtual bool poisons_connection() const noexcept override { return true; }

  virtual std::string_view name() const noexcept override
  {
    return "statement_completion_unknown";
  }
};


/// The ongoing transaction has deadlocked.  Retrying it may help.
struct PQXX_LIBEXPORT deadlock_detected : transaction_rollback
{
  PQXX_ZARGS explicit deadlock_detected(
    std::string const &whatarg, std::string const &q,
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          transaction_rollback{whatarg, q, sqlstate, loc}
  {}

  virtual ~deadlock_detected() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "deadlock_detected";
  }
};


/// Internal error in libpqxx library
struct PQXX_LIBEXPORT internal_error : failure
{
  explicit internal_error(std::string const &, sl = sl::current());

  virtual ~internal_error() noexcept;

  /// When this happens, all bets are off.  It _may_ work, but don't risk it.
  virtual bool poisons_connection() const noexcept override { return true; }

  /// When this happens, all bets are off.  It _may_ work, but don't risk it.
  virtual bool poisons_transaction() const noexcept override { return true; }

  virtual std::string_view name() const noexcept override
  {
    return "internal_error";
  }
};


/// Error in usage of libpqxx library, similar to std::logic_error
struct PQXX_LIBEXPORT usage_error : failure
{
  explicit usage_error(std::string const &whatarg, sl loc = sl::current()) :
          failure{whatarg, loc}
  {}

  virtual ~usage_error() noexcept;

  /// Your transaction will probably still work, but something is badly wrong.
  virtual bool poisons_transaction() const noexcept override { return true; }

  virtual std::string_view name() const noexcept override
  {
    return "usage_error";
  }
};


/// Invalid argument passed to libpqxx, similar to std::invalid_argument
struct PQXX_LIBEXPORT argument_error : failure
{
  explicit argument_error(std::string const &whatarg, sl loc = sl::current()) :
          failure{whatarg, loc}
  {}

  virtual ~argument_error() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "argument_error";
  }
};


/// Value conversion failed, e.g. when converting "Hello" to int.
struct PQXX_LIBEXPORT conversion_error : failure
{
  explicit conversion_error(
    std::string const &whatarg, sl loc = sl::current()) :
          failure{whatarg, loc}
  {}

  virtual ~conversion_error() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "conversion_error";
  }
};


/// Could not convert null value: target type does not support null.
struct PQXX_LIBEXPORT unexpected_null : conversion_error
{
  explicit unexpected_null(
    std::string const &whatarg, sl loc = sl::current()) :
          conversion_error{whatarg, loc}
  {}

  virtual ~unexpected_null() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "unexpected_null";
  }
};


/// Could not convert value to string: not enough buffer space.
struct PQXX_LIBEXPORT conversion_overrun : conversion_error
{
  explicit conversion_overrun(
    std::string const &whatarg, sl loc = sl::current()) :
          conversion_error{whatarg, loc}
  {}

  virtual ~conversion_overrun() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "conversion_error";
  }
};


/// Something is out of range, similar to std::out_of_range
struct PQXX_LIBEXPORT range_error : failure
{
  explicit range_error(std::string const &whatarg, sl loc = sl::current()) :
          failure{whatarg, loc}
  {}

  virtual ~range_error() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "range_error";
  }
};


/// Query returned an unexpected number of rows.
struct PQXX_LIBEXPORT unexpected_rows : range_error
{
  explicit unexpected_rows(std::string const &msg, sl loc = sl::current()) :
          range_error{msg, loc}
  {}

  virtual ~unexpected_rows() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "unexpected_rows";
  }
};


/// Database feature not supported in current setup.
struct PQXX_LIBEXPORT feature_not_supported : sql_error
{
  PQXX_ZARGS explicit feature_not_supported(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}

  virtual ~feature_not_supported() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "feature_not_supported";
  }

  /// It all depends on the details, but this _can_ break your connection.
  virtual bool poisons_connection() const noexcept override { return true; }

  /// If this poisons your connection, it also poisons your transaction.
  virtual bool poisons_transaction() const noexcept override { return true; }
};


/// Error in data provided to SQL statement.
struct PQXX_LIBEXPORT data_exception : sql_error
{
  PQXX_ZARGS explicit data_exception(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}

  virtual ~data_exception() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "data_exception";
  }
};


struct PQXX_LIBEXPORT integrity_constraint_violation : sql_error
{
  PQXX_ZARGS explicit integrity_constraint_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}

  virtual ~integrity_constraint_violation() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "integrity_constraint_violation";
  }
};


struct PQXX_LIBEXPORT restrict_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit restrict_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc}
  {}

  virtual ~restrict_violation() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "restrict_violation";
  }
};


struct PQXX_LIBEXPORT not_null_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit not_null_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc}
  {}

  virtual ~not_null_violation() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "not_null_violation";
  }
};


struct PQXX_LIBEXPORT foreign_key_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit foreign_key_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc}
  {}

  virtual ~foreign_key_violation() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "foreign_key_violation";
  }
};


struct PQXX_LIBEXPORT unique_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit unique_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc}
  {}

  virtual ~unique_violation() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "unique_violation";
  }
};


// XXX: Add virtual destructors from here on down.
struct PQXX_LIBEXPORT check_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit check_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc}
  {}

  virtual ~check_violation() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "check_violation";
  }
};


struct PQXX_LIBEXPORT invalid_cursor_state : sql_error
{
  PQXX_ZARGS explicit invalid_cursor_state(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}

  virtual ~invalid_cursor_state() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "invalid_cursor_state";
  }
};


struct PQXX_LIBEXPORT invalid_sql_statement_name : sql_error
{
  PQXX_ZARGS explicit invalid_sql_statement_name(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}

  virtual ~invalid_sql_statement_name() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "invalid_sql_statement_name";
  }
};


struct PQXX_LIBEXPORT invalid_cursor_name : sql_error
{
  PQXX_ZARGS explicit invalid_cursor_name(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}

  virtual ~invalid_cursor_name() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "invalid_cursor_name";
  }
};


struct PQXX_LIBEXPORT syntax_error : sql_error
{
  /// Approximate position in string where error occurred, or -1 if unknown.
  int const error_position;

  PQXX_ZARGS explicit syntax_error(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, int pos = -1, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}, error_position{pos}
  {}

  virtual ~syntax_error() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "syntax_error";
  }
};


struct PQXX_LIBEXPORT undefined_column : syntax_error
{
  PQXX_ZARGS explicit undefined_column(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          // TODO: Can we get the column?
          syntax_error{err, Q, sqlstate, -1, loc}
  {}

  virtual ~undefined_column() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "undefined_column";
  }
};


struct PQXX_LIBEXPORT undefined_function : syntax_error
{
  PQXX_ZARGS explicit undefined_function(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          // TODO: Can we get the column?
          syntax_error{err, Q, sqlstate, -1, loc}
  {}

  virtual ~undefined_function() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "undefined_function";
  }
};


struct PQXX_LIBEXPORT undefined_table : syntax_error
{
  PQXX_ZARGS explicit undefined_table(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          // TODO: Can we get the column?
          syntax_error{err, Q, sqlstate, -1, loc}
  {}

  virtual ~undefined_table() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "undefined_table";
  }
};


struct PQXX_LIBEXPORT insufficient_privilege : sql_error
{
  PQXX_ZARGS explicit insufficient_privilege(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}

  virtual ~insufficient_privilege() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "insufficient_privilege";
  }
};


/// Resource shortage on the server.
struct PQXX_LIBEXPORT insufficient_resources : sql_error
{
  PQXX_ZARGS explicit insufficient_resources(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}

  virtual ~insufficient_resources() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "insufficient_resources";
  }
};


struct PQXX_LIBEXPORT disk_full : insufficient_resources
{
  PQXX_ZARGS explicit disk_full(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          insufficient_resources{err, Q, sqlstate, loc}
  {}

  virtual ~disk_full() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "disk_full";
  }
};


struct PQXX_LIBEXPORT server_out_of_memory : insufficient_resources
{
  PQXX_ZARGS explicit server_out_of_memory(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          insufficient_resources{err, Q, sqlstate, loc}
  {}

  virtual ~server_out_of_memory() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "server_out_of_memory";
  }
};


struct PQXX_LIBEXPORT too_many_connections : broken_connection
{
  explicit too_many_connections(
    std::string const &err, sl loc = sl::current()) :
          broken_connection{err, loc}
  {}

  virtual ~too_many_connections() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "too_many_connections";
  }
};


/// PL/pgSQL error
/** Exceptions derived from this class are errors from PL/pgSQL procedures.
 */
struct PQXX_LIBEXPORT plpgsql_error : sql_error
{
  PQXX_ZARGS explicit plpgsql_error(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}

  virtual ~plpgsql_error() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "pgplsql_error";
  }
};


/// Exception raised in PL/pgSQL procedure
struct PQXX_LIBEXPORT plpgsql_raise : plpgsql_error
{
  PQXX_ZARGS explicit plpgsql_raise(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          plpgsql_error{err, Q, sqlstate, loc}
  {}

  virtual ~plpgsql_raise() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "pgplsql_raise";
  }
};


struct PQXX_LIBEXPORT plpgsql_no_data_found : plpgsql_error
{
  PQXX_ZARGS explicit plpgsql_no_data_found(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          plpgsql_error{err, Q, sqlstate, loc}
  {}

  virtual ~plpgsql_no_data_found() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "pgplsql_no_data_found";
  }
};


struct PQXX_LIBEXPORT plpgsql_too_many_rows : plpgsql_error
{
  PQXX_ZARGS explicit plpgsql_too_many_rows(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          plpgsql_error{err, Q, sqlstate, loc}
  {}

  virtual ~plpgsql_too_many_rows() noexcept;

  virtual std::string_view name() const noexcept override
  {
    return "pgplsql_too_many_rows";
  }
};

/**
 * @}
 */
} // namespace pqxx
#endif
