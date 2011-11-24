/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/connection_base.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::connection_base abstract base class.
 *   pqxx::connection_base encapsulates a frontend to backend connection
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/connection_base instead.
 *
 * Copyright (c) 2001-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_CONNECTION_BASE
#define PQXX_H_CONNECTION_BASE

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <bitset>
#include <list>
#include <map>
#include <memory>

#include "pqxx/errorhandler"
#include "pqxx/except"
#include "pqxx/prepared_statement"
#include "pqxx/strconv"
#include "pqxx/util"


/* Use of the libpqxx library starts here.
 *
 * Everything that can be done with a database through libpqxx must go through
 * a connection object derived from connection_base.
 */

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */

namespace pqxx
{
class binarystring;
class connectionpolicy;
class notification_receiver;
class result;
class transaction_base;

namespace internal
{
class reactivation_avoidance_exemption;
class sql_cursor;

class reactivation_avoidance_counter
{
public:
  reactivation_avoidance_counter() : m_counter(0) {}

  void add(int n) throw () { m_counter += n; }
  void clear() throw () { m_counter = 0; }
  int get() const throw () { return m_counter; }

private:
  int m_counter;
};

}


/// Encrypt password for given user.  Requires libpq 8.2 or better.
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
 *
 * @since libpq 8.2
 */
PGSTD::string PQXX_LIBEXPORT encrypt_password(				//[t0]
	const PGSTD::string &user,
	const PGSTD::string &password);


namespace internal
{
namespace gate
{
class connection_dbtransaction;
class connection_errorhandler;
class connection_largeobject;
class connection_notification_receiver;
class connection_parameterized_invocation;
class connection_pipeline;
class connection_prepare_invocation;
class connection_reactivation_avoidance_exemption;
class connection_sql_cursor;
class connection_transaction;
} // namespace pqxx::internal::gate
} // namespace pqxx::internal


/// connection_base abstract base class; represents a connection to a database.
/** This is the first class to look at when you wish to work with a database
 * through libpqxx.  Depending on the implementing concrete child class, a
 * connection can be automatically opened when it is constructed, or when it is
 * first used, or somewhere inbetween.  The connection is automatically closed
 * upon destruction (if it hasn't been closed already).
 *
 * To query or manipulate the database once connected, use one of the
 * transaction classes (see pqxx/transaction_base.hxx) or preferably the
 * transactor framework (see pqxx/transactor.hxx).
 *
 * If a network connection to the database server fails, the connection will be
 * restored automatically (although any transaction going on at the time will
 * have to be aborted).  This also means that any information set in previous
 * transactions that is not stored in the database, such as temp tables or
 * connection-local variables defined with PostgreSQL's SET command, will be
 * lost.  Whenever you create such state, either keept it local to one
 * transaction, where possible, or inhibit automatic reactivation of the
 * connection using the inhibit_reactivation() method.
 *
 * When a connection breaks, you will typically get a broken_connection
 * exception.  This can happen at almost any point, and the details may depend
 * on which connection class (all derived from this one) you use.
 *
 * As a general rule, always avoid raw queries if libpqxx offers a dedicated
 * function for the same purpose.  There may be hidden logic to hide certain
 * complications from you, such as reinstating session variables when a
 * broken or disabled connection is reactivated.
 *
 * @warning On Unix-like systems, including GNU and BSD systems, your program
 * may receive the SIGPIPE signal when the connection to the backend breaks.  By
 * default this signal will abort your program.  Use "signal(SIGPIPE, SIG_IGN)"
 * if you want your program to continue running after a connection fails.
 */
class PQXX_LIBEXPORT connection_base
{
public:
  /// Explicitly close connection.
  void disconnect() throw ();						//[t2]

   /// Is this connection open at the moment?
  /** @warning This function is @b not needed in most code.  Resist the
   * temptation to check it after opening a connection; instead, rely on the
   * broken_connection exception that will be thrown on connection failure.
   */
  bool PQXX_PURE is_open() const throw ();				//[t1]

