/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/nontransaction.h
 *
 *   DESCRIPTION
 *      This header is deprecated.  Use "nontransaction" instead (no ".h")
 *
 * Copyright (c) 2002-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_NONTRANSACTION_H
#define PQXX_NONTRANSACTION_H

#include "pqxx/util.h"
#include "pqxx/nontransaction"


namespace pqxx
{
/// @deprecated For compatibility with the old NonTransaction class
typedef nontransaction NonTransaction;
}

#endif

