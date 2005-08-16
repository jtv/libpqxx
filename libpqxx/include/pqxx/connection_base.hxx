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
 * Copyright (c) 2001-2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/libcompiler.h"

#include <map>
#include <memory>

#ifdef _WIN32
#include <winsock2.h>	// for fd_set
#endif	// _WIN32

#include "pqxx/except"
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
class result;
class transaction_base;
class trigger;


/**
 * @addtogroup noticer Error/warning output
 */
//@{
/// Base class for user-definable error/warning message processor
/** To define a custom method of handling notices, derive a new class from
 * noticer and override the virtual function "operator()(const char[]) throw()"
 * to process the message passed to it.
 */
struct PQXX_LIBEXPORT noticer : PGSTD::unary_function<const char[], void>
{
  noticer(){}		// Silences bogus warning in some gcc versions
  virtual ~noticer() throw () {}
  virtual void operator()(const char Msg[]) throw () =0;
};


/// No-op message noticer; produces no output
struct PQXX_LIBEXPORT nonnoticer : noticer
{
  nonnoticer(){}	// Silences bogus warning in some gcc versions
  virtual void operator()(const char []) throw () {}
};
//@}


/**
 * @addtogroup connection Connection classes
 */
//@{
/// connection_base abstract base class; represents a connection to a database.
/** This is the first class to look at when you wish to work with a database
 * through libpqxx.  Depending on the implementing concrete child class, a
 * connection can be automatically opened when it is constructed, or when it is
 * first used.  The connection is automatically closed upon destruction, if it
 * hasn't already been closed manually.
 *
 * To query or manipulate the database once connected, use one of the
 * transaction classes (see pqxx/transaction_base.hxx) or preferably the
 * transactor framework (see pqxx/transactor.hxx).
 *
 * A word of caution: if a network connection to the database server fails, the
 * connection will be restored automatically (although any transaction going on
 * at the time will have to be aborted).  This also means that any information
 * set in previous transactions that is not stored in the database, such as
 * connection-local variables defined with PostgreSQL's SET command, will be
 * lost.  Whenever you need to create such state, either do it within each
 * transaction that may need it, or if at all possible, use specialized
 * functions made available by libpqxx.  Always avoid raw queries if libpqxx
 * offers a dedicated function for the same purpose.
 */
class PQXX_LIBEXPORT connection_base
{
public:
  /// Set up connection based on PostgreSQL connection string
  /**
   * @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.  These values
   * override any of the environment variables recognized by libpq that may have
   * been defined for the same parameters.
   *
   * The README file for libpqxx gives a quick overview of how connection
   * strings work; see the PostgreSQL documentation (particularly for libpq, the
   * C-level interface) for a complete list.
   */
  explicit connection_base(const PGSTD::string &ConnInfo);		//[t2]

  /// Set up connection based on PostgreSQL connection string
  /** @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.  As a special
   * case, a null pointer is taken as the empty string.
   */
  explicit connection_base(const char ConnInfo[]);			//[t2]

  /// Destructor.  Implicitly closes the connection.
  virtual ~connection_base() =0;					//[t1]

  /**
   * @name Connection state
   */
  //@{
  /// Explicitly close connection.
  void disconnect() throw ();						//[t2]

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
   * it is needed, or when the client program requests it (although for reasons
   * of integrity, never inside a transaction; but retrying the whole
   * transaction may implicitly cause the connection to be restored).  In normal
   * use this is quite a convenient thing to have and presents a simple, safe,
   * predictable interface.
   *
   * There is at least one situation where this feature is not desirable,
   * however.  Although most session state (prepared statements, session
   * variables) is automatically restored to its working state upon connection
   * reactivation, temporary tables are not.
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
  void inhibit_reactivation(bool inhibit) 				//[t86]
	{ m_inhibit_reactivation=inhibit; }

  /// Is this connection open at the moment?
  /** @warning This function is @b not needed in most code.  Resist the
   * temptation to check it after opening a connection; rely on the exception
   * that will be thrown on connection failure instead.
   */
  bool is_open() const throw ();					//[t1]
  //@}

  /**
   * @addtogroup noticer Error/warning output
   */
  //@{
  // TODO: Define a default noticer (mainly to help out Windows users)
  /// Set handler for postgresql errors or warning messages.
  /** The use of auto_ptr implies ownership, so unless the returned value is
   * copied to another auto_ptr, it will be deleted directly after the call.
   * This may be important when running under Windows, where a DLL cannot free
   * memory allocated by the main program.  The auto_ptr will delete the object
   * from your code context, rather than from inside the library.
   *
   * If a noticer exists when the connection_base is destructed, it will also be
   * deleted.
   *
   * @param N New message handler; must not be null or equal to the old one
   * @return Previous handler
   */
  PGSTD::auto_ptr<noticer> set_noticer(PGSTD::auto_ptr<noticer> N)
    throw ();								//[t14]
  noticer *get_noticer() const throw () { return m_Noticer.get(); }	//[t14]