 /**
   * @name Activation
   *
   * Connections can be temporarily deactivated, or they can break because of
   * overly impatient firewalls dropping TCP connections.  Where possible,
   * libpqxx will try to re-activate these when resume using them, or you can
   * wake them up explicitly.  You probably won't need this feature, but you
   * should be aware of it.
   */
  //@{
  /// Explicitly activate deferred or deactivated connection.
  /** Use of this method is entirely optional.  Whenever a connection is used
   * while in a deferred or deactivated state, it will transparently try to
   * bring itself into an activated state.  This function is best viewed as an
   * explicit hint to the connection that "if you're not in an active state, now
   * would be a good time to get into one."  Whether a connection is currently
   * in an active state or not makes no real difference to its functionality.
   * There is also no particular need to match calls to activate() with calls to
   * deactivate().  A good time to call activate() might be just before you
   * first open a transaction on a lazy connection.
   */
  void activate();							//[t12]

  /// Explicitly deactivate connection.
  /** Like its counterpart activate(), this method is entirely optional.
   * Calling this function really only makes sense if you won't be using this
   * connection for a while and want to reduce the number of open connections on
   * the database server.
   * There is no particular need to match or pair calls to deactivate() with
   * calls to activate(), but calling deactivate() during a transaction is an
   * error.
   */
  void deactivate();							//[t12]

  /// Disallow (or permit) connection recovery
  /** A connection whose underlying socket is not currently connected to the
   * server will normally (re-)establish communication with the server whenever
   * needed, or when the client program requests it (although for reasons of
   * integrity, never inside a transaction; but retrying the whole transaction
   * may implicitly cause the connection to be restored).  In normal use this is
   * quite a convenient thing to have and presents a simple, safe, predictable
   * interface.
   *
   * There is at least one situation where this feature is not desirable,
   * however.  Although most session state (prepared statements, session
   * variables) is automatically restored to its working state upon connection
   * reactivation, temporary tables and so-called WITH HOLD cursors (which can
   * live outside transactions) are not.
   *
   * Cursors that live outside transactions are automatically handled, and the
   * library will quietly ignore requests to deactivate or reactivate
   * connections while they exist; it does not want to give you the illusion of
   * being back in your transaction when in reality you just dropped a cursor.
   * With temporary tables this is not so easy: there is no easy way for the
   * library to detect their creation or track their lifetimes.
   *
   * So if your program uses temporary tables, and any part of this use happens
   * outside of any database transaction (or spans multiple transactions), some
   * of the work you have done on these tables may unexpectedly be undone if the
   * connection is broken or deactivated while any of these tables exists, and
   * then reactivated or implicitly restored before you are finished with it.
   *
   * If this describes any part of your program, guard it against unexpected
   * reconnections by inhibiting reconnection at the beginning.  And if you want
   * to continue doing work on the connection afterwards that no longer requires
   * the temp tables, you can permit it again to get the benefits of connection
   * reactivation for the remainder of the program.
   *
   * @param inhibit should reactivation be inhibited from here on?
   *
   * @warning Some connection types (the lazy and asynchronous types) defer
   * completion of the socket-level connection until it is actually needed by
   * the client program.  Inhibiting reactivation before this connection is
   * really established will prevent these connection types from doing their
   * work.  For those connection types, if you are sure that reactivation needs
   * to be inhibited before any query goes across the connection, activate() the
   * connection first.  This will ensure that definite activation happens before
   * you inhibit it.
   */
  void inhibit_reactivation(bool inhibit)				//[t86]
	{ m_inhibit_reactivation=inhibit; }

  /// Make the connection fail.  @warning Do not use this except for testing!
  /** Breaks the connection in some unspecified, horrible, dirty way to enable
   * failure testing.
   *
   * Do not use this in normal programs.  This is only meant for testing.
   */
  void simulate_failure();						//[t94]
  //@}

  /// Invoke notice processor function.  The message should end in newline.
  void process_notice(const char[]) throw ();				//[t14]
  /// Invoke notice processor function.  Newline at end is recommended.
  void process_notice(const PGSTD::string &) throw ();			//[t14]

