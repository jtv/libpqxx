/* Common code and definitions for the transaction classes.
 *
 * pqxx::transaction_base defines the interface for any abstract class that
 * represents a database transaction.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/transaction_base instead.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_TRANSACTION_BASE
#define PQXX_H_TRANSACTION_BASE

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

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
#include "pqxx/encoding_group.hxx"
#include "pqxx/internal/stream_query.hxx"
#include "pqxx/isolation.hxx"
#include "pqxx/prepared_statement.hxx"
#include "pqxx/result.hxx"
#include "pqxx/row.hxx"
#include "pqxx/util.hxx"

namespace pqxx::internal::gate
{
class transaction_subtransaction;
class transaction_sql_cursor;
class transaction_stream_to;
class transaction_transaction_focus;
} // namespace pqxx::internal::gate


namespace pqxx
{
using namespace std::literals;


class transaction_focus;


/**
 * @defgroup transactions Transaction classes
 *
 * All database access goes through instances of these classes.  In libpqxx
 * you can't execute SQL directly on the connection object; that all happens
 * only on a transaction object.  If you don't actually want to start a
 * transaction on the server, there's a @ref nontransaction class which
 * operates in _autocommit,_ i.e. without a transaction.
 *
 * (Why do you always need a transaction object?  It ended up being the cleaner
 * choice in terms of interface design.  It avoids a bunch of API maladies:
 * duplicating API between classes, messy inheritance, inviting mistakes by
 * making the transaction afterthought, and so on.)
 *
 * Like most other things in libpqxx, transactions follow RAII principles.
 * Creating a transaction object starts the transaction on the backend (if
 * appropriate), and to destroying one ends the transaction.  But there's one
 * extra step: if you want to make the transaction's changes permanent, you
 * need to _commit_ it before you destroy it.  If you destroy the transaction
 * object without committing, or if you call its `abort()` member function,
 * then any transaction type (other than @ref nontransaction) will roll back
 * its changes to the database instead.
 *
 * There is a choice of transaction types.  To start with you'll probably want
 * to use @ref pqxx::work, represents a regular, vanilla transaction with the
 * default isolation level.
 *
 * All the actual transaction functionality, including all the functions for
 * executing SQL statements, lives in the abstract @ref transaction_base class.
 * It defines the API for each type of transaction.  You create a transaction,
 * you use it by calling @ref transaction_base member functions, and then you
 * either commit or (in the case of failure) abort.  If you destroy your
 * transaction object without doing either, it automatically aborts.
 *
 * Once you're done with your transaction, you can start a new one using the
 * same connection.  But there can be only one main transaction going on on a
 * connection at any given time.  (You _can_ have more "nested" transactions,
 * but I'm not counting those as "main" transactions here.  See below.)
 *
 * The concrete transaction types, all derived from @ref transaction_base, are:
 *
 * First and foremost, the plain @ref pqxx::transaction template.  Template
 * parameters let you select isolation level, and whether it should be
 * read-only.  Two aliases are usually more convenient: @ref pqxx::work is a
 * regular, run-of-the-mill default transaction.  @ref pqxx::read_transaction
 * is a read-only transaction that will not let you modify the database.
 *
 * Then there's @ref nontransaction.  This one runs in autocommit, meaning
 * that we don't start any transaction at all.  (Technically in this mode each
 * SQL command runs in its own little transaction, hence the term
 * "autocommit."  There is no way to "undo" an SQL statement in this kind of
 * transaction.)  Autocommit is sometimes a bit faster, and sometimes a bit
 * slower.  Mainly you'll use it for specific operations that cannot be done
 * inside a database transaction, such as some kinds of schema changes.
 *
 * And then there's @ref pqxx::robusttransaction to help you deal with those
 * painful situations where you don't know for sure whether a transaction
 * actually succeeded.  This can happen if you lose your network connection to
 * the database _just_ while you're trying to commit your transaction, before
 * you receive word about the outcome.  You can re-connect and find out, but
 * what if the server is still executing the commit?
 *
 * You could say that @ref pqxx::robusttransaction is not more robust, exactly,
 * but it goes to some extra effort to try and figure these situations out and
 * give you clarity.  Extra effort does actually mean more things that can go
 * wrong, and it may be a little slower, so investigate carefully before using
 * this transaction class.
 *
 * All of the transaction types that actually begin and commit/abort on the
 * database itself are derived from @ref pqxx::dbtransaction, which can be a
 * useful type if your code needs a reference to such a transaction but doesn't
 * need to enforce a particular one.  These types are @ref transaction,
 * @ref pqxx::work, @ref pqxx::read_transaction, and
 * @ref pqxx::robusttransaction.
 *
 * Finally, there's @ref pqxx::subtransaction.  This one is not at all like the
 * others: it can only exist inside a @ref pqxx::dbtransaction.  (Which
 * includes @ref pqxx::subtransaction itself: you can nest them freely.)  You
 * can only operate on the "innermost" active subtransaction at any given time,
 * until you either commit or abort it.  Subtransactions are built on
 * _savepoints_ in the database; these are efficient to a point but do consume
 * some server resources.  So use them when they make sense, e.g. to try an SQL
 * statement but continue your main transaction if it fails.  But don't create
 * them in enormous numbers, or performance may start to suffer.
 */

/// Interface definition (and common code) for "transaction" classes.
/**
 * @ingroup transactions
 *
 * Abstract base class for all transaction types.
 */
class PQXX_LIBEXPORT PQXX_NOVTABLE transaction_base
{
public:
  transaction_base() = delete;
  transaction_base(transaction_base const &) = delete;
  transaction_base(transaction_base &&) = delete;
  transaction_base &operator=(transaction_base const &) = delete;
  transaction_base &operator=(transaction_base &&) = delete;

  virtual ~transaction_base() = 0;

