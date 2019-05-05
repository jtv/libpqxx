/** Definition of the pqxx::connection_base abstract base class.
 *
 * pqxx::connection_base encapsulates a frontend to backend connection
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/connection_base instead.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_CONNECTION_BASE
#define PQXX_H_CONNECTION_BASE

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <list>
#include <map>
#include <memory>

#include "pqxx/errorhandler.hxx"
#include "pqxx/except.hxx"
#include "pqxx/prepared_statement.hxx"
#include "pqxx/strconv.hxx"
#include "pqxx/util.hxx"
#include "pqxx/version.hxx"


/* Use of the libpqxx library starts here.
 *
 * Everything that can be done with a database through libpqxx must go through
 * a connection object derived from connection_base.
 */

/* Methods tested in eg. self-test program test1 are marked with "//[t01]"
 */

namespace pqxx
{
namespace internal
{
class sql_cursor;
}


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
	const std::string &user,
	const std::string &password);


namespace internal
{
namespace gate
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
} // namespace pqxx::internal


// TODO: Document connection strings and environment variables.
/// connection_base abstract base class; represents a connection to a database.
/** This is the first class to look at when you wish to work with a database
 * through libpqxx.  Depending on which type of connection you choose, a
 * connection can be automatically opened immediately when it is constructed,
 * or later when you ask for it, or even somewhere inbetween.  The connection
 * automatically closes upon destruction, if it hasn't been closed already.
 *
 * To query or manipulate the database once connected, use one of the
 * transaction classes (see pqxx/transaction_base.hxx) and perhaps also the
  transactor framework (see pqxx/transactor.hxx).
 *
 * When a connection breaks, you will typically get a broken_connection
 * exception.  This can happen at almost any point.
 *
 * As a general rule, always avoid raw queries if libpqxx offers a dedicated
 * function for the same purpose.  There may be hidden logic to hide certain
 * complications from you, such as caching.
 *
 * @warning On Unix-like systems, including GNU and BSD systems, your program
 * may receive the SIGPIPE signal when the connection to the backend breaks.  By
 * default this signal will abort your program.  Use "signal(SIGPIPE, SIG_IGN)"
 * if you want your program to continue running after a connection fails.
 */
class PQXX_LIBEXPORT connection_base
{
public:
  explicit connection_base(std::string options=std::string{}) :
	m_options{options}
  {
    // Check library version.  The check_library_version template is declared
    // for any library version, but only actually defined for the version of
    // the libpqxx binary against which the code is linked.
    //
    // If the library binary is a different version than the one declared in
    // these headers, then this call will fail to link: there will be no
    // definition for the function with these exact template parameter values.
    // There will be a definition, but the version in the parameter values will
    // be different.
    //
    // There is no particular reason to do this here in this constructor, except
    // to ensure that every meaningful libpqxx client will execute it.  The call
    // must be in the execution path somewhere or the compiler won't try to link
    // it.  We can't use it to initialise a global or class-static variable,
    // because a smart compiler might resolve it at compile time.
    //
    // On the other hand, we don't want to make a useless function call too
    // often for performance reasons.  A local static variable is initialised
    // only on the definition's first execution.  Compilers will be well
    // optimised for this behaviour, so there's a minimal one-time cost.
    static const auto version_ok =
      internal::check_library_version<PQXX_VERSION_MAJOR, PQXX_VERSION_MINOR>();
    ignore_unused(version_ok);

    init();
  }

  ~connection_base() { close(); }

// XXX: Remove.
  /// Explicitly close connection.
  void disconnect() noexcept;						//[t02]

   /// Is this connection open at the moment?
  /** @warning This function is @b not needed in most code.  Resist the
   * temptation to check it after opening a connection.  Instead, just use the
   * connection and rely on getting a broken_connection exception if it failed.
   */
  bool PQXX_PURE is_open() const noexcept;				//[t01]

// XXX: Remove.
  /// Explicitly activate the connection if it's in an inactive state.
  void activate();							//[t12]

  /// Make the connection fail.  @warning Do not use this except for testing!
  /** Breaks the connection in some unspecified, horrible, dirty way to enable
   * failure testing.
   *
   * Do not use this in normal code.  This is only meant for testing.
   */
  void simulate_failure();						//[t94]

  /// Invoke notice processor function.  The message should end in newline.
  void process_notice(const char[]) noexcept;				//[t14]
  /// Invoke notice processor function.  Newline at end is recommended.
  void process_notice(const std::string &) noexcept;			//[t14]

  /// Enable tracing to a given output stream, or nullptr to disable.
  void trace(std::FILE *) noexcept;					//[t03]

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
  void set_client_encoding(const std::string &encoding);		//[t07]

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
   * @param Var Variable to set.
   * @param Value New value for Var: an identifier, a quoted string, or a
   * number.
   */
  void set_variable(							//[t60]
	const std::string &Var,
	const std::string &Value);

  /// Read session variable, using SQL's @c SHOW command.
  /** @warning This executes an SQL query, so do not get or set variables while
   * a table stream or pipeline is active on the same connection.
   */
  std::string get_variable(const std::string &);			//[t60]
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
   * void foo(connection_base &c)
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
  void prepare(const std::string &name, const std::string &definition);

  /// Define a nameless prepared statement.
  /**
   * This can be useful if you merely want to pass large binary parameters to a
   * statement without otherwise wishing to prepare it.  If you use this
   * feature, always keep the definition and the use close together to avoid
   * the nameless statement being redefined unexpectedly by code somewhere else.
   */
  void prepare(const std::string &definition);

  /// Drop prepared statement.
  void unprepare(const std::string &name);

  /**
   * @}
   */

