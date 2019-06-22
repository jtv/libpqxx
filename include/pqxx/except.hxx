/* Definition of libpqxx exception classes.
 *
 * pqxx::sql_error, pqxx::broken_connection, pqxx::in_doubt_error, ...
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/except instead.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_EXCEPT
#define PQXX_H_EXCEPT

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <stdexcept>
#include <string>


namespace pqxx
{
/**
 * @addtogroup exception Exception classes
 *
 * These exception classes follow, roughly, the two-level hierarchy defined by
 * the PostgreSQL error codes (see Appendix A of the PostgreSQL documentation
 * corresponding to your server version).  This is not a complete mapping
 * though.  There are other differences as well, e.g. the error code
 * @c statement_completion_unknown has a separate status in libpqxx as
 * @c in_doubt_error, and @c too_many_connections is classified as a
 * @c broken_connection rather than a subtype of @c insufficient_resources.
 *
 * @see http://www.postgresql.org/docs/9.4/interactive/errcodes-appendix.html
 *
 * @{
 */

/// Run-time failure encountered by libpqxx, similar to std::runtime_error
struct PQXX_LIBEXPORT failure : std::runtime_error
{
  explicit failure(const std::string &);
};


/// Exception class for lost or failed backend connection.
/**
 * @warning When this happens on Unix-like systems, you may also get a SIGPIPE
 * signal.  That signal aborts the program by default, so if you wish to be able
 * to continue after a connection breaks, be sure to disarm this signal.
 *
 * If you're working on a Unix-like system, see the manual page for
 * @c signal (2) on how to deal with SIGPIPE.  The easiest way to make this
 * signal harmless is to make your program ignore it:
 *
 * @code
 * #include <signal.h>
 *
 * int main()
 * {
 *   signal(SIGPIPE, SIG_IGN);
 *   // ...
 * @endcode
 */
struct PQXX_LIBEXPORT broken_connection : failure
{
  broken_connection();
  explicit broken_connection(const std::string &);
};


/// Exception class for failed queries.
/** Carries, in addition to a regular error message, a copy of the failed query
 * and (if available) the SQLSTATE value accompanying the error.
 */
class PQXX_LIBEXPORT sql_error : public failure
{
  /// Query string.  Empty if unknown.
  const std::string m_query;
  /// SQLSTATE string describing the error type, if known; or empty string.
  const std::string m_sqlstate;

public:
  explicit sql_error(
	const std::string &msg="",
	const std::string &Q="",
	const char sqlstate[]=nullptr);
  virtual ~sql_error() noexcept;

  /// The query whose execution triggered the exception
  PQXX_PURE const std::string &query() const noexcept;			//[t56]

  /// SQLSTATE error code if known, or empty string otherwise.
  PQXX_PURE const std::string &sqlstate() const noexcept;
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
  explicit in_doubt_error(const std::string &);
};


/// The backend saw itself forced to roll back the ongoing transaction.
struct PQXX_LIBEXPORT transaction_rollback : failure
{
  explicit transaction_rollback(const std::string &);
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
  explicit serialization_failure(const std::string &);
};


/// We can't tell whether our last statement succeeded.
struct PQXX_LIBEXPORT statement_completion_unknown : transaction_rollback
{
  explicit statement_completion_unknown(const std::string &);
};


/// The ongoing transaction has deadlocked.  Retrying it may help.
struct PQXX_LIBEXPORT deadlock_detected : transaction_rollback
{
  explicit deadlock_detected(const std::string &);
};


/// Internal error in libpqxx library
struct PQXX_LIBEXPORT internal_error : std::logic_error
{
  explicit internal_error(const std::string &);
};


/// Error in usage of libpqxx library, similar to std::logic_error
struct PQXX_LIBEXPORT usage_error : std::logic_error
{
  explicit usage_error(const std::string &);
};


/// Invalid argument passed to libpqxx, similar to std::invalid_argument
struct PQXX_LIBEXPORT argument_error : std::invalid_argument
{
  explicit argument_error(const std::string &);
};


/// Value conversion failed, e.g. when converting "Hello" to int.
struct PQXX_LIBEXPORT conversion_error : std::domain_error
{
  explicit conversion_error(const std::string &);
};