  /// Commit the transaction.
  /** Make the effects of this transaction definite.  If you destroy a
   * transaction without invoking its @ref commit() first, that will implicitly
   * abort it.  (For the @ref nontransaction class though, "commit" and "abort"
   * really don't do anything, hence its name.)
   *
   * There is, however, a minute risk that you might lose your connection to
   * the database at just the wrong moment here.  In that case, libpqxx may be
   * unable to determine whether the database was able to complete the
   * transaction, or had to roll it back.  In that scenario, @ref commit() will
   * throw an in_doubt_error.  There is a different transaction class called
   * @ref robusttransaction which takes some special precautions to reduce this
   * risk.
   */
  void commit(sl = sl::current());

  /// Abort the transaction.
  /** No special effort is required to call this function; it will be called
   * implicitly when the transaction is destructed.
   */
  void abort(sl = sl::current());

  /**
   * @ingroup escaping-functions
   *
   * Use these when writing SQL queries that incorporate C++ values as SQL
   * constants.
   *
   * The functions you see here are just convenience shortcuts to the same
   * functions on the connection object.
   */
  //@{
  PQXX_ZARGS [[nodiscard]] std::string
  esc(char const str[], sl loc = sl::current())
  {
    return conn().esc(str, loc);
  }

  /// Escape string for use as SQL string literal in this transaction.
  [[nodiscard]] std::string_view esc(
    std::string_view text, std::span<char> buffer,
    sl loc = sl::current()) const
  {
    return conn().esc(text, buffer, loc);
  }

  /// Escape string for use as SQL string literal in this transaction.
  [[nodiscard]] std::string
  esc(std::string_view text, sl loc = sl::current()) const
  {
    return conn().esc(text, loc);
  }

  /// Escape binary string for use as SQL string literal.
  template<binary DATA> [[nodiscard]] std::string esc(DATA const &data) const
  {
    return conn().esc(data);
  }

  /// Escape binary string for use as SQL string literal, into `buffer`.
  /** Use this variant when you want to re-use the same buffer across multiple
   * calls.  If that's not the case, or convenience and simplicity are more
   * important, use the single-argument variant.
   *
   * For every byte in `data`, there must be at least two bytes of space in
   * `buffer`; plus there must be two bytes of space for a header and one for
   * a trailing zero.  Throws @ref range_error if this space is not available.
   *
   * Returns a reference to the escaped string, which is actually stored in
   * `buffer`.
   */
  template<binary DATA>
  [[nodiscard]] zview
  esc(DATA const &data, std::span<char> buffer, sl loc = sl::current()) const
  {
    return conn().esc(data, buffer, loc);
  }

  /// Escape binary string for use as SQL string literal, into `buffer`.
  template<binary DATA>
  [[nodiscard,
    deprecated("Use esc(), not esc_raw(), even on binary data.")]] zview
  esc_raw(DATA const &data, sl loc = sl::current()) const
  {
    return conn().esc(data, loc);
  }

  /// Escape binary string for use as SQL string literal, into `buffer`.
  template<binary DATA>
  [[nodiscard,
    deprecated("Use esc(), not esc_raw(), even on binary data.")]] zview
  esc_raw(
    DATA const &data, std::span<char> buffer, sl loc = sl::current()) const
  {
    return conn().esc(data, buffer, loc);
  }

  /// Unescape binary data, e.g. from a `bytea` field.
  /** Takes a binary string as escaped by PostgreSQL, and returns a restored
   * copy of the original binary data.
   */
  [[nodiscard]] bytes unesc_bin(zview text, sl loc = sl::current())
  {
    return conn().unesc_bin(text, loc);
  }

  /// Unescape binary data, e.g. from a `bytea` field.
  /** Takes a binary string as escaped by PostgreSQL, and returns a restored
   * copy of the original binary data.
   */
  PQXX_ZARGS [[nodiscard]] bytes
  unesc_bin(char const text[], sl loc = sl::current())
  {
    return conn().unesc_bin(text, loc);
  }

  // TODO: "Into buffer" variant.
  /// Represent object as SQL string, including quoting & escaping.
  /** Nulls are recognized and represented as SQL nulls. */
  template<typename T>
  [[nodiscard]] std::string quote(T const &t, sl loc = sl::current()) const
  {
    return conn().quote(t, loc);
  }

  /// Binary-escape and quote a binary string for use as an SQL constant.
  /** For binary data you can also just use @ref quote(data). */
  template<binary DATA>
  [[nodiscard]] std::string
  quote_raw(DATA const &data, sl loc = sl::current()) const
  {
    return conn().quote_raw(data, loc);
  }

  // TODO: "Into buffer" variant.
  /// Escape an SQL identifier for use in a query.
  [[nodiscard]] std::string quote_name(std::string_view identifier) const
  {
    return conn().quote_name(identifier);
  }

  // TODO: "Into buffer" variant.
  /// Escape string for literal LIKE match.
  [[nodiscard]] std::string esc_like(
    std::string_view bin, char escape_char = '\\',
    sl loc = sl::current()) const
  {
    return conn().esc_like(bin, escape_char, loc);
  }
  //@}

  /**
   * @name Command execution
   *
   * There are many functions for executing (or "performing") a command (or
   * "query").  This is the most fundamental thing you can do in libpqxx, and
   * it always starts at a transaction class.
   *
   * Command execution can throw many types of exception, including sql_error,
   * broken_connection, and many sql_error subtypes such as
   * feature_not_supported or insufficient_privilege.  But any exception thrown
   * by the C++ standard library may also occur here.  All exceptions you will
   * see libpqxx throw are derived from std::exception.
   *
   * Most of the differences between the query execution functions are in how
   * they return the query's results.
   *
   * * The "query" functions run your query, wait for it to complete, and load
   *   all of the results into memory on the client side.  You can then access
   *   rows of result data, converted to C++ types that you request.
   * * The "stream" functions execute your query in a completely different way.
   *   Called _streaming queries,_ these don't support quite the full range of
   *   SQL queries, and they're a bit slower to start.  But they are
   *   significantly _faster_ for queries that return larger numbers of rows.
   *   They don't load the entire result set, so you can start processing data
   *   as soon as the first row of data comes in from the database.  This can
   *   This can save you a lot of time.  Processing itself may also be faster.
   *   And of course, it also means you don't need enough memory to hold the
   *   entire result set, just the row you're working on.
   * * The "exec" functions are a more low-level interface.  Most of them
   *   return a pqxx::result object.  This is an object that contains all
   *   information abouut the query's result: the data itself, but also the
   *   number of rows in the result, the column names, the number of rows that
   *   your query may have modified, and so on.
   */
  //@{