  /// Suffix unique number to name to make it unique within session context.
  /** Used internally to generate identifiers for SQL objects (such as cursors
   * and nested transactions) based on a given human-readable base name.
   */
  std::string adorn_name(const std::string &);				//[90]

  /**
   * @defgroup escaping-functions String-escaping functions
   */
  //@{
  /// Escape string for use as SQL string literal on this connection.
  std::string esc(const char str[]) const;

  /// Escape string for use as SQL string literal on this connection.
  std::string esc(const char str[], size_t maxlen) const;

  /// Escape string for use as SQL string literal on this connection.
  std::string esc(const std::string &str) const;

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
  std::string unesc_raw(const char *text) const;

  /// Escape and quote a string of binary data.
  std::string quote_raw(const unsigned char str[], size_t len) const;

  /// Escape and quote an SQL identifier for use in a query.
  std::string quote_name(const std::string &identifier) const;

  /// Represent object as SQL string, including quoting & escaping.
  /**
   * Nulls are recognized and represented as SQL nulls.  They get no quotes.
   */
  template<typename T>
  std::string quote(const T &t) const
  {
    if (string_traits<T>::is_null(t)) return "NULL";
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
  std::string esc_like(const std::string &str, char escape_char='\\') const;
  //@}

  /// Attempt to cancel the ongoing query, if any.
  void cancel_query();

  /// Error verbosity levels.
  enum error_verbosity
  {
      // These values must match those in libpq's PGVerbosity enum.
      terse=0,
      normal=1,
      verbose=2
  };

  /// Set session verbosity.
  /** Set the verbosity of error messages to "terse", "normal" (i.e. default) or
   * "verbose."
   *
   *  If "terse", returned messages include severity, primary text, and position
   *  only; this will normally fit on a single line. "normal" produces messages
   *  that include the above plus any detail, hint, or context fields (these
   *  might span multiple lines).  "verbose" includes all available fields.
   */
  void set_verbosity(error_verbosity verbosity) noexcept;

   /// Retrieve current error verbosity.
  error_verbosity get_verbosity() const noexcept {return m_verbosity;}

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

  const std::string &options() const noexcept { return m_options; }

protected:
  void init();

  void close() noexcept;
  void wait_read() const;
  void wait_read(long seconds, long microseconds) const;

private:

  result make_result(internal::pq::PGresult *rhs, const std::string &query);

  void PQXX_PRIVATE set_up_state();
  void PQXX_PRIVATE check_result(const result &);

  void PQXX_PRIVATE internal_set_trace() noexcept;
  int PQXX_PRIVATE PQXX_PURE status() const noexcept;

  friend class internal::gate::const_connection_largeobject;
  const char * PQXX_PURE err_msg() const noexcept;

  std::string PQXX_PRIVATE raw_get_var(const std::string &);
  void PQXX_PRIVATE process_notice_raw(const char msg[]) noexcept;

  void read_capabilities();

  result exec_prepared(const std::string &statement, const internal::params &);

  /// Connection handle.
  internal::pq::PGconn *m_conn = nullptr;

  /// Active transaction on connection, if any.
  internal::unique<transaction_base> m_trans;

  /// Set libpq notice processor to call connection's error handlers chain.
  void set_notice_processor();
  /// Clear libpq notice processor.
  void clear_notice_processor();
  std::list<errorhandler *> m_errorhandlers;

  /// File to trace to, if any.
  std::FILE *m_trace = nullptr;

  using receiver_list =
	std::multimap<std::string, pqxx::notification_receiver *>;
  /// Notification receivers.
  receiver_list m_receivers;

  /// Server version.
  int m_serverversion = 0;

  /// Unique number to use as suffix for identifiers (see adorn_name()).
  int m_unique_id = 0;

  /// Have we finished our attempt to establish our connection?
  bool m_completed = false;

  /// Current verbosity level.
  error_verbosity m_verbosity = normal;

  /// Connection string.
  std::string m_options;

  friend class internal::gate::connection_errorhandler;
  void PQXX_PRIVATE register_errorhandler(errorhandler *);
  void PQXX_PRIVATE unregister_errorhandler(errorhandler *) noexcept;

  friend class internal::gate::connection_transaction;
  result PQXX_PRIVATE exec(const char[]);
  void PQXX_PRIVATE register_transaction(transaction_base *);
  void PQXX_PRIVATE unregister_transaction(transaction_base *) noexcept;
  bool PQXX_PRIVATE read_copy_line(std::string &);
  void PQXX_PRIVATE write_copy_line(const std::string &);
  void PQXX_PRIVATE end_copy_write();
  void PQXX_PRIVATE raw_set_var(const std::string &, const std::string &);

  friend class internal::gate::connection_largeobject;
  internal::pq::PGconn *raw_connection() const { return m_conn; }

  friend class internal::gate::connection_notification_receiver;
  void add_receiver(notification_receiver *);
  void remove_receiver(notification_receiver *) noexcept;

  friend class internal::gate::connection_pipeline;
  void PQXX_PRIVATE start_exec(const std::string &);
  bool PQXX_PRIVATE consume_input() noexcept;
  bool PQXX_PRIVATE is_busy() const noexcept;
  internal::pq::PGresult *get_result();

  friend class internal::gate::connection_dbtransaction;
  friend class internal::gate::connection_sql_cursor;

  result exec_params(
	const std::string &query,
	const internal::params &args);

  connection_base(const connection_base &) =delete;
  connection_base &operator=(const connection_base &) =delete;
};


using connection = connection_base;


namespace internal
{
void wait_read(const internal::pq::PGconn *);
void wait_read(const internal::pq::PGconn *, long seconds, long microseconds);
void wait_write(const internal::pq::PGconn *);
} // namespace pqxx::internal

} // namespace pqxx

#include "pqxx/compiler-internal-post.hxx"
#endif