  /// Enable tracing to a given output stream, or NULL to disable.
  void trace(PGSTD::FILE *) throw ();					//[t3]

  /**
   * @name Connection properties
   *
   * These are probably not of great interest, since most are derived from
   * information supplied by the client program itself, but they are included
   * for completeness.
   */
  //@{
  /// Name of database we're connected to, if any.
  /** @warning This activates the connection, which may fail with a
   * broken_connection exception.
   */
  const char *dbname();							//[t1]

  /// Database user ID we're connected under, if any.
  /** @warning This activates the connection, which may fail with a
   * broken_connection exception.
   */
  const char *username();						//[t1]

  /// Address of server, or NULL if none specified (i.e. default or local)
  /** @warning This activates the connection, which may fail with a
   * broken_connection exception.
   */
  const char *hostname();						//[t1]

  /// Server port number we're connected to.
  /** @warning This activates the connection, which may fail with a
   * broken_connection exception.
   */
  const char *port();							//[t1]

  /// Process ID for backend process.
  /** Use with care: connections may be lost and automatically re-established
   * without your knowledge, in which case this process ID may no longer be
   * correct.  You may, however, assume that this number remains constant and
   * reliable within the span of a successful backend transaction.  If the
   * transaction fails, which may be due to a lost connection, then this number
   * will have become invalid at some point within the transaction.
   *
   * @return Process identifier, or 0 if not currently connected.
   */
  int PQXX_PURE backendpid() const throw ();				//[t1]

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
   * possibility that there is no socket.  The socket may change or even go away
   * during any invocation of libpqxx code, no matter how trivial.
   */
  int PQXX_PURE sock() const throw ();					//[t87]

  /** 
   * @name Capabilities
   *
   * Some functionality is only available in certain versions of the backend,
   * or only when speaking certain versions of the communications protocol that
   * connects us to the backend.  This includes clauses for SQL statements that
   * were not accepted in older database versions, but are required in newer
   * versions to get the same behaviour.
   */
  //@{
 
  /// Session capabilities
  enum capability
  {
    /// Does the backend support prepared statements?  (If not, we emulate them)
    cap_prepared_statements,

    /// Can we specify WITH OIDS with CREATE TABLE?
    cap_create_table_with_oids,

    /// Can transactions be nested in other transactions?
    cap_nested_transactions,

    /// Can cursors be declared SCROLL?
    cap_cursor_scroll,
    /// Can cursors be declared WITH HOLD?
    cap_cursor_with_hold,
    /// Can cursors be updateable?
    cap_cursor_update,
    /// Can cursors fetch zero elements?  (Used to trigger a "fetch all")
    cap_cursor_fetch_0,

    /// Can we ask what table column a result column came from?
    cap_table_column,

    /// Can transactions be READ ONLY?
    cap_read_only_transactions,

    /// Do prepared statements support varargs?
    cap_statement_varargs,

    /// Is the unnamed prepared statement supported?
    cap_prepare_unnamed_statement,

    /// Can this connection execute parameterized statements?
    cap_parameterized_statements,

    /// Can notifications carry payloads?
    cap_notify_payload,

    /// Not a capability value; end-of-enumeration marker
    cap_end
  };


  /// Does this connection seem to support the given capability?
  /** Don't try to be smart by caching this information anywhere.  Obtaining it
   * is quite fast (especially after the first time) and what's more, a
   * capability may "suddenly" appear or disappear if the connection is broken
   * or deactivated, and then restored.  This may happen silently any time no
   * backend transaction is active; if it turns out that the server was upgraded
   * or restored from an older backup, or the new connection goes to a different
   * backend, then the restored session may have different capabilities than
   * were available previously.
   *
   * Some guesswork is involved in establishing the presence of any capability;
   * try not to rely on this function being exactly right.
   *
   * @warning Make sure your connection is active before calling this function,
   * or the answer will always be "no."  In particular, if you are using this
   * function on a newly-created lazyconnection, activate the connection first.
   */
  bool supports(capability c) const throw () { return m_caps.test(c); }	//[t88]

