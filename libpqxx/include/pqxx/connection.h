/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/connection.h
 *
 *   DESCRIPTION
 *      definition of the pqxx::Connection and pqxx::LazyConnection classes.
 *   Different ways of setting up a backend connection.
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_CONNECTION_H
#define PQXX_CONNECTION_H

#include "pqxx/connectionitf.h"


/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */

namespace pqxx
{

/// Connection class; represents an immediate connection to a database.
/** This is the class you typically need when you first work with a database 
 * through libpqxx.  Its constructor immediately opens a connection.  Another
 * option is to defer actual connection to the database until it's actually
 * needed; the LazyConnection class implements such "lazy" behaviour.
 *
 * The advantage of having an immediate connection is that errors in setting
 * up the connection will occur during construction of the connection object,
 * rather than at some later point further down your program.
 *
 * This class is a near-trivial implementation of the ConnectionItf
 * interface defined in connectionitf.h.  All features of any interest to
 * client programmers are defined there.
 */
class PQXX_LIBEXPORT Connection : public ConnectionItf
{
public:
  /// Constructor.  Sets up connection based on PostgreSQL connection string.
  /** @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.
   */
  explicit Connection(const PGSTD::string &ConnInfo);			//[t2]

  /// Constructor.  Sets up connection based on PostgreSQL connection string.
  /** @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.  As a special
   * case, a null pointer is taken as the empty string.
   */
  explicit Connection(const char ConnInfo[]);				//[t1]
};


/// Lazy connection class; represents a deferred connection to a database.
/** This is Connection's lazy younger brother.  Its constructor does not 
 * actually open a connection; the connection is only created when it is 
 * actually used.
 *
 * This class is a trivial implementation of the ConnectionItf interface 
 * defined in connectionitf.h.  All features of any interest to client 
 * programmers are defined there.
 */
class PQXX_LIBEXPORT LazyConnection : public ConnectionItf
{
public:
  /// Constructor.  Sets up lazy connection.
  /** @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.
   */
  explicit LazyConnection(const PGSTD::string &ConnInfo) :		//[t21]
    ConnectionItf(ConnInfo) {}

  /// Constructor.  Sets up lazy connection.
  /** @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.  As a special
   * case, a null pointer is taken as the empty string.
   */
  explicit LazyConnection(const char ConnInfo[]) :			//[t22]
    ConnectionItf(ConnInfo) {}
};

}

#endif

