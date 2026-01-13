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
  failure(failure const &) = default;
  failure(failure &&) = default;
  explicit failure(sl loc = sl::current());
  explicit failure(std::string whatarg, sl loc = sl::current());
  virtual ~failure() noexcept;

  failure &operator=(failure const &) = default;
  failure &operator=(failure &&) = default;

  /// Error message.
  [[nodiscard]] PQXX_PURE virtual char const *what() const noexcept override;

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

protected:
  /// For constructing derived exception types with the additional
  /// fields.
  failure(
    std::string whatarg, std::string stat, std::string sqls,
    sl loc = sl::current());

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

    block(sl &&loc) : location{std::move(loc)} {}
    block(std::string &&msg, sl &&loc) :
            message{std::move(msg)}, location{std::move(loc)}
    {}
    block(
      std::string &&msg, std::string &&stat, std::string &&sqls, sl &&loc) :
            message{std::move(msg)},
            statement{std::move(stat)},
            sqlstate{std::move(sqls)},
            location{std::move(loc)}
    {}
  };

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
  broken_connection(sl loc = sl::current());
  explicit broken_connection(std::string const &, sl = sl::current());
};


/// Exception class for micommunication with the server.
/** This happens when the conversation between libpq and the server gets messed
 * up.  There aren't many situations where this happens, but one known instance
 * is when you call a parameterised or prepared statement with th ewrong number
 * of parameters.
 *
 * So even though this is a `broken_connection`, it signals that retrying is
 * _not_ likely to make the problem go away.
 */
struct PQXX_LIBEXPORT protocol_violation : broken_connection
{
  explicit protocol_violation(std::string const &, sl = sl::current());
};


/// The caller attempted to set a variable to null, which is not allowed.
struct PQXX_LIBEXPORT variable_set_to_null : failure
{
  explicit variable_set_to_null(std::string const &, sl = sl::current());
};


/// Exception class for failed queries.
/** Carries, in addition to a regular error message, a copy of the failed query
 * and (if available) the SQLSTATE value accompanying the error.
 */
struct PQXX_LIBEXPORT sql_error : public failure
{
  virtual ~sql_error();

  PQXX_ZARGS explicit sql_error(
    std::string const &whatarg = {}, std::string const &stmt = {},
    std::string const &sqls = {}, sl = sl::current());
  sql_error(sql_error const &other) = default;
  sql_error(sql_error &&other) = default;
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
  explicit in_doubt_error(std::string const &, sl = sl::current());
};


/// The backend saw itself forced to roll back the ongoing transaction.
struct PQXX_LIBEXPORT transaction_rollback : sql_error
{
  PQXX_ZARGS explicit transaction_rollback(
    std::string const &whatarg, std::string const &q = {},
    std::string const &sqlstate = {}, sl = sl::current());
};


/// Transaction failed to serialize.  Please retry it.
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
    std::string const &sqlstate = {}, sl = sl::current());
};


/// We can't tell whether our last statement succeeded.
struct PQXX_LIBEXPORT statement_completion_unknown : transaction_rollback
{
  PQXX_ZARGS explicit statement_completion_unknown(
    std::string const &whatarg, std::string const &q,
    std::string const &sqlstate = {}, sl = sl::current());
};


/// The ongoing transaction has deadlocked.  Retrying it may help.
struct PQXX_LIBEXPORT deadlock_detected : transaction_rollback
{
  PQXX_ZARGS explicit deadlock_detected(
    std::string const &whatarg, std::string const &q,
    std::string const &sqlstate = {}, sl = sl::current());
};


/// Internal error in libpqxx library
struct PQXX_LIBEXPORT internal_error : std::logic_error
{
  explicit internal_error(std::string const &, sl = sl::current());

  /// A `std::source_location` as a hint to the origin of the problem.
  sl location;
};


/// Error in usage of libpqxx library, similar to std::logic_error
struct PQXX_LIBEXPORT usage_error : std::logic_error
{
  explicit usage_error(std::string const &, sl = sl::current());

  /// A `std::source_location` as a hint to the origin of the problem.
  sl location;
};


/// Invalid argument passed to libpqxx, similar to std::invalid_argument
struct PQXX_LIBEXPORT argument_error : std::invalid_argument
{
  explicit argument_error(std::string const &, sl = sl::current());

  /// A `std::source_location` as a hint to the origin of the problem.
  sl location;
};


/// Value conversion failed, e.g. when converting "Hello" to int.
struct PQXX_LIBEXPORT conversion_error : std::domain_error
{
  explicit conversion_error(std::string const &, sl = sl::current());

  /// A `std::source_location` as a hint to the origin of the problem.
  sl location;
};


/// Could not convert null value: target type does not support null.
struct PQXX_LIBEXPORT unexpected_null : conversion_error
{
  explicit unexpected_null(std::string const &, sl = sl::current());
};