  /// What version of the PostgreSQL protocol is this connection using?
  /** The answer can be 0 (when there is no connection, or the libpq version
   * being used is too old to obtain the information); 2 for protocol 2.0; 3 for
   * protocol 3.0; and possibly higher values as newer protocol versions are
   * taken into use.
   *
   * If the connection is broken and restored, the restored connection could
   * possibly a different server and protocol version.  This would normally
   * happen if the server is upgraded without shutting down the client program,
   * for example.
   *
   * Requires libpq version from PostgreSQL 7.4 or better.
   */
  int PQXX_PURE protocol_version() const throw ();			//[t1]

  /// What version of the PostgreSQL server are we connected to?
  /** The result is a bit complicated: each of the major, medium, and minor
   * release numbers is written as a two-digit decimal number, and the three
   * are then concatenated.  Thus server version 7.4.2 will be returned as the
   * decimal number 70402.  If there is no connection to the server, of if the
   * libpq version is too old to obtain the information, zero is returned.
   *
   * @warning When writing version numbers in your code, don't add zero at the
   * beginning!  Numbers beginning with zero are interpreted as octal (base-8)
   * in C++.  Thus, 070402 is not the same as 70402, and 080000 is not a number
   * at all because there is no digit "8" in octal notation.  Use strictly
   * decimal notation when it comes to these version numbers.
   */
  int PQXX_PURE server_version() const throw ();			//[t1]
  //@}

  /// Set client-side character encoding
  /** Search the PostgreSQL documentation for "multibyte" or "character set
   * encodings" to find out more about the available encodings, how to extend
   * them, and how to use them.  Not all server-side encodings are compatible
   * with all client-side encodings or vice versa.
   * @param Encoding Name of the character set encoding to use
   */
  void set_client_encoding(const PGSTD::string &Encoding)		//[t7]
	{ set_variable("CLIENT_ENCODING", Encoding); }

  /// Set session variable
  /** Set a session variable for this connection, using the SET command.  If the
   * connection to the database is lost and recovered, the last-set value will
   * be restored automatically.  See the PostgreSQL documentation for a list of
   * variables that can be set and their permissible values.
   * If a transaction is currently in progress, aborting that transaction will
   * normally discard the newly set value.  Known exceptions are nontransaction
   * (which doesn't start a real backend transaction) and PostgreSQL versions
   * prior to 7.3.
   * @warning Do not mix the set_variable interface with manual setting of
   * variables by executing the corresponding SQL commands, and do not get or
   * set variables while a tablestream or pipeline is active on the same
   * connection.
   * @param Var Variable to set
   * @param Value Value vor Var to assume: an identifier, a quoted string, or a
   * number.
   */
  void set_variable(const PGSTD::string &Var,
		    const PGSTD::string &Value);			//[t60]

  /// Read session variable
  /** Will try to read the value locally, from the list of variables set with
   * the set_variable function.  If that fails, the database is queried.
   * @warning Do not mix the set_variable interface with manual setting of
   * variables by executing the corresponding SQL commands, and do not get or
   * set variables while a tablestream or pipeline is active on the same
   * connection.
   */
  PGSTD::string get_variable(const PGSTD::string &);			//[t60]
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
   * @return Number of notifications processed
   */
  int get_notifs();							//[t4]


  /// Wait for a notification to come in
  /** The wait may also be terminated by other events, such as the connection
   * to the backend failing.  Any pending or received notifications are
   * processed as part of the call.
   *
   * @return Number of notifications processed
   */
  int await_notification();						//[t78]

