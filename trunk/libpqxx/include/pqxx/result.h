/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/result.h
 *
 *   DESCRIPTION
 *      This header is deprecated.  Use "result" instead (no ".h")
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_RESULT_H
#define PQXX_RESULT_H

#include "pqxx/util.h"
#include "pqxx/result"

namespace pqxx
{

/// @deprecated For compatibility with old BinaryString class
typedef binarystring BinaryString;

/// @deprecated For compatibility with old Result class
typedef result Result;

}

#endif