  /// Execute a command.
  /**
   * @param query Query or command to execute.
   * @return A result set describing the query's or command's result.
   */
  [[deprecated("The desc parameter is going away.")]]
  result
  exec(std::string_view query, std::string_view desc, sl = sl::current());

  // TODO: Wrap PQdescribePrepared().

  result
  exec(std::string_view query, params const &parms, sl loc = sl::current())
  {
    return internal_exec_params(query, parms.make_c_params(loc), loc);
  }

  /// Execute a command.
  /**
   * @param query Query or command to execute.
   * @return A result set describing the query's or command's result.
   */
  result exec(std::string_view query, sl = sl::current());

  /// Execute a command.
  /**
   * @param query Query or command to execute.
   * @param desc Optional identifier for query, to help pinpoint SQL errors.
   * @return A result set describing the query's or command's result.
   */
  [[deprecated(
    "Pass your query as a std::string_view, not stringstream.")]] result
  exec(std::stringstream const &query, std::string_view desc)
  {
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    return exec(query.str(), desc);
#include "pqxx/internal/ignore-deprecated-post.hxx"
  }

  /// Execute command, which should return zero rows of data.
  /** Works like @ref exec, but fails if the result contains data.  It still
   * returns a result, however, which may contain useful metadata.
   *
   * @throw unexpected_rows If the query returned the wrong number of rows.
   */
  [[deprecated("Use exec(string_view) and call no_rows() on the result.")]]
  result exec0(std::string_view query, std::string_view desc)
  {
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    return exec(query, desc).no_rows();
#include "pqxx/internal/ignore-deprecated-post.hxx"
  }

  /// Execute command, which should return zero rows of data.
  /** Works like @ref exec, but fails if the result contains data.  It still
   * returns a result, however, which may contain useful metadata.
   *
   * @throw unexpected_rows If the query returned the wrong number of rows.
   */
  [[deprecated("Use exec() and call no_rows() on the result.")]]
  result exec0(std::string_view query)
  {
    return exec(query).no_rows();
  }

  /// Execute command returning a single row of data.
  /** Works like @ref exec, but requires the result to contain exactly one row.
   * The row can be addressed directly, without the need to find the first row
   * in a result set.
   *
   * @throw unexpected_rows If the query returned the wrong number of rows.
   */
  [[deprecated("Use exec(string_view), and call one_row() on the result.")]]
  row exec1(std::string_view query, std::string_view desc)
  {
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    return exec(query, desc).one_row();
#include "pqxx/internal/ignore-deprecated-post.hxx"
  }

  /// Execute command returning a single row of data.
  /** Works like @ref exec, but requires the result to contain exactly one row.
   * The row can be addressed directly, without the need to find the first row
   * in a result set.
   *
   * @throw unexpected_rows If the query returned the wrong number of rows.
   */
  [[deprecated("Use exec() instead, and call one_row() on the result.")]]
  row exec1(std::string_view query)
  {
    return exec(query).one_row();
  }

  /// Execute command, expect given number of rows.
  /** Works like @ref exec, but checks that the result has exactly the expected
   * number of rows.
   *
   * @throw unexpected_rows If the query returned the wrong number of rows.
   */
  [[deprecated("Use exec() instead, and call expect_rows() on the result.")]]
  result exec_n(
    result::size_type rows, std::string_view query, std::string_view desc);

  /// Execute command, expect given number of rows.
  /** Works like @ref exec, but checks that the result has exactly the expected
   * number of rows.
   *
   * @throw unexpected_rows If the query returned the wrong number of rows.
   */
  [[deprecated("Use exec() instead, and call expect_rows() on the result.")]]
  result exec_n(result::size_type rows, std::string_view query)
  {
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    return exec(query, std::string_view{}).expect_rows(rows);
#include "pqxx/internal/ignore-deprecated-post.hxx"
  }

  /// Perform query, expecting exactly 1 row with 1 field, and convert it.
  /** This is convenience shorthand for querying exactly one value from the
   * database.  It returns that value, converted to the type you specify.
   */
  template<not_borrowed TYPE>
  [[deprecated("The desc parameter is going away.")]]
  TYPE query_value(std::string_view query, std::string_view desc)
  {
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    return exec(query, desc).one_field().as<TYPE>();
#include "pqxx/internal/ignore-deprecated-post.hxx"
  }

  /// Perform query, expecting exactly 1 row with 1 field, and convert it.
  /** This is convenience shorthand for querying exactly one value from the
   * database.  It returns that value, converted to the type you specify.
   *
   * @throw unexpected_rows If the query did not return exactly 1 row.
   * @throw usage_error If the row did not contain exactly 1 field.
   */
  template<not_borrowed TYPE>
  TYPE query_value(std::string_view query, sl loc = sl::current())
  {
    // The result and field_ref objects are temporaries, but under C++20
    // lifetime rules they'll live until after we convert the field to TYPE.
    return exec(query, loc).one_field_ref(loc).as<TYPE>(loc);
  }

