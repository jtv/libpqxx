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
/**
 * @addtogroup exception Exception classes
 *
 * All exception types thrown by libpqxx are derived from @ref pqxx::failure,
 * so that should be your starting point in exploring them.  When you write a
 * `catch` clause in your code, it's probably worth either having a separate
 * clause for `failure` and its subclasses, or checking at run time whether an
 * error is of this type using `dynamic_cast<pqxx::failure const *>(...)`.  if
 * the exception is a libpqxx exception, you can get a lot of extra information
 * that's not normally available in exceptions.
 *
 * There is no multiple inheritance in this class hierarchy.  That means that
 * the different kinds of libpqxx exception are not reflected in inheritance
 * from `std::logic_error,` `std::runtime_error`, and so on.  They are _all_
 * derived from @ref pqxx::failure (whether directly or indirectly), which in
 * turn inherits directly from `std::exception`.
 *
 * You may wonder whether this was the best design decision.  Before libpqxx 8,
 * various libpqxx exceptions derived from different types in the standard C++
 * exception hierarchy (`std::out_of_range`, `std::argument_error`, and so on).
 * But in practice we saw that:
 * 1. The standard exception type distinctions are not very _actionable._
 * 2. Applications would generally just catch `std::exception` anyway.
 * 3. Important properties never quite follow a neat hierarchy.
 * 4. Making effective use of the hierarchy was hard, finicky work.
 * 5. A `try` could end up with a lot of highly similar `catch` clauses.
 * 6. Shaping the hierarchy around one exception property would confuse
 * another.
 *
 * So as of libpqxx 8, if you want more detail in how you handle different
 * types of exceptions, you use member functions, mostly at run time.  All of
 * the properties are available right from the top down, in the @ref failure
 * base class.  Some of the strings will not apply to all types of exception,
 * but in those cases, they will simply be empty strings.
 *
 * In libpqxx, the exception hierarchy exists not for taxonomy's sake but to
 * enable your application to function well without requiring too much code or
 * effort from you.  In most cases, when an exception occurs, both the safest
 * and the easiest thing to do is drop the objects involved in the error,
 * report what happened, and move on from a reliable state.  That is what these
 * classes are here to support.
 *
 * @{
 */

/// Base class for all exceptions specific to libpqxx.
struct PQXX_LIBEXPORT failure : std::exception
{
  failure(failure const &) = default;
  failure(failure &&) = default;
  explicit failure(sl loc = sl::current(), st tr = st::current()) :
          m_block{std::make_shared<block>(loc, std::move(tr))}
  {}
  explicit failure(
    std::string whatarg, sl loc = sl::current(), st tr = st::current()) :
          m_block{
            std::make_shared<block>(std::move(whatarg), loc, std::move(tr))}
  {}

  ~failure() noexcept override;

  failure &operator=(failure const &) = default;
  failure &operator=(failure &&) = default;

  /// Error message.
  [[nodiscard]] PQXX_PURE char const *what() const noexcept override
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

  /// If available in this compiler, a `std::stacktrace` of this exception.
  /** The stacktrace does not include the creation of the exception object
   * itself.  By default it goes right up to that point.  You can pass a
   * stacktrace of your own choosing to the exception constructors, if you
   * really want, but it generally makes more sense to use the default.
   *
   * If `std::stacktrace` was not available when the libpqxx build was
   * configured, this returns an empty ("null") pointer to
   * `pqxx::stacktrace_placeholder`.
   *
   * This is meant to keep the ABI for `pqxx::failure` stable across builds
   * with and without `std::stacktrace`, and make it easy to upgrade the
   * compiler and get the feature.
   */
  [[nodiscard]] PQXX_PURE std::shared_ptr<st const> trace() const noexcept
  {
    return m_block->trace;
  }

  /// SQLSTATE error code, or empty string if unavailable.
  /** The PostgreSQL error codes are documented here:
   *
   * https://www.postgresql.org/docs/current/errcodes-appendix.html
   */
  [[nodiscard]] PQXX_PURE std::string_view sqlstate() const noexcept
  {
    return m_block->sqlstate;
  }

