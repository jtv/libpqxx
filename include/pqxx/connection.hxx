/* Definition of the connection class.
 *
 * pqxx::connection encapsulates a connection to a database.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/connection instead.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_CONNECTION
#define PQXX_H_CONNECTION

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include <ctime>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string_view>
#include <tuple>

#if __has_include(<ranges>)
#  include <ranges>
#endif

#include "pqxx/errorhandler.hxx"
#include "pqxx/except.hxx"
#include "pqxx/prepared_statement.hxx"
#include "pqxx/strconv.hxx"
#include "pqxx/util.hxx"
#include "pqxx/zview.hxx"


/**
 * @addtogroup connection
 *
 * Use of the libpqxx library starts here.
 *
 * Everything that can be done with a database through libpqxx must go through
 * a @c connection object.  It connects to a database when you create it, and
 * it terminates that communication during destruction.
 *
 * Many things come together in this class.  Handling of error and warning
 * messages, for example, is defined by @c errorhandler objects in the context
 * of a connection.  Prepared statements are also defined here.
 *
 * When you connect to a database, you pass a connection string containing any
 * parameters and options, such as the server address and the database name.
 *
 * These are identical to the ones in libpq, the C language binding upon which
 * libpqxx itself is built:
 *
 * https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-CONNSTRING
 *
 * There are also environment variables you can set to provide defaults, again
 * as defined by libpq:
 *
 * https://www.postgresql.org/docs/current/libpq-envars.html
 */

namespace pqxx::internal
{
class sql_cursor;

#if defined(PQXX_HAVE_CONCEPTS)
/// Concept: T is a range of pairs of zero-terminated strings.
template<typename T>
concept ZKey_ZValues = std::ranges::input_range<T> and requires()
{
  {std::tuple_size<typename std::ranges::iterator_t<T>::value_type>::value};
}
and std::tuple_size_v<typename std::ranges::iterator_t<T>::value_type> == 2 and
  requires(T t)
{
  {
    std::get<0>(*std::cbegin(t))
  }
  ->ZString;
  {
    std::get<1>(*std::cbegin(t))
  }
  ->ZString;
};
#endif // PQXX_HAVE_CONCEPTS
} // namespace pqxx::internal


namespace pqxx::internal::gate
{
class connection_dbtransaction;
class connection_errorhandler;
class connection_largeobject;
class connection_notification_receiver;
class connection_pipeline;
class connection_sql_cursor;
class connection_stream_from;
class connection_stream_to;
class connection_transaction;
class const_connection_largeobject;
} // namespace pqxx::internal::gate


namespace pqxx
{
/// Encrypt a password.  @deprecated Use connection::encrypt_password instead.
[[nodiscard]] std::string PQXX_LIBEXPORT
encrypt_password(char const user[], char const password[]);

/// Encrypt password.  @deprecated Use connection::encrypt_password instead.
[[nodiscard]] inline std::string
encrypt_password(std::string const &user, std::string const &password)
{
  return encrypt_password(user.c_str(), password.c_str());
}


/// Error verbosity levels.
enum class error_verbosity : int
{
  // These values must match those in libpq's PGVerbosity enum.
  terse = 0,
  normal = 1,
  verbose = 2
};


/// Connection to a database.
/** This is the first class to look at when you wish to work with a database
 * through libpqxx.  The connection opens during construction, and closes upon
 * destruction.
 *
 * When creating a connection, you can pass a connection URI or a postgres
 * connection string, to specify the database server's address, a login
 * username, and so on.  If none is given, the connection will try to obtain
 * them from certain environment variables.  If those are not set either, the
 * default is to try and connect to the local system's port 5432.
 *
 * Find more about connection strings here:
 *
 * https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-CONNSTRING
 *
 * The variables are documented here:
 *
 * https://www.postgresql.org/docs/current/libpq-envars.html
 *
 * To query or manipulate the database once connected, use one of the
 * transaction classes (see pqxx/transaction_base.hxx) and perhaps also the
 * transactor framework (see pqxx/transactor.hxx).
 *
 * When a connection breaks, you will typically get a broken_connection
 * exception.  This can happen at almost any point.
 *
 * @warning On Unix-like systems, including GNU and BSD systems, your program
 * may receive the SIGPIPE signal when the connection to the backend breaks. By
 * default this signal will abort your program.  Use "signal(SIGPIPE, SIG_IGN)"
 * if you want your program to continue running after a connection fails.
 */
class PQXX_LIBEXPORT connection
{
public:
  connection() : connection{""} {}