  /// Perform query returning exactly one row, and convert its fields.
  /** This is a convenient way of querying one row's worth of data, and
   * converting its fields to a tuple of the C++-side types you specify.
   *
   * @throw unexpected_rows If the query did not return exactly 1 row.
   * @throw usage_error If the number of columns in the result does not match
   * the number of fields in the tuple.
   */
  template<not_borrowed... TYPE>
  [[nodiscard]] std::tuple<TYPE...>
  query1(std::string_view query, sl loc = sl::current())
  {
    // The result and row_ref objects are temporaries, but under C++20 lifetime
    // rules they'll live until after we convert the field to TYPE.
    return exec(query, loc)
      .expect_columns(sizeof...(TYPE), loc)
      .one_row_ref(loc)
      .as<TYPE...>(loc);
  }

  /// Query at most one row of data, and if there is one, convert it.
  /** If the query produced a row of data, this converts it to a tuple of the
   * C++ types you specify.  Otherwise, this returns no tuple.
   *
   * @throw unexpected_rows If the query returned more than 1 row.
   * @throw usage_error If the number of columns in the result does not match
   * the number of fields in the tuple.
   */
  template<not_borrowed... TYPE>
  [[nodiscard]] std::optional<std::tuple<TYPE...>>
  query01(std::string_view query, sl loc = sl::current())
  {
    auto const res{exec(query, loc)};
    std::optional<row_ref> const r{res.opt_row_ref(loc)};
    if (r)
      return {r->as<TYPE...>(loc)};
    else
      return {};
  }

  // C++20: Update type requirements (see bottom of docs for this function).
  /// Execute a query, in streaming fashion; loop over the results row by row.
  /** Converts the rows to `std::tuple`, of the column types you specify.
   *
   * Use this with a range-based "for" loop.  It executes the query, and
   * directly maps the resulting rows onto a `std::tuple` of the types you
   * specify.  Unlike with the "exec" functions, processing can start before
   * all the data from the server is in.
   *
   * @warning You can't pass parmaeters to a streaming query.  This is a
   * limitation at a lower level in the software stack.  You can work around
   * this by defining a view or function that you can then call without
   * parameters in the streaming query; or you can compose query strings that
   * wrap the parameters in calls to @ref pqxx::transaction_base::quote() or
   * @ref pqxx::transaction_base::esc().
   *
   * Streaming is also documented in @ref streams.
   *
   * The column types must all be types that have conversions from PostgreSQL's
   * text format defined.  Many built-in types such as `int` or `std::string`
   * have pre-defined conversions; if you want to define your own conversions
   * for additional types, see @ref datatypes.
   *
   * `TYPE...` may include `std::string_view` and @ref zview fields (which will
   * be highly efficient), but the strings to which they point will only remain
   * valid until you extract the next row.  After that, the memory holding the
   * string may be overwritten or deallocated.
   *
   * If any of the columns can be null, and you're converting it to a C++ type
   * which does not support a null value, you can wrap the type in a
   * `std::optional<>` (or if you prefer, a `std::shared_ptr<>` or a
   * `std::unique_ptr`).  These templates do support null values, and libpqxx
   * will know how to convert to them.
   *
   * The stream lives entirely within the lifetime of the transaction.  Make
   * sure you complete the stream before you destroy the transaction.  Until
   * the stream has finished, the transaction and the connection are in a
   * special state where they cannot be used for anything else.
   *
   * @warning If the stream fails, you will have to destroy the transaction
   * and the connection.  If this is a problem, use the "exec" functions
   * instead.
   *
   * Streaming your query is likely to be faster than the `exec()` methods for
   * queries that return lots of rows, but slower for ones that don't.  You'll
   * be able to start useful processing sooner, before all the result data is
   * in.  Also, `stream()` scales better in terms of memory usage: it only
   * needs to keep the current row in memory, not the whole result.  The
   * "exec" functions read the entire result into memory at once.
   *
   * Your query executes as part of a COPY command, not as a stand-alone query,
   * so there are limitations to what you can do in the query.  It can be
   * either a SELECT or VALUES query; or an INSERT, UPDATE, or DELETE with a
   * RETURNING clause.  See the documentation for PostgreSQL's
   * [COPY command](https://www.postgresql.org/docs/current/sql-copy.html) for
   * the exact restrictions.
   *
   * Iterating in this way does require each of the field types you pass to be
   * default-constructible, copy-constructible, and assignable.  These
   * requirements may loosen a bit once libpqxx moves on to C++20.
   */
  template<typename... TYPE>
  [[nodiscard]] auto stream(std::string_view query, sl loc = sl::current()) &
  {
    return pqxx::internal::stream_query<TYPE...>{
      *this, query, make_context(loc)};
  }

  /// Perform a streaming query, and for each result row, call `func`.
  /** Here, `func` can be a function, a `std::function`, a lambda, or an
   * object that supports the function call operator.  Of course `func` must
   * have an unambiguous signature; it can't be overloaded or generic.
   *
   * The `for_stream` function executes `query` in a stream similar to
   * @ref stream.  Every time a row of data comes in from the server, it
   * converts the row's fields to the types of `func`'s respective parameters,
   * and calls `func` with those values.
   *
   * This will not work for all queries, but straightforward `SELECT` and
   * `UPDATE ... RETURNING` queries should work.  Consult the documentation for
   * @ref pqxx::internal::stream_query and PostgreSQL's underlying `COPY`
   * command for the full details.
   *
   * Streaming a query like this is likely to be slower than the @ref exec()
   * functions for small result sets, but faster for larger result sets.  So if
   * performance matters, you'll want to use `for_stream` if you query large
   * amounts of data, but not if you do lots of queries with small outputs.
   *
   * However, the transaction and the connection are in a special state while
   * the iteration is ongoing.  If `func` throws an exception, or the iteration
   * fails in some way, the only way out is to destroy the transaction and the
   * connection.
   *
   * Each of the parameter types must have a conversion from PostgreSQL's text
   * format defined.  To define conversions for additional types, see
   * @ref datatypes.
   */
  template<typename CALLABLE>
  auto
  for_stream(std::string_view query, CALLABLE &&func, sl loc = sl::current())
  {
    // TODO: Can we pass loc into func if appropriate?
    using param_types =
      pqxx::internal::strip_types_t<pqxx::internal::args_t<CALLABLE>>;
    param_types const *const sample{nullptr};
    auto data_stream{stream_like(query, sample, loc)};
    for (auto const &fields : data_stream) std::apply(func, fields);
  }