  /// SQL statement that encountered the error, if applicable; or empty string.
  /** In some cases there will be a placeholder string, to give a rough
   * indication of an SQL operation that's being performed in your name, such
   * as beginning or committing a transaction.  Those will be in square
   * brackets: `[COMMIT]` etc.
   */
  [[nodiscard]] PQXX_PURE std::string_view query() const noexcept
  {
    return m_block->statement;
  }

  /// Does this type of error make the current @ref connection unusable?
  /** If the answer is `true`, assume that there is nothing more you can get
   * done using your existing connection object.  It _may_ still be possible,
   * but it may fail, or things could conceivably be in a confused state where
   * it's just not a good idea to try.
   *
   * If your connection is poisoned, then it follows that any ongoing
   * transaction will no longer be usable either.
   */
  virtual bool poisons_connection() const noexcept { return false; }

  /// Does this type of error make an ongoing @ref dbtransaction unusable?
  /** When this is the case, before you try to do anything else, you'll want to
   * close any transaction you may have open.  If necessary, a call to
   * @ref poisons_connection() will tell you whether it is still possible (and
   * advisable) to open a new one on the same connection.
   *
   * If you are using a @ref nontransaction, bear in mind that it is not
   * derived from @ref dbtransaction.  So if you see an exception which reports
   * that it poisons transactions but not the connection, then a
   * @ref nontransaction should still be usable.
   */
  virtual bool poisons_transaction() const noexcept { return false; }

  // C++26: Implement using reflection.
  /// The name of this exception type: "failure", "sql_error", etc.
  /** Do not count on this being exact, or eternal as libpqxx evolves.  There
   * can be mistakes, names may change, new exception subclasses show up during
   * libpqxx development.
   *
   * @note If you need to check whether an exception you've caught is of a
   * specific type, e.g. `pqxx::deadlock_detected`, _do not_ use `name()` for
   * this, since its answer may change as libpqxx evolves.  Instead, use
   * something like:
   *
   * ```cxx
   * try
   * {
   *   // ...
   * }
   * catch (pqxx::failure const &e)
   * {
   *   auto const deadlock = dynamic_cast<pqxx::deadlock_detected const *>(&e);
   *   if (deadlock == nullptr)
   *   {
   *     // `e` is some other type of exception, not a deadlock.
   *   }
   *   else
   *   {
   *     // `e` is a `deadlock_detected` error, and `deadlock` points to it
   *     // by its more specific static type.
   *   }
   * }
   * ```
   */
  virtual std::string_view name() const noexcept;

protected:
  /// For constructing derived exception types with the additional
  /// fields.
  failure(
    std::string whatarg, std::string stat, std::string sqls,
    sl loc = sl::current(), st tr = st::current()) :
          m_block{std::make_shared<block>(
            std::move(whatarg), std::move(stat), std::move(sqls), loc,
            std::move(tr))}
  {}

private:
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
    /// Stack trace, or if not supported, an empty placeholder.
    std::shared_ptr<st const> trace;

    block(sl loc, st &&tr) :
            location{loc}, trace{make_trace_ptr(std::move(tr))}
    {}

    block(std::string &&msg, sl loc, st &&tr) :
            message{std::move(msg)},
            location{loc},
            trace{make_trace_ptr(std::move(tr))}
    {}

    block(
      std::string &&msg, std::string &&stat, std::string &&sqls, sl loc,
      st &&tr) :
            message{std::move(msg)},
            statement{std::move(stat)},
            sqlstate{std::move(sqls)},
            location{loc},
            trace{make_trace_ptr(std::move(tr))}
    {}

