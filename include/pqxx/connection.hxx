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
 * Copyright (c) 2001-2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/connectionpolicy"
#include "pqxx/basic_connection"

namespace pqxx
{

/**
 * @addtogroup connection Connection classes
 */
//@{


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
  explicit connect_direct(const PGSTD::string &opts) : connectionpolicy(opts) {}
  virtual handle do_startconnect(handle);
};


typedef basic_connection<connect_direct> connection;


/// Lazy connection policy; causes connection to be deferred until first use.
/** This is connect_direct's lazy younger brother.  It does not open a
 * connection right away; the connection is only created when it is actually
 * used.
 */
class PQXX_LIBEXPORT connect_lazy : public connectionpolicy
{
public:
  explicit connect_lazy(const PGSTD::string &opts) : connectionpolicy(opts) {}
  virtual handle do_completeconnect(handle);
};


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
  explicit connect_async(const PGSTD::string &opts);
  virtual handle do_startconnect(handle);
  virtual handle do_completeconnect(handle);
  virtual handle do_dropconnect(handle) throw ();

private:
  /// Is a connection attempt in progress?
  bool m_connecting;
};


typedef basic_connection<connect_async> asyncconnection;


/// Nonfunctional, always-down connection policy for debugging purposes
/** @warning You don't want to use this policy in normal code.
 * Written purely for debugging of exception handling, this "connection policy"
 * always fails to connect, and the internal connection pointer always remains
 * null.
 */
class PQXX_LIBEXPORT connect_null  : public connectionpolicy
{
public:
  explicit connect_null(const PGSTD::string &opts) : connectionpolicy(opts) {}
};

typedef basic_connection<connect_null> nullconnection;


//@}

}