  template<typename CALLABLE>
  [[deprecated(
    "pqxx::transaction_base::for_each is now called for_stream.")]] auto
  for_each(std::string_view query, CALLABLE &&func, sl loc = sl::current())
  {
    return for_stream(query, std::forward<CALLABLE>(func), loc);
  }

  /// Execute query, read full results, then iterate rows of data.
  /** Converts each row of the result to a `std::tuple` of the types you pass
   * as template arguments.  (The number of template arguments must match the
   * number of columns in the query's result.)
   *
   * Example:
   *
   * ```cxx
   *     for (
   *         auto [name, salary] :
   *             tx.query<std::string_view, int>(
   *                 "SELECT name, salary FROM employee"
                 )
   *     )
   *         std::cout << name << " earns " << salary << ".\n";
   * ```
   *
   * You can't normally convert a field value to `std::string_view`, but this
   * is one of the places where you can.  The underlying string to which the
   * `string_view` points exists only for the duration of the one iteration.
   * After that, the buffer that holds the actual string may have disappeared,
   * or it may contain a new string value.
   *
   * If you expect a lot of rows from your query, it's probably faster to use
   * transaction_base::stream() instead.  Or if you need to access metadata of
   * the result, such as the number of rows in the result, or the number of
   * rows that your query updates, then you'll need to use
   * transaction_base::exec() instead.
   *
   * @return Something you can iterate using "range `for`" syntax.  The actual
   * type details may change.
   */
  template<typename... TYPE>
  auto query(std::string_view query, sl loc = sl::current())
  {
    return exec(query, loc).iter<TYPE...>();
  }

  /// Perform query, expect given number of rows, iterate results.
  template<typename... TYPE>
  [[deprecated("Use query() instead, and call expect_rows() on the result.")]]
  auto query_n(
    result::size_type rows, std::string_view query, sl loc = sl::current())
  {
    return exec(query, loc).expect_rows(rows, loc).iter<TYPE...>();
  }

  /// Execute a query, load the full result, and perform `func` for each row.
  /** Converts each row to data types matching `func`'s parameter types.  The
   * number of columns in the result set must match the number of parameters.
   *
   * This is a lot like for_stream().  The differences are:
   * 1. It can execute some unusual queries that for_stream() can't.
   * 2. The `exec` functions are faster for small results, but slower for large
   *    results.
   */
  template<typename CALLABLE>
  void
  for_query(std::string_view query, CALLABLE &&func, sl loc = sl::current())
  {
    exec(query).for_each(std::forward<CALLABLE>(func), loc);
  }

  /**
   * @name Parameterized statements
   *
   * You'll often need parameters in the queries you execute: "select the
   * car with this licence plate."  If the parameter is a string, you need to
   * quote it and escape any special characters inside it, or it may become a
   * target for an SQL injection attack.  If it's an integer (for example),
   * you need to convert it to a string, but in the database's format, without
   * locale-specific niceties such as "," separators between the thousands.
   *
   * Parameterised statements are an easier and safer way to do this.  They're
   * like prepared statements, but for a single use.  You don't need to name
   * them, and you don't need to prepare them first.
   *
   * Your query will include placeholders like `$1` and `$2` etc. in the places
   * where you want the arguments to go.  Then, you pass the argument values
   * and the actual query is constructed for you.
   *
   * Pass the exact right number of parameters, and in the right order.  The
   * parameters in the query don't have to be neatly ordered from `$1` to
   * `$2` to `$3` - but you must pass the argument for `$1` first, the one
   * for `$2` second, etc.
   *
   * @warning Beware of "nul" bytes.  Any string you pass as a parameter will
   * end at the first char with value zero.  If you pass a string that contains
   * a zero byte, the last byte in the value will be the one just before the
   * zero.
   */
  //@{

  /// Execute an SQL statement with parameters.
  /** This is like calling `exec()`, except it will substitute the first
   * parameter after `query` (the first in `args`) for a `$1` in the query, the
   * next one for `$2`, etc.
   */
  template<typename... Args>
  [[deprecated("Use exec(std::string_view, params) instead.")]]
  result exec_params(std::string_view query, Args &&...args)
  {
    return exec(query, params{*this, args...}, sl::current());
  }

  // Execute parameterised statement, expect a single-row result.
  /** @throw unexpected_rows if the result does not consist of exactly one row.
   */
  template<typename... Args>
  [[deprecated("Use exec() instead, and call one_row() on the result.")]]
  row exec_params1(std::string_view query, Args &&...args)
  {
    return exec(query, params{*this, args...}).one_row();
  }

  // Execute parameterised statement, expect a result with zero rows.
  /** @throw unexpected_rows if the result contains rows.
   */
  template<typename... Args>
  [[deprecated(
    "Use exec(string_view, params) and call no_rows() on the result.")]]
  result exec_params0(std::string_view query, Args &&...args)
  {
    return exec(query, params{*this, args...}).no_rows();
  }

  // Execute parameterised statement, expect exactly a given number of rows.
  /** @throw unexpected_rows if the result contains the wrong number of rows.
   */
  template<typename... Args>
  [[deprecated("Use exec(), and call expect_rows() on the result.")]]
  result
  exec_params_n(std::size_t rows, std::string_view query, Args &&...args)
  {
    sl loc{m_created_loc};
    return exec(query, params{*this, args...}, loc)
      .expect_rows(
        check_cast<result_size_type>(rows, "number of rows", loc), loc);
  }

