/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/connection.h
 *
 *   DESCRIPTION
 *      This header is deprecated.  Use "connection" instead (no ".h")
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_CONNECTION_H
#define PQXX_CONNECTION_H

#include "pqxx/util.h"
#include "pqxx/connection"
#include "pqxx/transactor"

namespace pqxx
{
/// @deprecated For compatibility with old Connection class
typedef connection Connection;


/// @deprecated For compatibility with old LazyConnection class
typedef lazyconnection LazyConnection;
}

#endif