  /// Wait for a notification to come in, or for given timeout to pass
  /** The wait may also be terminated by other events, such as the connection
   * to the backend failing.  Any pending or received notifications are
   * processed as part of the call.

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
   * Prepared statement definitions are not sensitive to transaction boundaries;
   * a statement defined inside a transaction will remain defined outside that
   * transaction, even if the transaction itself is subsequently aborted.  Once
   * a statement has been prepared, only closing the connection or explicitly
   * "unpreparing" it can make it go away.
   *
   * Use the transaction classes' @c prepared().exec() function to execute a
   * prepared statement.  Use @c prepared().exists() to find out whether a
   * statement has been prepared under a given name.
   *
   * A special case is the nameless prepared statement.  You may prepare a
   * statement without a name.  The unnamed statement can be redefined at any
   * time, without un-preparing it first.
   *
   * @warning Prepared statements are not necessarily defined on the backend
   * right away; libpqxx generally does that lazily.  This means that you can
   * prepare statements before the connection is fully established, and that
   * it's relatively cheap to pre-prepare lots of statements that you may or may
   * not use during the session.  On the other hand, it also means that errors
   * in a prepared statement may not show up until you first try to invoke it.
   * Such an error may then break the transaction it occurs in.
   *
   * @warning Never try to prepare, execute, or unprepare a prepared statement
   * manually using direct SQL queries.  Always use the functions provided by
   * libpqxx.
   *
   * @{
   */

  /// Define a prepared statement
  /** 
   * The statement's definition can refer to a parameter using the parameter's
   * positional number n in the definition.  For example, the first parameter
   * can be used as a variable "$1", the second as "$2" and so on.
   *
   * Here's an example of how to use prepared statements.  Note the unusual
   * syntax for passing parameters: every new argument is a parenthesized
   * expression that is simply tacked onto the end of the statement!
   *
   * @code
   * using namespace pqxx;
   * void foo(connection_base &C)
   * {
   *   C.prepare("findtable", "select * from pg_tables where name=$1");
   *   work W(C);
   *   result R = W.prepared("findtable")("mytable").exec();
   *   if (R.empty()) throw runtime_error("mytable not found!");
   * }
   * @endcode
   *
   * To save time, prepared statements aren't really registered with the backend
   * until they are first used.  If this is not what you want, e.g. because you
   * have very specific realtime requirements, you can use the @c prepare_now()
   * function to force immediate preparation.
   *
   * @warning The statement may not be registered with the backend until it is
   * actually used.  So if, for example, the statement is syntactically
   * incorrect, you may see a syntax_error here, or later when you try to call
   * the statement, or in a prepare_now() call.
   *
   * @param name unique name for the new prepared statement.
   * @param definition SQL statement to prepare.
   */
  void prepare(const PGSTD::string &name, const PGSTD::string &definition);

  /// Define a nameless prepared statement.
  /**
   * This can be useful if you merely want to pass large binary parameters to a
   * statement without otherwise wishing to prepare it.  If you use this
   * feature, always keep the definition and the use close together to avoid
   * the nameless statement being redefined unexpectedly by code somewhere else.
   */
  void prepare(const PGSTD::string &definition);

  /// Drop prepared statement
  void unprepare(const PGSTD::string &name);

  /// Request that prepared statement be registered with the server
  /** If the statement had already been fully prepared, this will do nothing.
   *
   * If the connection should break and be transparently restored, then the new
   * connection will again defer registering the statement with the server.
   * Since connections are never restored inside backend transactions, doing
   * this once at the beginning of your transaction ensures that the statement
   * will not be re-registered during that transaction.  In most cases, however,
   * it's probably better not to use this and let the connection decide when and
   * whether to register prepared statements that you've defined.
   */
  void prepare_now(const PGSTD::string &name);

  /**
   * @}
   */


  /**
   * @name Transactor framework
   *
   * See the transactor class template for more about transactors.  To use the
   * transactor framework, encapsulate your transaction code in a class derived
   * from an instantiation of the pqxx::transactor template.  Then, to execute
   * it, create an object of your transactor class and pass it to one of the
   * perform() functions here.
   *
   * The perform() functions may create and execute several copies of the
   * transactor before succeeding or ultimately giving up.  If there is any
   * doubt over whether execution succeeded (this can happen if the connection
   * to the server is lost just before the backend can confirm success), it is
   * no longer retried and an in_doubt_error is thrown.
   *
   * Take care: no member functions will ever be invoked on the transactor
   * object you pass into perform().  The object you pass in only serves as a
   * "prototype" for the job to be done.  The perform() function will
   * copy-construct transactors from the original you passed in, executing the
   * copies only.  The original object remains "clean" in its original state.
   *
   * @{
   */