  // Execute parameterised statement, expect exactly a given number of rows.
  /** @throw unexpected_rows if the result contains the wrong number of rows.
   */
  template<typename... Args>
  [[deprecated("Use exec(), and call expect_rows() on the result.")]]
  result
  exec_params_n(result::size_type rows, std::string_view query, Args &&...args)
  {
    return exec(query, params{*this, args...}).expect_rows(rows);
  }

  /// Execute parameterised query, read full results, iterate rows of data.
  /** Like @ref query, but the query can contain parameters.
   *
   * Converts each row of the result to a `std::tuple` of the types you pass
   * as template arguments.  (The number of template arguments must match the
   * number of columns in the query's result.)
   *
   * Example:
   *
   * ```cxx
   *     for (
   *         auto [name, salary] :
   *             tx.query<std::string_view, int>(
   *                 "SELECT name, salary FROM employee"
                 )
   *     )
   *         std::cout << name << " earns " << salary << ".\n";
   * ```
   *
   * You can't normally convert a field value to `std::string_view`, but this
   * is one of the places where you can.  The underlying string to which the
   * `string_view` points exists only for the duration of the one iteration.
   * After that, the buffer that holds the actual string may have disappeared,
   * or it may contain a new string value.
   *
   * If you expect a lot of rows from your query, it's probably faster to use
   * transaction_base::stream() instead.  Or if you need to access metadata of
   * the result, such as the number of rows in the result, or the number of
   * rows that your query updates, then you'll need to use
   * transaction_base::exec() instead.
   *
   * @return Something you can iterate using "range `for`" syntax.  The actual
   * type details may change.
   */
  template<typename... TYPE>
  auto
  query(std::string_view query, params const &parms, sl loc = sl::current())
  {
    return exec(query, parms, loc).iter<TYPE...>();
  }

  /// Perform query parameterised, expect given number of rows, iterate
  /// results.
  /** Works like @ref query, but checks that the result has exactly the
   * expected number of rows.
   *
   * @throw unexpected_rows If the query returned the wrong number of rows.
   *
   * @return Something you can iterate using "range `for`" syntax.  The actual
   * type details may change.
   */
  template<typename... TYPE>
  [[deprecated("Use exec(), and call expect_rows() & iter() on the result.")]]
  auto
  query_n(result::size_type rows, std::string_view query, params const &parms)
  {
    return exec(query, parms).expect_rows(rows).iter<TYPE...>();
  }

  /// Perform query, expecting exactly 1 row with 1 field, and convert it.
  /** This is convenience shorthand for querying exactly one value from the
   * database.  It returns that value, converted to the type you specify.
   *
   * @throw unexpected_rows If the query did not return exactly 1 row.
   * @throw usage_error If the row did not contain exactly 1 field.
   */
  template<not_borrowed TYPE>
  TYPE query_value(
    std::string_view query, params const &parms, sl loc = sl::current())
  {
    // The result and field_ref objects are temporaries, but under C++20
    // lifetime rules they'll live until after we convert the field to TYPE.
    return exec(query, parms, loc)
      .expect_columns(1, loc)
      .one_field_ref(loc)
      .as<TYPE>(loc);
  }

  /// Perform query returning exactly one row, and convert its fields.
  /** This is a convenient way of querying one row's worth of data, and
   * converting its fields to a tuple of the C++-side types you specify.
   *
   * @throw unexpected_rows If the query did not return exactly 1 row.
   * @throw usage_error If the number of columns in the result does not match
   * the number of fields in the tuple.
   */
  template<not_borrowed... TYPE>
  [[nodiscard]]
  std::tuple<TYPE...>
  query1(std::string_view query, params const &parms, sl loc = sl::current())
  {
    // The result and row_ref objects are temporaries, but under C++20 lifetime
    // rules they'll live until after we convert the field to TYPE.
    return exec(query, parms, loc).one_row_ref(loc).as<TYPE...>(loc);
  }

  /// Query at most one row of data, and if there is one, convert it.
  /** If the query produced a row of data, this converts it to a tuple of the
   * C++ types you specify.  Otherwise, this returns no tuple.
   *
   * @throw unexpected_rows If the query returned more than 1 row.
   * @throw usage_error If the number of columns in the result does not match
   * the number of fields in the tuple.
   */
  template<not_borrowed... TYPE>
  [[nodiscard]] std::optional<std::tuple<TYPE...>>
  query01(std::string_view query, params const &parms, sl loc = sl::current())
  {
    auto const res{exec(query, parms, loc)};
    std::optional<row_ref> r{res.opt_row_ref(loc)};
    if (r)
      return {r->as<TYPE...>(loc)};
    else
      return {};
  }

  /// Execute a query, load the full result, and perform `func` for each row.
  /** The query may use parameters.  So for example, the query may contain `$1`
   * to denote the first parameter value in `parms`, and so on.
   *
   * Converts each row to data types matching `func`'s parameter types.  The
   * number of columns in the result set must match the number of parameters.
   *
   * This is a lot like for_stream().  The differences are:
   * 1. It can execute some unusual queries that for_stream() can't.
   * 2. The `exec` functions are faster for small results, but slower for large
   *    results.
   */
  template<typename CALLABLE>
  void for_query(
    std::string_view query, CALLABLE &&func, params const &parms,
    sl loc = sl::current())
  {
    exec(query, parms, loc).for_each(std::forward<CALLABLE>(func), loc);
  }

  /// Send a notification.
  /** Convenience shorthand for executing a "NOTIFY" command.  Most of the
   * logic for handling _incoming_ notifications is in @ref pqxx::connection
   * (particularly @ref pqxx::connection::listen), but _outgoing_
   * notifications happen here.
   *
   * Unless this transaction is a nontransaction, the actual notification only
   * goes out once the outer transaction is committed.
   *
   * @param channel Name of the "channel" on which clients will need to be
   * listening in order to receive this notification.
   *
   * @param payload Optional argument string which any listeners will also
   * receive.  If you leave this out, they will receive an empty string as the
   * payload.
   */
  void notify(
    std::string_view channel, std::string_view payload, sl = sl::current());

