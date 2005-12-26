/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/robusttransaction.h
 *
 *   DESCRIPTION
 *      This header is deprecated.  Use "robusttransaction" instead (no ".h")
 *
 * Copyright (c) 2002-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_ROBUSTTRANSACTION_H
#define PQXX_ROBUSTTRANSACTION_H

#include "pqxx/util.h"
#include "pqxx/robusttransaction"


namespace pqxx
{

/// @deprecated For compatibility with old RobustTransaction class
typedef robusttransaction<read_committed> RobustTransaction;

}

#endif