    /// Move `tr` into a `shared_ptr<st const>`.
    /** If no `std::stacktrace` support is available, return an empty pointer.
     * This makes it easier for users of `pqxx::failure` to detect the case
     * where `std::stacktrace` is not supported.
     */
    [[nodiscard]] static std::shared_ptr<st const>
    make_trace_ptr([[maybe_unused]] st &&tr)
    {
#if defined(PQXX_HAVE_STACKTRACE)
      // We have std::stacktrace support.  Move tr from the stack to the heap.
      return std::make_shared<st const>(std::move(tr));
#else
      // We don't have std::stacktrace.  Keep the pointer empty.
      return {};
#endif
    }
  };

  /// The data for this exception, as a shared pointer for easy copying.
  /** The "block" never, ever moves, changes, or disappears during the
   * exception object's lifetime.  That's why various functions can return
   * views or references pointing to the data in there.
   */
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
  explicit broken_connection(sl loc = sl::current(), st &&tr = st::current()) :
          failure{"Connection to database failed.", loc, std::move(tr)}
  {}

  explicit broken_connection(
    std::string const &whatarg, sl loc = sl::current(),
    st &&tr = st::current()) :
          failure{whatarg, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;

  /// By its nature, this type of error makes the connection unusable.
  bool poisons_connection() const noexcept override { return true; }

  /// When the connection breaks, so will an ongoing transaction.
  bool poisons_transaction() const noexcept override { return true; }
};


