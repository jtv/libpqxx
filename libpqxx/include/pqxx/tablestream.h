/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/tablestream.h
 *
 *   DESCRIPTION
 *      This header is deprecated.  Use "tablestream" instead (no ".h")
 *   pqxx::tablestream provides optimized batch access to a database table
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_TABLESTREAM_H
#define PQXX_TABLESTREAM_H

#include "pqxx/util.h"
#include "pqxx/tablestream"

namespace pqxx
{

/// @deprecated For compatibility with the old TableStream class
typedef tablestream TableStream;

}

#endif

