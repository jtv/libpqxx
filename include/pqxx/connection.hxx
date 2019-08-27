/* Definition of the connection class.
 *
 * pqxx::connection encapsulates a connection to a database.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/connection instead.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_CONNECTION
#define PQXX_H_CONNECTION

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <list>
#include <map>
#include <memory>
#include <string_view>

#include "pqxx/errorhandler.hxx"
#include "pqxx/except.hxx"
#include "pqxx/prepared_statement.hxx"
#include "pqxx/strconv.hxx"
#include "pqxx/util.hxx"


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

/* Methods tested in eg. self-test program test1 are marked with "//[t01]"
 */

namespace pqxx::internal
{
class sql_cursor;
} // namespace pqxx::internal


namespace pqxx::internal::gate
{
class connection_dbtransaction;
class connection_errorhandler;
class connection_largeobject;
class connection_notification_receiver;
class connection_pipeline;
class connection_sql_cursor;
class connection_transaction;
class const_connection_largeobject;
} // namespace pqxx::internal::gate


namespace pqxx
{
// TODO: Replace with a new connection method.
/// Encrypt password for given user.
/** Use this when setting a new password for the user if password encryption is
 * enabled.  Inputs are the username the password is for, and the plaintext
 * password.
 *
 * @return encrypted version of the password, suitable for encrypted PostgreSQL
 * authentication.
 *
 * Thus the password for a user can be changed with:
 * @code
 * void setpw(transaction_base &t, const string &user, const string &pw)
 * {
 *   t.exec("ALTER USER " + user + " "
 *   	"PASSWORD '" + encrypt_password(user,pw) + "'");
 * }
 * @endcode
 */
std::string PQXX_LIBEXPORT encrypt_password(				//[t00]
	const char user[],
	const char password[]);

/// Encrypt password for given user.
inline std::string encrypt_password(
	const std::string &user,
	const std::string &password)
{ return encrypt_password(user.c_str(), password.c_str()); }


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
 * may receive the SIGPIPE signal when the connection to the backend breaks.  By
 * default this signal will abort your program.  Use "signal(SIGPIPE, SIG_IGN)"
 * if you want your program to continue running after a connection fails.
 */
class PQXX_LIBEXPORT connection
{
public:
  connection()
  {
    check_version();
    init("");
  }

  explicit connection(const std::string &options)
  {
    check_version();
    init(options.c_str());
  }

  explicit connection(const char options[])
  {
    check_version();
    init(options);
  }

  explicit connection(zview options)
  {
    check_version();
    init(options.c_str());
  }

  /// Move constructor.
  /** Moving a connection is not allowed if it has an open transaction, or has
   * error handlers or notification receivers registered on it.  In those
   * situations, other objects may hold references to the old object which
   * would become invalid and might produce hard-to-diagnose bugs.
   */
  connection(connection &&rhs);

  ~connection() { try { close(); } catch (const std::exception &) {} }

  /// Move assignment.
  /** Neither connection can have an open transaction, registered error
   * handlers, or registered notification receivers.
   */
  connection &operator=(connection &&rhs);

  connection(const connection &) =delete;
  connection &operator=(const connection &) =delete;

   /// Is this connection open at the moment?
  /** @warning This function is @b not needed in most code.  Resist the
   * temptation to check it after opening a connection.  Instead, just use the
   * connection and rely on getting a broken_connection exception if it failed.
   */
  bool PQXX_PURE is_open() const noexcept;				//[t01]

  /// Invoke notice processor function.  The message should end in newline.
  void process_notice(const char[]) noexcept;				//[t14]
  /// Invoke notice processor function.  Newline at end is recommended.
  void process_notice(const std::string &) noexcept;			//[t14]

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
  const char *dbname() const;						//[t01]

  /// Database user ID we're connected under, if any.
  const char *username() const;						//[t01]

  /// Address of server, or nullptr if none specified (i.e. default or local)
  const char *hostname() const;						//[t01]

  /// Server port number we're connected to.
  const char *port() const;						//[t01]

  /// Process ID for backend process, or 0 if inactive.
  int PQXX_PURE backendpid() const noexcept;				//[t01]

  /// Socket currently used for connection, or -1 for none.  Use with care!
  /** Query the current socket number.  This is intended for event loops based
   * on functions such as select() or poll(), where multiple file descriptors
   * are watched.
   *
   * Please try to stay away from this function.  It is really only meant for
   * event loops that need to wait on more than one file descriptor.  If all you
   * need is to block until a notification arrives, for instance, use
   * await_notification().  If you want to issue queries and retrieve results in
   * nonblocking fashion, check out the pipeline class.
   *
   * @warning Don't store this value anywhere, and always be prepared for the
   * possibility that, at any given time, there may not be a socket!  The
   * socket may change or even go away during any invocation of libpqxx code on
   * the connection.
   */
  int PQXX_PURE sock() const noexcept;					//[t87]