  explicit connection(std::string const &options) : connection{options.c_str()}
  {
    // (Delegates to other constructor which calls check_version for us.)
  }

  explicit connection(char const options[])
  {
    check_version();
    init(options);
  }

  explicit connection(zview options) : connection{options.c_str()}
  {
    // (Delegates to other constructor which calls check_version for us.)
  }

  /// Move constructor.
  /** Moving a connection is not allowed if it has an open transaction, or has
   * error handlers or notification receivers registered on it.  In those
   * situations, other objects may hold references to the old object which
   * would become invalid and might produce hard-to-diagnose bugs.
   */
  connection(connection &&rhs);

#if defined(PQXX_HAVE_CONCEPTS)
  /// Connect, passing options as a range of key/value pairs.
  /** @warning Experimental.  Requires C++20 "concepts" support.  Define
   * @c PQXX_HAVE_CONCEPTS to enable it.
   *
   * There's no need to escape the parameter values.
   *
   * See the PostgreSQL libpq documentation for the full list of possible
   * options:
   *
   * https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-PARAMKEYWORDS
   *
   * The options can be anything that can be iterated as a series of pairs of
   * zero-terminated strings: @c std::pair<std::string, std::string>`, or
   * @c std::tuple<pqxx::zview, char const *>, or
   * @c std::map<std::string, pqxx::zview>, and so on.
   */
  template<internal::ZKey_ZValues MAPPING>
  inline connection(MAPPING const &params);
#endif // PQXX_HAVE_CONCEPTS

  ~connection()
  {
    try
    {
      close();
    }
    catch (std::exception const &)
    {}
  }

  /// Move assignment.
  /** Neither connection can have an open transaction, registered error
   * handlers, or registered notification receivers.
   */
  connection &operator=(connection &&rhs);

  connection(connection const &) = delete;
  connection &operator=(connection const &) = delete;

  /// Is this connection open at the moment?
  /** @warning This function is @b not needed in most code.  Resist the
   * temptation to check it after opening a connection.  Instead, just use the
   * connection and rely on getting a broken_connection exception if it failed.
   */
  [[nodiscard]] bool PQXX_PURE is_open() const noexcept;

  /// Invoke notice processor function.  The message should end in newline.
  void process_notice(char const[]) noexcept;
  /// Invoke notice processor function.  Newline at end is recommended.
  void process_notice(std::string const &msg) noexcept
  {
    process_notice(zview{msg});
  }
  /// Invoke notice processor function.  Newline at end is recommended.
  /** The zview variant, with a message ending in newline, is the most
   * efficient way to call process_notice.
   */
  void process_notice(zview) noexcept;

  /// Enable tracing to a given output stream, or nullptr to disable.
  void trace(std::FILE *) noexcept;

  /**
   * @name Connection properties
   *
   * These are probably not of great interest, since most are derived from
   * information supplied by the client program itself, but they are included
   * for completeness.
   *
   * The connection needs to be currently active for these to work.
   */
  //@{
  /// Name of database we're connected to, if any.
  [[nodiscard]] char const *dbname() const;

  /// Database user ID we're connected under, if any.
  [[nodiscard]] char const *username() const;

  /// Address of server, or nullptr if none specified (i.e. default or local)
  [[nodiscard]] char const *hostname() const;

  /// Server port number we're connected to.
  [[nodiscard]] char const *port() const;

  /// Process ID for backend process, or 0 if inactive.
  [[nodiscard]] int PQXX_PURE backendpid() const noexcept;