  /// Perform the transaction defined by a transactor-based object.
  /**
   * Invokes the given transactor, making at most Attempts attempts to perform
   * the encapsulated code.  If the code throws any exception other than
   * broken_connection, it will be aborted right away.
   *
   * @param T The transactor to be executed.
   * @param Attempts Maximum number of attempts to be made to execute T.
   */
  template<typename TRANSACTOR>
  void perform(const TRANSACTOR &T, int Attempts);			//[t4]

  /// Perform the transaction defined by a transactor-based object.
  /**
   * @param T The transactor to be executed.
   */
  template<typename TRANSACTOR>
  void perform(const TRANSACTOR &T) { perform(T, 3); }

  /**
   * @}
   */

  /// Suffix unique number to name to make it unique within session context
  /** Used internally to generate identifiers for SQL objects (such as cursors
   * and nested transactions) based on a given human-readable base name.
   */
  PGSTD::string adorn_name(const PGSTD::string &);			//[90]

  /**
   * @addtogroup escaping String escaping
   *
   * Use these functions to "groom" user-provided strings before using them in
   * your SQL statements.  This reduces the chance of failures when users type
   * unexpected characters, but more importantly, it helps prevent so-called SQL
   * injection attacks.
   *
   * To understand what SQL injection vulnerabilities are and why they should be
   * prevented, imagine you use the following SQL statement somewhere in your
   * program:
   *
   * @code
   *	TX.exec("SELECT number,amount "
   *		"FROM accounts "
   *		"WHERE allowed_to_see('" + userid + "','" + password + "')");
   * @endcode
   *
   * This shows a logged-in user important information on all accounts he is
   * authorized to view.  The userid and password strings are variables entered
   * by the user himself.
   *
   * Now, if the user is actually an attacker who knows (or can guess) the
   * general shape of this SQL statement, imagine he enters the following
   * password:
   *
   * @code
   *	x') OR ('x' = 'x
   * @endcode
   *
   * Does that make sense to you?  Probably not.  But if this is inserted into
   * the SQL string by the C++ code above, the query becomes:
   *
   * @code
   *	SELECT number,amount
   *	FROM accounts
   *	WHERE allowed_to_see('user','x') OR ('x' = 'x')
   * @endcode
   *
   * Is this what you wanted to happen?  Probably not!  The neat
   * allowed_to_see() clause is completely circumvented by the
   * "<tt>OR ('x' = 'x')</tt>" clause, which is always @c true.  Therefore, the
   * attacker will get to see all accounts in the database!
   *
   * To prevent this from happening, use the transaction's esc() function:
   *
   * @code
   *	TX.exec("SELECT number,amount "
   *		"FROM accounts "
   *		"WHERE allowed_to_see('" + TX.esc(userid) + "', "
   *			"'" + TX.esc(password) + "')");
   * @endcode
   *
   * Now, the quotes embedded in the attacker's string will be neatly escaped so
   * they can't "break out" of the quoted SQL string they were meant to go into:
   *
   * @code
   *	SELECT number,amount
   *	FROM accounts
   *	WHERE allowed_to_see('user', 'x'') OR (''x'' = ''x')
   * @endcode
   *
   * If you look carefully, you'll see that thanks to the added escape
   * characters (a single-quote is escaped in SQL by doubling it) all we get is
   * a very strange-looking password string--but not a change in the SQL
   * statement.
   */
  //@{
  /// Escape string for use as SQL string literal on this connection
  PGSTD::string esc(const char str[]);

  /// Escape string for use as SQL string literal on this connection
  PGSTD::string esc(const char str[], size_t maxlen);