/// Could not convert value to string: not enough buffer space.
struct PQXX_LIBEXPORT conversion_overrun : conversion_error
{
  explicit conversion_overrun(const std::string &);
};


/// Something is out of range, similar to std::out_of_range
struct PQXX_LIBEXPORT range_error : std::out_of_range
{
  explicit range_error(const std::string &);
};


/// Query returned an unexpected number of rows.
struct PQXX_LIBEXPORT unexpected_rows : public range_error
{
  explicit unexpected_rows(const std::string &msg) : range_error{msg} {}
};


/// Database feature not supported in current setup
struct PQXX_LIBEXPORT feature_not_supported : sql_error
{
  explicit feature_not_supported(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    sql_error{err, Q, sqlstate} {}
};

/// Error in data provided to SQL statement
struct PQXX_LIBEXPORT data_exception : sql_error
{
  explicit data_exception(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    sql_error{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT integrity_constraint_violation : sql_error
{
  explicit integrity_constraint_violation(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    sql_error{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT restrict_violation : integrity_constraint_violation
{
  explicit restrict_violation(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    integrity_constraint_violation{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT not_null_violation : integrity_constraint_violation
{
  explicit not_null_violation(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    integrity_constraint_violation{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT foreign_key_violation : integrity_constraint_violation
{
  explicit foreign_key_violation(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    integrity_constraint_violation{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT unique_violation : integrity_constraint_violation
{
  explicit unique_violation(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    integrity_constraint_violation{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT check_violation : integrity_constraint_violation
{
  explicit check_violation(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    integrity_constraint_violation{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT invalid_cursor_state : sql_error
{
  explicit invalid_cursor_state(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    sql_error{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT invalid_sql_statement_name : sql_error
{
  explicit invalid_sql_statement_name(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    sql_error{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT invalid_cursor_name : sql_error
{
  explicit invalid_cursor_name(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    sql_error{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT syntax_error : sql_error
{
  /// Approximate position in string where error occurred, or -1 if unknown.
  const int error_position;

  explicit syntax_error(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr,
	int pos=-1) :
    sql_error{err, Q, sqlstate}, error_position{pos} {}
};

struct PQXX_LIBEXPORT undefined_column : syntax_error
{
  explicit undefined_column(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    syntax_error{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT undefined_function : syntax_error
{
  explicit undefined_function(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    syntax_error{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT undefined_table : syntax_error
{
  explicit undefined_table(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    syntax_error{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT insufficient_privilege : sql_error
{
  explicit insufficient_privilege(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    sql_error{err, Q, sqlstate} {}
};

/// Resource shortage on the server
struct PQXX_LIBEXPORT insufficient_resources : sql_error
{
  explicit insufficient_resources(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    sql_error{err,Q, sqlstate} {}
};

struct PQXX_LIBEXPORT disk_full : insufficient_resources
{
  explicit disk_full(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    insufficient_resources{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT out_of_memory : insufficient_resources
{
  explicit out_of_memory(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    insufficient_resources{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT too_many_connections : broken_connection
{
  explicit too_many_connections(const std::string &err) :
	broken_connection{err} {}
};

/// PL/pgSQL error
/** Exceptions derived from this class are errors from PL/pgSQL procedures.
 */
struct PQXX_LIBEXPORT plpgsql_error : sql_error
{
  explicit plpgsql_error(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    sql_error{err, Q, sqlstate} {}
};

/// Exception raised in PL/pgSQL procedure
struct PQXX_LIBEXPORT plpgsql_raise : plpgsql_error
{
  explicit plpgsql_raise(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    plpgsql_error{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT plpgsql_no_data_found : plpgsql_error
{
  explicit plpgsql_no_data_found(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    plpgsql_error{err, Q, sqlstate} {}
};

struct PQXX_LIBEXPORT plpgsql_too_many_rows : plpgsql_error
{
  explicit plpgsql_too_many_rows(
	const std::string &err,
	const std::string &Q="",
	const char sqlstate[]=nullptr) :
    plpgsql_error{err, Q, sqlstate} {}
};

/**
 * @}
 */
}

#include "pqxx/compiler-internal-post.hxx"
#endif
