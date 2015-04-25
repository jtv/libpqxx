/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/connection.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::connection and pqxx::lazyconnection classes.
 *   Different ways of setting up a backend connection.
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/connection instead.
 *
 * Copyright (c) 2001-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_CONNECTION
#define PQXX_H_CONNECTION

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/connectionpolicy"
#include "pqxx/basic_connection"

namespace pqxx
{

/**
 * @addtogroup connection Connection classes
 *
 * The connection classes are where the use of a database begins.  You must
 * connect to a database in order to access it.  Your connection represents a
 * session with the database.  In the context of that connection you can create
 * transactions, which in turn you can use to execute SQL.  A connection can
 * have only one regular transaction open at a time, but you can break your work
 * down into any number of consecutive transactions and there is also some
 * support for transaction nesting (using the subtransaction class).
 *
 * Many things come together in the connection classes.  Handling of error and
 * warning messages, for example, is defined by @e errorhandlers in the context
 * of a connection.  Prepared statements are also defined here.
 * 
 * Several types of connections are available, including plain connection and
 * lazyconnection.  These types are typedefs combining a derivative of the
 * connection_base class (where essentially all connection-related functionality
 * is defined) with a policy class which governs how the connection is to be
 * established.  You pass details such as the database you wish to connect to,
 * username and password, and so on as as PostgreSQL "connection string" and
 * certain environment variables that you can learn more about from the core
 * postgres documentation.
 *
 * See the connection_base documentation for a full list of features inherited
 * by all connection classes.  Connections can be deactivated and reactivated if
 * needed (within reason, of course--you can't do this in the middle of a
 * transaction), and where possible, disabled or broken connections are
 * transparently re-enabled when you use them again.  This is called
 * "reactivation," and you may need to understand it because you'll want it
 * disabled in certain situations.
 *
 * You can also set certain variables defined by the backend to influence its
 * behaviour for the duration of your session, such as the applicable text
 * encoding.  You can query the connection's capabilities (because some features
 * will depend on the versions of libpq and of the server backend that you're
 * using) and parameters that you set in your connection string and/or
 * environment variables.
 *
 * @{
 */

/// Connection policy; creates an immediate connection to a database.
/** This is the policy you typically need when you work with a database through
 * libpqxx.  It connects to the database immediately.  Another option is to
 * defer setting up the underlying connection to the database until it's
 * actually needed; the connect_lazy policy implements such "lazy" * behaviour.
 *
 * The advantage of having an "immediate" connection (as this policy gives you)
 * is that any errors in setting up the connection will occur during
 * construction of the connection object, rather than at some later point
 * further down your program.
 */
class PQXX_LIBEXPORT connect_direct : public connectionpolicy
{
public:
  explicit connect_direct(const std::string &opts) : connectionpolicy(opts) {}
  virtual handle do_startconnect(handle) PQXX_OVERRIDE;
};

/// The "standard" connection type: connect to database right now
typedef basic_connection<connect_direct> connection;


/// Lazy connection policy; causes connection to be deferred until first use.
/** This is connect_direct's lazy younger brother.  It does not attempt to open
 * a connection right away; the connection is only created when it is actually
 * used.
 */
class PQXX_LIBEXPORT connect_lazy : public connectionpolicy
{
public:
  explicit connect_lazy(const std::string &opts) : connectionpolicy(opts) {}
  virtual handle do_completeconnect(handle) PQXX_OVERRIDE;
};


/// A "lazy" connection type: connect to database only when needed
typedef basic_connection<connect_lazy> lazyconnection;


/// Asynchronous connection policy; connects "in the background"
/** Connection is initiated immediately, but completion is deferred until the
 * connection is actually needed.
 *
 * This may help performance by allowing the client to do useful work while
 * waiting for an answer from the server.
 */
class PQXX_LIBEXPORT connect_async : public connectionpolicy
{
public:
  explicit connect_async(const std::string &opts);
  virtual handle do_startconnect(handle) PQXX_OVERRIDE;
  virtual handle do_completeconnect(handle) PQXX_OVERRIDE;
  virtual handle do_dropconnect(handle) PQXX_NOEXCEPT PQXX_OVERRIDE;
  virtual bool is_ready(handle) const PQXX_NOEXCEPT PQXX_OVERRIDE;

private:
  /// Is a connection attempt in progress?
  bool m_connecting;
};


/// "Asynchronous" connection type: start connecting, but don't wait for it
typedef basic_connection<connect_async> asyncconnection;


/// Nonfunctional, always-down connection policy for testing/debugging purposes
/** @warning You don't want to use this policy in normal code.
 * Written for debugging and testing, this "connection policy" always fails to
 * connect, and the internal connection pointer always remains null.
 */
class PQXX_LIBEXPORT connect_null  : public connectionpolicy
{
public:
  explicit connect_null(const std::string &opts) : connectionpolicy(opts) {}
};


/// A "dummy" connection type: don't connect to any database at all
typedef basic_connection<connect_null> nullconnection;

/**
 * @}
 */

}

#include "pqxx/compiler-internal-post.hxx"

#endif