  /// Escape string for use as SQL string literal on this connection
  PGSTD::string esc(const PGSTD::string &str);

  /// Escape binary string for use as SQL string literal on this connection
  PGSTD::string esc_raw(const unsigned char str[], size_t len);

  /// Escape and quote a string of binary data.
  PGSTD::string quote_raw(const unsigned char str[], size_t len);

  /// Escape and quote an SQL identifier for use in a query.
  PGSTD::string quote_name(const PGSTD::string &identifier);

  /// Represent object as SQL string, including quoting & escaping.
  /** Nulls are recognized and represented as SQL nulls. */
  template<typename T>
  PGSTD::string quote(const T &t)
  {
    if (string_traits<T>::is_null(t)) return "NULL";
    return "'" + this->esc(to_string(t)) + "'";
  }

  PGSTD::string quote(const binarystring &);
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
  void set_verbosity(error_verbosity verbosity) throw();
   /// Retrieve current error verbosity
  error_verbosity get_verbosity() const throw() {return m_verbosity;}

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
  PGSTD::vector<errorhandler *> get_errorhandlers() const;

protected:
  explicit connection_base(connectionpolicy &);
  void init();

  void close() throw ();
  void wait_read() const;
  void wait_read(long seconds, long microseconds) const;
  void wait_write() const;

private:

  result make_result(internal::pq::PGresult *rhs, const PGSTD::string &query);

  void PQXX_PRIVATE clearcaps() throw ();
  void PQXX_PRIVATE SetupState();
  void PQXX_PRIVATE check_result(const result &);

  void PQXX_PRIVATE InternalSetTrace() throw ();
  int PQXX_PRIVATE PQXX_PURE Status() const throw ();
  const char * PQXX_PURE ErrMsg() const throw ();
  void PQXX_PRIVATE Reset();
  void PQXX_PRIVATE RestoreVars();
  PGSTD::string PQXX_PRIVATE RawGetVar(const PGSTD::string &);
  void PQXX_PRIVATE process_notice_raw(const char msg[]) throw ();

  void read_capabilities() throw ();

  prepare::internal::prepared_def &find_prepared(const PGSTD::string &);

  prepare::internal::prepared_def &register_prepared(const PGSTD::string &);

  friend class internal::gate::connection_prepare_invocation;
  result prepared_exec(const PGSTD::string &,
	const char *const[],
	const int[],
	const int[],
	int);
  bool prepared_exists(const PGSTD::string &) const;

  /// Connection handle.
  internal::pq::PGconn *m_Conn;

  connectionpolicy &m_policy;

  /// Active transaction on connection, if any.
  internal::unique<transaction_base> m_Trans;

  PGSTD::list<errorhandler *> m_errorhandlers;

  /// File to trace to, if any
  PGSTD::FILE *m_Trace;

  typedef PGSTD::multimap<PGSTD::string, pqxx::notification_receiver *>
	receiver_list;
  /// Notification receivers.
  receiver_list m_receivers;

  /// Variables set in this session
  PGSTD::map<PGSTD::string, PGSTD::string> m_Vars;

  typedef PGSTD::map<PGSTD::string, prepare::internal::prepared_def> PSMap;

  /// Prepared statements existing in this section
  PSMap m_prepared;

  /// Server version
  int m_serverversion;

  /// Stacking counter: known objects that can't be auto-reactivated
  internal::reactivation_avoidance_counter m_reactivation_avoidance;

  /// Unique number to use as suffix for identifiers (see adorn_name())
  int m_unique_id;

  /// Have we successfully established this connection?
  bool m_Completed;

  /// Is reactivation currently inhibited?
  bool m_inhibit_reactivation;

  /// Set of session capabilities
  PGSTD::bitset<cap_end> m_caps;

  /// Current verbosity level
  error_verbosity m_verbosity;

  friend class internal::gate::connection_errorhandler;
  void PQXX_PRIVATE register_errorhandler(errorhandler *);
  void PQXX_PRIVATE unregister_errorhandler(errorhandler *) throw ();