  /// Invoke notice processor function.  The message should end in newline.
  void process_notice(const char[]) throw ();				//[t14]
  /// Invoke notice processor function.  Newline at end is recommended.
  void process_notice(const PGSTD::string &) throw ();			//[t14]
  //@}

  /// Enable tracing to a given output stream, or NULL to disable.
  void trace(FILE *) throw ();						//[t3]

  /**
   * @name Connection properties
   *
   * These are probably not needed very often: since most are derived from
   * information supplied by the client program itself, but are included for
   * completeness.
   */
  //@{
  /// Name of database we're connected to, if any.
  const char *dbname();							//[t1]

  /// Database user ID we're connected under, if any.
  const char *username();						//[t1]

  /// Address of server (NULL for local connections).
  const char *hostname();						//[t1]

  /// Server port number we're connected to.
  const char *port();		 					//[t1]

  /// Full connection string as used to set up this connection.
  const char *options() const throw () 					//[t1]
  	{ return m_ConnInfo.c_str(); }


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
  int backendpid() const throw ();					//[t1]

  /// Socket currently used for connection, or -1 for none
  /** Query the current socket number.  This is intended for event loops based
   * on functions such as select() or poll().
   *
   * @warning Don't keep this value anywhere, and always be prepared for the
   * possibility that there is no socket.  The socket may change or even go away
   * during any invocation of libpqxx code.
   */
  int sock() const throw ();						//[]

  /// Session capabilities
  /** Some functionality is only available in certain versions of the backend,
   * or only when speaking certain versions of the communications protocol that
   * connects us to the backend.  This includes clauses for SQL statements that
   * were not accepted in older database versions, but are required in newer
   * versions to get the same behaviour.
   */
  enum capability
  {
    /// Can we specify WITH OIDS with CREATE TABLE?  If we can, we should.
    cap_create_table_with_oids,

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
   * try not to rely on this function being exactly right.  Older versions of
   * libpq may not detect any capabilities.
   */
  bool supports(capability) const throw ();

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
   * @addtogroup notification Notifications and Triggers
   */
  //@{
  /// Check for pending trigger notifications and take appropriate action.
  /** 
   * All notifications found pending at call time are processed by finding
   * any matching triggers and invoking those.  If no triggers matched the
   * notification string, none are invoked but the notification is considered
   * processed.
   *
   * Exceptions thrown by client-registered trigger handlers are reported, but
   * not passed on outside this function.
   *
   * @return Number of notifications processed
   */
  int get_notifs();							//[t4]


  /// Wait for a trigger notification notification to come in
  /** The wait may also be terminated by other events, such as the connection
   * to the backend failing.  Any pending or received notifications are
   * processed as part of the call.
   *
   * @return Number of notifications processed
   */
  int await_notification();						//[t78]

  /// Wait for a trigger notification to come in, or for given timeout to pass
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
   * Prepared statements are not held to transaction boundaries; a statement
   * defined inside a transaction will remain defined outside that transaction,
   * even if the transaction is subsequently aborted.  Once a statement has been
   * prepared, only closing the connection or explicitly "unpreparing" it can
   * make it go away.
   *
   * Use the transaction classes' exec_prepared() functions to execute a
   * prepared statement.
   *
   * @warning Prepared statements are not necessarily defined on the backend
   * right away; they may be cached by libpqxx.  This means that statements may
   * be prepared before the connection is fully established, and that it's
   * relatively cheap to pre-prepare lots of statements that may or may not be
   * used during the session.  It also means, however, that errors in the
   * prepared statement may not show up until it is first used.  Such failure
   * may cause the current transaction to roll back.
   *
   * @warning Never try to prepare, execute, or unprepare a prepared statement
   * manually using direct SQL queries.  Always use the functions provided by
   * libpqxx.
   */
  //@{
  /// Define prepared statement that takes no parameters
  void prepare(const PGSTD::string &name, const PGSTD::string &def)	//[t85]
	{ pq_prepare(name, def, ""); }

  /// Define prepared statement with given parameter list
  template<typename ITER>
    void prepare(const PGSTD::string &name,
	const PGSTD::string &def,
	ITER beginparms,
	ITER endparms)							//[t85]
  {
    pq_prepare(name, def, 
	(beginparms==endparms) ? 
		"" : ("("+separated_list(",",beginparms,endparms)+")"));
  }