  /// Socket currently used for connection, or -1 for none.  Use with care!
  /** Query the current socket number.  This is intended for event loops based
   * on functions such as select() or poll(), where multiple file descriptors
   * are watched.
   *
   * Please try to stay away from this function.  It is really only meant for
   * event loops that need to wait on more than one file descriptor.  If all
   * you need is to block until a notification arrives, for instance, use
   * await_notification().  If you want to issue queries and retrieve results
   * in nonblocking fashion, check out the pipeline class.
   *
   * @warning Don't store this value anywhere, and always be prepared for the
   * possibility that, at any given time, there may not be a socket!  The
   * socket may change or even go away during any invocation of libpqxx code on
   * the connection.
   */
  [[nodiscard]] int PQXX_PURE sock() const noexcept;

  /// What version of the PostgreSQL protocol is this connection using?
  /** The answer can be 0 (when there is no connection); 3 for protocol 3.0; or
   * possibly higher values as newer protocol versions come into use.
   */
  [[nodiscard]] int PQXX_PURE protocol_version() const noexcept;

  /// What version of the PostgreSQL server are we connected to?
  /** The result is a bit complicated: each of the major, medium, and minor
   * release numbers is written as a two-digit decimal number, and the three
   * are then concatenated.  Thus server version 9.4.2 will be returned as the
   * decimal number 90402.  If there is no connection to the server, this
   * returns zero.
   *
   * @warning When writing version numbers in your code, don't add zero at the
   * beginning!  Numbers beginning with zero are interpreted as octal (base-8)
   * in C++.  Thus, 070402 is not the same as 70402, and 080000 is not a number
   * at all because there is no digit "8" in octal notation.  Use strictly
   * decimal notation when it comes to these version numbers.
   */
  [[nodiscard]] int PQXX_PURE server_version() const noexcept;
  //@}

  /// @name Text encoding
  /**
   * Each connection is governed by a "client encoding," which dictates how
   * strings and other text is represented in bytes.  The database server will
   * send text data to you in this encoding, and you should use it for the
   * queries and data which you send to the server.
   *
   * Search the PostgreSQL documentation for "character set encodings" to find
   * out more about the available encodings, how to extend them, and how to use
   * them.  Not all server-side encodings are compatible with all client-side
   * encodings or vice versa.
   *
   * Encoding names are case-insensitive, so e.g. "UTF8" is equivalent to
   * "utf8".
   *
   * You can change the client encoding, but this may not work when the
   * connection is in a special state, such as when streaming a table.  It's
   * not clear what happens if you change the encoding during a transaction,
   * and then abort the transaction.
   */
  //@{
  /// Get client-side character encoding, by name.
  [[nodiscard]] std::string get_client_encoding() const;

  /// Set client-side character encoding, by name.
  /**
   * @param encoding Name of the character set encoding to use.
   */
  void set_client_encoding(std::string const &encoding)
  {
    set_client_encoding(encoding.c_str());
  }

  /// Set client-side character encoding, by name.
  /**
   * @param encoding Name of the character set encoding to use.
   */
  void set_client_encoding(char const encoding[]);

  /// Get the connection's encoding, as a PostgreSQL-defined code.
  [[nodiscard]] int PQXX_PRIVATE encoding_id() const;

  //@}

  /// Set session variable, using SQL's @c SET command.
  /** Set a session variable for this connection.  See the PostgreSQL
   * documentation for a list of variables that can be set and their
   * permissible values.
   *
   * If a transaction is currently in progress, aborting that transaction will
   * normally discard the newly set value.  That is not true for nontransaction
   * however, since it does not start a real backend transaction.
   *
   * @warning This executes an SQL query, so do not get or set variables while
   * a table stream or pipeline is active on the same connection.
   *
   * @param var Variable to set.
   * @param value New value for Var: an identifier, a quoted string, or a
   * number.
   */
  void set_variable(std::string_view var, std::string_view value);

  /// Read session variable, using SQL's @c SHOW command.
  /** @warning This executes an SQL query, so do not get or set variables while
   * a table stream or pipeline is active on the same connection.
   */
  std::string get_variable(std::string_view);
  //@}