/// Could not convert value to string: not enough buffer space.
struct PQXX_LIBEXPORT conversion_overrun : conversion_error
{
  explicit conversion_overrun(std::string const &, sl = sl::current());
};


/// Something is out of range, similar to std::out_of_range
struct PQXX_LIBEXPORT range_error : std::out_of_range
{
  explicit range_error(std::string const &, sl = sl::current());

  /// A `std::source_location` as a hint to the origin of the problem.
  sl location;
};


/// Query returned an unexpected number of rows.
struct PQXX_LIBEXPORT unexpected_rows : range_error
{
  explicit unexpected_rows(std::string const &msg, sl loc = sl::current()) :
          range_error{msg, loc}
  {}
};


/// Database feature not supported in current setup.
struct PQXX_LIBEXPORT feature_not_supported : sql_error
{
  PQXX_ZARGS explicit feature_not_supported(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}
};

/// Error in data provided to SQL statement.
struct PQXX_LIBEXPORT data_exception : sql_error
{
  PQXX_ZARGS explicit data_exception(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}
};

struct PQXX_LIBEXPORT integrity_constraint_violation : sql_error
{
  PQXX_ZARGS explicit integrity_constraint_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}
};

struct PQXX_LIBEXPORT restrict_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit restrict_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc}
  {}
};

struct PQXX_LIBEXPORT not_null_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit not_null_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc}
  {}
};

struct PQXX_LIBEXPORT foreign_key_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit foreign_key_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc}
  {}
};

struct PQXX_LIBEXPORT unique_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit unique_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc}
  {}
};

struct PQXX_LIBEXPORT check_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit check_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc}
  {}
};

struct PQXX_LIBEXPORT invalid_cursor_state : sql_error
{
  PQXX_ZARGS explicit invalid_cursor_state(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}
};

struct PQXX_LIBEXPORT invalid_sql_statement_name : sql_error
{
  PQXX_ZARGS explicit invalid_sql_statement_name(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}
};

struct PQXX_LIBEXPORT invalid_cursor_name : sql_error
{
  PQXX_ZARGS explicit invalid_cursor_name(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}
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
};

struct PQXX_LIBEXPORT undefined_column : syntax_error
{
  PQXX_ZARGS explicit undefined_column(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          // TODO: Can we get the column?
          syntax_error{err, Q, sqlstate, -1, loc}
  {}
};

struct PQXX_LIBEXPORT undefined_function : syntax_error
{
  PQXX_ZARGS explicit undefined_function(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          // TODO: Can we get the column?
          syntax_error{err, Q, sqlstate, -1, loc}
  {}
};

struct PQXX_LIBEXPORT undefined_table : syntax_error
{
  PQXX_ZARGS explicit undefined_table(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          // TODO: Can we get the column?
          syntax_error{err, Q, sqlstate, -1, loc}
  {}
};

struct PQXX_LIBEXPORT insufficient_privilege : sql_error
{
  PQXX_ZARGS explicit insufficient_privilege(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}
};

/// Resource shortage on the server
struct PQXX_LIBEXPORT insufficient_resources : sql_error
{
  PQXX_ZARGS explicit insufficient_resources(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          sql_error{err, Q, sqlstate, loc}
  {}
};

struct PQXX_LIBEXPORT disk_full : insufficient_resources
{
  PQXX_ZARGS explicit disk_full(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          insufficient_resources{err, Q, sqlstate, loc}
  {}
};

struct PQXX_LIBEXPORT out_of_memory : insufficient_resources
{
  PQXX_ZARGS explicit out_of_memory(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          insufficient_resources{err, Q, sqlstate, loc}
  {}
};

struct PQXX_LIBEXPORT too_many_connections : broken_connection
{
  explicit too_many_connections(
    std::string const &err, sl loc = sl::current()) :
          broken_connection{err, loc}
  {}
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
};

/// Exception raised in PL/pgSQL procedure
struct PQXX_LIBEXPORT plpgsql_raise : plpgsql_error
{
  PQXX_ZARGS explicit plpgsql_raise(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          plpgsql_error{err, Q, sqlstate, loc}
  {}
};

struct PQXX_LIBEXPORT plpgsql_no_data_found : plpgsql_error
{
  PQXX_ZARGS explicit plpgsql_no_data_found(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          plpgsql_error{err, Q, sqlstate, loc}
  {}
};

struct PQXX_LIBEXPORT plpgsql_too_many_rows : plpgsql_error
{
  PQXX_ZARGS explicit plpgsql_too_many_rows(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current()) :
          plpgsql_error{err, Q, sqlstate, loc}
  {}
};

/**
 * @}
 */
} // namespace pqxx
#endif