  /// Define prepared statement with given parameter list
  template<typename CNTNR>
    void prepare(const PGSTD::string &name,
	const PGSTD::string &def,
	const CNTNR &params)						//[t85]
	{ prepare(name, def, params.begin(), params.end()); }

  /// Drop prepared statement
  void unprepare(const PGSTD::string &name);				//[t85]
  //@}


  /**
   * @addtogroup transactor Transactor framework
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
   */
  //@{
  /// Perform the transaction defined by a transactor-based object.
  /** 
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
  //@}

#ifdef PQXX_DEPRECATED_HEADERS
  /**
   * @name 1.x API
   * 
   * These are all deprecated; they were defined in the libpqxx 1.x API but are
   * no longer actively supported.
   */
  //@{
  /// @deprecated Use disconnect() instead
  void Disconnect() throw () { disconnect(); }
  /// @deprecated Use perform() instead
  template<typename TRANSACTOR> void Perform(const TRANSACTOR &T, int A=3)
	{ return perform(T,A); }
  /// @deprecated Use set_noticer() instead
  PGSTD::auto_ptr<noticer> SetNoticer(PGSTD::auto_ptr<noticer> N)
  	{ return set_noticer(N); }
  /// @deprecated Use get_noticer() instead
  noticer *GetNoticer() const throw ()
  	{ return get_noticer(); }
  /// @deprecated Use process_notice() instead
  void ProcessNotice(const char msg[]) throw () { return process_notice(msg); }
  /// @deprecated Use process_notice() instead
  void ProcessNotice(const PGSTD::string &msg) throw ()
  	{ return process_notice(msg); }
  /// @deprecated Use trace() instead
  void Trace(FILE *F) { trace(F); }
  /// @deprecated Use get_notifs() instead
  void GetNotifs() { get_notifs(); }
  /// @deprecated Use dbname() instead
  const char *DbName() { return dbname(); }
  /// @deprecated Use username() instead
  const char *UserName() { return username(); }
  /// @deprecated Use hostname() instead
  const char *HostName() { return hostname(); }
  /// @deprecated Use port() instead
  const char *Port() { return port(); }
  /// @deprecated Use options() instead
  const char *Options() const throw () { return options(); }
  /// @deprecated Use backendpid() instead
  int BackendPID() const { return backendpid(); }
  /// @deprecated Use activate() instead
  void Activate() { activate(); }
  /// @deprecated Use deactivate() instead
  void Deactivate() { deactivate(); }
  /// @deprecated Use set_client_encoding() instead
  void SetClientEncoding(const PGSTD::string &E) { set_client_encoding(E); }
  /// @deprecated Use set_variable() instead
  void SetVariable(const PGSTD::string &Var, const PGSTD::string &Val)
  	{ set_variable(Var, Val); }
  //@}
#endif


protected:
  /// Overridable: initiate a connection
  /** @callgraph */
  virtual void startconnect() =0;

  /// Overridable: complete an initiated connection
  /** @callgraph */
  virtual void completeconnect() =0;

  /// Overridable: drop any specialized state related to connection attempt
  /** @callgraph */
  virtual void dropconnect() throw () {}

  /// For implementation classes: do we have a connection structure?
  internal::pq::PGconn *get_conn() const throw () { return m_Conn; }

  /// For implementation classes: set connection structure pointer
  void set_conn(internal::pq::PGconn *C) throw () { m_Conn = C; }

  void close() throw ();
  void wait_read() const;
  void wait_read(long seconds, long microseconds) const;
  void wait_write() const;

private:
  void PQXX_PRIVATE SetupState();
  void PQXX_PRIVATE check_result(const result &, const char Query[]);

  void PQXX_PRIVATE InternalSetTrace() throw ();
  int PQXX_PRIVATE Status() const throw ();
  const char *ErrMsg() const throw ();
  void PQXX_PRIVATE Reset();
  void PQXX_PRIVATE RestoreVars();
  int PQXX_PRIVATE set_fdmask() const;
  void PQXX_PRIVATE clear_fdmask() throw ();
  PGSTD::string PQXX_PRIVATE RawGetVar(const PGSTD::string &);
  void PQXX_PRIVATE process_notice_raw(const char msg[]) throw ();

  void read_capabilities() const throw ();

  /// Connection string
  PGSTD::string m_ConnInfo;

  /// Connection handle
  internal::pq::PGconn *m_Conn;
  /// Active transaction on connection, if any
  internal::unique<transaction_base> m_Trans;