  /**
   * @name Notifications and Receivers
   */
  //@{
  /// Check for pending notifications and take appropriate action.
  /**
   * All notifications found pending at call time are processed by finding
   * any matching receivers and invoking those.  If no receivers matched the
   * notification string, none are invoked but the notification is considered
   * processed.
   *
   * Exceptions thrown by client-registered receivers are reported using the
   * connection's errorhandlers, but the exceptions themselves are not passed
   * on outside this function.
   *
   * @return Number of notifications processed.
   */
  int get_notifs();


  /// Wait for a notification to come in.
  /** The wait may also be terminated by other events, such as the connection
   * to the backend failing.
   *
   * If a notification comes in, the call will process it.  It will also
   * process any notifications that may have been pending.
   *
   * @return Number of notifications processed.
   */
  int await_notification();

  /// Wait for a notification to come in, or for given timeout to pass.
  /** The wait may also be terminated by other events, such as the connection
   * to the backend failing.
   *
   * If a notification comes in, the call will process it.  It will also
   * process any notifications that may have been pending.
   *
   * @return Number of notifications processed
   */
  int await_notification(std::time_t seconds, long microseconds);
  //@}

  /// Encrypt a password for a given user.
  /** Use this when setting a new password for the user if password encryption
   * is enabled.  Inputs are the SQL name for the user for whom you with to
   * encrypt a password; the plaintext password; and the hash algorithm.
   *
   * The algorithm must be one of "md5", "scram-sha-256" (introduced in
   * PostgreSQL 10), or @c nullptr.  If the pointer is null, this will query
   * the @c password_encryption setting from the server, and use the default
   * algorithm as defined there.
   *
   * @return encrypted version of the password, suitable for encrypted
   * PostgreSQL authentication.
   *
   * Thus the password for a user can be changed with:
   * @code
   * void setpw(transaction_base &t, string const &user, string const &pw)
   * {
   *   t.exec0("ALTER USER " + user + " "
   *   	"PASSWORD '" + t.conn().encrypt_password(user,pw) + "'");
   * }
   * @endcode
   *
   * When building this against a libpq older than version 10, this will use
   * an older function which only supports md5.  In that case, requesting a
   * different algorithm than md5 will result in a @c feature_not_supported
   * exception.
   */
  [[nodiscard]] std::string encrypt_password(
    char const user[], char const password[], char const *algorithm = nullptr);
  [[nodiscard]] std::string
  encrypt_password(zview user, zview password, zview algorithm)
  {
    return encrypt_password(user.c_str(), password.c_str(), algorithm.c_str());
  }

  /**
   * @name Prepared statements
   *
   * PostgreSQL supports prepared SQL statements, i.e. statements that can be
   * registered under a client-provided name, optimized once by the backend,
   * and executed any number of times under the given name.
   *
   * Prepared statement definitions are not sensitive to transaction
   * boundaries. A statement defined inside a transaction will remain defined
   * outside that transaction, even if the transaction itself is subsequently
   * aborted.  Once a statement has been prepared, it will only go away if you
   * close the connection or explicitly "unprepare" the statement.
   *
   * Use the @c pqxx::transaction_base::exec_prepared functions to execute a
   * prepared statement.  See \ref prepared for a full discussion.
   *
   * @warning Using prepared statements can save time, but if your statement
   * takes parameters, it may also make your application significantly slower!
   * The reason is that the server works out a plan for executing the query
   * when you prepare it.  At that time, the values for the parameters are of
   * course not yet known.  If you execute a query without preparing it, then
   * the server works out the plan on the spot, with knowledge of the parameter
   * values.  It can use knowlege about these values for optimisation, and the
   * prepared version does not have this knowledge.
   *
   * @{
   */