/// Could not establish connection due to version mismatch.
struct PQXX_LIBEXPORT version_mismatch : broken_connection
{
  explicit version_mismatch(
    std::string const &whatarg, sl loc = sl::current(),
    st &&tr = st::current()) :
          broken_connection{whatarg, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


/// The caller attempted to set a variable to null, which is not allowed.
struct PQXX_LIBEXPORT variable_set_to_null : failure
{
  explicit variable_set_to_null(
    std::string const &whatarg, sl loc = sl::current(),
    st &&tr = st::current()) :
          failure{whatarg, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


/// Exception class for failed queries.
/** Carries, in addition to a regular error message, a copy of the failed query
 * and (if available) the SQLSTATE value accompanying the error.
 *
 * These exception classes follow, roughly, the two-level hierarchy defined by
 * the PostgreSQL SQLSTATE error codes (see Appendix A of the PostgreSQL
 * documentation corresponding to your server version).  This is not a complete
 * mapping though.  There are other differences as well, e.g. the error code
 * for `statement_completion_unknown` has a separate status in libpqxx as
 * @ref in_doubt_error, and `too_many_connections` is classified as a
 * `broken_connection` rather than a subtype of `insufficient_resources`.
 */
struct PQXX_LIBEXPORT sql_error : public failure
{
  PQXX_ZARGS explicit sql_error(
    std::string const &whatarg = {}, std::string const &stmt = {},
    std::string const &sqls = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          failure{whatarg, stmt, sqls, loc, std::move(tr)}
  {}
  sql_error(sql_error const &other) = default;
  sql_error(sql_error &&other) = default;

  std::string_view name() const noexcept override;

  /// If a transaction was ongoing, an SQL error will break it.
  bool poisons_transaction() const noexcept override { return true; }
};


/// Exception class for mis-communication with the server.
/** This happens when the conversation between libpq and the server gets messed
 * up.  There aren't many situations where this happens, but one known instance
 * is when you call a parameterised or prepared statement with the wrong number
 * of parameters.
 *
 * When this happens, your connection will most likely be in a broken state and
 * you're probably best off discarding it and starting a new one.  In that
 * sense it is like @ref broken_connection.
 *
 * Retrying your statement is not likely to make this problem go away.
 */
struct PQXX_LIBEXPORT protocol_violation : sql_error
{
  explicit protocol_violation(
    std::string const &whatarg, std::string const &stmt = {},
    std::string const &sqls = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          sql_error{whatarg, stmt, sqls, loc, std::move(tr)}
  {}

  /// When this happens, the connection is in a confused state.
  bool poisons_connection() const noexcept override { return true; }

  /// Since the connection is broken, so is a transaction.
  bool poisons_transaction() const noexcept override { return true; }

  std::string_view name() const noexcept override;
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
  explicit in_doubt_error(
    std::string const &whatarg, sl loc = sl::current(),
    st &&tr = st::current()) :
          failure{whatarg, loc, std::move(tr)}
  {}

  /// This kind of error can only happen when the connection breaks.
  bool poisons_connection() const noexcept override { return true; }

  /// The transaction is already closed, and the connection is broken.
  bool poisons_transaction() const noexcept override { return true; }

  std::string_view name() const noexcept override;
};


/// The backend saw itself forced to roll back the ongoing transaction.
struct PQXX_LIBEXPORT transaction_rollback : sql_error
{
  PQXX_ZARGS explicit transaction_rollback(
    std::string const &whatarg, std::string const &q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          sql_error{whatarg, q, sqlstate, loc, std::move(tr)}
  {}

  /// Some earlier failure broke the transaction.
  bool poisons_transaction() const noexcept override { return true; }

  std::string_view name() const noexcept override;
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
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          transaction_rollback{whatarg, q, sqlstate, loc, std::move(tr)}
  {}

  /// To retry the transaction, you'll need to start a fresh one.
  bool poisons_transaction() const noexcept override { return true; }

  std::string_view name() const noexcept override;
};


/// We can't tell whether our last statement succeeded.
struct PQXX_LIBEXPORT statement_completion_unknown : transaction_rollback
{
  PQXX_ZARGS explicit statement_completion_unknown(
    std::string const &whatarg, std::string const &q,
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          transaction_rollback{whatarg, q, sqlstate, loc, std::move(tr)}
  {}

  /// It's not advisable to continue using the connection after this.
  bool poisons_connection() const noexcept override { return true; }

  std::string_view name() const noexcept override;
};


/// The ongoing transaction has deadlocked.  Retrying it may help.
struct PQXX_LIBEXPORT deadlock_detected : transaction_rollback
{
  PQXX_ZARGS explicit deadlock_detected(
    std::string const &whatarg, std::string const &q,
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          transaction_rollback{whatarg, q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


/// Internal error in libpqxx library
struct PQXX_LIBEXPORT internal_error : failure
{
  explicit internal_error(
    std::string const &, sl = sl::current(), st &&tr = st::current());

  /// When this happens, all bets are off.  It _may_ work, but don't risk it.
  bool poisons_connection() const noexcept override { return true; }

  /// When this happens, all bets are off.  It _may_ work, but don't risk it.
  bool poisons_transaction() const noexcept override { return true; }

  std::string_view name() const noexcept override;
};


/// Error in usage of libpqxx library, similar to std::logic_error
struct PQXX_LIBEXPORT usage_error : failure
{
  explicit usage_error(
    std::string const &whatarg, sl loc = sl::current(),
    st &&tr = st::current()) :
          failure{whatarg, loc, std::move(tr)}
  {}

  /// Your transaction will probably still work, but something is badly wrong.
  bool poisons_transaction() const noexcept override { return true; }

  std::string_view name() const noexcept override;
};


/// Invalid argument passed to libpqxx, similar to std::invalid_argument
struct PQXX_LIBEXPORT argument_error : failure
{
  explicit argument_error(
    std::string const &whatarg, sl loc = sl::current(),
    st &&tr = st::current()) :
          failure{whatarg, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


/// Value conversion failed, e.g. when converting "Hello" to int.
struct PQXX_LIBEXPORT conversion_error : failure
{
  explicit conversion_error(
    std::string const &whatarg, sl loc = sl::current(),
    st &&tr = st::current()) :
          failure{whatarg, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


/// Could not convert null value: target type does not support null.
struct PQXX_LIBEXPORT unexpected_null : conversion_error
{
  explicit unexpected_null(
    std::string const &whatarg, sl loc = sl::current(),
    st &&tr = st::current()) :
          conversion_error{whatarg, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


/// Could not convert value to string: not enough buffer space.
struct PQXX_LIBEXPORT conversion_overrun : conversion_error
{
  explicit conversion_overrun(
    std::string const &whatarg, sl loc = sl::current(),
    st &&tr = st::current()) :
          conversion_error{whatarg, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


/// Something is out of range, similar to std::out_of_range
struct PQXX_LIBEXPORT range_error : failure
{
  explicit range_error(
    std::string const &whatarg, sl loc = sl::current(),
    st &&tr = st::current()) :
          failure{whatarg, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


/// Query returned an unexpected number of rows.
struct PQXX_LIBEXPORT unexpected_rows : range_error
{
  explicit unexpected_rows(
    std::string const &msg, sl loc = sl::current(), st &&tr = st::current()) :
          range_error{msg, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


/// Database feature not supported in current setup.
struct PQXX_LIBEXPORT feature_not_supported : sql_error
{
  PQXX_ZARGS explicit feature_not_supported(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          sql_error{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;

  /// It all depends on the details, but this _can_ break your connection.
  bool poisons_connection() const noexcept override { return true; }

  /// If this poisons your connection, it also poisons your transaction.
  bool poisons_transaction() const noexcept override { return true; }
};


/// Error in data provided to SQL statement.
struct PQXX_LIBEXPORT data_exception : sql_error
{
  PQXX_ZARGS explicit data_exception(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          sql_error{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT integrity_constraint_violation : sql_error
{
  PQXX_ZARGS explicit integrity_constraint_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          sql_error{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT restrict_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit restrict_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT not_null_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit not_null_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT foreign_key_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit foreign_key_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT unique_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit unique_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT check_violation : integrity_constraint_violation
{
  PQXX_ZARGS explicit check_violation(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          integrity_constraint_violation{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT invalid_cursor_state : sql_error
{
  PQXX_ZARGS explicit invalid_cursor_state(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          sql_error{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT invalid_sql_statement_name : sql_error
{
  PQXX_ZARGS explicit invalid_sql_statement_name(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          sql_error{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT invalid_cursor_name : sql_error
{
  PQXX_ZARGS explicit invalid_cursor_name(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          sql_error{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT syntax_error : sql_error
{
  /// Approximate position in string where error occurred, or -1 if unknown.
  int const error_position;

  PQXX_ZARGS explicit syntax_error(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, int pos = -1, sl loc = sl::current(),
    st &&tr = st::current()) :
          sql_error{err, Q, sqlstate, loc, std::move(tr)}, error_position{pos}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT undefined_column : syntax_error
{
  PQXX_ZARGS explicit undefined_column(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          // TODO: Can we get the column?
          syntax_error{err, Q, sqlstate, -1, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT undefined_function : syntax_error
{
  PQXX_ZARGS explicit undefined_function(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          // TODO: Can we get the column?
          syntax_error{err, Q, sqlstate, -1, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT undefined_table : syntax_error
{
  PQXX_ZARGS explicit undefined_table(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          // TODO: Can we get the column?
          syntax_error{err, Q, sqlstate, -1, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT insufficient_privilege : sql_error
{
  PQXX_ZARGS explicit insufficient_privilege(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          sql_error{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


/// Resource shortage on the server.
struct PQXX_LIBEXPORT insufficient_resources : sql_error
{
  PQXX_ZARGS explicit insufficient_resources(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          sql_error{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT disk_full : insufficient_resources
{
  PQXX_ZARGS explicit disk_full(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          insufficient_resources{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT server_out_of_memory : insufficient_resources
{
  PQXX_ZARGS explicit server_out_of_memory(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          insufficient_resources{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT too_many_connections : broken_connection
{
  explicit too_many_connections(
    std::string const &err, sl loc = sl::current(), st &&tr = st::current()) :
          broken_connection{err, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


/// PL/pgSQL error.
/** Exceptions derived from this class are errors from PL/pgSQL procedures.
 */
struct PQXX_LIBEXPORT plpgsql_error : sql_error
{
  PQXX_ZARGS explicit plpgsql_error(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          sql_error{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


/// Exception raised in PL/pgSQL procedure
struct PQXX_LIBEXPORT plpgsql_raise : plpgsql_error
{
  PQXX_ZARGS explicit plpgsql_raise(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          plpgsql_error{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT plpgsql_no_data_found : plpgsql_error
{
  PQXX_ZARGS explicit plpgsql_no_data_found(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          plpgsql_error{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};


struct PQXX_LIBEXPORT plpgsql_too_many_rows : plpgsql_error
{
  PQXX_ZARGS explicit plpgsql_too_many_rows(
    std::string const &err, std::string const &Q = {},
    std::string const &sqlstate = {}, sl loc = sl::current(),
    st &&tr = st::current()) :
          plpgsql_error{err, Q, sqlstate, loc, std::move(tr)}
  {}

  std::string_view name() const noexcept override;
};

/**
 * @}
 */
} // namespace pqxx
#endif
