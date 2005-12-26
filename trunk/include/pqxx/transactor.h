/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/transactor.h
 *
 *   DESCRIPTION
 *      This header is obsolete.  Use "transactor" instead (no ".h")
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_TRANSACTOR_H
#define PQXX_TRANSACTOR_H

#include "pqxx/util.h"
#include "pqxx/transactor"


namespace pqxx
{

/// @deprecated For compatibility with the old Transactor class
typedef transactor<transaction<read_committed> > Transactor;

}

#endif