  /// What version of the PostgreSQL protocol is this connection using?
  /** The answer can be 0 (when there is no connection); 3 for protocol 3.0; or
   * possibly higher values as newer protocol versions come into use.
   */
  int PQXX_PURE protocol_version() const noexcept;			//[t01]

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
  int PQXX_PURE server_version() const noexcept;			//[t01]
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
  std::string get_client_encoding() const;

  /// Set client-side character encoding, by name.
  /**
   * @param Encoding Name of the character set encoding to use.
   */
  void set_client_encoding(const std::string &encoding)
  { set_client_encoding(encoding.c_str()); }

  /// Set client-side character encoding, by name.
  /**
   * @param Encoding Name of the character set encoding to use.
   */
  void set_client_encoding(const char encoding[]);			//[t07]

  /// Get the connection's encoding, as a PostgreSQL-defined code.
  int PQXX_PRIVATE encoding_id() const;

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
  void set_variable(							//[t60]
	std::string_view var,
	std::string_view value);

  /// Read session variable, using SQL's @c SHOW command.
  /** @warning This executes an SQL query, so do not get or set variables while
   * a table stream or pipeline is active on the same connection.
   */
  std::string get_variable(std::string_view);   			//[t60]
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
  int get_notifs();							//[t04]


  /// Wait for a notification to come in.
  /** The wait may also be terminated by other events, such as the connection
   * to the backend failing.
   *
   * If a notification comes in, the call will process it.  It will also
   * process any notifications that may have been pending.
   *
   * @return Number of notifications processed.
   */
  int await_notification();						//[t78]

  /// Wait for a notification to come in, or for given timeout to pass.
  /** The wait may also be terminated by other events, such as the connection
   * to the backend failing.
   *
   * If a notification comes in, the call will process it.  It will also
   * process any notifications that may have been pending.
   *
   * @return Number of notifications processed
   */
  int await_notification(long seconds, long microseconds);		//[t79]
  //@}


  /**
   * @name Prepared statements
   *
   * PostgreSQL supports prepared SQL statements, i.e. statements that can be
   * registered under a client-provided name, optimized once by the backend, and
   * executed any number of times under the given name.
   *
   * Prepared statement definitions are not sensitive to transaction boundaries.
   * A statement defined inside a transaction will remain defined outside that
   * transaction, even if the transaction itself is subsequently aborted.  Once
   * a statement has been prepared, it will only go away if you close the
   * connection or explicitly "unprepare" the statement.
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
  void prepare(const char name[], const char definition[]);

  void prepare(const std::string &name, const std::string &definition)
  { prepare(name.c_str(), definition.c_str()); }

  /// Define a nameless prepared statement.
  /**
   * This can be useful if you merely want to pass large binary parameters to a
   * statement without otherwise wishing to prepare it.  If you use this
   * feature, always keep the definition and the use close together to avoid
   * the nameless statement being redefined unexpectedly by code somewhere else.
   */
  void prepare(const char definition[]);
  void prepare(const std::string &definition)
  { return prepare(definition.c_str()); }

  /// Drop prepared statement.
  void unprepare(std::string_view name);

  /**
   * @}
   */

  /// Suffix unique number to name to make it unique within session context.
  /** Used internally to generate identifiers for SQL objects (such as cursors
   * and nested transactions) based on a given human-readable base name.
   */
  std::string adorn_name(std::string_view);				//[90]

  /**
   * @defgroup escaping-functions String-escaping functions
   */
  //@{

  /// Escape string for use as SQL string literal on this connection.
  /** @warning This accepts a length, and it does not require a terminating
   * zero byte.  But if there is a zero byte, escaping stops there even if
   * it's not at the end of the string!
   */
  std::string esc(const char str[], size_t maxlen) const
  { return esc(std::string_view(str, maxlen)); }

  /// Escape string for use as SQL string literal on this connection.
  std::string esc(const char str[]) const
  { return esc(std::string_view(str)); }

  /// Escape string for use as SQL string literal on this connection.
  /** @warning If the string contains a zero byte, escaping stops there even
   * if it's not at the end of the string!
   */
  std::string esc(std::string_view str) const;