  /// Define a prepared statement.
  /**
   * The statement's definition can refer to a parameter using the parameter's
   * positional number n in the definition.  For example, the first parameter
   * can be used as a variable "$1", the second as "$2" and so on.
   *
   * Here's an example of how to use prepared statements.
   *
   * @code
   * using namespace pqxx;
   * void foo(connection &c)
   * {
   *   c.prepare("findtable", "select * from pg_tables where name=$1");
   *   work tx{c};
   *   result r = tx.exec_prepared("findtable", "mytable");
   *   if (r.empty()) throw runtime_error{"mytable not found!"};
   * }
   * @endcode
   *
   * @param name unique name for the new prepared statement.
   * @param definition SQL statement to prepare.
   */
  void prepare(char const name[], char const definition[]);

  void prepare(std::string const &name, std::string const &definition)
  {
    prepare(name.c_str(), definition.c_str());
  }

  void prepare(zview name, zview definition)
  {
    prepare(name.c_str(), definition.c_str());
  }

  /// Define a nameless prepared statement.
  /**
   * This can be useful if you merely want to pass large binary parameters to a
   * statement without otherwise wishing to prepare it.  If you use this
   * feature, always keep the definition and the use close together to avoid
   * the nameless statement being redefined unexpectedly by code somewhere
   * else.
   */
  void prepare(char const definition[]);
  void prepare(zview definition) { return prepare(definition.c_str()); }

  /// Drop prepared statement.
  void unprepare(std::string_view name);

  /**
   * @}
   */

  /// Suffix unique number to name to make it unique within session context.
  /** Used internally to generate identifiers for SQL objects (such as cursors
   * and nested transactions) based on a given human-readable base name.
   */
  [[nodiscard]] std::string adorn_name(std::string_view);

  /**
   * @defgroup escaping-functions String-escaping functions
   */
  //@{

  /// Escape string for use as SQL string literal on this connection.
  /** @warning This accepts a length, and it does not require a terminating
   * zero byte.  But if there is a zero byte, escaping stops there even if
   * it's not at the end of the string!
   */
  [[nodiscard]] std::string esc(char const text[], std::size_t maxlen) const
  {
    return esc(std::string_view(text, maxlen));
  }

  /// Escape string for use as SQL string literal on this connection.
  [[nodiscard]] std::string esc(char const text[]) const
  {
    return esc(std::string_view(text));
  }

  /// Escape string for use as SQL string literal on this connection.
  /** @warning If the string contains a zero byte, escaping stops there even
   * if it's not at the end of the string!
   */
  [[nodiscard]] std::string esc(std::string_view text) const;

  /// Escape binary string for use as SQL string literal on this connection.
  [[nodiscard]] std::string
  esc_raw(unsigned char const bin[], std::size_t len) const;

  /// Unescape binary data, e.g. from a table field or notification payload.
  /** Takes a binary string as escaped by PostgreSQL, and returns a restored
   * copy of the original binary data.
   */
  [[nodiscard]] std::string unesc_raw(std::string const &text) const
  {
    return unesc_raw(text.c_str());
  }

  /// Unescape binary data, e.g. from a table field or notification payload.
  /** Takes a binary string as escaped by PostgreSQL, and returns a restored
   * copy of the original binary data.
   */
  [[nodiscard]] std::string unesc_raw(zview text) const
  {
    return unesc_raw(text.c_str());
  }

  /// Unescape binary data, e.g. from a table field or notification payload.
  /** Takes a binary string as escaped by PostgreSQL, and returns a restored
   * copy of the original binary data.
   */
  [[nodiscard]] std::string unesc_raw(char const text[]) const;

  /// Escape and quote a string of binary data.
  [[nodiscard]] std::string
  quote_raw(unsigned char const bin[], std::size_t len) const;

  /// Escape and quote an SQL identifier for use in a query.
  [[nodiscard]] std::string quote_name(std::string_view identifier) const;

  /// Represent object as SQL string, including quoting & escaping.
  /**
   * Nulls are recognized and represented as SQL nulls.  They get no quotes.
   */
  template<typename T>[[nodiscard]] inline std::string quote(T const &t) const;

  [[nodiscard]] std::string quote(binarystring const &) const;

