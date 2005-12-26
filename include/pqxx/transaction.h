/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/transaction.h
 *
 *   DESCRIPTION
 *      This header is deprecated.  Use "transaction" instead (no ".h")
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_TRANSACTION_H
#define PQXX_TRANSACTION_H

#include "pqxx/util.h"
#include "pqxx/transaction"

namespace pqxx
{

/// @deprecated Compatibility with the old Transaction class
typedef transaction<read_committed> Transaction;

}

#endif