  /// Escape binary string for use as SQL string literal on this connection.
  std::string esc_raw(const unsigned char str[], size_t len) const;

  /// Unescape binary data, e.g. from a table field or notification payload.
  /** Takes a binary string as escaped by PostgreSQL, and returns a restored
   * copy of the original binary data.
   */
  std::string unesc_raw(const std::string &text) const
					    { return unesc_raw(text.c_str()); }

  /// Unescape binary data, e.g. from a table field or notification payload.
  /** Takes a binary string as escaped by PostgreSQL, and returns a restored
   * copy of the original binary data.
   */
  std::string unesc_raw(const char text[]) const;

  /// Escape and quote a string of binary data.
  std::string quote_raw(const unsigned char str[], size_t len) const;

  /// Escape and quote an SQL identifier for use in a query.
  std::string quote_name(std::string_view identifier) const;

  /// Represent object as SQL string, including quoting & escaping.
  /**
   * Nulls are recognized and represented as SQL nulls.  They get no quotes.
   */
  template<typename T>
  std::string quote(const T &t) const
  {
    if (is_null(t)) return "NULL";
    return "'" + this->esc(to_string(t)) + "'";
  }

  std::string quote(const binarystring &) const;

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
  std::string esc_like(std::string_view str, char escape_char='\\') const;
  //@}

  /// Attempt to cancel the ongoing query, if any.
  void cancel_query();

  /// Set session verbosity.
  /** Set the verbosity of error messages to "terse", "normal" (the default),
   * or "verbose."
   *
   *  If "terse", returned messages include severity, primary text, and position
   *  only; this will normally fit on a single line. "normal" produces messages
   *  that include the above plus any detail, hint, or context fields (these
   *  might span multiple lines).  "verbose" includes all available fields.
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
  std::vector<errorhandler *> get_errorhandlers() const;

  /// Close the connection now.
  void close();

private:
  void init(const char options[]);

  void wait_read() const;
  void wait_read(long seconds, long microseconds) const;

  result make_result(internal::pq::PGresult *rhs, const std::string &query);

  void PQXX_PRIVATE set_up_state();
  void PQXX_PRIVATE check_result(const result &);

  int PQXX_PRIVATE PQXX_PURE status() const noexcept;

  friend class internal::gate::const_connection_largeobject;
  const char * PQXX_PURE err_msg() const noexcept;

  void PQXX_PRIVATE process_notice_raw(const char msg[]) noexcept;

  void read_capabilities();

  result exec_prepared(const std::string &statement, const internal::params &);

  /// Set libpq notice processor to call connection's error handlers chain.
  void set_notice_processor();
  /// Clear libpq notice processor.
  void clear_notice_processor();

  /// Throw @c usage_error if this connection is not in a movable state.
  void check_movable() const;
  /// Throw @c usage_error if not in a state where it can be move-assigned.
  void check_overwritable() const;

  friend class internal::gate::connection_errorhandler;
  void PQXX_PRIVATE register_errorhandler(errorhandler *);
  void PQXX_PRIVATE unregister_errorhandler(errorhandler *) noexcept;

  friend class internal::gate::connection_transaction;
  result PQXX_PRIVATE exec(const char[]);
  void PQXX_PRIVATE register_transaction(transaction_base *);
  void PQXX_PRIVATE unregister_transaction(transaction_base *) noexcept;
  bool PQXX_PRIVATE read_copy_line(std::string &);
  void PQXX_PRIVATE write_copy_line(std::string_view);
  void PQXX_PRIVATE end_copy_write();

  friend class internal::gate::connection_largeobject;
  internal::pq::PGconn *raw_connection() const { return m_conn; }

  friend class internal::gate::connection_notification_receiver;
  void add_receiver(notification_receiver *);
  void remove_receiver(notification_receiver *) noexcept;

  friend class internal::gate::connection_pipeline;
  void PQXX_PRIVATE start_exec(const char query[]);
  bool PQXX_PRIVATE consume_input() noexcept;
  bool PQXX_PRIVATE is_busy() const noexcept;
  internal::pq::PGresult *get_result();

  friend class internal::gate::connection_dbtransaction;
  friend class internal::gate::connection_sql_cursor;

  result exec_params(
	const std::string &query,
	const internal::params &args);

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
} // namespace pqxx


namespace pqxx::internal
{
void wait_read(const internal::pq::PGconn *);
void wait_read(const internal::pq::PGconn *, long seconds, long microseconds);
void wait_write(const internal::pq::PGconn *);
} // namespace pqxx::internal

#include "pqxx/compiler-internal-post.hxx"
#endif