  /// Escape string for literal LIKE match.
  /** Use this when part of an SQL "LIKE" pattern should match only as a
   * literal string, not as a pattern, even if it contains "%" or "_"
   * characters that would normally act as wildcards.
   *
   * The string does not get string-escaped or quoted.  You do that later.
   *
   * For instance, let's say you have a string @c name entered by the user,
   * and you're searching a @c file column for items that match @c name
   * followed by a dot and three letters.  Even if @c name contains wildcard
   * characters "%" or "_", you only want those to match literally, so "_"
   * only matches "_" and "%" only matches a single "%".
   *
   * You do that by "like-escaping" @c name, appending the wildcard pattern
   * @c ".___", and finally, escaping and quoting the result for inclusion in
   * your query:
   *
   *    tx.exec(
   *        "SELECT file FROM item WHERE file LIKE " +
   *        tx.quote(tx.esc_like(name) + ".___"));
   *
   * The SQL "LIKE" operator also lets you choose your own escape character.
   * This is supported, but must be a single-byte character.
   */
  [[nodiscard]] std::string
  esc_like(std::string_view text, char escape_char = '\\') const;
  //@}

  /// Attempt to cancel the ongoing query, if any.
  /** You can use this from another thread, and/or while a query is executing
   * in a pipeline, but it's up to you to ensure that you're not canceling the
   * wrong query.  This may involve locking.
   */
  void cancel_query();

  /// Set session verbosity.
  /** Set the verbosity of error messages to "terse", "normal" (the default),
   * or "verbose."
   *
   *  If "terse", returned messages include severity, primary text, and
   * position only; this will normally fit on a single line. "normal" produces
   * messages that include the above plus any detail, hint, or context fields
   * (these might span multiple lines).  "verbose" includes all available
   * fields.
   */
  void set_verbosity(error_verbosity verbosity) noexcept;

  /// Return pointers to the active errorhandlers.
  /** The entries are ordered from oldest to newest handler.
   *
   * You may use this to find errorhandlers that your application wants to
   * delete when destroying the connection.  Be aware, however, that libpqxx
   * may also add errorhandlers of its own, and those will be included in the
   * list.  If this is a problem for you, derive your errorhandlers from a
   * custom base class derived from pqxx::errorhandler.  Then use dynamic_cast
   * to find which of the error handlers are yours.
   *
   * The pointers point to the real errorhandlers.  The container it returns
   * however is a copy of the one internal to the connection, not a reference.
   */
  [[nodiscard]] std::vector<errorhandler *> get_errorhandlers() const;

  /// Return a connection string encapsulating this connection's options.
  /** The connection must be currently open for this to work.
   *
   * Returns a reconstruction of this connection's connection string.  It may
   * not exactly match the connection string you passed in when creating this
   * connection.
   */
  [[nodiscard]] std::string connection_string() const;

  /// Explicitly close the connection.
  /** You won't normally need this.  Destroying a connection object will have
   * the same effect.
   */
  void close();

private:
  // Initialise based on connection string.
  void init(char const options[]);
  // Initialise based on parameter names and values.
  void init(char const *params[], char const *values[]);
  void complete_init();

  void wait_read() const;
  void wait_read(std::time_t seconds, long microseconds) const;

  result make_result(
    internal::pq::PGresult *pgr, std::shared_ptr<std::string> const &query);

  void PQXX_PRIVATE set_up_state();

  int PQXX_PRIVATE PQXX_PURE status() const noexcept;

  /// Escape a string, into a buffer allocated by the caller.
  /** The buffer must have room for at least @c 2*text.size()+1 bytes.
   *
   * Returns the number of bytes written, including the trailing zero.
   */
  std::size_t esc_to_buf(std::string_view text, char *buf) const;

  friend class internal::gate::const_connection_largeobject;
  char const *PQXX_PURE err_msg() const noexcept;

  void PQXX_PRIVATE process_notice_raw(char const msg[]) noexcept;

  result exec_prepared(std::string_view statement, internal::params const &);

  /// Throw @c usage_error if this connection is not in a movable state.
  void check_movable() const;
  /// Throw @c usage_error if not in a state where it can be move-assigned.
  void check_overwritable() const;