  /// Send a notification (without payload).
  /** Convenience shorthand for executing a "NOTIFY" command.  Most of the
   * logic for handling _incoming_ notifications is in @ref pqxx::connection
   * (particularly @ref pqxx::connection::listen), but _outgoing_
   * notifications happen here.
   *
   * Unless this transaction is a nontransaction, the actual notification only
   * goes out once the outer transaction is committed.
   *
   * @param channel Name of the "channel" on which clients will need to be
   * listening in order to receive this notification.
   */
  void notify(std::string_view channel, sl loc = sl::current())
  {
    notify(channel, {}, loc);
  }
  //@}

  /// Execute a prepared statement, with optional arguments.
  template<typename... Args>
  [[deprecated("Use exec(prepped, params) instead.")]]
  result exec_prepared(zview statement, Args &&...args)
  {
    return exec(prepped{statement}, params{*this, args...});
  }

  /// Execute a prepared statement taking no parameters.
  result exec(prepped statement, sl loc = sl::current())
  {
    params pp;
    return internal_exec_prepared(statement, pp.make_c_params(loc), loc);
  }

  /// Execute prepared statement, read full results, iterate rows of data.
  /** Like @ref query(std::string_view), but using a prepared statement.
   *
   * @return Something you can iterate using "range `for`" syntax.  The actual
   * type details may change.
   */
  template<typename... TYPE>
  auto query(prepped statement, params const &parms, sl loc = sl::current())
  {
    return exec(statement, parms, loc).iter<TYPE...>();
  }

  /// Execute prepared statement, read full results, iterate rows of data.
  /** Like @ref query(std::string_view), but using a prepared statement.
   *
   * @return Something you can iterate using "range `for`" syntax.  The actual
   * type details may change.
   */
  template<typename... TYPE>
  auto query(prepped statement, sl loc = sl::current())
  {
    return exec(statement, {}, loc).iter<TYPE...>();
  }

  /// Perform prepared statement returning exactly 1 value.
  /** This is just like @ref query_value(std::string_view), but using a
   * prepared statement.
   */
  template<not_borrowed TYPE>
  TYPE
  query_value(prepped statement, params const &parms, sl loc = sl::current())
  {
    // The result and field_ref objects are temporaries, but under C++20
    // lifetime rules they'll live until after we convert the field to TYPE.
    return exec(statement, parms, loc)
      .expect_columns(1, loc)
      .one_field_ref(loc)
      .as<TYPE>(loc);
  }

  /// Perform prepared statement returning exactly 1 value.
  /** This is just like @ref query_value(std::string_view), but using a
   * prepared statement.
   */
  template<not_borrowed TYPE>
  TYPE query_value(prepped statement, sl loc = sl::current())
  {
    return exec(statement, {}, loc)
      .expect_columns(1, loc)
      .one_field_ref(loc)
      .as<TYPE>(loc);
  }

  /// Execute prepared statement, load result, perform `func` for each row.
  /** This is just like @ref for_query(std::string_view), but using a prepared
   * statement.
   */
  template<typename CALLABLE>
  void for_query(
    prepped statement, CALLABLE &&func, params const &parms,
    sl loc = sl::current())
  {
    exec(statement, parms, loc).for_each(std::forward<CALLABLE>(func), loc);
  }

  /// Execute prepared statement, load result, perform `func` for each row.
  /** This is just like @ref for_query(std::string_view), but using a prepared
   * statement.
   */
  template<typename CALLABLE>
  void for_query(prepped statement, CALLABLE &&func, sl loc = sl::current())
  {
    exec(statement, {}, loc).for_each(std::forward<CALLABLE>(func), loc);
  }

  /// Execute a prepared statement with parameters.
  result exec(prepped statement, params const &parms, sl loc = sl::current())
  {
    return internal_exec_prepared(statement, parms.make_c_params(loc), loc);
  }

  /// Execute a prepared statement, and expect a single-row result.
  /** @throw pqxx::unexpected_rows if the result was not exactly 1 row.
   */
  template<typename... Args>
  [[deprecated(
    "Use exec(string_view, params) and call one_row() on the result.")]]
  row exec_prepared1(zview statement, Args &&...args)
  {
    sl loc{m_created_loc};
    return exec(prepped{statement}, params{*this, args...}).one_row(loc);
  }

  /// Execute a prepared statement, and expect a result with zero rows.
  /** @throw pqxx::unexpected_rows if the result contained rows.
   */
  template<typename... Args>
  [[deprecated(
    "Use exec(prepped, params), and call no_rows() on the result.")]]
  result exec_prepared0(zview statement, Args &&...args)
  {
    sl loc{m_created_loc};
    return exec(prepped{statement}, params{*this, args...}).no_rows(loc);
  }

  /// Execute a prepared statement, expect a result with given number of rows.
  /** @throw pqxx::unexpected_rows if the result did not contain exactly the
   *  given number of rows.
   */
  template<typename... Args>
  [[deprecated(
    "Use exec(prepped, params), and call expect_rows() on the result.")]]
  result
  exec_prepared_n(result::size_type rows, zview statement, Args &&...args)
  {
    sl loc{m_created_loc};
    return exec(pqxx::prepped{statement}, params{*this, args...})
      .expect_rows(rows, loc);
  }

  /**
   * @name Error/warning output
   */
  //@{
  /// Have connection process a warning message.
  PQXX_ZARGS void process_notice(char const msg[]) const
  {
    m_conn.process_notice(msg);
  }
  /// Have connection process a warning message.
  void process_notice(zview msg) const { m_conn.process_notice(msg); }
  //@}

  /// The connection in which this transaction lives.
  [[nodiscard]] constexpr connection &conn() const noexcept { return m_conn; }

