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
 * Copyright (c) 2001-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/connection_base"


/* Methods tested in eg. self-test program test001 are marked with "//[t1]"
 */

namespace pqxx
{

/// Connection class; represents an immediate connection to a database.
/** This is the class you typically need when you first work with a database 
 * through libpqxx.  Its constructor immediately opens a connection.  Another
 * option is to defer setting up the underlying connection to the database until
 * it's actually needed; the lazyconnection class implements such "lazy" 
 * behaviour.  Most of the documentation that you'll need to use this class is 
 * in its base class, connection_base.
 *
 * The advantage of having an "immediate" connection (represented by this class)
 * is that errors in setting up the connection will probably occur during 
 * construction of the connection object, rather than at some later point 
 * further down your program.
 *
 * This class is a near-trivial implementation of the connection_base
 * interface defined in connection_base.hxx.  All features of any interest to
 * client programmers are defined there.
 */
class PQXX_LIBEXPORT connection : public connection_base
{
public:
  /// Constructor.  Sets up connection without connection string.
  /** Only default values will be used, or if any of the environment variables
   * recognized by libpq (PGHOST etc.) are defined, those will override the
   * defaults.
   */
  connection();								//[t1]

  /// Constructor.  Sets up connection based on PostgreSQL connection string.
  /** 
   * @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.  These values
   * override any environment variables that may have been set for the same 
   * parameters.
   *
   * The README file for libpqxx gives a quick overview of how connection
   * strings work; see the PostgreSQL documentation (particularly for libpq, the
   * C-level interface) for a complete list.
   */
  explicit connection(const PGSTD::string &ConnInfo);			//[t2]

  /// Constructor.  Sets up connection based on PostgreSQL connection string.
  /** @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.  As a special
   * case, a null pointer is taken as the empty string.
   *
   * The README file for libpqxx gives a quick overview of how connection
   * strings work; see the PostgreSQL documentation (particularly for libpq, the
   * C-level interface) for a complete list.
   */
  explicit connection(const char ConnInfo[]);				//[t3]

  virtual ~connection() throw ();

private:
  virtual void startconnect();
  virtual void completeconnect();
};


/// Lazy connection class; represents a deferred connection to a database.
/** This is connection's lazy younger brother.  Its constructor does not 
 * actually open a connection; the connection is only created when it is 
 * actually used.
 *
 * This class is a trivial implementation of the connection_base interface 
 * defined in connection_base.hxx.  All features of any interest to client 
 * programmers are defined there.
 */
class PQXX_LIBEXPORT lazyconnection : public connection_base
{
public:
  /// Constructor.  Sets up lazy connection.
  lazyconnection();							//[t23]

  /// Constructor.  Sets up lazy connection.
  /**
   * @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.
   *
   * The README file for libpqxx gives a quick overview of how connection
   * strings work; see the PostgreSQL documentation (particularly for libpq, the
   * C-level interface) for a complete list.
   */
  explicit lazyconnection(const PGSTD::string &ConnInfo);		//[t21]

  /// Constructor.  Sets up lazy connection.
  /** 
   * @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.  As a special
   * case, a null pointer is taken as the empty string.
   *
   * The README file for libpqxx gives a quick overview of how connection
   * strings work; see the PostgreSQL documentation (particularly for libpq, the
   * C-level interface) for a complete list.
   */
  explicit lazyconnection(const char ConnInfo[]);			//[t22]

  virtual ~lazyconnection() throw ();

private:
  virtual void startconnect() {}
  virtual void completeconnect();
};


/// Asynchronous connection class; connects "in the background"
class PQXX_LIBEXPORT asyncconnection : public connection_base
{
public:
  /// Constructor.  Initiates asynchronous connection setup.
  asyncconnection(); 							//[t63]

  /// Constructor.  Initiates asynchronous connection setup.
  /**
   * @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.
   *
   * The README file for libpqxx gives a quick overview of how connection
   * strings work; see the PostgreSQL documentation (particularly for libpq, the
   * C-level interface) for a complete list.
   */
  explicit asyncconnection(const PGSTD::string &ConnInfo);		//[t65]

  /// Constructor.  Initiates asynchronous connection setup.
  /** 
   * @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.  As a special
   * case, a null pointer is taken as the empty string.
   *
   * The README file for libpqxx gives a quick overview of how connection
   * strings work; see the PostgreSQL documentation (particularly for libpq, the
   * C-level interface) for a complete list.
   */
  explicit asyncconnection(const char ConnInfo[]);			//[t64]

  virtual ~asyncconnection() throw ();

private:
  virtual void startconnect();
  virtual void completeconnect();
  virtual void dropconnect() throw () { m_connecting = false; }

  /// Is a connection attempt in progress?
  bool m_connecting;
};

}