  friend class internal::gate::connection_errorhandler;
  void PQXX_PRIVATE register_errorhandler(errorhandler *);
  void PQXX_PRIVATE unregister_errorhandler(errorhandler *) noexcept;

  friend class internal::gate::connection_transaction;
  result PQXX_PRIVATE exec(std::string_view);
  result PQXX_PRIVATE exec(std::shared_ptr<std::string>);
  void PQXX_PRIVATE register_transaction(transaction_base *);
  void PQXX_PRIVATE unregister_transaction(transaction_base *) noexcept;

  friend class internal::gate::connection_stream_from;
  std::pair<std::unique_ptr<char, std::function<void(char *)>>, std::size_t>
    PQXX_PRIVATE read_copy_line();

  friend class internal::gate::connection_stream_to;
  void PQXX_PRIVATE write_copy_line(std::string_view);
  void PQXX_PRIVATE end_copy_write();

  friend class internal::gate::connection_largeobject;
  internal::pq::PGconn *raw_connection() const { return m_conn; }

  friend class internal::gate::connection_notification_receiver;
  void add_receiver(notification_receiver *);
  void remove_receiver(notification_receiver *) noexcept;

  friend class internal::gate::connection_pipeline;
  void PQXX_PRIVATE start_exec(char const query[]);
  bool PQXX_PRIVATE consume_input() noexcept;
  bool PQXX_PRIVATE is_busy() const noexcept;
  internal::pq::PGresult *get_result();

  friend class internal::gate::connection_dbtransaction;
  friend class internal::gate::connection_sql_cursor;

  result exec_params(std::string_view query, internal::params const &args);

  /// Connection handle.
  internal::pq::PGconn *m_conn = nullptr;

  /// Active transaction on connection, if any.
  internal::unique<transaction_base> m_trans;

  std::list<errorhandler *> m_errorhandlers;

  using receiver_list =
    std::multimap<std::string, pqxx::notification_receiver *>;
  /// Notification receivers.
  receiver_list m_receivers;

  /// Unique number to use as suffix for identifiers (see adorn_name()).
  int m_unique_id = 0;
};


/// @deprecated Old base class for connection.  They are now the same class.
using connection_base = connection;


template<typename T> inline std::string connection::quote(T const &t) const
{
  if constexpr (nullness<T>::always_null)
  {
    return "NULL";
  }
  else
  {
    if (is_null(t))
      return "NULL";
    auto const text{to_string(t)};

    // Okay, there's an easy way to do this and there's a hard way.  The easy
    // way was "quote, esc(to_string(t)), quote".  I'm going with the hard way
    // because it's going to save some string manipulation that will probably
    // incur some unnecessary memory allocations and deallocations.
    std::string buf{'\''};
    buf.resize(2 + 2 * text.size() + 1);
    auto const content_bytes{esc_to_buf(text, buf.data() + 1)};
    auto const closing_quote{1 + content_bytes};
    buf[closing_quote] = '\'';
    auto const end{closing_quote + 1};
    buf.resize(end);
    return buf;
  }
}


#if defined(PQXX_HAVE_CONCEPTS)
template<internal::ZKey_ZValues MAPPING>
inline connection::connection(MAPPING const &params)
{
  check_version();

  std::vector<char const *> keys, values;
  if constexpr (std::ranges::sized_range<MAPPING>)
  {
    auto const size{std::ranges::size(params) + 1};
    keys.reserve(size);
    values.reserve(size);
  }
  for (auto const &[key, value] : params)
  {
    keys.push_back(internal::as_c_string(key));
    values.push_back(internal::as_c_string(value));
  }
  keys.push_back(nullptr);
  values.push_back(nullptr);
  init(keys.data(), values.data());
}
#endif // PQXX_HAVE_CONCEPTS
} // namespace pqxx


namespace pqxx::internal
{
PQXX_LIBEXPORT void wait_read(internal::pq::PGconn const *);
PQXX_LIBEXPORT void wait_read(
  internal::pq::PGconn const *, std::time_t seconds, long microseconds);
PQXX_LIBEXPORT void wait_write(internal::pq::PGconn const *);
} // namespace pqxx::internal

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