  /// Set session variable using SQL "SET" command.
  /** @deprecated To set a transaction-local variable, execute an SQL `SET`
   * command.  To set a session variable, use the connection's
   * @ref set_session_var function.
   *
   * @warning When setting a string value, you must make sure that the string
   * is "safe."  If you call @ref quote() on the string, it will return a
   * safely escaped and quoted version for use as an SQL literal.
   *
   * @warning This function executes SQL.  Do not try to set or get variables
   * while a pipeline or table stream is active.
   *
   * @param var The variable to set.
   * @param value The new value to store in the variable.  This can be any SQL
   * expression.
   */
  [[deprecated("Set transaction-local variables using SQL SET statements.")]]
  void set_variable(std::string_view var, std::string_view value);

  /// Read session variable using SQL "SHOW" command.
  /** @warning This executes SQL.  Do not try to set or get variables while a
   * pipeline or table stream is active.
   */
  [[deprecated("Read variables using SQL SHOW statements.")]]
  std::string get_variable(std::string_view);

  /// Transaction name, if you passed one to the constructor; or empty string.
  [[nodiscard]] constexpr std::string_view name() const & noexcept
  {
    return m_name;
  }

protected:
  /// Create a transaction (to be called by implementation classes only).
  /** The name, if nonempty, must begin with a letter and may contain letters
   * and digits only.
   */
  transaction_base(
    connection &, std::string_view, std::shared_ptr<std::string> rollback_cmd,
    sl loc = sl::current());

  /// Create a transaction (to be called by implementation classes only).
  /** Its rollback command will be "ROLLBACK".
   *
   * The name, if nonempty, must begin with a letter and may contain letters
   * and digits only.
   */
  transaction_base(
    connection &cx, std::string_view tname, sl loc = sl::current());

  /// Create a transaction (to be called by implementation classes only).
  explicit transaction_base(connection &cx, sl loc = sl::current());

  /// Register this transaction with the connection.
  void register_transaction();

  /// End transaction.  To be called by implementing class' destructor.
  void close(sl = sl::current()) noexcept;

  /// To be implemented by derived implementation class: commit transaction.
  virtual void do_commit(sl) = 0;

  /// Transaction type-specific way of aborting a transaction.
  /** @warning This will become "final", since this function can be called
   * from the implementing class destructor.
   */
  virtual void do_abort(sl);

  /// Set the rollback command.
  void set_rollback_cmd(std::shared_ptr<std::string> cmd)
  {
    m_rollback_cmd = std::move(cmd);
  }

  /// Execute query on connection directly.
  result direct_exec(std::string_view, std::string_view desc, sl);
  result direct_exec(std::string_view query, sl loc)
  {
    return direct_exec(query, "", loc);
  }
  result direct_exec(std::shared_ptr<std::string>, std::string_view desc, sl);
  result direct_exec(std::shared_ptr<std::string> query, sl loc)
  {
    return direct_exec(std::move(query), "", loc);
  }

  // TODO: Can this be noexcept?
  /// The `std::source_location` for this transaction's creation.
  [[nodiscard]] sl created_loc() const { return m_created_loc; }

private:
  enum class status
  {
    active,
    aborted,
    committed,
    in_doubt
  };

  /// Compose a @ref conversion_context.
  /** Gets its @ref encoding_group from the @ref connection, but uses the
   * `std::source_location` that you pass.
   */
  conversion_context make_context(sl) const;

  PQXX_PRIVATE void check_pending_error();

  result internal_exec_prepared(
    std::string_view statement, internal::c_params const &args, sl);

  result internal_exec_params(
    std::string_view query, internal::c_params const &args, sl);

  /// Describe this transaction to humans, e.g. "transaction 'foo'".
  [[nodiscard]] std::string description() const;

  friend class pqxx::internal::gate::transaction_transaction_focus;
  PQXX_PRIVATE void register_focus(transaction_focus *);
  PQXX_PRIVATE void unregister_focus(transaction_focus *) noexcept;
  PQXX_PRIVATE void register_pending_error(zview, sl) noexcept;
  PQXX_PRIVATE void register_pending_error(std::string &&, sl) noexcept;

  /// Like @ref stream(), but takes a tuple rather than a parameter pack.
  template<typename... ARGS>
  auto stream_like(
    std::string_view query, std::tuple<ARGS...> const *,
    sl loc = sl::current())
  {
    return stream<ARGS...>(query, loc);
  }

  connection &m_conn;

  /// Current "focus": a pipeline, a nested transaction, a stream...
  /** This pointer is used for only one purpose: sanity checks against mistakes
   * such as opening one while another is still active.
   */
  transaction_focus const *m_focus = nullptr;

  std::string m_name;
  std::string m_pending_error;

  /// SQL command for aborting this type of transaction.
  std::shared_ptr<std::string> m_rollback_cmd;

  status m_status = status::active;

  bool m_registered = false;

  /// A `std::source_location` for where this transaction object was created.
  sl m_created_loc;

  static constexpr std::string_view s_type_name{"transaction"sv};
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
  "BEGIN"_zv};
template<>
inline constexpr zview begin_cmd<read_committed, write_policy::read_only>{
  "BEGIN READ ONLY"_zv};
template<>
inline constexpr zview begin_cmd<repeatable_read, write_policy::read_write>{
  "BEGIN ISOLATION LEVEL REPEATABLE READ"_zv};
template<>
inline constexpr zview begin_cmd<repeatable_read, write_policy::read_only>{
  "BEGIN ISOLATION LEVEL REPEATABLE READ READ ONLY"_zv};
template<>
inline constexpr zview begin_cmd<serializable, write_policy::read_write>{
  "BEGIN ISOLATION LEVEL SERIALIZABLE"_zv};
template<>
inline constexpr zview begin_cmd<serializable, write_policy::read_only>{
  "BEGIN ISOLATION LEVEL SERIALIZABLE READ ONLY"_zv};
} // namespace pqxx::internal

#include "pqxx/internal/stream_query_impl.hxx"
#endif