  friend class internal::gate::connection_transaction;
  result PQXX_PRIVATE Exec(const char[], int Retries);
  void PQXX_PRIVATE RegisterTransaction(transaction_base *);
  void PQXX_PRIVATE UnregisterTransaction(transaction_base *) throw ();
  bool PQXX_PRIVATE ReadCopyLine(PGSTD::string &);
  void PQXX_PRIVATE WriteCopyLine(const PGSTD::string &);
  void PQXX_PRIVATE EndCopyWrite();
  void PQXX_PRIVATE RawSetVar(const PGSTD::string &, const PGSTD::string &);
  void PQXX_PRIVATE AddVariables(const PGSTD::map<PGSTD::string,
      PGSTD::string> &);

  friend class internal::gate::connection_largeobject;
  internal::pq::PGconn *RawConnection() const { return m_Conn; }

  friend class internal::gate::connection_notification_receiver;
  void add_receiver(notification_receiver *);
  void remove_receiver(notification_receiver *) throw ();

  friend class internal::gate::connection_pipeline;
  void PQXX_PRIVATE start_exec(const PGSTD::string &);
  bool PQXX_PRIVATE consume_input() throw ();
  bool PQXX_PRIVATE is_busy() const throw ();
  int PQXX_PRIVATE encoding_code();
  internal::pq::PGresult *get_result();

  friend class internal::gate::connection_dbtransaction;

  friend class internal::gate::connection_sql_cursor;
  void add_reactivation_avoidance_count(int);

  friend class internal::gate::connection_reactivation_avoidance_exemption;

  friend class internal::gate::connection_parameterized_invocation;
  result parameterized_exec(
	const PGSTD::string &query,
	const char *const params[],
	const int paramlengths[],
	const int binaries[],
	int nparams);

  // Not allowed:
  connection_base(const connection_base &);
  connection_base &operator=(const connection_base &);
};



#ifdef PQXX_HAVE_AUTO_PTR
/// @deprecated Create an @c errorhandler instead.
struct PQXX_LIBEXPORT PQXX_NOVTABLE noticer :
  PGSTD::unary_function<const char[], void>
{
  virtual ~noticer() throw () {}
  virtual void operator()(const char[]) throw () =0;
};
/// @deprecated Use @c quiet_errorhandler instead.
struct PQXX_LIBEXPORT nonnoticer : noticer
{
  virtual void operator()(const char[]) throw () {}
};
/// @deprecated Create an @c errorhandler instead.
class PQXX_LIBEXPORT scoped_noticer : errorhandler
{
public:
  scoped_noticer(connection_base &c, PGSTD::auto_ptr<noticer> t) throw () :
    errorhandler(c), m_noticer(t.release()) {}
protected:
  scoped_noticer(connection_base &c, noticer *t) throw () :
    errorhandler(c), m_noticer(t) {}
  virtual bool operator()(const char msg[]) throw ()
  {
    (*m_noticer)(msg);
    return false;
  }
private:
  PGSTD::auto_ptr<noticer> m_noticer;
};
/// @deprecated Create a @c quiet_errorhandler instead.
class PQXX_LIBEXPORT disable_noticer : scoped_noticer
{
public:
  explicit disable_noticer(connection_base &c) :
    scoped_noticer(c, new nonnoticer) {}
};
#endif


namespace internal
{

/// Scoped exemption to reactivation avoidance
class PQXX_LIBEXPORT reactivation_avoidance_exemption
{
public:
  explicit reactivation_avoidance_exemption(connection_base &C);
  ~reactivation_avoidance_exemption();

  void close_connection() throw () { m_open = false; }

private:
  connection_base &m_home;
  int m_count;
  bool m_open;
};


void wait_read(const internal::pq::PGconn *);
void wait_read(const internal::pq::PGconn *, long seconds, long microseconds);
void wait_write(const internal::pq::PGconn *);
} // namespace pqxx::internal


} // namespace pqxx

#include "pqxx/compiler-internal-post.hxx"

#endif