  /// User-defined notice processor, if any
  PGSTD::auto_ptr<noticer> m_Noticer;
  /// File to trace to, if any
  FILE *m_Trace;

  typedef PGSTD::multimap<PGSTD::string, pqxx::trigger *> TriggerList;
  /// Triggers this session is listening on
  TriggerList m_Triggers;

  /// Variables set in this session
  PGSTD::map<PGSTD::string, PGSTD::string> m_Vars;

  /// Internal state: definition of a prepared statement
  struct PQXX_PRIVATE prepared_def
  {
    /// Text of prepared query
    PGSTD::string definition;
    /// Parameter type list
    PGSTD::string parameters;
    /// Has this prepared statement been prepared in the current session?
    bool registered;

    prepared_def() : definition(), parameters(), registered(false) {}
    prepared_def(const PGSTD::string &def, const PGSTD::string &params) :
      definition(def), parameters(params), registered(false) {}
  };

  typedef PGSTD::map<PGSTD::string, prepared_def> PSMap;

  /// Prepared statements existing in this section
  PSMap m_prepared;

  mutable fd_set m_fdmask;

  /// Set of session capabilities
  mutable bool m_caps[cap_end];
  /// Have capabilities for this connection been determined?
  mutable bool m_caps_known;

  /// Is reactivation currently inhibited?
  bool m_inhibit_reactivation;

  friend class transaction_base;
  result PQXX_PRIVATE Exec(const char[], int Retries);
  void pq_prepare(const PGSTD::string &name,
      const PGSTD::string &def,
      const PGSTD::string &params);
  result pq_exec_prepared(const PGSTD::string &, int, const char *const *);
  void PQXX_PRIVATE RegisterTransaction(transaction_base *);
  void PQXX_PRIVATE UnregisterTransaction(transaction_base *) throw ();
  void PQXX_PRIVATE MakeEmpty(result &);
  bool PQXX_PRIVATE ReadCopyLine(PGSTD::string &);
  void PQXX_PRIVATE WriteCopyLine(const PGSTD::string &);
  void PQXX_PRIVATE EndCopyWrite();
  void PQXX_PRIVATE start_exec(const PGSTD::string &);
  internal::pq::PGresult *get_result();

  void PQXX_PRIVATE RawSetVar(const PGSTD::string &, const PGSTD::string &);
  void PQXX_PRIVATE AddVariables(const PGSTD::map<PGSTD::string,
      PGSTD::string> &);

  friend class largeobject;
  internal::pq::PGconn *RawConnection() const { return m_Conn; }

  friend class trigger;
  void AddTrigger(trigger *);
  void RemoveTrigger(trigger *) throw ();

  friend class pipeline;
  bool PQXX_PRIVATE consume_input() throw ();
  bool PQXX_PRIVATE is_busy() const throw ();

  // Not allowed:
  connection_base(const connection_base &);
  connection_base &operator=(const connection_base &);
};

//@}


// Put this here so on Windows, any noticers can be deleted in caller's context
inline connection_base::~connection_base()
{
  // Visual C++ seems to have a problem with output during destructors!
#ifdef PQXX_QUIET_DESTRUCTORS
  set_noticer(PGSTD::auto_ptr<noticer>(new nonnoticer()));
#endif
}


namespace internal
{

/// Temporarily set different noticer for connection
/** Set different noticer in given connection for the duration of the
 * scoped_noticer's lifetime.  After that, the original noticer is restored.
 *
 * No effort is made to respect any new noticer that may have been set in the
 * meantime, so don't do that.
 */
class PQXX_LIBEXPORT scoped_noticer
{
public:
  /// Start period of different noticer
  /**
   * @param c connection object whose noticer should be temporarily changed
   * @param t temporary noticer object to use; will be destroyed on completion
   */
  scoped_noticer(connection_base &c, PGSTD::auto_ptr<noticer> t) throw () :
    m_c(c), m_org(c.set_noticer(t)) { }

  ~scoped_noticer() { m_c.set_noticer(m_org); }

private:
  connection_base &m_c;
  PGSTD::auto_ptr<noticer> m_org;

  /// Not allowed
  scoped_noticer();
  scoped_noticer(const scoped_noticer &);
  scoped_noticer operator=(const scoped_noticer &);
};


/// Temporarily disable the notice processor
class PQXX_LIBEXPORT disable_noticer : scoped_noticer
{
public:
  explicit disable_noticer(connection_base &c) :
    scoped_noticer(c, PGSTD::auto_ptr<noticer>(new nonnoticer)) {}
};


} // namespace pqxx::internal


} // namespace pqxx

