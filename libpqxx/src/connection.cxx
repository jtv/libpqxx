/*-------------------------------------------------------------------------
 *
 *   FILE
 *	connection.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::connection and pqxx::lazyconnection classes.
 *   Different ways of setting up a backend connection.
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include "pqxx/connection"

pqxx::connection::connection() :
  connection_base(0)
{
  Connect();
}

pqxx::connection::connection(const PGSTD::string &ConnInfo) :
  connection_base(ConnInfo)
{
  Connect();
}

pqxx::connection::connection(const char ConnInfo[]) :
  connection_base(ConnInfo)
{
  Connect();
}

// Work around problem with Sun CC 5.1
pqxx::connection::~connection()
{
}


// Work around problem with Sun CC 5.1
pqxx::lazyconnection::~lazyconnection()
{
}


